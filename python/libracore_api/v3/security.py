"""Small pure security primitives; no application or domain state."""

import hashlib
import hmac
import secrets
import time
from collections import deque
from threading import Lock
from argon2 import PasswordHasher, Type
from argon2.exceptions import VerificationError, InvalidHashError
from ..services import DomainError

HASHER = PasswordHasher(time_cost=3, memory_cost=65536, parallelism=1, type=Type.ID)
DUMMY = HASHER.hash(secrets.token_urlsafe(32))


def digest(token: str) -> str:
    return hashlib.sha256(token.encode()).hexdigest()


def hash_password(password: str) -> str:
    if len(password) < 12 or len(password) > 128:
        raise DomainError("INVALID_INPUT", "Password must contain 12–128 characters")
    return HASHER.hash(password)


def verify_password(encoded: str, password: str) -> bool:
    try:
        return HASHER.verify(encoded, password)
    except (VerificationError, InvalidHashError):
        return False


def check_csrf(token: str, expected_hash: str) -> None:
    if not token or not hmac.compare_digest(digest(token), expected_hash):
        raise DomainError("FORBIDDEN", "CSRF token missing or invalid")


class RateLimiter:
    """Bounded single-process sliding windows, no trusted client-supplied forwarded IP."""

    def __init__(self, capacity: int = 10000):
        self.capacity = capacity
        self.entries: dict[str, deque[float]] = {}
        self.lock = Lock()

    def check(self, key: str, count: int, window: int = 60) -> None:
        with self.lock:
            now = time.monotonic()
            self.entries = {k: v for k, v in self.entries.items() if v and v[-1] > now - window}
            q = self.entries.setdefault(key, deque())
            while q and q[0] <= now - window:
                q.popleft()
            if len(q) >= count or len(self.entries) > self.capacity:
                raise DomainError("RATE_LIMITED", "Too many requests; try again later")
            q.append(now)
