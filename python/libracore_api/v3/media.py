"""Validated presentation media; no inventory or financial authority lives here."""

from io import BytesIO
from pathlib import Path
from typing import Protocol, Any
from uuid import uuid4
import re
import warnings
from PIL import Image, ImageOps, UnidentifiedImageError
from ..services import DomainError

MAX_BYTES = 5 * 1024 * 1024
MAX_PIXELS = 16_000_000


class MediaStorage(Protocol):
    def put(self, content: bytes) -> str: ...
    def path(self, key: str) -> Path: ...
    def remove(self, key: str) -> None: ...


class LocalMediaStorage:
    def __init__(self, root: str):
        self.root = Path(root).resolve()

    def path(self, key: str) -> Path:
        if not re.fullmatch(r"[a-f0-9]{32}\.webp", key):
            raise DomainError("INVALID_INPUT", "Invalid cover identifier")
        target = self.root / key
        if target.is_symlink() or target.resolve().parent != self.root:
            raise DomainError("INVALID_INPUT", "Invalid cover path")
        return target

    def put(self, content: bytes) -> str:
        self.root.mkdir(parents=True, exist_ok=True)
        key = uuid4().hex + ".webp"
        # Exclusive creation; key is never taken from a client filename.
        with self.path(key).open("xb") as output:
            output.write(content)
        return key

    def remove(self, key: str) -> None:
        self.path(key).unlink(missing_ok=True)


def validated_cover(content: bytes, filename: str, content_type: str) -> bytes:
    if len(content) > MAX_BYTES:
        raise DomainError("INVALID_INPUT", "Cover must be 5 MB or smaller")
    extensions = {".jpg": "JPEG", ".jpeg": "JPEG", ".png": "PNG", ".webp": "WEBP"}
    suffix = Path(filename).suffix.lower()
    if (
        not filename
        or len(filename) > 200
        or any(c in filename for c in ("/", "\\", "\x00"))
        or suffix not in extensions
        or content_type not in {"image/jpeg", "image/png", "image/webp"}
    ):
        raise DomainError("INVALID_INPUT", "Choose a JPG, PNG or WEBP cover with a plain filename")
    try:
        with warnings.catch_warnings():
            warnings.simplefilter("error", Image.DecompressionBombWarning)
            with Image.open(BytesIO(content)) as source:
                if source.format != extensions[suffix] or source.width * source.height > MAX_PIXELS:
                    raise ValueError("Invalid image dimensions or format")
                source.load()
                image = ImageOps.exif_transpose(source).convert("RGB")
                image.thumbnail((1600, 2000))
                output = BytesIO()
                image.save(output, format="WEBP", quality=85)
                return output.getvalue()  # Re-encode: remove metadata, trailing payloads and animation.
    except (
        UnidentifiedImageError,
        OSError,
        ValueError,
        Image.DecompressionBombError,
        Image.DecompressionBombWarning,
    ):
        raise DomainError("INVALID_INPUT", "This file is not a valid supported cover image") from None


class CoverStore:
    def __init__(self, pool: Any, storage: MediaStorage):
        self.pool, self.storage = pool, storage

    def key(self, resource_id: str) -> str | None:
        with self.pool.connection() as c:
            row = c.execute(
                "SELECT storage_key FROM resource_covers WHERE resource_id=%s", (resource_id,)
            ).fetchone()
            return row["storage_key"] if row else None

    def replace(self, resource_id: str, content: bytes) -> str:
        key = self.storage.put(content)
        try:
            with self.pool.connection() as c:
                c.execute(
                    "INSERT INTO resource_covers(resource_id,storage_key) VALUES(%s,%s) ON CONFLICT(resource_id) DO UPDATE SET storage_key=excluded.storage_key,updated_at=now()",
                    (resource_id, key),
                )
        except Exception:
            self.storage.remove(key)
            raise
        # Old immutable files retained for in-flight reads; operator garbage collection.
        return key

    def clear(self, resource_id: str) -> None:
        with self.pool.connection() as c:
            c.execute("DELETE FROM resource_covers WHERE resource_id=%s", (resource_id,))
