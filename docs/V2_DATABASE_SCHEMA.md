# PostgreSQL storage

## Migration lifecycle

`python -m libracore_api.database` connects using typed environment settings and
runs sorted SQL migrations in one PostgreSQL transaction. `schema_migrations`
records filenames and SHA-256 checksums. An advisory transaction lock serializes
migration runners. An already applied file with changed contents fails rather
than silently mutating history. Empty database initialization needs only these
migrations; seed data is separate. Migration administration uses psycopg; runtime
domain data access uses libpqxx exclusively.

`001_initial_schema.sql` creates relational state and initial counters.
`002_indexes.sql` enables PostgreSQL's bundled `pg_trgm` extension and query
indexes. Compose's database owner has the required extension privileges.

## Tables and mapping decisions

| Table | Purpose and key fields | Integrity / ownership |
|---|---|---|
| app_state | Singleton ID, next order/loan, taxes, default loan days | ID=1; positive counters and validated configuration; repository unit-of-work lock |
| resources | Resource ID/type/category, title/publisher/year, currency, unit price, holdings/lent, timestamps | PK; nonblank ID; nonnegative BIGINT price; 0 ≤ lent ≤ holdings; unique ID/currency |
| resource_details | One typed metadata row per resource | PK/FK cascading with resource; ISBN/ISSN, edition/binding, subscription fields, URL/license, platform fee/currency, format/DRM, narrator/duration, thesis fields |
| authors | Stable numeric author ID and unique name | Identity PK; resource reconstruction does not parse joined author strings |
| resource_authors | Ordered many-author relationships | Resource/position PK; author FK; preserves order and even repeated authors allowed by V1 |
| vendors | Vendor ID/name/rating/delivery days | PK; finite range 0–5 and nonnegative days; globally consistent metadata |
| vendor_offers | Resource/vendor price/currency | Composite PK, vendor FK, resource/currency composite FK prevents mixed-currency offers |
| users | User ID, name, email, role, optional department, active, created timestamp | PK, unique email, role CHECK, department FK; C++ rejects inactive borrowers |
| departments | ID/name, active, creation time | PK, unique name; finance separate from shared resource inventory |
| budgets | Budget ID, optional department, fiscal year, currency, total/spent | PK, department FK and uniqueness; partial unique default-budget index; 0 ≤ spent ≤ total |
| budget_quotas | Category unit/spend/title limits | Budget/category PK; optional title limit means unlimited; nonnegative limits |
| budget_usage | Category units/spend | Budget/category PK; nonnegative usage |
| budget_title_usage | Category/resource unit counts | Composite PK; resource FK; positive units, deleted when usage becomes zero |
| loans | ID, resource/borrower, original/returned quantities, UTC dates and status | FKs; positive quantity; return bounds; due after borrow; consistency between status/return date/outstanding |
| purchase_records | Submitted resource/department, quantity, title/category, amounts/currency, status/reason, type, original order link, policy/fiscal year, vendor snapshot and timestamp | Order PK; nullable resolved FKs permit rejected missing-entity requests; related-order FK; unique cancellation per original order |
| acquisition_requests | Request ID, operation, canonical input and immutable result receipt | PK; transactional idempotency, not authoritative financial state |
| audit_events | Event ID/type, resource, quantity, actor, optional user/department FKs, reference and timestamp | Identity PK, positive quantity, resource/identity FKs; append-oriented |
| budget_rollovers | Budget/from/to years, prior total/spent, new allocation/currency and timestamp | Budget FK; unique budget/from-year; persistent closed-year boundary |
| schema_migrations | Version and checksum | PK; deployment tooling only |

The request's **queryable** resource/department, status, rejection reason, quantity,
selected vendor policy and price are ordinary `purchase_records` columns. The
receipt table covers a whole application operation, including multi-item batches,
without duplicating each item's authoritative record. `vendor_snapshot` JSONB
preserves the immutable offer metadata used for an order even if a future offer
changes. These and the idempotency receipt are the only flexible JSON documents.

Common resource metadata and one typed details table avoid six nearly identical
metadata schemas. Unused subtype columns have harmless defaults; reconstruction
selects by `resource_type`. V1 category semantics remain: Thesis → Book;
AudioBook → ElectronicResource. All monetary values use BIGINT minor units;
ratings and utilization are not authoritative monetary values.

## Indexes and search semantics

Primary keys cover ID lookups and composite offer/budget keys. Extra indexes cover
lowercase title/author trigrams, publication year, category, ISBN/ISSN, reverse
author relationships, borrower/resource loans, active loans, purchase department/
resource and descending event time. GIN tsvector indexes cover title+publisher and
authors. SQL combines ordinary filters with AND. `%`, `_` and backslash are escaped
for literal substring matching; all user values remain parameters.

Full-text `q` uses `plainto_tsquery('simple', q)`, matching title/publisher or an
author. `rank` sorts by title/publisher `ts_rank`, then resource ID; author-only
matches can have zero title rank. This is lexical full-text search, not semantic
embedding retrieval. Exact ISBN/ISSN lookup remains separate. V1 substring search
is case-insensitive; PostgreSQL's Unicode/collation handling may differ from V1's
byte-oriented case folding. Every sort has an ID tie-breaker. Price sort checks
all matched currencies before pagination and rejects mixed currencies.

## Integrity and access boundaries

C++ validates business rules and PostgreSQL validates relationships/checks.
No SQL accepts caller-selected table or ORDER BY identifiers. The ORDER BY fragment
is chosen from a fixed C++ map. `jsonb_populate_record` receives a bound JSON value
and a compile-time table type; it is not dynamic SQL. Application DB credentials
are powerful local development credentials; do not expose the server or API to
untrusted clients. Schema editing outside the application can violate invariants
that require several tables; migrations and test fixtures are the only supported
out-of-band writes.
