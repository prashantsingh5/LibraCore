# Testing and evidence

## Test layers

| Layer | Location | What it verifies |
|---|---|---|
| V1 domain | tests/test_*.cpp | Original assignment plus V1 domain regression coverage |
| V1 allocation failures | tests/fault_injection.cpp | Seven transaction scenarios, 58 allocation-failure points |
| Mapping | tests/v2/test_mapping.cpp | Six subtypes, Money, loan/record/time round trips |
| Direct PostgreSQL C++ | tests/v2/test_postgres.cpp | Real reconstruction, pricing and rejected-write invariants |
| Binding | python/tests/test_binding.py | Native dict conversion, six subtypes, Money, redacted connection failure |
| Configuration/provider/tools | python/tests/test_unit.py | Provider priority, schemas, SDK doubles, tool bounds, typed output, confirmations |
| API offline | python/tests/test_api.py | Endpoint dispatch, validation, structured errors, health, OpenAPI, assistant confirmation |
| Database / full stack | python/tests/test_database.py | Migrations, CRUD/reconstruction, search, budgets/quotas, transactions, races, API→C++→DB |

The offline API tests use a service double to test routing, not a fake replacement
for authoritative business rules. They do not count as database or end-to-end
verification. The database tests instantiate the real C++ binding and real
PostgreSQL connections.

## Normal commands

```bash
./scripts/setup_v2.sh
./scripts/test_v2.sh
```

The full script starts/checks Compose, builds all C++ targets, runs CTest and the
original test executables, requires the native extension to import, enables the
database test group, then runs pytest. Any failing command exits nonzero. Tests
force `LLM_PROVIDER=mock` and `RUN_LIVE_AI_TESTS=false`, even if `.env` has real keys.
The pytest autouse fixture blocks both external SDK constructors. SDK adapter
unit tests use injected response doubles, not live credentials.

If PostgreSQL cannot run in the current execution environment:

```bash
./scripts/test_offline.sh
```

This requires previously built V2 bindings. For a standalone V1 environment use
ordinary CMake with `LIBRACORE_V2` off. Direct `pytest` skips database cases unless
`LIBRACORE_RUN_DB_TESTS=1`; every skip explicitly states the environment blocker.
Do not interpret a green offline subset as a completed V2 release verification.

## Database safety

Tests use a separate DB named exactly `libracore_test`. The settings validate the
URL's database name and ensure it differs from development. Before dropping the
public schema, the reset helper independently checks `current_database()`.
Each database test resets and migrates the test schema and seeds deterministic
fixtures. Do not run tests or benchmarks in parallel against this same database.
Never point `DATABASE_URL` at the test DB: it would fail the isolation guard.

The full-stack test creates a FastAPI TestClient backed by the real binding. It
searches, quotes, purchases, borrows, returns partially and fully, reads reports
and events, uses Mock tools, confirms cancellation and checks persisted inventory.
Other tests open fresh repository instances to verify persistence, not cached
Python values.

## Failure and concurrency scenarios

For purchase, cancellation, borrow, return and atomic batch, tests install a
PostgreSQL trigger that raises during audit insertion, after earlier updates.
Snapshots compare inventory, budgets, category/title usage, loans, purchase
history, events, request receipts and counters before/after rollback. This tests
actual database atomicity rather than only pre-validation errors.

Separate connections synchronize on a thread barrier to race:

- Two borrowers against one remaining copy: exactly one succeeds.
- Two 700-minor purchases against a 1000-minor department budget: one succeeds.
- Two cancellations against one order: exactly one succeeds.
- Two purchases against a one-unit/title quota: one succeeds.
- Two identical request IDs: both receive success but inventory increments once.

Schema tests additionally attempt invalid relational state. Fiscal rollover,
closed-year cancellation, subtype metadata, currency price-sort rejection,
literal wildcard escaping and SQL-injection-shaped input have dedicated checks.

## Benchmarks

`./scripts/benchmark_v2.sh 10000` and `100000` create exactly that many additional
synthetic resources plus six fixed seed resources. Data creation time is separate
from query timings. Query results and transaction outcomes are asserted/consumed.
IDs derive from a unique numeric sequence and the resource PK enforces uniqueness.
The database benchmark reports per-call wall time including connection and binding
overhead. It does not measure the same operation mix as the V1 in-memory benchmark.

## Evidence status

See `V2_COMPLETION_REPORT.md` and `docs/verification-v2/` for actual outputs.
Database migration parsing and repository compilation are useful checks but are
not database execution. No persistent benchmark number is invented. Local WSL
execution must fill that verification gap before freezing V2.
