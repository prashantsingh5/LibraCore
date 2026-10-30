import uvicorn
from ..config import Settings


def main():
    try:
        settings = Settings()
    except Exception:
        raise SystemExit("Invalid configuration; credentials were not logged") from None
    uvicorn.run(
        "libracore_api.v3.main:create_app",
        factory=True,
        host=settings.api_host,
        port=settings.api_port,
        workers=1,
        proxy_headers=False,
        access_log=False,
    )


if __name__ == "__main__":
    main()
