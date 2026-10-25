import uvicorn
from .config import Settings
from .main import create_app
if __name__ == '__main__':
    try:
        settings = Settings()
        app = create_app(settings)
    except Exception:
        raise SystemExit('Startup failed. Check required build dependencies, provider/model selection and local configuration; no secrets logged.') from None
    uvicorn.run(app, host=settings.api_host, port=settings.api_port)
