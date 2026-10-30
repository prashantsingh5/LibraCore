"""Default application runner now uses the secure V3 entry point."""
from .v3.run import main
if __name__ == "__main__":
    main()
