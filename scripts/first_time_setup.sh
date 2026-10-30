#!/usr/bin/env bash
set -Eeuo pipefail

# LibraCore V3 beginner-friendly first-time setup for Ubuntu / WSL2.
# Usage:
#   bash scripts/first_time_setup.sh          # setup only
#   bash scripts/first_time_setup.sh --demo   # setup, then launch demo app

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

START_DEMO=false
case "${1:-}" in
  "") ;;
  --demo) START_DEMO=true ;;
  -h|--help)
    cat <<'HELP'
LibraCore V3 first-time setup

Usage:
  bash scripts/first_time_setup.sh          Complete setup and stop.
  bash scripts/first_time_setup.sh --demo   Complete setup, provision demo users, and start the app.

Windows/WSL prerequisite:
  Docker Desktop must be installed and running, with WSL integration enabled for Ubuntu.
HELP
    exit 0
    ;;
  *)
    echo "Unknown option: $1" >&2
    echo "Use --help for usage." >&2
    exit 2
    ;;
esac

if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
  echo "ERROR: Run this script as your normal WSL user, not with sudo." >&2
  echo "The script will ask for sudo only when Ubuntu packages need to be installed." >&2
  exit 1
fi

step() { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
ok()   { printf '\033[1;32m[OK]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[WARN]\033[0m %s\n' "$*"; }
fail() { printf '\033[1;31m[ERROR]\033[0m %s\n' "$*" >&2; exit 1; }

on_error() {
  local code=$?
  echo >&2
  echo "Setup stopped because a command failed (exit code $code)." >&2
  echo "Read SETUP.md -> Troubleshooting, then rerun the same command." >&2
  exit "$code"
}
trap on_error ERR

step "Checking the project"
[[ -f CMakeLists.txt && -f docker-compose.yml && -d scripts && -d frontend ]] \
  || fail "Run this command from the extracted LibraCore V3 project."
ok "LibraCore V3 project found at $ROOT_DIR"

# Windows ZIP extraction can leave NTFS alternate-stream marker files visible in Linux.
find . -type f -name '*:Zone.Identifier' -delete 2>/dev/null || true

step "Installing Ubuntu build tools"
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential \
  cmake \
  python3 \
  python3-dev \
  python3-venv \
  libpq-dev \
  libpqxx-dev \
  nlohmann-json3-dev \
  pkg-config \
  unzip \
  curl \
  git \
  ca-certificates
ok "Ubuntu build dependencies are installed"

step "Ensuring Node.js 22 is available"
need_node=true
if command -v node >/dev/null 2>&1; then
  node_major="$(node -p 'process.versions.node.split(".")[0]' 2>/dev/null || true)"
  [[ "$node_major" == "22" ]] && need_node=false
fi

if $need_node; then
  export NVM_DIR="${NVM_DIR:-$HOME/.nvm}"
  if [[ ! -s "$NVM_DIR/nvm.sh" ]]; then
    echo "Installing nvm for this WSL user..."
    curl -fsSL https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.8/install.sh | bash
  fi
  # shellcheck disable=SC1090
  source "$NVM_DIR/nvm.sh"
  nvm install 22
  nvm alias default 22
  nvm use 22
else
  if [[ -s "$HOME/.nvm/nvm.sh" ]]; then
    export NVM_DIR="$HOME/.nvm"
    # shellcheck disable=SC1090
    source "$NVM_DIR/nvm.sh"
    nvm use 22 >/dev/null 2>&1 || true
  fi
fi

command -v node >/dev/null 2>&1 || fail "Node.js was not installed successfully."
command -v npm >/dev/null 2>&1 || fail "npm was not installed successfully."
[[ "$(node -p 'process.versions.node.split(".")[0]')" == "22" ]] \
  || fail "LibraCore V3 expects Node 22. Current version: $(node --version)"
ok "Node $(node --version) and npm $(npm --version)"

step "Checking Docker Desktop / Docker Engine"
if ! command -v docker >/dev/null 2>&1; then
  cat >&2 <<'DOCKER_HELP'

Docker is not visible inside this WSL Ubuntu installation.

On Windows:
  1. Install and open Docker Desktop.
  2. Docker Desktop -> Settings -> Resources -> WSL Integration.
  3. Enable integration for Ubuntu / Ubuntu-24.04.
  4. Click Apply & Restart.
  5. In Windows PowerShell run:  wsl --shutdown
  6. Reopen Ubuntu and rerun:
       bash scripts/first_time_setup.sh

Do NOT install a second PostgreSQL server in Ubuntu.
DOCKER_HELP
  exit 2
fi

if ! docker info >/dev/null 2>&1; then
  warn "Docker is installed but the Docker daemon is not responding. Trying to start Docker Desktop..."
  docker desktop start >/dev/null 2>&1 || true
  for _ in {1..30}; do
    docker info >/dev/null 2>&1 && break
    sleep 2
  done
fi

docker compose version >/dev/null 2>&1 || fail "Docker Compose is unavailable. Update/start Docker Desktop."
docker info >/dev/null 2>&1 || fail "Docker Desktop is not running. Open Docker Desktop, wait until it is ready, then rerun this script."
ok "Docker and Docker Compose are available"

step "Preparing .env"
if [[ ! -f .env ]]; then
  cp .env.example .env
  echo "Created .env from .env.example. Live Gemini/OpenAI credentials remain optional."
fi
chmod 600 .env
sed -i 's/\r$//' .env

python3 - <<'PYENV'
from pathlib import Path
p = Path('.env')
text = p.read_text()
if not any(line.startswith('PUBLIC_ORIGIN=') for line in text.splitlines()):
    if text and not text.endswith('\n'):
        text += '\n'
    text += 'PUBLIC_ORIGIN=http://localhost:5173\n'
    p.write_text(text)
PYENV

ok ".env is present with Linux line endings and private permissions"

get_env() {
  local key="$1"
  python3 - "$key" <<'PYGET'
from pathlib import Path
import sys
key=sys.argv[1]
for raw in Path('.env').read_text().splitlines():
    line=raw.strip()
    if not line or line.startswith('#') or '=' not in line:
        continue
    k,v=line.split('=',1)
    if k.strip()==key:
        print(v.strip())
        break
PYGET
}

set_env() {
  local key="$1" value="$2"
  python3 - "$key" "$value" <<'PYSET'
from pathlib import Path
import sys
key,value=sys.argv[1],sys.argv[2]
p=Path('.env')
lines=p.read_text().splitlines()
out=[]
found=False
for line in lines:
    if line.startswith(key+'='):
        out.append(f'{key}={value}')
        found=True
    else:
        out.append(line)
if not found:
    out.append(f'{key}={value}')
p.write_text('\n'.join(out)+'\n')
PYSET
}

step "Checking PostgreSQL port"
pg_port="$(get_env POSTGRES_PORT)"
pg_port="${pg_port:-5432}"
port_busy=false
if command -v ss >/dev/null 2>&1 && ss -ltnH "sport = :$pg_port" 2>/dev/null | grep -q .; then
  port_busy=true
fi

this_postgres_running=false
if docker compose ps --status running postgres 2>/dev/null | grep -q 'postgres'; then
  this_postgres_running=true
fi

if $port_busy && ! $this_postgres_running; then
  db_url="$(get_env DATABASE_URL)"
  test_db_url="$(get_env TEST_DATABASE_URL)"
  if [[ -n "$db_url" || -n "$test_db_url" ]]; then
    fail "Port $pg_port is already in use and .env contains explicit database URLs. Stop the other PostgreSQL/container or edit .env before rerunning."
  fi
  new_port=""
  for candidate in $(seq 5433 5450); do
    if ! ss -ltnH "sport = :$candidate" 2>/dev/null | grep -q .; then
      new_port="$candidate"
      break
    fi
  done
  [[ -n "$new_port" ]] || fail "No free PostgreSQL port was found between 5433 and 5450."
  warn "Port $pg_port is already in use. V3 will use port $new_port so another project is not disturbed."
  set_env POSTGRES_PORT "$new_port"
  pg_port="$new_port"
fi
ok "PostgreSQL port for V3: $pg_port"

step "Running the existing verified V3 setup"
chmod +x scripts/*.sh
./scripts/setup_v3.sh

step "Installing browser runtime dependencies for Playwright"
(
  cd frontend
  npx playwright install-deps chromium
  npx playwright install chromium
)
ok "Playwright Chromium and Linux runtime libraries are installed"

step "Running final setup smoke checks"
docker compose ps postgres

docker compose exec -T postgres sh -lc 'pg_isready -U "$POSTGRES_USER" -d "$POSTGRES_DB"' >/dev/null

PYTHONPATH="$PWD/build-v3/python:$PWD/python" \
  .venv/bin/python -c "import _libracore; print('Native C++/Python binding: OK')"

.venv/bin/python - <<'PYCHECK'
from pathlib import Path
assert Path('build-v3').exists()
assert Path('frontend/node_modules').exists()
print('Python environment and frontend dependencies: OK')
PYCHECK

(
  cd frontend
  npx playwright screenshot --browser=chromium about:blank /tmp/libracore-playwright-smoke.png >/dev/null
)
[[ -s /tmp/libracore-playwright-smoke.png ]] || fail "Playwright Chromium did not complete its launch smoke test."
ok "Playwright browser launch: OK"

cat <<SUMMARY

============================================================
LibraCore V3 setup is complete.
============================================================

Project:      $ROOT_DIR
PostgreSQL:   running in Docker on localhost:$pg_port
Python:       $($ROOT_DIR/.venv/bin/python --version 2>&1)
Node:         $(node --version)
Frontend:     dependencies installed
Playwright:   Chromium launch verified

Next commands:

  Start the demo application:
      ./scripts/demo_v3.sh

  Run the complete automated verification:
      ./scripts/test_v3.sh

  Stop PostgreSQL when you are finished:
      docker compose down

Do NOT use "docker compose down -v" unless you intentionally want to delete the local database volume.
SUMMARY

if $START_DEMO; then
  echo
  echo "Starting the demo application now..."
  echo "You will be asked to choose a local demo password."
  exec ./scripts/demo_v3.sh
fi
