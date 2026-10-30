# V3 API changes

Product V3 continues to use `/api/v1`. This is an intentional security extension: legacy callers must now authenticate and provide Origin/CSRF for mutations. The C++ binding and its named operations remain available to trusted local programs; authentication does not pretend to sandbox native code running on the host.

## Authentication

| Method | Path | Contract |
|---|---|---|
| POST | `/auth/login` | email/password → public user, CSRF token, HttpOnly cookie |
| GET | `/auth/me` | current public user/permissions and CSRF token |
| POST | `/auth/logout` | revoke current session |
| POST | `/auth/logout-all` | revoke all sessions |
| POST | `/auth/change-password` | current_password/new_password; revoke all sessions |

## Administrative and browse extensions

- `GET /users`, `/departments`, `/vendors`, `/loans`, `/acquisitions`: bounded lists with existing Page semantics (1–200, default 50). Acquisitions accepts a department filter, mandatory scope for managers.
- `PUT /users/{id}/access`: role, department_id and active; admin-only and revokes target sessions.
- `POST /users/{id}/credentials`: new password, admin-only; revokes all target sessions.
- `PUT /vendors/{id}`: validated name, rating and estimated_delivery_days; explicit metadata update without rewriting historical purchase snapshots.
- `GET /security-events?before=ID`: admin-only identity event list, newest IDs first, up to 50 records.
- `GET /events/feed?before=ID&limit=20`: cursor domain event feed with `items` and `next_cursor`.
- `GET /resources?sort=id&after=ID`: keyset page. Requires zero offset and ID ordering; response adds nullable `next_cursor`. All other filters remain AND-combined. `available_only=true` filters available holdings.

V2 `/events` offset ordering is preserved. V2 `session_id` remains accepted on assistant bodies but is ignored as an identity source; the server's authenticated session is authoritative.

Quotes retain `cost` and add `pre_tax`, `tax`, `selected_vendor`, `budget_before`; checks additionally expose `budget_after` when simulation succeeds. All financial values come from C++ Money/Budget/AcquisitionManager. No pre-tax formula was duplicated in Python or JavaScript.

`/live` is process liveness. `/ready` and `/health` check native/database readiness and V3 auth schema availability, returning 503 when unavailable. They do not call Gemini/OpenAI. Operational deployment should use `/ready`; no sensitive DB details are returned.

New errors retain `{error:{code,message,details}}`: UNAUTHENTICATED=401, FORBIDDEN=403 and RATE_LIMITED=429. Each response carries `X-Request-ID`, including security rejections. Input validation responses list invalid field locations without echoing credentials.

## Polish additions (backwards compatible)

- `GET /api/v1/resources/{resource_id}/cover`: protected Book image or bundled default; missing media falls back safely.
- `PUT` at the same path: raw raster body with MIME Content-Type and URL-encoded `X-Filename`; catalog-write authorization and CSRF required. Returns success metadata.
- `DELETE` at the same path: remove the metadata reference and restore the default.
- Resource creation accepts strict nonnegative integer `initial_holdings` (default zero). Nonzero opening stock on update is rejected. Stock is already-owned inventory, **not an acquisition**; no budget debit or invented purchase occurs.
- Native search `identifier` matches resource ID/ISBN/ISSN. General `q` also matches exact identifiers, literal title/publisher substrings and existing full-text title/author/publisher search. Exact resource ID ranks first; requested sort and ID tie-break remain stable. Ranked search uses offset, not an ID cursor.
- Loan and event results include resource titles. Acquisition lists include derived `current_status` and `cancellation_order_id`; original purchase records remain immutable. Quote/check results include title/quantity and formatted selected-vendor price. Budget summaries identify fiscal year and department.

These are structured additive fields; clients do not need to parse console strings. See [engineering notes](V3_POLISH_ENGINEERING.md).
