"""HTTP security integration with identity/storage doubles, not PostgreSQL proof."""

from contextlib import contextmanager
from unittest.mock import Mock
import pytest
from fastapi.testclient import TestClient
from libracore_api.config import Settings
from libracore_api.services import DomainError
from libracore_api.v3.main import create_app
from libracore_api.v3.policy import Principal


class IdentityDouble:
    def __init__(self, role="Student"):
        self.p = Principal("U", "Asha Rao", role, "CS", "session", "hash")
        self.active = True

    def authenticate(self, token, csrf="", mutating=False):
        if not self.active or token != "valid":
            raise DomainError("UNAUTHENTICATED", "Sign in")
        if mutating and csrf != "csrf":
            raise DomainError("FORBIDDEN", "CSRF invalid")
        return self.p

    @contextmanager
    def guard(self, p):
        if not self.active:
            raise DomainError("UNAUTHENTICATED", "Revoked")
        yield self.p

    def login(self, email, password):
        return self.p, "valid", "csrf"

    def logout(self, p, all_sessions=False):
        self.active = False

    def ready(self):
        pass

    def close(self):
        pass


@pytest.fixture
def client():
    auth = IdentityDouble()
    core = Mock()
    core.execute.return_value = {"items": [], "limit": 50, "offset": 0}
    c = TestClient(
        create_app(Settings(_env_file=None, llm_provider="mock"), core, auth=auth),
        raise_server_exceptions=False,
    )
    c.cookies.set("libracore_session", "valid")
    c.headers.update({"Origin": "http://localhost:5173", "X-CSRF-Token": "csrf"})
    return c, core, auth


@pytest.mark.parametrize(
    "path", ["/api/v1/resources", "/api/v1/users", "/api/v1/reports/budget", "/api/v1/assistant/message"]
)
def test_requires_session(client, path):
    c, core, _ = client
    c.cookies.clear()
    assert c.get(path).status_code == 401
    core.execute.assert_not_called()


@pytest.mark.parametrize(
    "path", ["/api/v1/users", "/api/v1/vendors", "/api/v1/events", "/api/v1/reports/budget"]
)
def test_student_direct_denial(client, path):
    c, core, _ = client
    assert c.get(path).status_code == 403
    core.execute.assert_not_called()


@pytest.mark.parametrize(
    "headers",
    [
        {"Origin": "https://evil.invalid", "X-CSRF-Token": "csrf"},
        {"Origin": "http://localhost:5173", "X-CSRF-Token": "wrong"},
    ],
)
def test_csrf_and_origin(client, headers):
    c, core, _ = client
    c.headers.update(headers)
    assert (
        c.post("/api/v1/loans", json={"resource_id": "B", "borrower_id": "U", "request_id": "x"}).status_code
        == 403
    )
    core.execute.assert_not_called()


def test_missing_csrf(client):
    c, _, _ = client
    del c.headers["X-CSRF-Token"]
    assert c.post("/api/v1/auth/logout", json={}).status_code == 403


def test_ownership(client):
    c, core, _ = client
    assert (
        c.post(
            "/api/v1/loans", json={"resource_id": "B", "borrower_id": "other", "request_id": "x"}
        ).status_code
        == 403
    )
    core.execute.assert_not_called()


def test_safe_resource_metadata(client):
    c, core, _ = client
    core.execute.return_value = {"resource_id": "B", "vendors": [{"private": "offer"}]}
    r = c.get("/api/v1/resources/B")
    assert r.status_code == 200 and "vendors" not in r.json()


def test_logout_replay(client):
    c, _, _ = client
    assert c.post("/api/v1/auth/logout", json={}).status_code == 200
    c.cookies.set("libracore_session", "valid")
    assert c.get("/api/v1/resources").status_code == 401


def test_department_scope(client):
    c, core, auth = client
    auth.p = Principal("U", "Asha", "DepartmentManager", "CS", "session", "hash")
    assert c.get("/api/v1/reports/departments/EE").status_code == 403
    core.execute.assert_not_called()


def test_ai_student_cancel(client):
    c, core, _ = client
    r = c.post("/api/v1/assistant/message", json={"message": "Cancel order 1", "session_id": "forged"})
    assert r.status_code == 403
    core.execute.assert_not_called()


def test_ai_confirmation_bound_to_authenticated_session(client):
    c, core, auth = client
    auth.p = Principal("U", "Asha", "Administrator", "CS", "real", "hash")
    r = c.post(
        "/api/v1/assistant/message", json={"message": "Cancel order 1", "session_id": "attacker-chosen"}
    )
    assert r.status_code == 200
    token = r.json()["confirmations"][0]["token"]
    core.execute.assert_not_called()
    auth.p = Principal("V", "Other", "Administrator", "CS", "different", "hash")
    assert (
        c.post(
            "/api/v1/assistant/confirm", json={"token": token, "session_id": "attacker-chosen"}
        ).status_code
        == 409
    )


def test_readiness_and_headers(client):
    c, core, _ = client
    core.execute.return_value = {"database": "ok"}
    c.cookies.clear()
    r = c.get("/ready")
    assert r.status_code == 200
    assert r.headers["X-Content-Type-Options"] == "nosniff"
    assert r.headers["Cache-Control"] == "no-store"


def test_idempotency_namespaced(client):
    c, core, _ = client
    core.execute.return_value = {"loan_id": "L"}
    assert (
        c.post(
            "/api/v1/loans", json={"resource_id": "B", "borrower_id": "U", "request_id": "same"}
        ).status_code
        == 201
    )
    assert (
        core.execute.call_args.args[1]["request_id"]
        == __import__("hashlib").sha256(b"U").hexdigest() + ":same"
    )


@pytest.mark.parametrize("role", ["Student", "Faculty"])
def test_polish_purchase_refusal_explains_role(client, role):
    from dataclasses import replace

    c, core, auth = client
    auth.p = replace(auth.p, role=role)
    response = c.post(
        "/api/v1/assistant/message", json={"message": "Please buy 1 copy of B001", "session_id": "browser"}
    )
    assert response.status_code == 403
    assert "Purchasing is restricted" in response.json()["error"]["message"]
    core.execute.assert_not_called()


@pytest.mark.parametrize("role", ["Librarian", "DepartmentManager"])
def test_polish_staff_proposal_has_no_mutation(client, role):
    from dataclasses import replace
    from libracore_api.ai.providers import LLMResponse, ToolCall

    c, core, auth = client
    auth.p = replace(auth.p, role=role)
    c.app.state.assistant.provider = Mock()
    c.app.state.assistant.provider.interpret.return_value = LLMResponse(
        [ToolCall("submit_purchase", {"resource_id": "B001", "quantity": 1, "department_id": "CS"})]
    )
    response = c.post("/api/v1/assistant/message", json={"message": "purchase B001", "session_id": "browser"})
    assert response.status_code == 200
    assert len(response.json()["confirmations"]) == 1
    core.execute.assert_not_called()


@pytest.mark.parametrize(
    "role,allowed",
    [
        ("Administrator", True),
        ("Librarian", True),
        ("Student", False),
        ("Faculty", False),
        ("DepartmentManager", False),
    ],
)
def test_vendor_api_role_matrix(client, role, allowed):
    c, core, auth = client
    auth.p = Principal("U", "Asha", role, "CS", "session", "hash")
    response = c.get("/api/v1/vendors")
    assert response.status_code == (200 if allowed else 403)
    if not allowed:
        core.execute.assert_not_called()
