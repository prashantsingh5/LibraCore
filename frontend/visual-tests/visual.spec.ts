// Browser component tests with explicit transport fixtures. NOT full-stack E2E.
import { test, expect, type Page } from "@playwright/test";
import { readFileSync } from "node:fs";
import AxeBuilder from "@axe-core/playwright";
const money = { minor_units: 10000, currency: "INR", formatted: "INR 100.00" };
const user = {
  user_id: "DEMO-admin",
  name: "Arjun Sen",
  role: "Administrator",
  department_id: "Computer Science",
  permissions: [
    "catalog.read",
    "resource.manage",
    "loan.self",
    "loan.manage",
    "acquisition.manage",
    "budget.read",
    "budget.manage",
    "vendor.manage",
    "report.read",
    "event.read",
    "user.manage",
    "department.manage",
  ],
};
const resources = [
  "Effective C++",
  "Designing Data-Intensive Applications",
  "The Design of Everyday Things",
].map((title, i) => ({
  resource_id: ["B001", "E001", "A001"][i],
  title,
  resource_type: ["Book", "EBook", "AudioBook"][i],
  authors: [["Scott Meyers", "Martin Kleppmann", "Don Norman"][i]],
  publisher: "Academic Collection",
  publication_year: 2020,
  price: money,
  currency: "INR",
  unit_price_minor: 10000,
  holdings: 5,
  lent: 1,
  isbn: "978-123",
  binding: 0,
  edition: 1,
  license: 0,
  format: 1,
  drm: false,
  issn: "",
  vendors: [],
}));
async function fixtures(page: Page, loggedIn = true, identity = user) {
  await page.route("**/ready", (r) =>
    r.fulfill({ json: { database: "ready", native: "ready", ai: "mock" } }),
  );
  await page.route("**/api/v1/**", async (route) => {
    const url = new URL(route.request().url());
    let body: unknown = {};
    if (url.pathname.endsWith("/cover")) {
      if (route.request().method() === "GET") {
        await route.fulfill({
          contentType: "image/svg+xml",
          body: readFileSync("public/art/default-cover.svg", "utf8"),
        });
        return;
      }
      body = { ok: true };
    } else if (url.pathname === "/api/v1/auth/me") {
      if (!loggedIn) {
        await route.fulfill({
          status: 401,
          json: { error: { code: "UNAUTHENTICATED", message: "Sign in" } },
        });
        return;
      }
      body = { user: identity, csrf_token: "fixture" };
    } else if (url.pathname === "/api/v1/resources")
      body = { items: resources, limit: 12, offset: 0, next_cursor: null };
    else if (url.pathname.startsWith("/api/v1/resources/")) body = resources[0];
    else if (url.pathname.includes("/loans"))
      body = [
        {
          loan_id: "L1",
          title: "Effective C++",
          resource_id: "B001",
          borrower_id: user.user_id,
          quantity: 1,
          returned_quantity: 0,
          borrowed_at: "2026-10-24T00:00:00Z",
          due_at: "2026-11-08T00:00:00Z",
          status: 0,
        },
      ];
    else if (url.pathname.includes("/reports/budget"))
      body = {
        total: { ...money, formatted: "INR 1,000,000.00" },
        spent: { ...money, formatted: "INR 245,000.00" },
        remaining: { ...money, formatted: "INR 755,000.00" },
        percentage_used: 24.5,
        categories: [{ category: 0, units: 85, percentage_used: 24.5 }],
      };
    else if (url.pathname === "/api/v1/departments")
      body = [{ department_id: "Computer Science", name: "Computer Science" }];
    else if (url.pathname === "/api/v1/users")
      body = [{ ...identity, email: "arjun@libracore.test", active: true }];
    else if (url.pathname.includes("/vendors"))
      body = [
        {
          vendor_id: "V001",
          name: "Economy Books",
          rating: 4,
          estimated_delivery_days: 7,
        },
      ];
    else if (url.pathname.includes("/events/feed"))
      body = {
        items: [
          {
            event_id: 1,
            event_type: 2,
            title: "Effective C++",
            resource_id: "B001",
            quantity: 1,
            actor: "Asha Rao",
            occurred_at: "2026-10-28T10:00:00Z",
          },
        ],
        next_cursor: null,
      };
    else if (url.pathname.includes("/reports/lending"))
      body = {
        total_holdings: 350,
        borrowed: 85,
        available: 265,
        active_loans: 62,
      };
    else if (url.pathname.includes("/reports/acquisitions"))
      body = {
        approved: 48,
        rejected: 2,
        cancelled: 3,
        pre_tax: money,
        post_tax: money,
      };
    else if (url.pathname === "/api/v1/acquisitions")
      body = [
        {
          order_id: 10,
          title: "Effective C++",
          quantity: 1,
          approved: true,
          status: 1,
          current_status: 3,
          cancellation_order_id: 11,
          post_tax_minor: 9000,
          currency: "INR",
          vendor: "Economy Books",
        },
      ];
    else if (url.pathname.includes("/assistant/message"))
      body = {
        message: "Authoritative application results attached.",
        tool_results: [
          { operation: "search_resources", result: { items: resources } },
        ],
        confirmations: [],
      };
    else body = [];
    await route.fulfill({ json: body });
  });
}
for (const width of [1440, 1024, 768, 390]) {
  test("screens and accessibility " + width, async ({ page }) => {
    await page.setViewportSize({ width, height: 1000 });
    test.setTimeout(90000);
    await fixtures(page);
    const errors: string[] = [];
    page.on("pageerror", (e) => errors.push(e.message));
    for (const [path, name] of [
      ["/", "dashboard"],
      ["/catalog", "catalog"],
      ["/catalog/B001", "resource"],
      ["/budgets", "budget"],
      ["/loans", "loans"],
      ["/assistant", "assistant"],
      ["/acquisitions", "acquisitions"],
      ["/vendors", "vendors"],
      ["/reports", "reports"],
      ["/events", "activity"],
      ["/users", "administration"],
      ["/settings", "settings"],
    ]) {
      await page.goto(path);
      await expect(page.locator("#main h1")).toBeVisible();
      await page.waitForTimeout(250);
      if (name === "assistant") {
        await page
          .getByPlaceholder("Ask about your collection…")
          .fill("Find Effective C++");
        await page.getByRole("button", { name: "Send message" }).click();
        await expect(page.locator(".assistant-results")).toBeVisible();
      }
      const violations = (
        await new AxeBuilder({ page }).analyze()
      ).violations.filter((v) =>
        ["serious", "critical"].includes(v.impact ?? ""),
      );
      expect(
        violations.map((v) => ({
          id: v.id,
          nodes: v.nodes.map((n) => n.target),
        })),
      ).toEqual([]);
      expect(
        await page.evaluate(
          () => document.documentElement.scrollWidth <= innerWidth + 1,
        ),
      ).toBe(true);
      if (width === 1440 || width === 390)
        await page.screenshot({
          path: "../docs/screenshots/" + name + "-" + width + ".png",
          fullPage: true,
        });
    }
    expect(errors).toEqual([]);
    await page.getByLabel("Color theme").selectOption("dark");
    await expect(page.locator("html")).toHaveAttribute("data-theme", "dark");
    expect(
      (await new AxeBuilder({ page }).analyze()).violations.filter((v) =>
        ["serious", "critical"].includes(v.impact ?? ""),
      ),
    ).toEqual([]);
    for (const route of ["/catalog", "/catalog/B001"]) {
      await page.goto(route);
      await expect(page.locator("#main h1")).toBeVisible();
      expect(
        (await new AxeBuilder({ page }).analyze()).violations.filter((v) =>
          ["serious", "critical"].includes(v.impact ?? ""),
        ),
      ).toEqual([]);
    }
    await page.screenshot({
      path: "../docs/screenshots/dark-" + width + ".png",
      fullPage: true,
    });
  });
}
test("login desktop and mobile", async ({ page }) => {
  await fixtures(page, false);
  for (const width of [1440, 390]) {
    await page.setViewportSize({ width, height: 1000 });
    await page.goto("/");
    await expect(
      page.getByRole("button", { name: "Enter workspace" }),
    ).toBeVisible();
    expect(
      (await new AxeBuilder({ page }).analyze()).violations.filter((v) =>
        ["serious", "critical"].includes(v.impact ?? ""),
      ),
    ).toEqual([]);
    await page.screenshot({
      path: "../docs/screenshots/login-" + width + ".png",
      fullPage: true,
    });
  }
});
test("mobile navigation keyboard and focus", async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await fixtures(page);
  await page.goto("/");
  const menu = page.getByRole("button", { name: "Open navigation" });
  await expect(page.locator("#workspace-navigation")).toHaveAttribute(
    "inert",
    "",
  );
  await menu.focus();
  await page.keyboard.press("Enter");
  await expect(page.locator("#workspace-navigation")).not.toHaveAttribute(
    "inert",
    "",
  );
  await expect(page.locator("#workspace-navigation .wordmark")).toBeFocused();
  await page.keyboard.press("Escape");
  await expect(menu).toBeFocused();
  await expect(page.locator("#workspace-navigation")).toHaveAttribute(
    "inert",
    "",
  );
});
test("type-aware resource modal and cover workflow", async ({ page }) => {
  await fixtures(page);
  await page.setViewportSize({ width: 390, height: 900 });
  await page.goto("/catalog");
  await page.getByRole("button", { name: "Add resource" }).click();
  await expect(page.getByRole("dialog")).toBeVisible();
  await page
    .getByLabel("Resource type", { exact: true })
    .selectOption("AudioBook");
  await expect(page.getByLabel("Narrator", { exact: true })).toBeVisible();
  await expect(page.getByLabel("Binding", { exact: true })).toHaveCount(0);
  await page.getByLabel("Resource type", { exact: true }).selectOption("Book");
  await page.getByLabel("Upload book cover").setInputFiles({
    name: "cover.png",
    mimeType: "image/png",
    buffer: Buffer.from(
      "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+a0S8AAAAASUVORK5CYII=",
      "base64",
    ),
  });
  await page.screenshot({
    path: "../docs/screenshots/create-cover-390.png",
    fullPage: true,
  });
  await page.keyboard.press("Escape");
  await page.goto("/catalog/B001");
  await page.getByRole("button", { name: "Edit resource" }).click();
  await page.getByLabel("Revert to default cover").check();
  await page.getByRole("button", { name: "Save resource" }).click();
  await expect(page.getByText("Default cover restored.")).toBeVisible();
  expect(
    (await new AxeBuilder({ page }).analyze()).violations.filter((v) =>
      ["serious", "critical"].includes(v.impact ?? ""),
    ),
  ).toEqual([]);
  await page.screenshot({
    path: "../docs/screenshots/edit-cover-390.png",
    fullPage: true,
  });
});
for (const role of [
  "Student",
  "Faculty",
  "Librarian",
  "DepartmentManager",
  "Administrator",
])
  test("administration route boundaries " + role, async ({ page }) => {
    await fixtures(page, true, {
      ...user,
      role,
      permissions:
        role === "Administrator"
          ? user.permissions
          : ["catalog.read", "loan.self"],
    });
    for (const path of ["/administration", "/users"]) {
      await page.goto(path);
      if (role !== "Administrator") {
        await expect(
          page.getByRole("heading", { name: "Access is restricted" }),
        ).toBeVisible();
        await expect(
          page.getByRole("link", { name: "Return to workspace" }),
        ).toBeVisible();
        if (role === "Student" && path === "/administration")
          await page.screenshot({
            path: "../docs/screenshots/restricted-1440.png",
            fullPage: true,
          });
      } else
        await expect(
          page.getByRole("heading", { name: "A well-connected institution." }),
        ).toBeVisible();
    }
  });
