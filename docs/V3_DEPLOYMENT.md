# Production deployment model

V3 is a single-node modular monolith: nginx static frontend/reverse proxy, one FastAPI worker with native C++ binding, and PostgreSQL. There is no Redis, queue, microservice split or Kubernetes dependency.

The backend multi-stage image compiles C++17 on Ubuntu 24.04, copies the native extension and Python environment into a runtime image and runs as UID 10001. The frontend builds Vite assets in Node 22, then serves them with unprivileged nginx. Backend capabilities are dropped and its filesystem is read-only with a temporary `/tmp` mount. Build context excludes secrets and development artifacts.

## Prepare configuration

Use a private `.env` with a unique POSTGRES_PASSWORD, `LIBRACORE_ENV=production` and `PUBLIC_ORIGIN=https://your-real-host`. Configure an HTTPS edge to proxy that host to `127.0.0.1:8080`; do not publish the plaintext port to the internet. TLS certificate provisioning belongs to the deployment host. Set model/key configuration only if you want a live provider; Mock needs no external key.

```bash
docker compose -f docker-compose.prod.yml build
docker compose -f docker-compose.prod.yml up -d
# Bootstrap one administrator interactively; prompts without logging the password:
docker compose -f docker-compose.prod.yml exec backend python -m libracore_api.v3.bootstrap --admin-id library-admin --name 'Library Administrator' --email admin@your-real-host
```

The one-shot migration service waits for PostgreSQL, applies checksummed migrations, then permits backend startup. Readiness checks verify the native application and auth tables. Nginx starts after backend health. Production does not seed demo accounts or financial data automatically. For a fresh database, an administrator must create departments/default and department budgets and catalog resources using the application. Preserve a V2 backup when upgrading.

Use an institutional DB role with only the privileges needed at runtime and a separate migration role where operationally practical. Compose provides a simple local baseline, not managed production backup/secret rotation infrastructure. Volumes are never deleted by the supplied scripts. Back up and restore PostgreSQL using your standard database procedures before upgrading; keep V2/V3 code and matching DB backup together for rollback.

Pending confirmations and rate windows disappear on restart. Existing server sessions persist. Run exactly one API worker until the proposal/rate-limit stores are made multi-process. Configure host monitoring, HTTPS, backup restore testing and log retention before public deployment.

Docker image builds and live production startup were not executed in this Work environment; see the completion matrix. No cloud deployment or repository push was performed.

## Cover media durability

The production backend has a writable named `libracore_media` volume at `/app/var/media`; Compose pins `MEDIA_ROOT` to that mount while the rest of the filesystem remains read-only. The image creates the directory owned by UID 10001. Development runs the backend on the host and stores covers at `var/media` by default; this directory survives normal startup/shutdown. It is excluded from Git, Docker build context and the source archive.

Back up the media directory together with PostgreSQL cover metadata, and restore both with matching versions. Removing a cover restores the default immediately but retains its old immutable file for in-flight readers. Replaced files are not automatically garbage-collected in this release. Monitor disk use; maintenance cleanup must preserve keys referenced by `resource_covers` and allow a grace period. Do not use `docker compose down -v` on data you intend to keep. Docker runtime/volume ownership checks remain part of final local regression.
