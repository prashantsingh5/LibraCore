"""Unit tests use SQL cursor doubles; PostgreSQL behavior has separate tests."""

import unittest
from contextlib import nullcontext
from unittest.mock import MagicMock, patch
from libracore_api.v3.auth import AuthStore
from libracore_api.v3.policy import Principal
from libracore_api.v3.security import digest, hash_password
from libracore_api.services import DomainError


class AuthStoreTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.encoded = hash_password("A test passphrase 2026")

    def setUp(self):
        self.c = MagicMock()
        self.pool = MagicMock()
        self.pool.connection.side_effect = lambda: nullcontext(self.c)
        with patch("libracore_api.v3.auth.ConnectionPool", return_value=self.pool):
            self.store = AuthStore("unused-test-url")
        self.p = Principal("U", "Asha", "Administrator", "CS", "session", "token-hash")
        self.row = {
            "user_id": "U",
            "name": "Asha",
            "role": "Administrator",
            "department_id": "CS",
            "session_id": "session",
            "token_hash": digest("token"),
            "csrf_hash": digest("csrf"),
            "active": True,
            "password_hash": self.encoded,
        }
        self.c.execute.return_value.fetchone.return_value = dict(self.row)

    def test_login_returns_opaque_credentials(self):
        p, token, csrf = self.store.login("asha@example.invalid", "A test passphrase 2026")
        self.assertEqual(p.user_id, "U")
        self.assertGreater(len(token), 40)
        self.assertEqual(len(csrf), 64)
        inserts = [x for x in self.c.execute.call_args_list if "INSERT INTO auth_sessions" in x.args[0]]
        self.assertEqual(inserts[0].args[1][2], digest(token))
        self.assertNotIn(token, inserts[0].args[1])

    def test_wrong_and_unknown_password(self):
        with self.assertRaises(DomainError):
            self.store.login("asha@example.invalid", "bad")
        self.c.execute.return_value.fetchone.return_value = None
        with self.assertRaises(DomainError):
            self.store.login("unknown", "bad")

    def test_inactive_cannot_login(self):
        self.c.execute.return_value.fetchone.return_value = {**self.row, "active": False}
        with self.assertRaises(DomainError):
            self.store.login("asha@example.invalid", "A test passphrase 2026")

    def test_authenticate_and_csrf(self):
        self.assertEqual(self.store.authenticate("token", "csrf", True).user_id, "U")
        with self.assertRaises(DomainError):
            self.store.authenticate("token", "wrong", True)
        for token in ["", "x" * 201]:
            with self.assertRaises(DomainError):
                self.store.authenticate(token)

    def test_missing_and_invalid_session(self):
        self.c.execute.return_value.fetchone.return_value = None
        with self.assertRaises(DomainError):
            self.store.authenticate("bad-token")
        # Identity lookup succeeds, valid-session query returns nothing after lock.
        self.c.execute.side_effect = [
            MagicMock(fetchone=lambda: {"user_id": "U"}),
            MagicMock(fetchone=lambda: {}),
            MagicMock(fetchone=lambda: None),
        ]
        with self.assertRaises(DomainError):
            self.store.authenticate("expired")

    def test_guard_uses_fresh_role(self):
        self.c.execute.return_value.fetchone.return_value = {**self.row, "role": "Student"}
        with self.store.guard(self.p) as fresh:
            self.assertEqual(fresh.role, "Student")

    def test_logout_and_logout_all(self):
        self.store.logout(self.p)
        self.store.logout(self.p, True)
        updates = [x for x in self.c.execute.call_args_list if "UPDATE auth_sessions" in x.args[0]]
        self.assertEqual([x.args[1][1] for x in updates], [False, True])

    def test_change_password_revokes(self):
        self.store.change_password(self.p, "A test passphrase 2026", "Changed passphrase 2026")
        self.assertTrue(any("revoked_at=now()" in c.args[0] for c in self.c.execute.call_args_list))
        with self.assertRaises(DomainError):
            self.store.change_password(self.p, "wrong", "Another passphrase 2026")

    def test_access_changes_revoke_sessions(self):
        self.store.manage(self.p, "V", "Faculty", "CS", True)
        self.assertTrue(any("UPDATE users SET role=" in c.args[0] for c in self.c.execute.call_args_list))
        with self.assertRaises(DomainError):
            self.store.manage(self.p, "U", "Student", "CS", True)

    def test_admin_actions_recheck_role(self):
        self.c.execute.return_value.fetchone.return_value = {**self.row, "role": "Student"}
        with self.assertRaises(DomainError):
            self.store.manage(self.p, "V", "Faculty", "CS", True)
        with self.assertRaises(DomainError):
            self.store.admin_provision(self.p, "V", "New passphrase 2026")

    def test_provision_and_admin_reset(self):
        self.store.provision("U", "Initial passphrase 2026")
        self.store.admin_provision(self.p, "V", "New passphrase 2026")
        self.assertTrue(
            any("INSERT INTO auth_credentials" in c.args[0] for c in self.c.execute.call_args_list)
        )
        self.store.close()
        self.pool.close.assert_called_once()

    def test_inactive_message_only_after_valid_password(self):
        self.c.execute.return_value.fetchone.return_value = {**self.row, "active": False}
        with self.assertRaisesRegex(DomainError, "currently inactive"):
            self.store.login("asha@example.invalid", "A test passphrase 2026")
        with self.assertRaisesRegex(DomainError, "Email or password"):
            self.store.login("asha@example.invalid", "wrong")
