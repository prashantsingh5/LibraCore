"""Non-destructive startup failure path; no Docker or database required."""

import os
from pathlib import Path
import subprocess


def test_daily_start_explains_unavailable_docker(tmp_path):
    docker = tmp_path / "docker"
    docker.write_text("#!/bin/sh\nexit 1\n")
    docker.chmod(0o755)
    root = Path(__file__).resolve().parents[2]
    env = {**os.environ, "PATH": str(tmp_path) + ":/usr/bin:/bin"}
    result = subprocess.run(
        ["bash", str(root / "scripts/start_v3.sh")], env=env, text=True, capture_output=True, timeout=10
    )
    assert result.returncode != 0
    assert "Docker Desktop" in result.stderr and "WSL integration" in result.stderr
