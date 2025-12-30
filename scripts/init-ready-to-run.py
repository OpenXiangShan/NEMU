#!/usr/bin/env python3

import argparse
import base64
import binascii
import hashlib
import json
import os
from pathlib import Path
from typing import Any, Dict, Iterable, Optional, Tuple
import shutil
import tempfile
import urllib.error
import urllib.request
from urllib.parse import urlparse
import tarfile
import zipfile

try:
    import yaml
except ImportError as e:  # pragma: no cover
    raise SystemExit("Missing dependency: PyYAML is required (pip install pyyaml)") from e

GITHUB_API_ROOT = "https://api.github.com"
USER_AGENT = "init-ready-to-run/1.0"
GITHUB_TOKEN: Optional[str] = None

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


def load_config(config_path: Path) -> Dict[str, Any]:
    with config_path.open("r", encoding="utf-8") as handle:
        data = yaml.safe_load(handle) or {}
    if not isinstance(data, dict):
        raise ValueError("Top-level YAML structure must be a mapping")
    return data


def build_github_headers(accept: Optional[str] = None) -> Dict[str, str]:
    token = GITHUB_TOKEN
    headers: Dict[str, str] = {}
    headers["User-Agent"] = USER_AGENT
    headers["X-GitHub-Api-Version"] = "2022-11-28"
    if accept:
        headers["Accept"] = accept
    if token:
        headers["Authorization"] = f"Bearer {token}"
    return headers


def github_request(url: str) -> Dict[str, Any]:
    headers = build_github_headers(accept="application/vnd.github+json")
    request = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(request) as response:
            payload = response.read().decode("utf-8")
    except urllib.error.HTTPError as error:  # pragma: no cover
        detail = error.read().decode("utf-8", errors="ignore") if error.fp else ""
        raise RuntimeError(f"GitHub request failed ({error.code}): {detail}") from error
    return json.loads(payload)


def resolve_asset(asset_cfg: Dict[str, Any], release_version: str) -> Tuple[str, str]:
    remote_template = asset_cfg.get("asset_name")
    if not remote_template:
        raise ValueError("Asset entry must define 'asset_name'")
    remote_name = remote_template.format(version=release_version)
    target_name = asset_cfg.get("save_as") or remote_name
    return remote_name, target_name


def find_release_asset(release: Dict[str, Any], asset_name: str) -> Dict[str, Any]:
    for asset in release.get("assets", []):
        if asset.get("name") == asset_name:
            return asset
    available = ", ".join(asset.get("name", "<unknown>") for asset in release.get("assets", []))
    raise FileNotFoundError(f"Asset '{asset_name}' not found in release; available: {available}")


def download_asset(url: str, destination: Path, headers: Dict[str, str] = {}) -> None:
    request = urllib.request.Request(url, headers=headers)
    destination.parent.mkdir(parents=True, exist_ok=True)

    with urllib.request.urlopen(request) as response, destination.open("wb") as output:
        print(response.status, response.reason)
        shutil.copyfileobj(response, output, 1024 * 1024)


def ensure_within_directory(base: Path, target: Path) -> None:
    base_resolved = base.resolve()
    target_resolved = target.resolve()
    if os.path.commonpath([str(base_resolved), str(target_resolved)]) != str(base_resolved):
        raise RuntimeError(f"Extraction target {target} escapes output directory {base}")


def extract_tar_archive(archive_path: Path, output_dir: Path) -> None:
    try:
        with tarfile.open(archive_path, "r:*") as archive:
            for member in archive.getmembers():
                ensure_within_directory(output_dir, output_dir / member.name)
            archive.extractall(path=output_dir)
    except tarfile.TarError as exc:
        raise RuntimeError(f"Failed to untar {archive_path}") from exc


def extract_zip_archive(archive_path: Path, output_dir: Path) -> None:
    try:
        with zipfile.ZipFile(archive_path) as archive:
            for name in archive.namelist():
                ensure_within_directory(output_dir, output_dir / name)
            archive.extractall(path=output_dir)
    except zipfile.BadZipFile as exc:
        raise RuntimeError(f"Failed to unzip {archive_path}") from exc


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


