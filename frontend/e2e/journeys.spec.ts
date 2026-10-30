import { test, expect, type Page } from "@playwright/test";
import AxeBuilder from "@axe-core/playwright";
const password = process.env.LIBRACORE_DEMO_PASSWORD;
async function login(page: Page, role: string) {
  if (!password)
    throw new Error(
      "Run scripts/e2e_v3.sh; an ephemeral test password is required",
    );
  await page.goto("/");
  await page.getByLabel("Institutional email").fill(role + "@libracore.test");
  await page.getByLabel("Password", { exact: true }).fill(password);
  await page.getByRole("button", { name: "Enter workspace" }).click();
  await expect(
    page.getByRole("heading", { name: /Welcome back/ }),
  ).toBeVisible();
}
async function navigate(page: Page, label: string) {
  const menu = page.getByRole("button", { name: "Open navigation" });
  if (await menu.isVisible()) await menu.click();
  await page
    .getByRole("navigation")
    .getByRole("link", { name: label, exact: true })
    .click();
}
async function accessible(page: Page) {
  const r = await new AxeBuilder({ page }).analyze();
  expect(
    r.violations.filter((v) =>
      ["critical", "serious"].includes(v.impact ?? ""),
    ),
  ).toEqual([]);
}
test("login is accessible", async ({ page }) => {
  await page.goto("/");
  await expect(
    page.getByRole("button", { name: "Enter workspace" }),
  ).toBeVisible();
  await accessible(page);
});
test("Student searches, borrows, returns and logs out", async ({ page }) => {
  await login(page, "student");
  await accessible(page);
  await navigate(page, "Catalog");
  await page
    .getByRole("textbox", { name: "Search catalog" })
    .fill("Effective C++");
  await page.getByRole("button", { name: "Search", exact: true }).click();
  await page
    .getByRole("link")
    .filter({
      has: page.getByRole("heading", { name: "Effective C++", exact: true }),
    })
    .click();
  await expect(
    page.getByRole("heading", { name: "Borrow this resource" }),
  ).toBeVisible();
  await accessible(page);
  await page
    .getByRole("button", { name: "Borrow resource", exact: true })
    .click();
  await page.getByRole("button", { name: "Confirm borrow resource" }).click();
  await expect(page.getByText("Loan recorded successfully")).toBeVisible();
  await navigate(page, "Lending");
  await accessible(page);
  await page
    .getByRole("button", { name: "Return resource", exact: true })
    .first()
    .click();
  await page
    .getByRole("dialog")
    .getByRole("button", { name: "Return resource", exact: true })
    .click();
  await page.getByRole("button", { name: "Confirm return resource" }).click();
  await expect(page.getByText("Resource returned successfully")).toBeVisible();
  await page.keyboard.press("Escape");
  const menu = page.getByRole("button", { name: "Open navigation" });
  if (await menu.isVisible()) await menu.click();
  await page.getByTitle("Sign out").click();
  await expect(
    page.getByRole("button", { name: "Enter workspace" }),
  ).toBeVisible();
});
test("Faculty sees personal lending", async ({ page }) => {
  await login(page, "faculty");
  await navigate(page, "Lending");
  await expect(
    page.getByRole("heading", { name: "Your reading desk" }),
  ).toBeVisible();
  const denied = await page.request.get("/api/v1/users");
  expect(denied.status()).toBe(403);
});
test("Librarian manages catalog and sees operations", async ({ page }) => {
  await login(page, "librarian");
  await navigate(page, "Catalog");
  await page.getByRole("button", { name: "Add resource" }).click();
  await page
    .getByLabel("Resource ID", { exact: true })
    .fill("E2E-" + Date.now());
  await page
    .getByLabel("Title", { exact: true })
    .fill("Algorithms for Shared Knowledge");
  await page.getByLabel("Price", { exact: true }).fill("125.00");
  await page
    .getByRole("button", { name: "Create resource", exact: true })
    .click();
  await expect(page.getByText("Resource saved successfully.")).toBeVisible();
  await page.keyboard.press("Escape");
  await navigate(page, "Lending");
  await page.getByLabel("All borrowers").check();
  await expect(
    page.getByRole("heading", { name: "Lending operations" }),
  ).toBeVisible();
  await navigate(page, "Acquisitions");
  await expect(
    page.getByRole("heading", { name: "Plan a purchase" }),
  ).toBeVisible();
});
test("Manager quotes and purchases for their department", async ({ page }) => {
  await login(page, "manager");
  await navigate(page, "Budgets");
  await expect(page.getByText("Category quotas & usage")).toBeVisible();
  await navigate(page, "Acquisitions");
  await page.getByLabel("Resource ID", { exact: true }).fill("B001");
  await page
    .getByRole("button", { name: "Get quote & check eligibility" })
    .click();
  await expect(page.getByRole("dialog")).toBeVisible();
  await page.getByRole("button", { name: "Confirm purchase" }).click();
  await expect(
    page.getByRole("heading", { name: "Purchase approved" }),
  ).toBeVisible();
  const denied = await page.request.get(
    "/api/v1/reports/departments/Electrical%20Engineering",
  );
  expect(denied.status()).toBe(403);
});
test("Administrator creates and manages user", async ({ page }) => {
  await login(page, "admin");
  await navigate(page, "Administration");
  await expect(
    page.getByRole("heading", { name: "System readiness" }),
  ).toBeVisible();
  await page.getByText("Create user", { exact: true }).first().click();
  const id = "VISITOR-" + Date.now();
  await page.getByLabel("User ID", { exact: true }).fill(id);
  await page.getByLabel("Full name").fill("Kavya Narayan");
  await page
    .getByLabel("Institutional email")
    .fill(id.toLowerCase() + "@libracore.test");
  await page.getByRole("button", { name: "Create user", exact: true }).click();
  await expect(page.getByText("Account created successfully.")).toBeVisible();
});
test("Assistant search and high-risk confirmation execute once", async ({
  page,
}) => {
  await login(page, "admin");
  await navigate(page, "Assistant");
  await accessible(page);
  await page.getByLabel("Message Libra").fill("Find books by Scott Meyers");
  await page.getByRole("button", { name: "Send message" }).click();
  await expect(page.getByText(/Tool completed/)).toBeVisible();
  const me = await (await page.request.get("/api/v1/auth/me")).json();
  const purchase = await page.request.post("/api/v1/acquisitions", {
    headers: { Origin: "http://localhost:5173", "X-CSRF-Token": me.csrf_token },
    data: { resource_id: "B001", quantity: 1, request_id: crypto.randomUUID() },
  });
  expect(purchase.ok()).toBeTruthy();
  const order = await purchase.json();
  expect(order.approved).toBeTruthy();
  await page.getByLabel("Message Libra").fill("Cancel order " + order.order_id);
  await page.getByRole("button", { name: "Send message" }).click();
  await expect(page.getByText("Human approval required")).toBeVisible();
  await page.getByRole("button", { name: "Confirm action" }).click();
  await expect(page.getByText("Action completed.")).toBeVisible();
  await expect(
    page.getByRole("button", { name: "Confirm action" }),
  ).toHaveCount(0);
});
