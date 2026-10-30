import os
import pytest

os.environ["LLM_PROVIDER"] = "mock"
os.environ["RUN_LIVE_AI_TESTS"] = "false"


@pytest.fixture(autouse=True)
def no_live(monkeypatch):
    def blocked(*args, **kwargs):
        raise AssertionError("Live AI is prohibited in tests")

    import google.genai
    import openai

    monkeypatch.setattr(google.genai, "Client", blocked)
    monkeypatch.setattr(openai, "OpenAI", blocked)
