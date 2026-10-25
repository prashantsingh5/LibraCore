#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"
activate_project
python - <<'PY'
from libracore_api.config import Settings
from libracore_api.ai.providers import make_provider
from libracore_api.ai.tools import declarations
try:
    s=Settings()
except Exception:
    raise SystemExit('Invalid AI configuration. Check provider, key and model settings; no secrets logged.') from None
if not s.run_live_ai_tests or s.provider not in ('gemini','openai'):
    raise SystemExit('Explicitly set RUN_LIVE_AI_TESTS=true and configure Gemini or OpenAI to opt in.')
from libracore_api.services import DomainError
try:
    reply=make_provider(s).interpret('How many copies of B001 are available?',declarations())
    assert reply.calls and reply.calls[0].name=='get_availability'
except DomainError as e:
    raise SystemExit(e.code+': '+e.message) from None
print('Live provider tool interpretation passed; no domain write executed.')
PY