def parse_asset_digest(asset: Dict[str, Any]) -> Optional[Tuple[str, str]]:
    digest = asset.get("digest")
    if not digest:
        return None
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


def verify_digest(path: Path, algorithm: str, expected: str) -> bool:
    digest = hashlib.new(algorithm)
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    actual_bytes = digest.digest()
    actual_hex = digest.hexdigest()
    if expected.lower() == actual_hex:
        return True
    try:
        expected_bytes = bytes.fromhex(expected)
        if expected_bytes == actual_bytes:
            return True
    except ValueError:
        pass
    try:
        expected_bytes = base64.b64decode(expected, validate=True)
    except binascii.Error:
        return False
    return expected_bytes == actual_bytes


def process_github_release(
    artifact_id: str,
    cfg: Dict[str, Any],
    output_dir: Path,
    force: bool,
) -> None:
    repo = cfg.get("repo")
    version = cfg.get("version")
    assets = cfg.get("assets")
    if not repo or not version or not assets:
        raise ValueError(f"Entry '{artifact_id}' must define repo, version, and assets")

    release_url = f"{GITHUB_API_ROOT}/repos/{repo}/releases/tags/{version}"
    release = github_request(release_url)

    for asset_cfg in ensure_iterable(assets):
        remote_name, target_name = resolve_asset(asset_cfg, version)
        destination = output_dir / target_name

        asset = find_release_asset(release, remote_name)
        digest_info = parse_asset_digest(asset)

        if destination.exists() and not force:
            if digest_info:
                algorithm, expected = digest_info
                if verify_digest(destination, algorithm, expected):
                    print(f"[skip] {artifact_id}: {target_name} already exists")
                    continue
                print(f"[redo] {artifact_id}: {target_name} digest mismatch, re-downloading")
            else:
                print(f"[skip] {artifact_id}: {target_name} already exists (no digest)")
                continue

        url = asset.get("browser_download_url")
        headers = build_github_headers(accept="application/octet-stream")
        if not url:
            raise RuntimeError(f"Asset '{remote_name}' missing download url")

        if destination.exists():
            destination.unlink()

        print(f"[download] {artifact_id}: {remote_name} -> {destination.relative_to(output_dir)}")
        download_asset(url, destination, headers=headers)

        if digest_info and not verify_digest(destination, *digest_info):
            destination.unlink(missing_ok=True)
            raise ValueError(f"Digest mismatch for {destination}")


def process_github_artifacts(
    artifact_id: str,
    cfg: Dict[str, Any],
    output_dir: Path,
    force: bool,
) -> None:
    if not GITHUB_TOKEN:
        raise ValueError(f"Entry '{artifact_id}' requires a GitHub token for artifact downloads")
    repo = cfg.get("repo")
    run_id = cfg.get("run_id")
    files = cfg.get("artifacts")
    if not repo or not run_id or not files:
        raise ValueError(f"Entry '{artifact_id}' must define repo, run_id, and files")

    list_url = f"{GITHUB_API_ROOT}/repos/{repo}/actions/runs/{run_id}/artifacts"
    listing = github_request(list_url)
    artifacts = {item.get("name"): item for item in listing.get("artifacts", [])}

    for file_cfg in ensure_iterable(files):
        if not isinstance(file_cfg, dict):
            raise ValueError(f"File entry in '{artifact_id}' must be a mapping")
        artifact_name = file_cfg.get("artifact_name")
        if not artifact_name:
            raise ValueError(f"File entry in '{artifact_id}' missing artifact_name")
        target_name = file_cfg.get("save_as") or artifact_name
        destination = output_dir / target_name

        artifact = artifacts.get(artifact_name)
        if not artifact:
            available = ", ".join(sorted(name for name in artifacts if name)) or "<none>"
            raise FileNotFoundError(
                f"Artifact '{artifact_name}' not found for run {run_id}; available: {available}"
            )

        if destination.exists():
            if force:
                destination.unlink()
            else:
                print(f"[skip] {artifact_id}: {target_name} already exists")
                continue

        download_url = artifact.get("archive_download_url")
        if not download_url:
            raise RuntimeError(f"Artifact '{artifact_name}' missing download url")

        print(f"[download] {artifact_id}: {artifact_name} -> {target_name}.zip")
        with tempfile.NamedTemporaryFile(delete=False, suffix=".zip") as tmp_handle:
            temp_path = Path(tmp_handle.name)
        try:
            headers = build_github_headers(accept="application/vnd.github+json")
            download_asset(download_url, temp_path, headers=headers)
            print(f"[extract] {artifact_id}: {target_name}")
            extract_single_file_from_zip(temp_path, destination)
        finally:
            temp_path.unlink(missing_ok=True)



