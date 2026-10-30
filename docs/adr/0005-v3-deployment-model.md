# ADR 0005 — Single-node production-style containers

## Context
The target local environment is WSL2 plus Docker Desktop. V3 needs reproducible builds and a credible deployment path without unnecessary distributed infrastructure.

## Decision
Multi-stage native backend image, static frontend in unprivileged nginx, PostgreSQL Compose service, one-shot migrations, health checks and one backend worker. Bind the web service to localhost behind an operator-configured HTTPS terminator. CI tests against a PostgreSQL service and builds images without deploying them.

## Alternatives considered
Kubernetes, service mesh and multiple services would exceed the task's modular-monolith boundary. Shipping plaintext public HTTP would contradict Secure-cookie authentication. Automatic public deployment would require real host/domain credentials and explicit operator setup.

## Why selected
This keeps builds reproducible while making external TLS, backup and runtime privilege decisions explicit.

## Consequences
The operator supplies HTTPS and secrets. Single-node rate limiting and confirmation state constrain scaling. Work-mode inability to execute Docker is an evidence gap, not permission to mark container tests passed.

## Future implications
Move proposal/rate storage out of process before multiple workers; replace the coarse domain lock only with the full concurrency regression suite.
