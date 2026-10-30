"""Explicit local administration: no public registration and no shipped passwords."""

import argparse
import getpass
import os
from ..config import Settings
from ..services import CoreService, DomainError
from ..seed import seed
from .auth import AuthStore

ACCOUNTS = [
    ("student", "Asha Rao", "Student"),
    ("faculty", "Dr. Meera Shah", "Faculty"),
    ("librarian", "Nikhil Verma", "Librarian"),
    ("manager", "Priya Menon", "DepartmentManager"),
    ("admin", "Arjun Sen", "Administrator"),
]


def demo(settings, test=False):
    if settings.libracore_env == "production":
        raise RuntimeError("Demo provisioning is prohibited in production")
    password = os.environ.get("LIBRACORE_DEMO_PASSWORD") or getpass.getpass(
        "Demo-only password for all five accounts (12–128 characters): "
    )
    core = CoreService(settings.db_url(test))
    store = AuthStore(settings.db_url(test))
    try:
        seed(core)

        def create(op, args):
            try:
                core.execute(op, args)
            except DomainError as e:
                if e.code != "CONFLICT":
                    raise

        create(
            "create_department", {"department_id": "Electrical Engineering", "name": "Electrical Engineering"}
        )
        create(
            "create_budget",
            {
                "budget_id": "ee",
                "department_id": "Electrical Engineering",
                "fiscal_year": 2026,
                "currency": "INR",
                "total_minor": 8000000,
            },
        )
        for key, name, role in ACCOUNTS:
            uid = "DEMO-" + key
            create(
                "create_user",
                {
                    "user_id": uid,
                    "name": name,
                    "email": key + "@libracore.test",
                    "role": role,
                    "department_id": "Computer Science",
                },
            )
            store.provision(uid, password)
        titles = {
            "B001": ("Effective C++", "Scott Meyers", "Addison-Wesley"),
            "J001": ("Journal of Intelligent Systems", "Editorial Board", "De Gruyter"),
            "E001": ("Designing Data-Intensive Applications", "Martin Kleppmann", "O’Reilly Media"),
            "A001": ("The Design of Everyday Things", "Don Norman", "Basic Books"),
            "T001": (
                "Learning Representations for Scientific Discovery",
                "Ananya Iyer",
                "Institute Research Archive",
            ),
            "D001": ("Digital Systems Research Collection", "Research Library", "Institutional Archive"),
        }
        for rid, (title, author, publisher) in titles.items():
            r = core.execute("get_resource", {"resource_id": rid})
            r.update(
                title=title,
                authors=[author],
                publisher=publisher,
                university="Institute of Technology",
                degree="MTech",
                supervisor="Dr. Meera Shah",
                narrator="Ravi Kapoor",
            )
            core.execute("update_resource", r)
        core.execute(
            "borrow_resource",
            {
                "resource_id": "E001",
                "borrower_id": "DEMO-student",
                "quantity": 1,
                "request_id": "demo-v3-student-loan",
            },
        )
        print(
            "Demo ready. Accounts: student, faculty, librarian, manager, admin @libracore.test. Password was not logged."
        )
    finally:
        store.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--demo", action="store_true")
    parser.add_argument("--test", action="store_true")
    parser.add_argument("--admin-id")
    parser.add_argument("--name")
    parser.add_argument("--email")
    a = parser.parse_args()
    s = Settings(ai_enabled=False)
    if a.demo:
        demo(s, a.test)
        return
    if not all([a.admin_id, a.name, a.email]):
        parser.error("Provide --admin-id, --name and --email, or --demo")
    password = getpass.getpass("Initial administrator password (12–128 characters): ")
    core = CoreService(s.db_url(a.test))
    store = AuthStore(s.db_url(a.test))
    try:
        core.execute(
            "create_user", {"user_id": a.admin_id, "name": a.name, "email": a.email, "role": "Administrator"}
        )
        store.provision(a.admin_id, password)
    finally:
        store.close()
    print("Administrator provisioned. No password was logged.")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        raise SystemExit(
            "Provisioning failed. Check database configuration and input; no credentials were logged."
        ) from None
