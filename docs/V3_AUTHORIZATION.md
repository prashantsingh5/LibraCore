# Authorization and ownership

The source of truth is `v3/policy.py`: `Permission`, `ROLE_POLICY`, `OPERATIONS` and `AuthorizationService`. The UI uses the server-supplied permission list for presentation, but hiding a link grants no security. Every protected native operation goes through the same server-side check.

| Role | Effective scope |
|---|---|
| Student / Faculty | Read catalog/availability; borrow for self, return own loans and view own history; read-oriented assistant |
| Librarian | Catalog, all lending, vendors, acquisitions, budget read, operational reports and domain events |
| DepartmentManager | Personal lending plus acquisitions, budget configuration/read within the assigned department |
| Administrator | All listed operations, user/department administration and authentication resets |

Department managers cannot access the default budget, another department's budgets, its orders or its cancellation flow. For orders, scope is obtained from the persisted `requested_department`; caller-provided fields cannot override it. Atomic/independent batches must have every request in the manager's assigned department. Managers with no assigned department are denied protected financial operations.

Borrower ownership is checked against a stored loan for reads/returns. Students and faculty cannot impersonate another borrower or choose artificial borrow, due or return timestamps. Librarians and administrators can manage lending. No expanded Faculty borrowing policy was invented.

For API mutations, idempotency request IDs are prefixed with a SHA-256 user namespace, preventing two users choosing the same key from receiving each other's saved receipts. This namespace does not change the native idempotency algorithm. A caller must reuse the same request ID to retry a request whose response was lost.

## AI enforcement

Tool declarations are filtered before provider invocation. Even a provider that ignores the declarations must pass the server policy before a proposal or call. Student/Faculty assistant tools are read-only, while lending remains available in the normal UI. Unknown tools, arbitrary SQL and confirmation tools are not exposed.

The authenticated session UUID replaces the request body's legacy `session_id` as the confirmation boundary. Client session strings remain accepted for contract compatibility but do not establish identity. High-risk confirmation is always enabled in V3. Tokens remain five-minute, bounded, mutex-protected, single-use proposals with immutable arguments. Confirming reauthenticates, reauthorizes and invokes C++ against current state. A role change between proposal and confirmation cannot preserve stale permissions.

No transcript persistence was added. Avoiding unnecessary user prompts in durable storage is an intentional V3 privacy decision; a future opt-in retention design can be added separately.

## Polish safeguards

Both `/users` and legacy `/administration` display the same 403 state for unauthorized roles. Server permissions remain authoritative. Cover writes reuse `resource.manage`; cover reads reuse `catalog.read`. The early Student/Faculty purchase-message check improves error wording only: it is a narrow UX heuristic, not an authorization engine. Typed-tool filtering, confirmation authentication and department restrictions remain the final authority.
