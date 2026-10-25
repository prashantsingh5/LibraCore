# API and binding contracts

Run `./scripts/demo_v2.sh`; interactive schemas live at `/docs` and `/openapi.json`.
All list routes use `limit` (1–200, default 50) and `offset` (≥0). Resource listings
return `{items, limit, offset}`. Other paged histories return arrays. Fields are
validated with Pydantic; unknown body/tool fields are rejected.

## Endpoint inventory

| Method / route | Contract / C++ operation |
|---|---|
| GET /health | API, binding, DB and configured AI status; no live LLM call |
| GET /api/v1/resources | Search: title, author, identifier, category, from_year, to_year, sort, currency, q, limit, offset |
| GET /api/v1/resources/{resource_id} | Full resource metadata and price |
| GET /api/v1/resources/{resource_id}/availability | holdings, borrowed, available |
| POST /api/v1/resources | Create typed resource; starts with zero holdings |
| PUT /api/v1/resources/{resource_id} | Update metadata/price; ID must match; type/currency immutable |
| DELETE /api/v1/resources/{resource_id} | Only unheld, unreferenced resources can be deleted |
| POST /api/v1/acquisitions/quote | resource_id, quantity, optional department_id/vendor_selection; returns cost |
| POST /api/v1/acquisitions/check | Same input; returns cost, allowed, reason |
| POST /api/v1/acquisitions | Same plus request_id; returns approved or rejected immutable record |
| POST /api/v1/acquisitions/batch | requests array, atomic flag, request_id |
| GET /api/v1/acquisitions/{order_id} | Original status plus separate current_status |
| POST /api/v1/acquisitions/{order_id}/cancel | order_id and request_id; path/body must match |
| POST /api/v1/loans | resource_id, borrower_id, quantity, request_id, optional borrowed_at/due_at |
| GET /api/v1/loans/{loan_id} | Persisted loan including returned quantity and status |
| POST /api/v1/loans/{loan_id}/return | loan_id, quantity, request_id, optional returned_at |
| GET /api/v1/users/{user_id}/loans | Paged borrower loans |
| POST /api/v1/users | user_id, name, email, role, optional department_id |
| POST /api/v1/departments | department_id, name |
| POST /api/v1/budgets | budget_id, fiscal_year, currency, total_minor, optional department_id |
| PUT /api/v1/budgets/quota | department_id, category, max_units, max_spend, optional max_titles |
| POST /api/v1/budgets/rollover | department_id, fiscal_year, new_allocation, carry_percent, request_id |
| PUT /api/v1/vendor-offers | resource_id, vendor_id, name, price_minor, currency, rating, estimated_delivery_days |
| GET /api/v1/reports/budget | Default budget/category/utilization summaries |
| GET /api/v1/reports/departments/{department_id} | Department summary with category quotas |
| GET /api/v1/reports/acquisitions | Approved/rejected/cancelled counts, net money totals |
| GET /api/v1/reports/lending | Holdings, borrowed, available, active loan count |
| GET /api/v1/events | Paged immutable event history, newest first |
| POST /api/v1/assistant/message | message, session_id; tool calls/results or confirmation proposal |
| POST /api/v1/assistant/confirm | token, session_id; no replacement action/parameters accepted |

## Values and examples

Categories: `0=Book`, `1=ElectronicResource`, `2=Journal`, `3=EBook`; search `-1`
means any. Acquisition status: `0=Requested`, `1=Approved`, `2=Rejected`,
`3=Cancelled`. Loan status: `0=Active`, `1=Returned`. Record type: `0=Purchase`,
`1=Cancellation`. Event type: `0=Purchase`, `1=Cancellation`, `2=Borrow`, `3=Return`.
Vendor selection: `Cheapest`, `Fastest`, `HighestRated`.
Search sort: `id`, `title_asc`, `title_desc`, `year_asc`, `year_desc`, `price_asc`,
`price_desc`, `rank`.

```json
{"resource_id":"B001","quantity":2,"vendor_selection":"Cheapest","request_id":"client-generated-unique-id"}
```

An application price is structured:

```json
{"minor_units":18000,"currency":"INR","formatted":"INR 180.00"}
```

Use timezone-aware ISO 8601 dates. API offsets normalize to UTC; direct C++
application dates require UTC. C++ mapping stores seconds and accepts years
1970–2200 to avoid platform time-point overflow. Omitted dates use the server clock
and persistent default loan duration.

Submitting an unaffordable purchase returns a persisted `approved=false` record
rather than pretending a successful purchase. The HTTP creation status refers to
creation of the request/history record. Quote/check do not mutate inventory.
A cancellation appends a record; the original order's `status` never changes.
`current_status` reflects cancellation separately.

## Errors

```json
{"error":{"code":"CONFLICT","message":"Duplicate entity or request","details":{}}}
```

Missing entity → 404; conflict/quota/budget → 409; malformed domain input → 422;
request shape → 422; unknown operation → 400; unavailable DB → 503;
provider error → 502; quota response → 429; disabled AI writes → 403;
unexpected error → 500 with a generic message. V1 raises `invalid_argument` for
some availability/return conflicts; these map to `INVALID_INPUT`/422. API handlers
never return SQL text, connection credentials, raw SDK exceptions or tracebacks.
Server logs contain route templates, request IDs, status and duration, not bodies
or secrets.

## pybind11

`_libracore.Application(database_url).execute(operation, parameters)` accepts and
returns Python-native dictionaries/lists. C++ releases the GIL for database/domain
execution. Its envelope is `{ok: true, data: ...}` or `{ok: false, error: ...}`.
`CoreService` converts the error envelope to a typed Python `DomainError`.
Operation names mirror the documented tools/routes. Additional administration
operations available through the binding: `configure` (print_tax,
electronic_tax, loan_days), `set_user_active`, `get_user` and bounded
`import_resources` (1–1000 resource definitions per atomic call).

The binding also exposes `money(minor, currency)` and `validate_resource` for
mapping tests. It exposes no raw SQL, internal mutable Catalog, budget setters,
loan counters or persistence adapter methods to Python. Roles are stored but
there is no authentication. Keep the API bound to trusted localhost.
