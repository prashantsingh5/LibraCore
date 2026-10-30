"""Secure V3 entry point. V2 factory remains available for frozen contract tests only."""

import json
import logging
import time
from datetime import datetime, timezone
from uuid import uuid4
from contextlib import asynccontextmanager
from typing import Literal
from fastapi import Request, Depends, Query
from fastapi.responses import JSONResponse, FileResponse
from pathlib import Path
from urllib.parse import unquote
from .media import CoverStore, LocalMediaStorage, validated_cover, MAX_BYTES
from starlette.concurrency import run_in_threadpool
from pydantic import Field
from ..main import create_app as legacy_app, STATUS
from ..config import Settings
from ..services import CoreService, DomainError
from .. import schemas as s
from .auth import AuthStore
from .policy import AuthorizedService, current, require_principal, Permission
from .security import RateLimiter, digest


class Login(s.Input):
    email: str = Field(min_length=3, max_length=254)
    password: str = Field(min_length=1, max_length=128)


class Password(s.Input):
    current_password: str = Field(min_length=1, max_length=128)
    new_password: str = Field(min_length=12, max_length=128)


class Reset(s.Input):
    password: str = Field(min_length=12, max_length=128)


class VendorUpdate(s.Input):
    name: s.Id
    rating: float = Field(ge=0, le=5, allow_inf_nan=False)
    estimated_delivery_days: int = Field(ge=0, le=2147483647)


class Access(s.Input):
    role: Literal["Student", "Faculty", "Librarian", "DepartmentManager", "Administrator"]
    department_id: str = ""
    active: bool = True


def public_user(p):
    return {
        "user_id": p.user_id,
        "name": p.name,
        "role": p.role,
        "department_id": p.department_id,
        "permissions": sorted(p.permissions),
    }


