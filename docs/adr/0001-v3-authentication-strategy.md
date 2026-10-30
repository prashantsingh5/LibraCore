# ADR 0001 — Server-side opaque sessions

## Context
V2 persistent users were borrower metadata, not authenticated identities. A browser application needs revocation, secret handling and safe cookie-based access.

## Decision
Use Argon2id credentials and opaque random sessions with hash-only PostgreSQL storage. Use HttpOnly/SameSite cookies, Secure host-prefixed cookies in production, exact Origin checks and session-bound CSRF headers. Revalidate stored user state; serialize revocations against in-flight protected operations with user-row locks.

## Alternatives considered
JWT browser storage complicates revocation and exposes bearer credentials to script storage. External identity providers would add account/hosting dependencies beyond the user's V3 scope. Signed client-side sessions would require another revocation mechanism.

## Why selected
Opaque sessions support immediate server revocation and a small explicit security model. Separating credentials from V2 user rows prevents accidental hash serialization.

## Consequences
A database lookup is required for authenticated requests. Pools reduce connection churn. Password recovery is administrative; no public email flow is invented.

## Future implications
Institutional SSO or MFA can issue the same session contract later. Add retention jobs and audit policy before larger deployments.
