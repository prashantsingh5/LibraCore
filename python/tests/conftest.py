import os
os.environ['LLM_PROVIDER']='mock'
os.environ['RUN_LIVE_AI_TESTS']='false'
import pytest
from libracore_api.config import Settings
from libracore_api.services import CoreService

@pytest.fixture(autouse=True)
def no_live_llm(monkeypatch):
    def blocked(*args,**kwargs):
        raise AssertionError('Live LLM SDK construction forbidden in normal tests')
    import google.genai
    import openai
    monkeypatch.setattr(google.genai,'Client',blocked)
    monkeypatch.setattr(openai,'OpenAI',blocked)
    monkeypatch.setenv('LLM_PROVIDER','mock')
    monkeypatch.setenv('RUN_LIVE_AI_TESTS','false')

@pytest.fixture
def settings():return Settings(_env_file=None,llm_provider='mock')

@pytest.fixture
def dbsettings():
    if os.environ.get('LIBRACORE_RUN_DB_TESTS')!='1':
        pytest.skip('NOT EXECUTED — ENVIRONMENT BLOCKED: set LIBRACORE_RUN_DB_TESTS=1 with Docker test DB')
    return Settings(ai_enabled=False)

@pytest.fixture
def db(dbsettings):
    from libracore_api.database import reset_test
    from libracore_api.seed import seed
    reset_test(dbsettings)
    service=CoreService(dbsettings.db_url(True))
    seed(service)
    return service
