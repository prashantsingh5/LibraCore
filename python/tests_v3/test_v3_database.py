"""Real PostgreSQL tests. Never silently passed without an isolated DB."""

import os
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch
import pytest
import psycopg
from libracore_api.config import Settings, ROOT
from libracore_api.database import reset_test, migrate
from libracore_api.services import CoreService, DomainError
from libracore_api.seed import seed
from libracore_api.v3.auth import AuthStore
from libracore_api.v3.security import digest

pytestmark = pytest.mark.database


@pytest.fixture
def real():
    if os.environ.get("LIBRACORE_RUN_DB_TESTS") != "1":
        pytest.skip("NOT EXECUTED — ENVIRONMENT BLOCKED: Docker PostgreSQL required")
    s = Settings(ai_enabled=False)
    reset_test(s)
    core = CoreService(s.db_url(True))
    seed(core)
    store = AuthStore(s.db_url(True))
    store.provision("U001", "Temporary test phrase 2026")
    yield s, core, store
    store.close()


def test_fresh_and_idempotent_migrations(real):
    s, _, _ = real
    migrate(s.db_url(True))
    with psycopg.connect(s.db_url(True)) as c:
        assert c.execute("SELECT count(*) FROM schema_migrations").fetchone()[0] >= 3
        assert c.execute("SELECT count(*) FROM auth_credentials").fetchone()[0] == 1


def test_upgrade_preserves_v2(real):
    s, _, _ = real
    with psycopg.connect(s.db_url(True)) as c:
        c.execute("DROP SCHEMA public CASCADE")
        c.execute("CREATE SCHEMA public")
    with TemporaryDirectory() as temp:
        root = Path(temp)
        dest = root / "database/migrations"
        dest.mkdir(parents=True)
        for name in ["001_initial_schema.sql", "002_indexes.sql"]:
            (dest / name).write_bytes((ROOT / "database/migrations" / name).read_bytes())
        with patch("libracore_api.database.ROOT", root):
            migrate(s.db_url(True))
    core = CoreService(s.db_url(True))
    seed(core)
    before = core.execute("get_resource", {"resource_id": "B001"})
    migrate(s.db_url(True))
    assert core.execute("get_resource", {"resource_id": "B001"}) == before


@pytest.mark.parametrize("action", ["expired", "revoked", "inactive", "idle"])
def test_invalid_sessions(real, action):
    s, _, a = real
    p, token, csrf = a.login("u001@example.invalid", "Temporary test phrase 2026")
    with psycopg.connect(s.db_url(True)) as c:
        queries = {
            "expired": "UPDATE auth_sessions SET created_at=now()-interval '2 days',expires_at=now()-interval '1 day'",
            "revoked": "UPDATE auth_sessions SET revoked_at=now()",
            "inactive": "UPDATE users SET active=false",
            "idle": "UPDATE auth_sessions SET last_used_at=now()-interval '1 hour'",
        }
        c.execute(queries[action])
    with pytest.raises(DomainError):
        a.authenticate(token)


def test_hash_only_and_csrf(real):
    s, _, a = real
    p, t, csrf = a.login("u001@example.invalid", "Temporary test phrase 2026")
    with psycopg.connect(s.db_url(True)) as c:
        row = c.execute("SELECT token_hash,csrf_hash FROM auth_sessions").fetchone()
        assert row == (digest(t), digest(csrf))
    assert a.authenticate(t, csrf, True).user_id == "U001"
    with pytest.raises(DomainError):
        a.authenticate(t, "wrong", True)


def test_wrong_password_and_change_revokes_all(real):
    _, _, a = real
    with pytest.raises(DomainError):
        a.login("u001@example.invalid", "wrong")
    p, t, _ = a.login("u001@example.invalid", "Temporary test phrase 2026")
    _, other, _ = a.login("u001@example.invalid", "Temporary test phrase 2026")
    a.change_password(p, "Temporary test phrase 2026", "Changed long password 2026")
    for token in [t, other]:
        with pytest.raises(DomainError):
            a.authenticate(token)
    assert a.login("u001@example.invalid", "Changed long password 2026")[0].user_id == "U001"


def test_concurrent_password_changes(real):
    _, _, a = real
    p, _, _ = a.login("u001@example.invalid", "Temporary test phrase 2026")

    def change(i):
        try:
            a.change_password(p, "Temporary test phrase 2026", "Changed password " + str(i))
            return True
        except DomainError:
            return False

    with ThreadPoolExecutor(2) as pool:
        assert sum(pool.map(change, range(2))) == 1


def test_revocation_waits_for_inflight_guard(real):
    _, _, a = real
    p, t, _ = a.login("u001@example.invalid", "Temporary test phrase 2026")
    with ThreadPoolExecutor(1) as pool:
        with a.guard(p):
            future = pool.submit(a.logout, p, True)
            assert not future.done()
        future.result(timeout=10)
    with pytest.raises(DomainError):
        a.authenticate(t)


