# Authentication and session lifecycle

Accounts are provisioned by administrators; there is no public signup. Existing V2 user records gain no password automatically. Upgrading V2 preserves borrowers and roles; an operator must bootstrap an administrator and provision credentials.

Passwords use Argon2id with 64 MiB memory, three iterations and one lane. Passwords must contain 12–128 characters. A dummy hash is verified for an unknown account to reduce account enumeration timing differences; unknown accounts and incorrect passwords receive the same generic error. Only after a correct password is verified does an inactive account receive “Account is inactive. Please contact an administrator.” No session is created for it. No password or encoded hash is returned by an API.

On successful login, the server generates 32 random bytes encoded as an opaque URL-safe token. PostgreSQL stores only its SHA-256 hash. The browser receives the token in an HttpOnly, SameSite=Strict cookie; production adds Secure and the `__Host-` prefix, Path=/ and no Domain attribute. Authentication tokens never enter localStorage.

A session has an eight-hour absolute lifetime and a 30-minute idle timeout. Authentication checks expiry, revocation, user activity and current role on the server. `auth/me` returns public user information and a CSRF token derived with a domain-separated SHA-256 input from the opaque session secret. The DB stores only the CSRF verifier. Learning the CSRF token does not reveal the cookie. The frontend retains CSRF only in memory and refreshes it using `auth/me` after reload.

Every browser mutation requires an exact trusted Origin. All mutations except login additionally require the session-bound `X-CSRF-Token`. Login uses strict Origin checking to prevent login CSRF before a session exists. Cross-origin credentialed CORS is not enabled; Vite proxies development `/api` traffic to preserve same-origin semantics.

Logout revokes the current session; logout-all and password changes revoke all user sessions. A password change requires the old password. Administrative role/department/activity or credential changes also revoke all target sessions. Self-demotion/deactivation is rejected so an administrator cannot accidentally remove their own administrative access from this screen.

## Tables in migration 003

- `auth_credentials`: user FK, Argon2id hash and changed timestamp.
- `auth_sessions`: UUID, user FK, token hash, CSRF hash, created/expiry/last-used/revoked timestamps; indexes on token, user and expiry.
- `security_events`: user FK, action and occurrence time. It stores no passwords, tokens, prompts or credential payloads.

Authentication persistence is separate from public V2 `users` rows. Consequently existing `get_user`/list queries cannot accidentally expose hashes through `to_jsonb(users)`.

## Operational lifecycle

Pending assistant proposals are invalidated by logout because they are bound to the authenticated session UUID and every confirmation reauthenticates. Session records are durable across API restarts; unconfirmed proposals are not. Expired/revoked sessions remain as metadata until operator-controlled retention cleanup. No background deletion daemon is introduced in V3.

References: [OWASP session management](https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html), [OWASP CSRF prevention](https://cheatsheetseries.owasp.org/cheatsheets/Cross-Site_Request_Forgery_Prevention_Cheat_Sheet.html), [argon2-cffi](https://argon2-cffi.readthedocs.io/).

Domain calls hold a `FOR NO KEY UPDATE` lock on the authenticated user while rechecking the session. This serializes revocation/access changes without blocking the C++ transaction's `KEY SHARE` foreign-key checks when inserting loans or events. A dedicated real-PostgreSQL test covers borrowing under this guard; it must run locally before release approval.
