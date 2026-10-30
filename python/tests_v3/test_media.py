"""Media validation/storage and secured HTTP routes, with explicit DB doubles."""

from io import BytesIO
from unittest.mock import Mock
import pytest
from PIL import Image
from fastapi.testclient import TestClient
from libracore_api.config import Settings
from libracore_api.services import DomainError
from libracore_api.v3.media import LocalMediaStorage, CoverStore, validated_cover, MAX_BYTES
from libracore_api.v3.main import create_app
from test_http import IdentityDouble


def png():
    out = BytesIO()
    Image.new("RGB", (20, 30), "green").save(out, format="PNG")
    return out.getvalue()


def test_cover_reencoded_and_unique(tmp_path):
    content = validated_cover(png(), "My Book.PNG", "image/png")
    with Image.open(BytesIO(content)) as im:
        assert im.format == "WEBP"
    store = LocalMediaStorage(str(tmp_path))
    a, b = store.put(content), store.put(content)
    assert a != b and store.path(a).read_bytes() == content
    store.remove(a)
    assert not store.path(a).exists()


@pytest.mark.parametrize(
    "name,mime",
    [
        ("a.svg", "image/svg+xml"),
        ("a.exe", "image/png"),
        ("../a.png", "image/png"),
        ("..\\a.png", "image/png"),
        ("a.jpg", "image/jpeg"),
    ],
)
def test_bad_upload(tmp_path, name, mime):
    with pytest.raises(DomainError):
        validated_cover(png(), name, mime)
    assert not list(tmp_path.iterdir())


def test_oversized_and_invalid():
    for data in [b"x" * (MAX_BYTES + 1), b"not-an-image"]:
        with pytest.raises(DomainError):
            validated_cover(data, "book.png", "image/png")


def test_path_traversal_and_symlink(tmp_path):
    store = LocalMediaStorage(str(tmp_path))
    for key in ["../../etc/passwd", "abc.png", "/tmp/abc.webp"]:
        with pytest.raises(DomainError):
            store.path(key)
    key = "a" * 32 + ".webp"
    (tmp_path / key).symlink_to(tmp_path.parent / "elsewhere")
    with pytest.raises(DomainError):
        store.path(key)


def test_failed_metadata_write_removes_new_file(tmp_path):
    pool = Mock()
    pool.connection.side_effect = RuntimeError("database unavailable")
    store = CoverStore(pool, LocalMediaStorage(str(tmp_path)))
    with pytest.raises(RuntimeError):
        store.replace("B", validated_cover(png(), "b.png", "image/png"))
    assert not list(tmp_path.iterdir())


class MemoryCovers:
    def __init__(self, path):
        self.storage = LocalMediaStorage(str(path))
        self.keys = {}

    def key(self, rid):
        return self.keys.get(rid)

    def replace(self, rid, content):
        self.keys[rid] = self.storage.put(content)

    def clear(self, rid):
        self.keys.pop(rid, None)


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
def test_cover_api_rbac_and_lifecycle(tmp_path, role, allowed):
    auth = IdentityDouble(role)
    core = Mock()
    core.execute.return_value = {"resource_id": "B", "resource_type": "Book"}
    covers = MemoryCovers(tmp_path)
    c = TestClient(create_app(Settings(_env_file=None, llm_provider="mock"), core, auth=auth, covers=covers))
    c.cookies.set("libracore_session", "valid")
    c.headers.update({"Origin": "http://localhost:5173", "X-CSRF-Token": "csrf"})
    assert c.get("/api/v1/resources/B/cover").headers["content-type"].startswith("image/svg+xml")

    def upload(name="cover.png", body=None):
        return c.put(
            "/api/v1/resources/B/cover",
            content=png() if body is None else body,
            headers={"content-type": "image/png", "x-filename": name},
        )

    response = upload()
    assert response.status_code == (200 if allowed else 403)
    if not allowed:
        assert not covers.keys
        assert c.delete("/api/v1/resources/B/cover").status_code == 403
        return
    old = covers.key("B")
    assert c.get("/api/v1/resources/B/cover").headers["content-type"].startswith("image/webp")
    assert upload().status_code == 200 and covers.key("B") != old
    assert upload("../cover.png").status_code == 422
    assert upload(body=b"x" * (MAX_BYTES + 1)).status_code == 422
    assert c.delete("/api/v1/resources/B/cover").status_code == 200
    assert covers.key("B") is None
    assert c.get("/api/v1/resources/B/cover").headers["content-type"].startswith("image/svg+xml")
    c.headers["X-CSRF-Token"] = "wrong"
    assert upload().status_code == 403
