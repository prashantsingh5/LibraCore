from dataclasses import dataclass, field
from typing import Protocol
import json
import re
from ..services import DomainError

@dataclass(frozen=True)
class ToolCall:
    name: str
    arguments: dict

@dataclass(frozen=True)
class ProviderMetadata:
    provider: str
    model: str = ''

@dataclass
class LLMResponse:
    calls: list[ToolCall] = field(default_factory=list)
    metadata: ProviderMetadata = field(default_factory=lambda: ProviderMetadata('mock'))

class LLMProvider(Protocol):
    def interpret(self, message: str, declarations: list[dict]) -> LLMResponse: ...

SYSTEM = ('Interpret the request using only declared tools. Never calculate or invent prices, budgets, '
          'availability or outcomes. Never confirm an action. Prefer read tools for questions. '
          'Use only IDs explicitly supplied by the user; use search to resolve titles. '
          'Do not invent identities. Return at most one tool call. No SQL or credentials.')

class MockLLMProvider:
    def interpret(self, message, declarations):
        rules = [
            (r'find books by (.+)', 'search_resources', lambda m: {'author': m[1].strip(), 'category': 0}),
            (r'how many copies of (\S+) are available\??', 'get_availability', lambda m: {'resource_id': m[1]}),
            (r'can (.+) buy (\d+) copies of (\S+)\??', 'check_purchase', lambda m: {'department_id': m[1], 'quantity': int(m[2]), 'resource_id': m[3].rstrip('?')}),
            (r'borrow (\S+) for (\S+)', 'borrow_resource', lambda m: {'resource_id': m[1], 'borrower_id': m[2], 'quantity': 1}),
            (r'cancel order (\d+)', 'cancel_purchase', lambda m: {'order_id': int(m[1])}),
            (r'return (\d+) of (\S+)', 'return_loan', lambda m: {'quantity': int(m[1]), 'loan_id': m[2]}),
        ]
        for pattern, name, args in rules:
            match = re.fullmatch(pattern, message.strip(), re.I)
            if match:
                return LLMResponse([ToolCall(name, args(match))])
        return LLMResponse()

def gemini_schema(schema):
    """Translate Pydantic JSON Schema to the SDK's supported Schema subset."""
    if isinstance(schema, list):
        return [gemini_schema(x) for x in schema]
    if not isinstance(schema, dict):
        return schema
    from google.genai import types
    allowed = set(types.Schema.model_fields)
    out = {}
    for key, value in schema.items():
        if key == 'properties':
            out[key] = {name: gemini_schema(item) for name, item in value.items()}
        elif key == 'exclusiveMinimum':
            out['minimum'] = value + (1 if schema.get('type') == 'integer' else 0)
        elif key in allowed:
            out[key] = gemini_schema(value)
    return out

class GeminiLLMProvider:
    def __init__(self, settings, client=None):
        self.model = settings.gemini_model
        if client is None:
            from google import genai
            from google.genai import types
            client = genai.Client(api_key=settings.gemini_api_key.get_secret_value(),
                                  http_options=types.HttpOptions(timeout=20000))
        self.client = client

    def interpret(self, message, declarations):
        from google.genai import types
        try:
            functions = [types.FunctionDeclaration(name=d['name'], description=d['description'], parameters=gemini_schema(d['parameters'])) for d in declarations]
            response = self.client.models.generate_content(model=self.model, contents=message,
                config=types.GenerateContentConfig(system_instruction=SYSTEM,
                    tools=[types.Tool(function_declarations=functions)],
                    automatic_function_calling=types.AutomaticFunctionCallingConfig(disable=True)))
            calls = [ToolCall(c.name, dict(c.args or {})) for c in (response.function_calls or [])]
            return LLMResponse(calls, ProviderMetadata('gemini', self.model))
        except Exception as exc:
            # SDK messages can include request metadata; never expose them.
            code = 'PROVIDER_QUOTA' if getattr(exc, 'code', None) == 429 else 'PROVIDER_ERROR'
            raise DomainError(code, 'Gemini request failed; no action was executed') from None

class OpenAILLMProvider:
    def __init__(self, settings, client=None):
        self.model = settings.openai_model
        if client is None:
            from openai import OpenAI
            client = OpenAI(api_key=settings.openai_api_key.get_secret_value(), timeout=20, max_retries=0)
        self.client = client

    def interpret(self, message, declarations):
        try:
            response = self.client.chat.completions.create(model=self.model,
                messages=[{'role':'system','content':SYSTEM},{'role':'user','content':message}],
                tools=[{'type':'function','function':d} for d in declarations], parallel_tool_calls=False)
            calls = [ToolCall(c.function.name, json.loads(c.function.arguments))
                     for c in (response.choices[0].message.tool_calls or [])]
            if any(not isinstance(c.arguments, dict) for c in calls):
                raise ValueError('Tool arguments must be objects')
            return LLMResponse(calls, ProviderMetadata('openai', self.model))
        except Exception as exc:
            code = 'PROVIDER_QUOTA' if getattr(exc, 'status_code', None) == 429 else 'PROVIDER_ERROR'
            raise DomainError(code, 'OpenAI request failed; no action was executed') from None

def make_provider(settings):
    return {'mock': MockLLMProvider, 'gemini': lambda: GeminiLLMProvider(settings),
            'openai': lambda: OpenAILLMProvider(settings), 'disabled': MockLLMProvider}[settings.provider]()
