import unittest
from unittest.mock import Mock, patch
from concurrent.futures import ThreadPoolExecutor
from libracore_api.v3.security import hash_password, verify_password, digest, check_csrf, RateLimiter
from libracore_api.v3.policy import Principal, AuthorizationService, AuthorizedService, current, Permission
from libracore_api.services import DomainError
from libracore_api.ai.confirmation import Confirmations


class SecurityTests(unittest.TestCase):
    def test_password(self):
        encoded = hash_password("A strong passphrase 2026")
        self.assertTrue(encoded.startswith("$argon2id$"))
        self.assertTrue(verify_password(encoded, "A strong passphrase 2026"))
        self.assertFalse(verify_password(encoded, "wrong"))
        self.assertFalse(verify_password("invalid", "anything"))

    def test_password_limits(self):
        for value in ["short", "x" * 129]:
            with self.assertRaises(DomainError):
                hash_password(value)

    def test_csrf(self):
        check_csrf("abc", digest("abc"))
        for value in ["", "wrong"]:
            with self.assertRaises(DomainError):
                check_csrf(value, digest("abc"))

    def test_limiter_expires(self):
        limiter = RateLimiter()
        with patch("libracore_api.v3.security.time.monotonic", return_value=1):
            limiter.check("x", 1)
            with self.assertRaises(DomainError):
                limiter.check("x", 1)
        with patch("libracore_api.v3.security.time.monotonic", return_value=62):
            limiter.check("x", 1)

    def test_limiter_capacity(self):
        r = RateLimiter(1)
        r.check("a", 3)
        with self.assertRaises(DomainError):
            r.check("b", 3)

    def test_confirmation_race(self):
        confirmations = Confirmations()
        token = confirmations.propose("session", "submit_purchase", {"quantity": 1})["token"]

        def consume(_):
            try:
                confirmations.consume(token, "session")
                return True
            except DomainError:
                return False

        with ThreadPoolExecutor(8) as pool:
            self.assertEqual(sum(pool.map(consume, range(20))), 1)


class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.policy = AuthorizationService()
        self.core = Mock()

    def p(self, role="Student", department="CS"):
        return Principal("U", "Asha", role, department, "session", "hash")

    def test_matrix(self):
        for role in ["Student", "Faculty"]:
            p = self.p(role)
            for op in ["search_resources", "get_resource", "borrow_resource"]:
                self.assertTrue(self.policy.allowed(p, op))
            for op in ["set_quota", "set_offer", "list_users", "cancel_purchase", "get_recent_events"]:
                with self.subTest(role=role, op=op):
                    self.assertFalse(self.policy.allowed(p, op))
        self.assertFalse(self.policy.allowed(self.p("Librarian"), "list_users"))
        self.assertTrue(self.policy.allowed(self.p("Administrator"), "set_quota"))
        self.assertFalse(self.policy.allowed(self.p("Administrator"), "execute_sql"))

    def test_department_boundary(self):
        for op in ["quote_purchase", "submit_purchase", "get_department_budget", "set_quota", "list_orders"]:
            self.policy.check(self.p("DepartmentManager"), op, {"department_id": "CS"}, self.core)
            for d in ["EE", ""]:
                with self.assertRaises(DomainError):
                    self.policy.check(self.p("DepartmentManager"), op, {"department_id": d}, self.core)

    def test_order_department_from_server(self):
        self.core.execute.return_value = {"requested_department": "EE"}
        with self.assertRaises(DomainError):
            self.policy.check(
                self.p("DepartmentManager"),
                "cancel_purchase",
                {"order_id": 1, "department_id": "CS"},
                self.core,
            )

    def test_batch_all_departments(self):
        with self.assertRaises(DomainError):
            self.policy.check(
                self.p("DepartmentManager"),
                "batch_purchase",
                {"requests": [{"department_id": "CS"}, {"department_id": "EE"}]},
                self.core,
            )

    def test_self_loans(self):
        self.policy.check(self.p(), "borrow_resource", {"borrower_id": "U"}, self.core)
        with self.assertRaises(DomainError):
            self.policy.check(self.p(), "borrow_resource", {"borrower_id": "other"}, self.core)
        self.core.execute.return_value = {"borrower_id": "other"}
        for op in ["get_loan", "return_loan"]:
            with self.assertRaises(DomainError):
                self.policy.check(self.p(), op, {"loan_id": "L"}, self.core)

    def test_self_dates_not_client_authority(self):
        with self.assertRaises(DomainError):
            self.policy.check(self.p(), "borrow_resource", {"borrower_id": "U", "due_at": "2099"}, self.core)

    def test_unknown_role_denied(self):
        self.assertEqual(self.p("unknown").permissions, set())

    def test_ai_filters(self):
        wrapper = AuthorizedService(self.core, Mock())
        token = current.set(self.p())
        try:
            declarations = [{"name": n} for n in ["search_resources", "cancel_purchase", "borrow_resource"]]
            self.assertEqual(wrapper.allowed_tools(declarations), [{"name": "search_resources"}])
            with self.assertRaises(DomainError):
                wrapper.assistant_authorize("cancel_purchase", {})
        finally:
            current.reset(token)

    def test_no_identity_fails_closed(self):
        with self.assertRaises(DomainError):
            AuthorizedService(self.core, Mock()).execute("search_resources", {})

    def test_admin_has_all_permissions(self):
        self.assertEqual(self.p("Administrator").permissions, set(Permission))


if __name__ == "__main__":
    unittest.main()
