# V3 security review

V3 establishes application security boundaries; it is not a claim of an independent penetration test or internet deployment approval.

## Controls implemented

- Argon2id password hashing and hash-only opaque session storage.
- HttpOnly/SameSite cookies; Secure and `__Host-` in production.
- Exact Origin checks and session-bound CSRF header validation.
- Central default-deny operation permissions, stored ownership checks and department restrictions.
- Server-selected AI tool subset, post-interpretation authorization and authenticated-session confirmation binding.
- Password/access changes revoke sessions; user-row locks order in-flight work against revocation.
- Login limit 10/minute by direct peer; authenticated writes 60/minute/user; assistant 20/minute/user. Sliding windows are bounded and process-local.
- Parameterized database statements. Raw DB/provider exceptions are not sent to clients.
- Structured logs contain UTC timestamp, generated request ID, authenticated user ID, route template, HTTP method, status and elapsed milliseconds. They do not contain prompts, query values, credentials, cookies, CSRF tokens or `.env` contents.
- CSP, no-sniff, frame restrictions, referrer and permissions policies. The reverse proxy has a 128 KiB request-body cap; its CSP allows inline styles for React/chart positioning, but no inline scripts/eval.
- Production runtime rejects a non-HTTPS origin and the default development database password. Database ports are not published by production Compose; frontend binds localhost behind the operator's TLS edge.

## Deployment assumptions

One API worker is required because rate windows and pending AI proposals are in process memory. Session hashes are persistent, but proposal state intentionally disappears on restart. Direct clients and production nginx share the same origin; no wildcard credentialed CORS is configured. The backend does not trust arbitrary forwarded headers.

Behind nginx, direct-peer login limiting is conservatively shared by the proxy's requests. This may throttle a busy installation; it is preferable to trusting forgeable forwarded IPs. A future trusted-proxy policy can improve per-client limits. This V3 deployment is single-node, not multi-replica.

The reverse proxy listens on localhost HTTP port 8080 for an existing HTTPS terminator. Deploying it publicly without TLS is unsupported; production Secure cookies intentionally will not work over plain HTTP. Configure TLS, backups, database privileges, log retention and a strong password before external exposure.

## Boundaries and residual risks

A browser with XSS can issue same-origin requests even with HttpOnly cookies; CSP and React escaping reduce exposure but are not a substitute for input/output review. There is no MFA, email recovery, public registration or authentication federation. Credentials are reset by an authorized administrator. Global write serialization, growing history/session tables and a single worker bound capacity.

Original V2 routes are preserved in a legacy factory for regression testing. The only deployable entry point is `libracore_api.v3.main:create_app` (or the default runner that delegates to it). No V3 script starts the unprotected factory.

Normal test suites force Mock and block live SDK constructors. No real uploaded key was required or used. The source archive excludes `.env`, virtual environments, build outputs and browser traces that could contain test passwords.

## Optional covers

Only authenticated catalog readers can fetch Book covers; only catalog managers can upload/revert them. Mutations retain CSRF, Origin and fresh authorization checks. Uploads are streamed with a 5 MiB cap, require JPG/JPEG/PNG/WEBP names and supported MIME types, and are decoded with Pillow. Actual image format must match the extension and dimensions must not exceed 16 million pixels. Decoded pixels are re-encoded to bounded WebP, stripping metadata/trailing data and animation. Client filenames never become storage paths; exclusive UUID filenames, a strict key pattern and resolved-path/symlink checks protect storage. The trusted bundled fallback is SVG; user SVG uploads are rejected.

Resource and media saves are separate transactions. If resource creation succeeds but its optional cover fails, the UI says so and allows retry against the created resource. A failed metadata commit removes the new file; old immutable files remain available to concurrent readers. Filesystem/process crashes can still leave unreferenced files, so operational retention/backup remains necessary.