def resolve_download_target(file_cfg: Dict[str, Any], url: str) -> str:
    target_name = file_cfg.get("save_as")
    if target_name:
        return target_name
    parsed = urlparse(url)
    derived = Path(parsed.path).name
    if derived:
        return derived
    raise ValueError(f"Unable to determine file name from url '{url}'")


def process_download_entry(
    artifact_id: str,
    cfg: Dict[str, Any],
    output_dir: Path,
    force: bool,
) -> None:
    files = cfg.get("files")
    if not files:
        raise ValueError(f"Entry '{artifact_id}' must define files")
    for file_cfg in ensure_iterable(files):
        if not isinstance(file_cfg, dict):
            raise ValueError(f"File entry in '{artifact_id}' must be a mapping")
        url = file_cfg.get("url")
        if not url:
            raise ValueError(f"File entry in '{artifact_id}' missing url")
        target_name = resolve_download_target(file_cfg, url)
        destination = output_dir / target_name

        if destination.exists():
            if force:
                destination.unlink()
            else:
                print(f"[skip] {artifact_id}: {target_name} already exists")
                continue

        print(f"[download] {artifact_id}: {url} -> {destination.relative_to(output_dir)}")
        download_asset(url, destination)

        if file_cfg.get("untar"):
            print(f"[untar] {artifact_id}: {target_name}")
            extract_tar_archive(destination, output_dir)
        if file_cfg.get("unzip"):
            print(f"[unzip] {artifact_id}: {target_name}")
            extract_zip_archive(destination, output_dir)

TYPE_HANDLERS = {
    "github-release": process_github_release,
    "github-artifacts": process_github_artifacts,
    "download": process_download_entry,
}


def ensure_iterable(value: Any) -> Iterable[Any]:
    if isinstance(value, list):
        return value
    return [value]


def process_entries(config: Dict[str, Any], output_dir: Path, force: bool) -> None:
    for artifact_id, cfg in config.items():
        entry_type = cfg.get("type")
        handler = TYPE_HANDLERS.get(entry_type)
        if not handler:
            print(f"[skip] {artifact_id}: unsupported type '{entry_type}'")
            continue
        handler(artifact_id, cfg, output_dir, force)


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Initialize ready-to-run assets from release definitions")
    parser.add_argument("--config", default=str(root / "ready-to-run.yml"), help="Path to YAML configuration file")
    parser.add_argument("--output", default=str(root / "ready-to-run"), help="Directory where assets are stored")
    parser.add_argument("--force", action="store_true", help="Re-download assets even if they already exist")
    parser.add_argument("--token", default=os.environ.get("GITHUB_TOKEN"), help="GitHub token for authenticated downloads (defaults to $GITHUB_TOKEN)")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    config_path = Path(args.config)
    output_dir = Path(args.output)
    global GITHUB_TOKEN
    GITHUB_TOKEN = args.token

    config = load_config(config_path)
    process_entries(config, output_dir, args.force)


if __name__ == "__main__":
    main()
