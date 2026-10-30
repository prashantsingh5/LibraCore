"""Explicit, destructive TEST-DB-only performance probe; no invented timings."""

import sys
import time
import statistics
from concurrent.futures import ThreadPoolExecutor
from ..config import Settings
from ..database import reset_test
from ..seed import seed
from ..services import CoreService


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 10000
    if count not in {10000, 100000, 1000000}:
        raise SystemExit("Choose 10000, 100000 or 1000000")
    settings = Settings(ai_enabled=False)
    reset_test(settings)
    service = CoreService(settings.db_url(True))
    seed(service)
    start = time.perf_counter()
    for base in range(0, count, 1000):
        service.execute(
            "import_resources",
            {
                "resources": [
                    {
                        "resource_id": f"PERF-{i:09}",
                        "resource_type": "Book",
                        "title": f"Computer Science Research {i}",
                        "authors": [f"Researcher {i % 100}"],
                        "unit_price_minor": 10000,
                        "currency": "INR",
                    }
                    for i in range(base, min(base + 1000, count))
                ]
            },
        )
    print(f"dataset={count}; generation_ms={(time.perf_counter() - start) * 1000:.3f}")
    for name, args in [
        ("title", {"title": "Research"}),
        ("author", {"author": "Researcher 10"}),
        ("combined", {"title": "Research", "author": "Researcher 10"}),
        ("offset", {"offset": count - 100}),
        ("cursor", {"after": f"PERF-{count - 100:09}"}),
    ]:
        samples = []
        for _ in range(10):
            start = time.perf_counter()
            result = service.execute("search_resources", args)
            samples.append((time.perf_counter() - start) * 1000)
            assert result["items"]
        print(f"{name}: median_ms={statistics.median(samples):.3f}, max_ms={max(samples):.3f}")
    start = time.perf_counter()
    with ThreadPoolExecutor(12) as pool:
        results = list(pool.map(lambda _: service.execute("health_check", {}), range(100)))
    assert all(r["database"] == "ok" for r in results)
    print(f"pool_100_reads_12_threads_ms={(time.perf_counter() - start) * 1000:.3f}")


if __name__ == "__main__":
    main()
