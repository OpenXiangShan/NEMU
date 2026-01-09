import argparse
import base64
import binascii
import hashlib
import json
import os
import shutil
import tarfile
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path
from typing import Any, List, Dict, Iterable, Optional, Tuple
from urllib.parse import urlparse
import yaml
from abc import ABC, abstractmethod

force_flag = False

class SafeRedirectHandler(urllib.request.HTTPRedirectHandler):
    """
    Do not forward Authorization headers on redirects.

    This is for GitHub workflow artifact downloads. GitHub REST API returns
    a 302 redirect to Azure CDN. GitHub REST API requires Authorization header,
    but the CDN refuses it. 
    """
    
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        new_req = super().redirect_request(req, fp, code, msg, headers, newurl)
        if new_req is None:
            return None
        if 'Authorization' in new_req.headers:
            del new_req.headers['Authorization']
        return new_req


# Install the custom redirect handler globally
opener = urllib.request.build_opener(SafeRedirectHandler)
urllib.request.install_opener(opener)


class DigestVerifier:
    @staticmethod
    def _parse_digest(digest: str) -> Tuple[str, str]:
        if ":" not in digest:
            raise ValueError(f"Unsupported digest format '{digest}'")
        algorithm, expected = digest.split(":", 1)
        algorithm = algorithm.strip().lower()
        expected = expected.strip()
        if not algorithm:
            raise ValueError(f"Missing digest algorithm in '{digest}'")
        if algorithm not in hashlib.algorithms_available:
            raise ValueError(f"Unsupported digest algorithm '{algorithm}'")
        return algorithm, expected

    @staticmethod
    def verify(file_path: Path, digest: str) -> bool:
        algorithm, expected = DigestVerifier._parse_digest(digest)
        hasher = hashlib.new(algorithm)
        with file_path.open("rb") as f:
            for chunk in iter(lambda: f.read(1024 * 1024), b""):
                hasher.update(chunk)
        computed = hasher.hexdigest()
        return computed.lower() == expected.lower()


class FileExtractor:
    @staticmethod
    def _safe_target(base: Path, member_name: str) -> Path:
        base = base.resolve()
        target = (base / member_name).resolve()
        if not target.is_relative_to(base):
            raise RuntimeError(f"Extraction target {target} escapes output directory {base}")
        return target

    @staticmethod
    def extract_tar_archive(archive_path: Path, output_dir: Path) -> None:
        try:
            output_dir = output_dir.resolve()
            with tarfile.open(archive_path, "r:*") as archive:
                for member in archive.getmembers():
                    FileExtractor._safe_target(output_dir, member.name)
                archive.extractall(path=output_dir)
        except tarfile.TarError as exc:
            raise RuntimeError(f"Failed to untar {archive_path}") from exc

    @staticmethod
    def extract_zip_archive(archive_path: Path, output_dir: Path) -> None:
        try:
            output_dir = output_dir.resolve()
            with zipfile.ZipFile(archive_path) as archive:
                for name in archive.namelist():
                    FileExtractor._safe_target(output_dir, name)
                archive.extractall(path=output_dir)
        except zipfile.BadZipFile as exc:
            raise RuntimeError(f"Failed to unzip {archive_path}") from exc

    @staticmethod
    def extract_single_file_from_zip(archive_path: Path, destination: Path) -> None:
        try:
            with zipfile.ZipFile(archive_path) as archive:
                members = [info for info in archive.infolist() if not info.is_dir()]
                if not members:
                    raise RuntimeError(f"Zip archive {archive_path} contains no files")
                if len(members) > 1:
                    raise RuntimeError(
                        f"Zip archive {archive_path} contains multiple files; provide a dedicated extraction entry"
                    )
                destination.parent.mkdir(parents=True, exist_ok=True)
                with archive.open(members[0]) as source, destination.open("wb") as target:
                    shutil.copyfileobj(source, target)
        except zipfile.BadZipFile as exc:
            raise RuntimeError(f"Failed to unzip {archive_path}") from exc

