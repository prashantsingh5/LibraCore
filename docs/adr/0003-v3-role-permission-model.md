# ADR 0003 — Central operations policy with object scope

## Context
Five V2 role strings exist but grant no security. Role-aware navigation alone cannot protect direct API requests.

## Decision
One permission enum and role/operation mapping, default deny. Check loan ownership and department scope from stored domain records. Return permissions to the UI for presentation only. Namespace browser idempotency IDs by authenticated user.

## Alternatives considered
Inline role comparisons in each route are easy to omit. A configurable policy language would add complexity without a demonstrated requirement. PostgreSQL RLS alone cannot enforce all current domain workflows through the aggregate repository.

## Why selected
A shared AuthorizedService guards both existing HTTP routes and AI tools without duplicating native rules.

## Consequences
New operations require a deliberate permission entry. New scopes require tests. Administrator account operations have separate identity transactions with role revalidation.

## Future implications
A future fine-grained permission model can replace role defaults without changing frontend authority. Re-run the direct API denial matrix on every policy change.