def create_app(settings=None, service=None, provider=None, auth=None, covers=None):
    settings = settings or Settings()
    production = settings.libracore_env == "production"
    if production and not settings.public_origin.startswith("https://"):
        raise RuntimeError("Production requires an HTTPS PUBLIC_ORIGIN")
    if (
        production
        and settings.postgres_password.get_secret_value() == "libracore_dev_password"
        and not settings.database_url.get_secret_value()
    ):
        raise RuntimeError("Production requires a non-default database password")
    settings = settings.model_copy(update={"ai_require_confirmation": True})
    raw = service or CoreService(settings.db_url())
    store = auth or AuthStore(settings.db_url())
    secured = AuthorizedService(raw, store)
    app = legacy_app(settings, secured, provider)
    app.title = "LibraCore V3"
    app.version = "3.0.0"
    app.state.auth = store
    covers = covers or CoverStore(getattr(store, "pool", None), LocalMediaStorage(settings.media_root))
    limiter = RateLimiter()
    cookie = "__Host-libracore" if production else "libracore_session"
    origins = {settings.public_origin}
    if not production:
        origins |= {"http://127.0.0.1:5173", "http://localhost:8000", "http://127.0.0.1:8000"}
    STATUS.update({"UNAUTHENTICATED": 401, "FORBIDDEN": 403, "RATE_LIMITED": 429})

    @asynccontextmanager
    async def lifespan(_):
        yield
        store.close()

    app.router.lifespan_context = lifespan

    @app.middleware("http")
    async def security(request: Request, call_next):
        token = None
        started = time.perf_counter()
        rid = str(uuid4())
        uid = None
        try:
            path = request.url.path
            mutate = request.method not in {"GET", "HEAD", "OPTIONS"}
            public = path in {"/health", "/live", "/ready", "/api/v1/auth/login"}
            if mutate:
                origin = request.headers.get("origin")
                if origin not in origins:
                    raise DomainError("FORBIDDEN", "A trusted Origin header is required")
            if path == "/api/v1/auth/login":
                limiter.check("login:" + (request.client.host if request.client else "unknown"), 10, 60)
            if not public:
                p = await run_in_threadpool(
                    store.authenticate,
                    request.cookies.get(cookie, ""),
                    request.headers.get("x-csrf-token", ""),
                    mutate,
                )
                token = current.set(p)
                uid = p.user_id
                if mutate:
                    limiter.check("write:" + p.user_id, 60)
                if "/assistant/" in path:
                    limiter.check("assistant:" + p.user_id, 20)
                if path.startswith("/api/v1/assistant") and not settings.ai_enabled:
                    raise DomainError("AI_DISABLED", "AI is disabled")
            response = await call_next(request)
        except DomainError as e:
            response = JSONResponse(
                status_code=STATUS.get(e.code, 500),
                content={"error": {"code": e.code, "message": e.message, "details": {}}},
            )
        except Exception:
            response = JSONResponse(
                status_code=503,
                content={
                    "error": {
                        "code": "UNAVAILABLE",
                        "message": "Service temporarily unavailable",
                        "details": {},
                    }
                },
            )
        finally:
            if token is not None:
                current.reset(token)
        response.headers["X-Request-ID"] = rid
        logging.getLogger("libracore.security").info(
            json.dumps(
                {
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    "request_id": rid,
                    "user_id": uid,
                    "method": request.method,
                    "route": getattr(request.scope.get("route"), "path", "unmatched"),
                    "status": response.status_code,
                    "duration_ms": round((time.perf_counter() - started) * 1000, 3),
                }
            )
        )
        response.headers.update(
            {
                "Cache-Control": "no-store",
                "X-Content-Type-Options": "nosniff",
                "Referrer-Policy": "same-origin",
                "X-Frame-Options": "DENY",
                "Permissions-Policy": "camera=(), microphone=(), geolocation=()",
                "Content-Security-Policy": "default-src 'self'; frame-ancestors 'none'; object-src 'none'; base-uri 'self'",
            }
        )
        return response

    def cover_resource(resource_id, write=False):
        principal = require_principal()
        with store.guard(principal) as fresh:
            secured.policy.check(fresh, "update_resource" if write else "get_resource", {}, raw)
            resource = raw.execute("get_resource", {"resource_id": resource_id})
            if resource["resource_type"] != "Book":
                raise DomainError("INVALID_INPUT", "Cover uploads are supported for Books")
        return resource

    @app.get("/api/v1/resources/{resource_id}/cover")
    def cover_get(resource_id: str):
        cover_resource(resource_id)
        key = covers.key(resource_id)
        path = covers.storage.path(key) if key else None
        if path is None or not path.is_file():
            return FileResponse(
                Path(__file__).parent / "assets/default-cover.svg", media_type="image/svg+xml"
            )
        return FileResponse(path, media_type="image/webp")

    @app.put("/api/v1/resources/{resource_id}/cover")
    async def cover_upload(resource_id: str, request: Request):
        await run_in_threadpool(cover_resource, resource_id, True)
        data = bytearray()
        async for chunk in request.stream():
            if len(data) + len(chunk) > MAX_BYTES:
                raise DomainError("INVALID_INPUT", "Cover must be 5 MB or smaller")
            data.extend(chunk)
        image = await run_in_threadpool(
            validated_cover,
            bytes(data),
            unquote(request.headers.get("x-filename", "")),
            request.headers.get("content-type", ""),
        )

        def save():
            with store.guard(require_principal()) as fresh:
                secured.policy.check(fresh, "update_resource", {}, raw)
                resource = raw.execute("get_resource", {"resource_id": resource_id})
                if resource["resource_type"] != "Book":
                    raise DomainError("INVALID_INPUT", "Cover uploads are supported for Books")
                covers.replace(resource_id, image)
            return {"ok": True, "message": "Book cover updated"}

        return await run_in_threadpool(save)

    @app.delete("/api/v1/resources/{resource_id}/cover")
    def cover_remove(resource_id: str):
        with store.guard(require_principal()) as fresh:
            secured.policy.check(fresh, "update_resource", {}, raw)
            raw.execute("get_resource", {"resource_id": resource_id})
            covers.clear(resource_id)
        return {"ok": True, "message": "Default cover restored"}

    @app.get("/live")
    def live():
        return {"status": "alive"}

    @app.get("/ready")
    def ready():
        try:
            raw.execute("health_check", {})
            store.ready()
            return {"database": "ready", "native": "ready", "ai": settings.provider}
        except Exception:
            return JSONResponse(status_code=503, content={"status": "not_ready"})

    # Replace legacy health dependency, which is deliberately not publicly authorized.
    for route in app.routes:
        if getattr(route, "path", None) == "/health":
            route.endpoint = ready
            route.dependant.call = ready

    @app.post("/api/v1/auth/login")
    def login(body: Login):
        p, token, csrf = store.login(body.email, body.password)
        response = JSONResponse({"user": public_user(p), "csrf_token": csrf})
        response.set_cookie(
            cookie, token, httponly=True, secure=production, samesite="strict", path="/", max_age=28800
        )
        return response

    @app.get("/api/v1/auth/me")
    def me(request: Request):
        return {
            "user": public_user(require_principal()),
            "csrf_token": digest("csrf:" + request.cookies.get(cookie, "")),
        }

    @app.post("/api/v1/auth/logout")
    def logout():
        store.logout(require_principal())
        response = JSONResponse({"ok": True})
        response.delete_cookie(cookie, path="/")
        return response

    @app.post("/api/v1/auth/logout-all")
    def logout_all():
        store.logout(require_principal(), True)
        response = JSONResponse({"ok": True})
        response.delete_cookie(cookie, path="/")
        return response

    @app.post("/api/v1/auth/change-password")
    def password(body: Password):
        store.change_password(require_principal(), body.current_password, body.new_password)
        response = JSONResponse({"ok": True})
        response.delete_cookie(cookie, path="/")
        return response

    @app.get("/api/v1/events/feed")
    def events_feed(before: int = Query(0, ge=0), limit: int = Query(20, ge=1, le=200)):
        return secured.execute("get_events_cursor", {"before": before, "limit": limit})

    @app.get("/api/v1/security-events")
    def security_events(before: int = Query(0, ge=0)):
        return store.security_events(require_principal(), before)

    @app.get("/api/v1/users")
    def users(page: s.Page = Depends()):
        return secured.execute("list_users", page.model_dump())

    @app.put("/api/v1/users/{uid}/access")
    def access(uid: str, body: Access):
        store.manage(require_principal(), uid, body.role, body.department_id, body.active)
        return {"ok": True}

    @app.post("/api/v1/users/{uid}/credentials")
    def reset(uid: str, body: Reset):
        p = require_principal()
        with store.guard(p) as fresh:
            if Permission.USER not in fresh.permissions:
                raise DomainError("FORBIDDEN", "Administrator permission required")
        # Provision rechecks caller while taking ordered user locks.
        store.admin_provision(p, uid, body.password)
        return {"ok": True}

    @app.get("/api/v1/departments")
    def departments(page: s.Page = Depends()):
        return secured.execute("list_departments", page.model_dump())

    @app.put("/api/v1/vendors/{vendor_id}")
    def vendor_update(vendor_id: str, body: VendorUpdate):
        return secured.execute("update_vendor", {"vendor_id": vendor_id, **body.model_dump()})

    @app.get("/api/v1/vendors")
    def vendors(page: s.Page = Depends()):
        return secured.execute("list_vendors", page.model_dump())

    @app.get("/api/v1/loans")
    def loans(page: s.Page = Depends()):
        return secured.execute("list_loans", page.model_dump())

    @app.get("/api/v1/acquisitions")
    def orders(department_id: str = "", page: s.Page = Depends()):
        return secured.execute("list_orders", {"department_id": department_id, **page.model_dump()})

    return app