def request_file(dest: Path, url: str, headers: Dict[str, str] = {}) -> None:
    request = urllib.request.Request(url, headers=headers)
    dest.parent.mkdir(parents=True, exist_ok=True)

    try:
        with urllib.request.urlopen(request) as response, dest.open("wb") as output:
            shutil.copyfileobj(response, output)
    except urllib.error.HTTPError as error:  # pragma: no cover
        detail = error.read().decode("utf-8", errors="ignore") if error.fp else ""
        raise RuntimeError(f"GitHub request failed ({error.code}): {detail}") from error
    
def download_file(
    dest: Path,
    url: str,
    headers: Dict[str, str] = {},
    digest: Optional[str] = None,
    force: bool = False
) -> None:
    
    if dest.exists() and not force:
        if digest:
            if DigestVerifier.verify(dest, digest):
                print(f"[skip] {dest}: already exists and verified")
                return
            print(f"[redo] {dest}: digest mismatch, re-downloading")
        else:
            print(f"[skip] {dest}: already exists (no digest provided)")
            return

    if dest.exists():
        dest.unlink()

    print(f"[down] {dest}: downloading from {url}")
    request_file(dest, url, headers)

    if digest:
        if not DigestVerifier.verify(dest, digest):
            raise RuntimeError(f"Digest verification failed for '{dest}'")
        print(f"[verify] {dest}: verified")


class GitHubClient:
    GITHUB_API_ROOT: str = "https://api.github.com"
    USER_AGENT: str = "OpenXiangShan-ready-to-run/0.1"
    DEFAULT_HEADERS: Dict[str, str] = {
        "User-Agent": USER_AGENT,
        "X-GitHub-Api-Version": "2022-11-28",
        "Accept": "application/vnd.github+json",
    }

    def __init__(self, token: Optional[str]) -> None:
        self.headers: Dict[str, str] = self.DEFAULT_HEADERS.copy()
        if token:
            self.headers['Authorization'] = f"Bearer {token}"

    def _build_headers(self, media_type: Optional[str] = None) -> Dict[str, str]:
        headers = self.headers.copy()
        if media_type:
            headers["Accept"] = f"application/vnd.github.{media_type}+json"
        return headers

    def request_json(self, url: str) -> Dict[str, Any]:
        headers = self._build_headers()
        request = urllib.request.Request(url, headers=headers)
        try:
            with urllib.request.urlopen(request) as response:
                payload = response.read().decode("utf-8")
        except urllib.error.HTTPError as error:  # pragma: no cover
            detail = error.read().decode("utf-8", errors="ignore") if error.fp else ""
            raise RuntimeError(f"GitHub request failed ({error.code}): {detail}") from error
        return json.loads(payload)
        
    def get_release_by_tag(self, owner: str, repo: str, tag: str) -> Dict[str, Any]:
        url = f"{self.GITHUB_API_ROOT}/repos/{owner}/{repo}/releases/tags/{tag}"
        print(f"Fetching release info from {url}")
        return self.request_json(url)
    
    def list_workflow_run_artifacts(self, owner: str, repo: str, run_id: str) -> Dict[str, Any]:
        url = f"{self.GITHUB_API_ROOT}/repos/{owner}/{repo}/actions/runs/{run_id}/artifacts"
        print(f"Listing workflow run artifacts from {url}")
        return self.request_json(url)

    def download_file(self, dest: Path, url: str, headers: Dict[str, str] = {}, digest: Optional[str] = None, force: bool = False) -> None:
        download_file(dest, url, headers, digest, force)


class Processor(ABC):
    def __init__(self, term_name: str, config: Dict[str, Any], args: argparse.Namespace) -> None:
        self.term_name = term_name
        self.config = config
        self.args = args
        
        self.output_dir = Path(args.output)
        self.force = args.force

    @abstractmethod
    def process(self) -> None:
        pass


