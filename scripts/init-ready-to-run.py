#!/usr/bin/env python3

import argparse
import base64
import binascii
import hashlib
import json
from pathlib import Path
from typing import Any, Dict, Iterable, Optional, Tuple
import urllib.error
import urllib.request

try:
    import yaml
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Missing dependency: PyYAML is required (pip install pyyaml)") from exc

GITHUB_API_ROOT = "https://api.github.com"
USER_AGENT = "init-ready-to-run/1.0"


def load_config(config_path: Path) -> Dict[str, Any]:
    with config_path.open("r", encoding="utf-8") as handle:
        data = yaml.safe_load(handle) or {}
    if not isinstance(data, dict):
        raise ValueError("Top-level YAML structure must be a mapping")
    return data


def github_request(url: str) -> Dict[str, Any]:
    headers = {"Accept": "application/vnd.github+json", "User-Agent": USER_AGENT}
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


def download_asset(url: str, destination: Path) -> None:
    headers = {"User-Agent": USER_AGENT}
    request = urllib.request.Request(url, headers=headers)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(request) as response, destination.open("wb") as output:
        while True:
            chunk = response.read(1024 * 1024)
            if not chunk:
                break
            output.write(chunk)


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


def process_github_release(artifact_id: str, cfg: Dict[str, Any], output_dir: Path, force: bool) -> None:
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
        if not url:
            raise RuntimeError(f"Asset '{remote_name}' missing browser_download_url")

        if destination.exists():
            destination.unlink()

        print(f"[download] {artifact_id}: {remote_name} -> {destination.relative_to(output_dir)}")
        download_asset(url, destination)

        if digest_info and not verify_digest(destination, *digest_info):
            destination.unlink(missing_ok=True)
            raise ValueError(f"Digest mismatch for {destination}")


TYPE_HANDLERS = {
    "github-release": process_github_release,
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
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    config_path = Path(args.config)
    output_dir = Path(args.output)

    config = load_config(config_path)
    process_entries(config, output_dir, args.force)


if __name__ == "__main__":
    main()