def test_cursor_pages_and_pool_concurrency(real):
    _, core, _ = real
    first = core.execute("search_resources", {"limit": 2, "sort": "id"})
    second = core.execute("search_resources", {"limit": 2, "after": first["next_cursor"]})
    assert set(r["resource_id"] for r in first["items"]).isdisjoint(r["resource_id"] for r in second["items"])
    with ThreadPoolExecutor(12) as pool:
        assert all(pool.map(lambda _: core.execute("health_check", {})["database"] == "ok", range(36)))


def test_native_v3_executable(real):
    import subprocess

    s, _, _ = real
    subprocess.run(
        [str(ROOT / os.environ.get("LIBRACORE_BUILD_DIR", "build-v3") / "libracore_v3_db_tests")],
        env={**os.environ, "TEST_DATABASE_URL": s.db_url(True)},
        check=True,
    )


def test_vendor_update_preserves_history(real):
    _, core, _ = real
    original = core.execute("get_order", {"order_id": 1})
    result = core.execute(
        "update_vendor",
        {"vendor_id": "V001", "name": "Economy Research Books", "rating": 4.3, "estimated_delivery_days": 4},
    )
    assert result["name"] == "Economy Research Books"
    assert core.execute("get_order", {"order_id": 1}) == original


def test_guard_allows_native_foreign_key_inserts(real):
    """The auth row lock must not block native loan/event foreign-key checks."""
    from libracore_api.v3.policy import AuthorizedService, current

    _, core, auth = real
    principal, _, _ = auth.login("u001@example.invalid", "Temporary test phrase 2026")
    token = current.set(principal)
    try:
        result = AuthorizedService(core, auth).execute(
            "borrow_resource",
            {"resource_id": "B001", "borrower_id": "U001", "quantity": 1, "request_id": "guard-borrow"},
        )
        assert result["borrower_id"] == "U001"
    finally:
        current.reset(token)


def test_polish_search_opening_stock_and_cover_migration(real):
    s, core, _ = real
    core.execute(
        "create_resource",
        {
            "resource_id": "TEST-LIB-001",
            "resource_type": "Book",
            "title": "Librarian Manual Test",
            "publisher": "Research House",
            "isbn": "978-test-polish",
            "authors": ["Asha Rao"],
            "unit_price_minor": 10000,
            "currency": "INR",
            "initial_holdings": 3,
        },
    )
    assert core.execute("get_availability", {"resource_id": "TEST-LIB-001"})["available"] == 3
    for args in [
        {"q": "TEST-LIB-001"},
        {"identifier": "TEST-LIB-001"},
        {"q": "978-test-polish"},
        {"q": "Librarian Manual Test"},
        {"q": "Research House"},
        {"q": "Librarian", "author": "Asha"},
    ]:
        assert core.execute("search_resources", args)["items"][0]["resource_id"] == "TEST-LIB-001"
    with psycopg.connect(s.db_url(True)) as c:
        assert c.execute("SELECT count(*) FROM resource_covers").fetchone()[0] == 0
    with pytest.raises(DomainError):
        core.execute(
            "create_resource",
            {
                "resource_id": "BAD-STOCK",
                "resource_type": "Book",
                "title": "Bad",
                "unit_price_minor": 1,
                "currency": "INR",
                "initial_holdings": -1,
            },
        )
    with pytest.raises(DomainError):
        core.execute("get_resource", {"resource_id": "BAD-STOCK"})


def test_upgrade_current_v3_keeps_stock_loans_and_migrations(real):
    s, core, _ = real
    # Simulate the pre-polish schema by removing only the new empty table and
    # its migration receipt in the isolated test DB. V1/V2/V3 rows are retained.
    before = core.execute("get_resource", {"resource_id": "B001"})
    with psycopg.connect(s.db_url(True)) as c:
        c.execute("DROP TABLE resource_covers")
        c.execute("DELETE FROM schema_migrations WHERE version LIKE '004%'")
    migrate(s.db_url(True))
    assert core.execute("get_resource", {"resource_id": "B001"}) == before
    with psycopg.connect(s.db_url(True)) as c:
        assert c.execute("SELECT count(*) FROM resource_covers").fetchone()[0] == 0


def test_cover_metadata_real_database_replace_and_clear(real, tmp_path):
    from libracore_api.v3.media import CoverStore, LocalMediaStorage

    _, _, auth = real
    media = CoverStore(auth.pool, LocalMediaStorage(str(tmp_path)))
    assert media.key("B001") is None
    first = media.replace("B001", b"fixture-storage-bytes")
    second = media.replace("B001", b"replacement-storage-bytes")
    assert first != second and media.key("B001") == second
    media.clear("B001")
    assert media.key("B001") is None
