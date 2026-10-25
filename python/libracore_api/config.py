from pathlib import Path
from typing import Literal
from urllib.parse import quote, urlparse
from pydantic import SecretStr, model_validator
from pydantic_settings import BaseSettings, SettingsConfigDict

ROOT = Path(__file__).resolve().parents[2]

class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=(ROOT / '.env.example', ROOT / '.env'), extra='ignore', case_sensitive=False, hide_input_in_errors=True)
    libracore_env: str = 'development'
    libracore_log_level: str = 'INFO'
    postgres_host: str = 'localhost'
    postgres_port: int = 5432
    postgres_db: str = 'libracore'
    postgres_user: str = 'libracore'
    postgres_password: SecretStr = SecretStr('libracore_dev_password')
    database_url: SecretStr = SecretStr('')
    test_postgres_db: str = 'libracore_test'
    test_database_url: SecretStr = SecretStr('')
    api_host: str = '127.0.0.1'
    api_port: int = 8000
    ai_enabled: bool = True
    llm_provider: Literal['auto', 'gemini', 'openai', 'mock'] = 'auto'
    gemini_api_key: SecretStr = SecretStr('')
    gemini_model: str = ''
    openai_api_key: SecretStr = SecretStr('')
    openai_model: str = ''
    llm_fallback_on_error: bool = False
    ai_write_actions_enabled: bool = True
    ai_require_confirmation: bool = True
    run_live_ai_tests: bool = False

    @model_validator(mode='after')
    def validate_settings(self):
        if not 1 <= self.postgres_port <= 65535 or not 1 <= self.api_port <= 65535:
            raise ValueError('Ports must be 1..65535')
        if self.llm_fallback_on_error:
            raise ValueError('Runtime LLM fallback is not supported; set LLM_FALLBACK_ON_ERROR=false')
        if self.libracore_log_level.upper() not in {'DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'}:
            raise ValueError('Invalid log level')
        if self.ai_enabled:
            provider = self.provider
            if provider != 'mock':
                if not getattr(self, provider + '_api_key').get_secret_value().strip():
                    raise ValueError(f'{provider.upper()}_API_KEY is required')
                if not getattr(self, provider + '_model').strip():
                    raise ValueError(f'{provider.upper()}_MODEL is required')
        return self

    @property
    def provider(self):
        if not self.ai_enabled:
            return 'disabled'
        if self.llm_provider != 'auto':
            return self.llm_provider
        if self.openai_api_key.get_secret_value().strip():
            return 'openai'
        if self.gemini_api_key.get_secret_value().strip():
            return 'gemini'
        return 'mock'

    def db_url(self, test=False):
        explicit = (self.test_database_url if test else self.database_url).get_secret_value()
        db = self.test_postgres_db if test else self.postgres_db
        url = explicit or (f'postgresql://{quote(self.postgres_user, safe="")}:'
                           f'{quote(self.postgres_password.get_secret_value(), safe="")}@'
                           f'{self.postgres_host}:{self.postgres_port}/{quote(db, safe="")}')
        if test:
            target, development = urlparse(url), urlparse(self.db_url())
            if target.path != '/libracore_test' or target.path == development.path:
                raise ValueError('Destructive tests require the isolated libracore_test database, distinct from development')
        return url