class ProcessorFactory:
    _registry = {}

    @classmethod
    def register(cls, name):
        def decorator(processor_cls):
            cls._registry[name] = processor_cls
            return processor_cls
        return decorator

    @classmethod
    def create(cls, term_name: str, config: Dict[str, Any], args: argparse.Namespace) -> Processor:
        processor_type = config.get("type")
        if processor_type not in cls._registry:
            raise ValueError(f"Unsupported type '{processor_type}' for entry '{term_name}'")
        return cls._registry[processor_type](term_name, config, args)
    

@ProcessorFactory.register('download')
class ProcessorDownload(Processor):
    def __init__(self, term_name: str, config: Dict[str, Any], args: argparse.Namespace) -> None:
        super().__init__(term_name, config, args)
        
        files = self.config.get("files")
        if files is None or not isinstance(files, list):
            raise ValueError(f"Missing or invalid 'files' field in entry '{self.term_name}'")
        self.files: List[Dict[str, Any]] = files


    def process(self) -> None:
        print(f"Processing download for '{self.term_name}'")

        for file_cfg in self.files:
            url = file_cfg.get("url")
            if not url:
                raise ValueError(f"Missing 'url' field in file entry of '{self.term_name}'")
            
            save_as = file_cfg.get("save_as")
            if not save_as:
                raise ValueError(f"Missing 'save_as' field in file entry of '{self.term_name}'")
            
            digest = file_cfg.get("digest")

            dest_file_path = self.output_dir / file_cfg["save_as"]
            download_file(
                dest=dest_file_path,
                url=url,
                digest=digest,
                force=self.force,
            )


class ProcessorGithub(Processor):
    def __init__(self, term_name: str, config: Dict[str, Any], args: argparse.Namespace) -> None:
        super().__init__(term_name, config, args)

        self.github_client = GitHubClient(args.token)

        owner_repo = self.config.get('repo')
        if owner_repo is None or '/' not in owner_repo:
            raise ValueError(f"Invalid or missing 'repo' field in entry '{term_name}'")
        (self.owner, self.repo) = owner_repo.split('/')


@ProcessorFactory.register('github-release')
class ProcessorGithubRelease(ProcessorGithub):
    def __init__(self, term_name: str, config: Dict[str, Any], args: argparse.Namespace) -> None:
        super().__init__(term_name, config, args)

        tag = self.config.get("tag")
        if tag is None:
            raise ValueError(f"Missing 'tag' field in entry '{term_name}'")
        self.tag: str = tag
        
        assets = self.config.get("assets")
        if assets is None or not isinstance(assets, list):
            raise ValueError(f"Missing or invalid 'assets' field in entry '{term_name}'")
        self.assets: List[Dict[str, Any]] = assets


    def _find_asset_by_name(self, release: Dict[str, Any], name: str) -> Dict[str, Any]:
        assets = release.get("assets", [])
        for asset in assets:
            if asset.get("name") == name:
                return asset
        raise RuntimeError(f"Asset '{name}' not found in release '{self.tag}'")


    def process(self) -> None:
        print(f"Processing GitHub release for '{self.term_name}'")

        release = self.github_client.get_release_by_tag(self.owner, self.repo, self.tag)

        for asset_cfg in self.assets:
            asset_name = asset_cfg.get("name")
            if not asset_name:
                raise ValueError(f"Missing 'name' field in asset entry of '{self.term_name}'")
            asset_name = asset_name.format(tag=self.tag)
            
            asset = self._find_asset_by_name(release, asset_name)

            download_url = asset["browser_download_url"]
            digest = asset.get("digest")
            if "save_as" in asset_cfg:
                asset_file_path = self.output_dir / asset_cfg["save_as"]
            else:
                asset_file_path = self.output_dir / asset_name
            
            self.github_client.download_file(
                dest=asset_file_path,
                url=download_url,
                digest=digest,
                force=self.force,
            )


