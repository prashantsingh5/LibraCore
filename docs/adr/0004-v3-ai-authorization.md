# ADR 0004 — AI uses the authenticated application boundary

## Context
V2 has provider-neutral tools and high-risk confirmation, but its client session ID was correlation rather than identity.

## Decision
Filter tool declarations by principal, authorize every returned call, bind proposals to the authenticated session UUID and recheck permission at confirmation. Force high-risk confirmation in V3. Preserve Mock/Gemini/OpenAI and existing typed tools.

## Alternatives considered
Trusting the model to follow role instructions is not authorization. Replacing the provider layer with LangChain/LangGraph would not solve an unmet V3 need. Persisting full transcripts would collect unnecessary sensitive content.

## Why selected
The deterministic authorization and C++ execution paths remain authoritative. There is no AI SQL access or self-confirmation route.

## Consequences
Single-process proposal storage requires one worker and loses pending proposals on restart. Students/faculty get read-only assistant behavior even though ordinary lending UI remains enabled.

## Future implications
A durable multi-worker proposal store needs transactional token consumption and explicit retention policy. It can be introduced without changing domain calculations.
