"""Deny by default, shared by HTTP and AI. Ownership is checked on server data."""

from contextvars import ContextVar
import hashlib
import re
from dataclasses import dataclass
from enum import StrEnum
from typing import Any
from ..services import DomainError


class Permission(StrEnum):
    CATALOG = "catalog.read"
    RESOURCE = "resource.manage"
    SELF_LOAN = "loan.self"
    LOANS = "loan.manage"
    ACQUISITION = "acquisition.manage"
    BUDGET = "budget.read"
    BUDGET_MANAGE = "budget.manage"
    VENDOR = "vendor.manage"
    REPORT = "report.read"
    EVENT = "event.read"
    USER = "user.manage"
    DEPARTMENT = "department.manage"


BASE = {Permission.CATALOG, Permission.SELF_LOAN}
ROLE_POLICY: dict[str, set[Permission]] = {
    "Student": BASE,
    "Faculty": BASE,
    "Librarian": BASE
    | {
        Permission.RESOURCE,
        Permission.LOANS,
        Permission.ACQUISITION,
        Permission.BUDGET,
        Permission.VENDOR,
        Permission.REPORT,
        Permission.EVENT,
    },
    "DepartmentManager": BASE | {Permission.ACQUISITION, Permission.BUDGET, Permission.BUDGET_MANAGE},
    "Administrator": set(Permission),
}
OPERATIONS = {
    **dict.fromkeys(["search_resources", "get_resource", "get_availability"], Permission.CATALOG),
    **dict.fromkeys(["create_resource", "update_resource", "delete_resource"], Permission.RESOURCE),
    **dict.fromkeys(["borrow_resource", "return_loan", "get_loan", "get_user_loans"], Permission.SELF_LOAN),
    "list_loans": Permission.LOANS,
    **dict.fromkeys(
        [
            "quote_purchase",
            "check_purchase",
            "submit_purchase",
            "cancel_purchase",
            "get_order",
            "batch_purchase",
            "list_orders",
        ],
        Permission.ACQUISITION,
    ),
    **dict.fromkeys(["get_budget_summary", "get_department_budget"], Permission.BUDGET),
    **dict.fromkeys(["create_budget", "set_quota", "rollover"], Permission.BUDGET_MANAGE),
    **dict.fromkeys(["set_offer", "list_vendors", "update_vendor"], Permission.VENDOR),
    **dict.fromkeys(["get_lending_summary", "get_acquisition_summary"], Permission.REPORT),
    "get_recent_events": Permission.EVENT,
    "get_events_cursor": Permission.EVENT,
    **dict.fromkeys(
        ["create_user", "get_user", "list_users", "set_user_active", "update_user"], Permission.USER
    ),
    "create_department": Permission.DEPARTMENT,
    "list_departments": Permission.CATALOG,
    "health_check": Permission.CATALOG,
}


@dataclass(frozen=True)
class Principal:
    user_id: str
    name: str
    role: str
    department_id: str
    session_id: str
    token_hash: str

    @property
    def permissions(self) -> set[Permission]:
        return ROLE_POLICY.get(self.role, set())


current: ContextVar[Principal | None] = ContextVar("principal", default=None)


def require_principal() -> Principal:
    p = current.get()
    if p is None:
        raise DomainError("UNAUTHENTICATED", "Sign in to continue")
    return p


class AuthorizationService:
    def allowed(self, p: Principal, operation: str) -> bool:
        return operation in OPERATIONS and OPERATIONS[operation] in p.permissions

    def check(self, p: Principal, operation: str, args: dict[str, Any], core: Any) -> None:
        if not self.allowed(p, operation):
            raise DomainError("FORBIDDEN", "You do not have permission for this action")
        if (
            operation in {"borrow_resource", "get_user_loans", "get_loan", "return_loan"}
            and Permission.LOANS not in p.permissions
        ):
            owner = args.get("borrower_id", args.get("user_id"))
            if operation in {"get_loan", "return_loan"}:
                owner = core.execute("get_loan", {"loan_id": args["loan_id"]})["borrower_id"]
            if owner != p.user_id:
                raise DomainError("FORBIDDEN", "Only your own loans are accessible")
            if operation == "borrow_resource" and ("borrowed_at" in args or "due_at" in args):
                raise DomainError("FORBIDDEN", "Loan dates are assigned by the library")
            if operation == "return_loan" and "returned_at" in args:
                raise DomainError("FORBIDDEN", "Return dates are assigned by the library")
        if p.role == "DepartmentManager" and operation in {
            "quote_purchase",
            "check_purchase",
            "submit_purchase",
            "cancel_purchase",
            "get_order",
            "batch_purchase",
            "list_orders",
            "get_budget_summary",
            "get_department_budget",
            "create_budget",
            "set_quota",
            "rollover",
        }:
            departments = [args.get("department_id", "")]
            if operation in {"get_order", "cancel_purchase"}:
                order = core.execute("get_order", {"order_id": args["order_id"]})
                departments = [order["requested_department"]]
            if operation == "batch_purchase":
                departments = [x.get("department_id", "") for x in args["requests"]]
            if not p.department_id or any(d != p.department_id for d in departments):
                raise DomainError("FORBIDDEN", "This action is restricted to your department")


class AuthorizedService:
    def __init__(self, core: Any, auth: Any):
        self.core, self.auth, self.policy = core, auth, AuthorizationService()

    def authorize(self, operation: str, parameters: dict[str, Any]) -> None:
        self.policy.check(require_principal(), operation, parameters, self.core)

    def assistant_intent_check(self, message: str) -> None:
        # UX-only early refusal for clear purchase intents. Never grants access;
        # tool authorization still enforces all policy after interpretation.
        if (
            require_principal().role in {"Student", "Faculty"}
            and re.search(r"\b(buy|purchase|acquire|order)\b", message, re.I)
            and not re.search(r"\b(history|status|what|explain)\b", message, re.I)
        ):
            raise DomainError(
                "FORBIDDEN", "Purchasing is restricted to authorized library or department staff."
            )

    def allowed_tools(self, declarations: list[dict[str, Any]]) -> list[dict[str, Any]]:
        p = require_principal()
        # Students/Faculty get read-only assistant tools; normal lending UI stays enabled.
        return [
            d
            for d in declarations
            if self.policy.allowed(p, d["name"])
            and not (p.role in {"Student", "Faculty"} and d["name"] in {"borrow_resource", "return_loan"})
        ]

    def assistant_authorize(self, operation: str, args: dict[str, Any]) -> None:
        if operation not in {d["name"] for d in self.allowed_tools([{"name": n} for n in OPERATIONS])}:
            raise DomainError("FORBIDDEN", "This assistant action is not permitted")
        self.authorize(operation, args)

    def execute(self, operation: str, parameters: dict[str, Any]) -> Any:
        p = require_principal()
        # A user-row lock orders revocation/role changes against in-flight domain calls.
        with self.auth.guard(p) as fresh:
            self.policy.check(fresh, operation, parameters, self.core)
            if "request_id" in parameters:
                parameters = {
                    **parameters,
                    "request_id": hashlib.sha256(fresh.user_id.encode()).hexdigest()
                    + ":"
                    + parameters["request_id"],
                }
            result = self.core.execute(operation, parameters)
            if (
                operation in {"get_resource", "search_resources"}
                and Permission.VENDOR not in fresh.permissions
            ):
                if operation == "get_resource":
                    result = {k: v for k, v in result.items() if k != "vendors"}
                else:
                    result = {
                        **result,
                        "items": [{k: v for k, v in r.items() if k != "vendors"} for r in result["items"]],
                    }
            return result
