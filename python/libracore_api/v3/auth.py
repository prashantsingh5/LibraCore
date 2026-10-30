"""PostgreSQL identity transactions. SQL here concerns authentication only."""

from contextlib import contextmanager
from datetime import datetime, timezone, timedelta
from secrets import token_urlsafe
from uuid import uuid4
from typing import Any, Iterator
from psycopg import Connection
from psycopg.rows import dict_row
from psycopg_pool import ConnectionPool
from ..services import DomainError
from .policy import Principal
from .security import DUMMY, digest, hash_password, verify_password, check_csrf


class AuthStore:
    def __init__(self, url: str):
        self.pool: ConnectionPool[Connection[dict[str, Any]]] = ConnectionPool(
            url,
            min_size=0,
            max_size=4,
            timeout=5,
            kwargs={
                "row_factory": dict_row,
                "connect_timeout": 5,
                "options": "-c statement_timeout=30000 -c lock_timeout=10000",
            },
            open=True,
        )

    def ready(self) -> None:
        with self.pool.connection() as c:
            c.execute("SELECT session_id FROM auth_sessions LIMIT 1")

    def close(self) -> None:
        self.pool.close()

    def _principal(self, row: dict[str, Any]) -> Principal:
        return Principal(
            row["user_id"],
            row["name"],
            row["role"],
            row["department_id"] or "",
            str(row["session_id"]),
            row["token_hash"],
        )

    def _lock_user(self, c: Any, uid: str) -> None:
        # Serialize access changes while permitting the native transaction's
        # foreign-key KEY SHARE checks on loans/events referencing this user.
        c.execute("SELECT user_id FROM users WHERE user_id=%s FOR NO KEY UPDATE", (uid,)).fetchone()

    def _session(self, c: Any, hashed: str) -> dict[str, Any]:
        row = c.execute("SELECT user_id FROM auth_sessions WHERE token_hash=%s", (hashed,)).fetchone()
        if not row:
            raise DomainError("UNAUTHENTICATED", "Session expired; sign in again")
        self._lock_user(c, row["user_id"])
        row = c.execute(
            """SELECT u.*,s.session_id,s.token_hash,s.csrf_hash FROM auth_sessions s
            JOIN users u USING(user_id) WHERE s.token_hash=%s AND s.revoked_at IS NULL
            AND s.expires_at>now() AND s.last_used_at>now()-interval '30 minutes' AND u.active""",
            (hashed,),
        ).fetchone()
        if not row:
            raise DomainError("UNAUTHENTICATED", "Session expired; sign in again")
        c.execute("UPDATE auth_sessions SET last_used_at=now() WHERE token_hash=%s", (hashed,))
        return row

    def authenticate(self, token: str, csrf: str = "", mutating: bool = False) -> Principal:
        if not token or len(token) > 200:
            raise DomainError("UNAUTHENTICATED", "Sign in to continue")
        with self.pool.connection() as c:
            row = self._session(c, digest(token))
            if mutating:
                check_csrf(csrf, row["csrf_hash"])
            return self._principal(row)

    @contextmanager
    def guard(self, p: Principal) -> Iterator[Principal]:
        with self.pool.connection() as c:
            yield self._principal(self._session(c, p.token_hash))

    def login(self, email: str, password: str) -> tuple[Principal, str, str]:
        with self.pool.connection() as c:
            row = c.execute("SELECT user_id FROM users WHERE email=%s", (email.strip(),)).fetchone()
            if row:
                self._lock_user(c, row["user_id"])
            user = c.execute(
                "SELECT u.*,a.password_hash FROM users u JOIN auth_credentials a USING(user_id) WHERE email=%s",
                (email.strip(),),
            ).fetchone()
            valid = verify_password(user["password_hash"] if user else DUMMY, password)
            if not user or not valid:
                raise DomainError("UNAUTHENTICATED", "Email or password is incorrect.")
            if not user["active"]:
                raise DomainError("UNAUTHENTICATED", "This account is currently inactive. Contact your library administrator.")
            token = token_urlsafe(32)
            csrf = digest("csrf:" + token)
            sid = str(uuid4())
            c.execute(
                "INSERT INTO auth_sessions(session_id,user_id,token_hash,csrf_hash,expires_at) VALUES(%s,%s,%s,%s,%s)",
                (
                    sid,
                    user["user_id"],
                    digest(token),
                    digest(csrf),
                    datetime.now(timezone.utc) + timedelta(hours=8),
                ),
            )
            c.execute("INSERT INTO security_events(user_id,action) VALUES(%s,'login')", (user["user_id"],))
            return self._principal({**user, "session_id": sid, "token_hash": digest(token)}), token, csrf

    def logout(self, p: Principal, all_sessions: bool = False) -> None:
        with self.pool.connection() as c:
            self._lock_user(c, p.user_id)
            c.execute(
                "UPDATE auth_sessions SET revoked_at=now() WHERE user_id=%s AND (%s OR token_hash=%s)",
                (p.user_id, all_sessions, p.token_hash),
            )
            c.execute("INSERT INTO security_events(user_id,action) VALUES(%s,'logout')", (p.user_id,))

    def change_password(self, p: Principal, old: str, new: str) -> None:
        encoded = hash_password(new)
        with self.pool.connection() as c:
            self._session(c, p.token_hash)
            row = c.execute(
                "SELECT password_hash FROM auth_credentials WHERE user_id=%s", (p.user_id,)
            ).fetchone()
            if not row or not verify_password(row["password_hash"], old):
                raise DomainError("UNAUTHENTICATED", "Current password is incorrect")
            c.execute(
                "UPDATE auth_credentials SET password_hash=%s,changed_at=now() WHERE user_id=%s",
                (encoded, p.user_id),
            )
            c.execute("UPDATE auth_sessions SET revoked_at=now() WHERE user_id=%s", (p.user_id,))
            c.execute(
                "INSERT INTO security_events(user_id,action) VALUES(%s,'password_changed')", (p.user_id,)
            )

    def provision(self, uid: str, password: str) -> None:
        encoded = hash_password(password)
        with self.pool.connection() as c:
            self._lock_user(c, uid)
            c.execute(
                "INSERT INTO auth_credentials(user_id,password_hash) VALUES(%s,%s) ON CONFLICT(user_id) DO UPDATE SET password_hash=excluded.password_hash,changed_at=now()",
                (uid, encoded),
            )
            c.execute("UPDATE auth_sessions SET revoked_at=now() WHERE user_id=%s", (uid,))
            c.execute("INSERT INTO security_events(user_id,action) VALUES(%s,'credentials_reset')", (uid,))

    def manage(self, p: Principal, uid: str, role: str, department: str, active: bool) -> None:
        # Stable order avoids opposing administrator updates deadlocking users.
        with self.pool.connection() as c:
            for key in sorted({p.user_id, uid}):
                self._lock_user(c, key)
            fresh = self._session(c, p.token_hash)
            if fresh["role"] != "Administrator":
                raise DomainError("FORBIDDEN", "Administrator permission required")
            if uid == p.user_id and (not active or role != "Administrator"):
                raise DomainError("CONFLICT", "Use another administrator to change your own access")
            row = c.execute(
                "UPDATE users SET role=%s,department_id=NULLIF(%s,''),active=%s WHERE user_id=%s RETURNING user_id",
                (role, department, active, uid),
            ).fetchone()
            if not row:
                raise DomainError("NOT_FOUND", "User not found")
            c.execute("UPDATE auth_sessions SET revoked_at=now() WHERE user_id=%s", (uid,))
            c.execute("INSERT INTO security_events(user_id,action) VALUES(%s,'access_changed')", (uid,))

    def admin_provision(self, p: Principal, uid: str, password: str) -> None:
        encoded = hash_password(password)
        with self.pool.connection() as c:
            for key in sorted({p.user_id, uid}):
                self._lock_user(c, key)
            fresh = self._session(c, p.token_hash)
            if fresh["role"] != "Administrator":
                raise DomainError("FORBIDDEN", "Administrator permission required")
            c.execute(
                "INSERT INTO auth_credentials(user_id,password_hash) VALUES(%s,%s) ON CONFLICT(user_id) DO UPDATE SET password_hash=excluded.password_hash,changed_at=now()",
                (uid, encoded),
            )
            c.execute("UPDATE auth_sessions SET revoked_at=now() WHERE user_id=%s", (uid,))
            c.execute("INSERT INTO security_events(user_id,action) VALUES(%s,'credentials_reset')", (uid,))

    def security_events(self, p: Principal, before: int = 0) -> list[dict[str, Any]]:
        with self.pool.connection() as c:
            fresh = self._session(c, p.token_hash)
            if fresh["role"] != "Administrator":
                raise DomainError("FORBIDDEN", "Administrator permission required")
            return list(
                c.execute(
                    "SELECT event_id,user_id,action,occurred_at FROM security_events WHERE (%s=0 OR event_id<%s) ORDER BY event_id DESC LIMIT 50",
                    (before, before),
                ).fetchall()
            )