@ProcessorFactory.register('github-artifacts')
class ProcessorGithubArtifacts(ProcessorGithub):
    def __init__(self, term_name: str, config: Dict[str, Any], args: argparse.Namespace) -> None:
        super().__init__(term_name, config, args)

        if not self.args.token:
            raise ValueError(f"GitHub token is required for 'github-artifacts' type in entry '{term_name}'")

        run_id = self.config.get('run_id')
        if not run_id:
            raise ValueError(f"Missing or invalid 'run_id' field in entry '{term_name}'")
        self.run_id: str = run_id

        artifacts = self.config.get("artifacts")
        if artifacts is None or not isinstance(artifacts, list):
            raise ValueError(f"Missing or invalid 'artifacts' field in entry '{term_name}'")
        self.artifacts: List[Dict[str, Any]] = artifacts

    
    def _find_artifact_by_name(self, workflow_run: Dict[str, Any], name: str) -> Dict[str, Any]:
        artifacts = workflow_run.get("artifacts", [])
        for artifact in artifacts:
            if artifact.get("name") == name:
                return artifact
        raise RuntimeError(f"Artifact '{name}' not found in workflow run {self.run_id}")
        

    def process(self) -> None:
        print(f"Processing GitHub artifacts for '{self.term_name}'")

        workflow_run = self.github_client.list_workflow_run_artifacts(self.owner, self.repo, self.run_id)
        
        for artifact_cfg in self.artifacts:
            artifact_name = artifact_cfg.get("name")
            if not artifact_name:
                raise ValueError(f"Missing 'name' field in artifact entry of '{self.term_name}'")
            
            artifact = self._find_artifact_by_name(workflow_run, artifact_name)

            is_expired = artifact.get("expired", False)
            if is_expired:
                raise RuntimeError(f"Artifact '{artifact_name}' in workflow run {self.run_id} has expired")

            download_url = artifact["archive_download_url"]
            digest = artifact.get("digest")
            artifact_zip_file_path = self.output_dir / (artifact_name + ".zip")
            
            self.github_client.download_file(
                dest=artifact_zip_file_path,
                url=download_url,
                digest=digest,
                force=self.force,
            )

            if "save_as" in artifact_cfg:
                dest_file_path = self.output_dir / artifact_cfg["save_as"]
                print(f"[extract] {self.term_name}: extracting {artifact_name} to {dest_file_path}")
                FileExtractor.extract_single_file_from_zip(
                    archive_path=artifact_zip_file_path,
                    destination=dest_file_path,
                )
            else:
                print(f"[extract] {self.term_name}: extracting {artifact_zip_file_path} to {self.output_dir}")
                FileExtractor.extract_zip_archive(
                    archive_path=artifact_zip_file_path,
                    output_dir=self.output_dir,
                )


def load_config(config_path: Path) -> Dict[str, Any]:
    with config_path.open("r", encoding="utf-8") as handle:
        data = yaml.safe_load(handle) or {}
    if not isinstance(data, dict):
        raise ValueError("Top-level YAML structure must be a mapping")
    return data


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Initialize ready-to-run assets from release definitions")
    parser.add_argument(
        "--config",
        default=str(root / "ready-to-run.yml"),
        help="Path to YAML configuration file",
    )
    parser.add_argument(
        "--output",
        default=str(root / "ready-to-run"),
        help="Directory where assets are stored",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-download assets even if they already exist",
    )
    parser.add_argument(
        "--token",
        default=os.environ.get("GITHUB_TOKEN"),
        help="GitHub token for authenticated downloads (defaults to $GITHUB_TOKEN)",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    config_path = Path(args.config)
    config = load_config(config_path)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    for term_name, cfg in config.items():
        processor = ProcessorFactory.create(term_name, cfg, args)
        processor.process()


if __name__ == "__main__":
    main()

