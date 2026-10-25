# AI interpretation and controlled execution

## Provider contract

`LLMProvider.interpret(message, declarations)` returns `LLMResponse`, containing
provider metadata and normalized `ToolCall(name, arguments)` values. The assistant
never depends on SDK response classes. Both adapters receive the same declared
tool names and validate the same application schemas after normalization.

`MockLLMProvider` uses a deliberately small set of deterministic intents: author
search, availability, purchase checking, borrowing, returning and cancellation.
It is a test provider, not a general language model. Unsupported messages produce
no operation and no invented result.

`GeminiLLMProvider` lazily constructs `google-genai.Client` only when selected.
It sends explicit `FunctionDeclaration` objects. `gemini_schema` adapts Pydantic
JSON Schema to the pinned SDK's supported subset. Automatic function execution is
explicitly disabled. Returned function calls are data only.

`OpenAILLMProvider` lazily constructs the OpenAI SDK client only when selected.
It uses Chat Completions function tools, disables parallel tool calls, sets a
20-second timeout and disables SDK automatic retries. It parses the function
argument JSON into the same internal ToolCall structure. Gemini also has a
20-second request timeout. The application accepts at most one tool call per
assistant message, regardless of provider behavior.

## Selection and errors

Explicit `mock` always wins over credentials. Explicit `gemini` / `openai` requires
its own nonempty key and model; missing configuration fails clearly. `auto`
selects OpenAI key first, then Gemini, then Mock. Whitespace-only keys are empty.
`AI_ENABLED=false` does not initialize any external client. No OpenAI key is needed
for Gemini or Mock operation. Settings, not source code, control model names.
The example's Gemini model must be available to the user's account; change the
environment value if necessary, without source edits.

Runtime fallback is not implemented: `LLM_FALLBACK_ON_ERROR=true` is rejected.
A provider timeout, malformed response or SDK failure yields `PROVIDER_ERROR`;
HTTP 429 SDK errors yield `PROVIDER_QUOTA`. Raw exception text is never returned.
No failed reasoning request executes a write or automatically invokes another
provider. Live quota is not a prerequisite for offline tests or V2 completion.

## Typed tools and authoritative results

`ai/tools.py` contains 14 allowed application tools with input schemas and action
classification. `OUTPUTS` defines validated output types: resource, search,
availability, Money, budget, order, loan and event contracts. Unknown tools,
extra parameters, invalid shapes, model-selected request IDs and multiple calls
are rejected before execution. The model never receives SQL or credentials.

READ: search/get resource/availability, budget/department budget, quote/check,
get order/user loans/recent events.
WRITE: borrow and return.
HIGH_RISK: all submit purchases and cancellations (conservatively including small
purchases). The API creates an unpredictable request ID for each AI mutation;
the model cannot choose or override it.

A successful tool invokes `CoreService` → pybind11 → C++ → PostgreSQL. The final
assistant response contains the structured tool result and deterministic framing.
Unverified model prose is not returned as an authoritative business answer. There
is intentionally no second free-form LLM pass that could replace a tool's price,
quota or availability with a hallucination.

## Confirmation lifecycle

With `AI_REQUIRE_CONFIRMATION=true`, high-risk actions produce a token, action,
validated parameters and expiry. No mutation occurs yet. `POST
/api/v1/assistant/confirm` accepts only that token and the originating session ID.
It cannot accept replacement parameters. Confirmation is not in the tool registry,
so the LLM cannot call it. A human/client must make the distinct confirmation
request. The token is unguessable, session-bound, single-use and consumed under a
thread lock before domain execution. C++ validates current state again on use.

Pending proposals last five minutes and are capped at 1000. Expired entries are
pruned during proposals. Restart invalidates pending proposals, which is safe but
requires the client to ask again. One API worker is required; multiple workers
would have independent proposal stores. There is no authentication, so session IDs
are correlation boundaries rather than proof of identity. This local prototype is
not an internet-facing authorization system.

`AI_WRITE_ACTIONS_ENABLED=false` blocks both new writes and confirmation of old
proposals. If a confirmed database operation fails, its token remains consumed;
the immutable request ID in the proposal permits deliberate diagnostic recovery,
but the assistant does not retry it automatically. Setting
`AI_REQUIRE_CONFIRMATION=false` explicitly disables high-risk confirmation for
trusted local experiments; write disable still takes precedence.

## Verification and references

Tests patch both live SDK constructors to fail, force Mock before settings load,
and inject SDK response doubles into adapter tests. They verify normalization,
error redaction, declarations, provider priority, invalid arguments, confirmation
expiry/replay/cross-session/concurrent consumption and disabled-write behavior.
The optional `scripts/live_ai_smoke.sh` is separate from pytest, requires
`RUN_LIVE_AI_TESTS=true`, and performs one interpretation request without a write.

SDK API references consulted during implementation:
- https://googleapis.github.io/python-genai/
- https://developers.openai.com/api/reference/python/resources/chat/subresources/completions/methods/create
- https://libpqxx.readthedocs.io/7.9.2/classpqxx_1_1params.html

Installed versions are pinned in `requirements-lock.txt`. These references explain
SDK contracts; they are not claims that a live provider call was executed here.
