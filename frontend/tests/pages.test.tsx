import { describe, it, expect, vi, beforeEach } from "vitest";
import { render, screen, fireEvent } from "@testing-library/react";
import { MemoryRouter, Routes, Route } from "react-router-dom";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import type { ReactNode } from "react";
import { Catalog } from "../src/features/catalog/Catalog";
import { ResourceDetail } from "../src/features/catalog/ResourceDetail";
import { Loans } from "../src/features/loans/Loans";
import { Budgets } from "../src/features/budgets/Budgets";
import { Users } from "../src/features/users/Users";
import { Vendors } from "../src/features/vendors/Vendors";
import { Reports } from "../src/features/reports/Reports";
import { Events } from "../src/features/events/Events";
import { Acquisitions } from "../src/features/acquisitions/Acquisitions";
import { Dashboard } from "../src/pages/Dashboard";
import { Shell } from "../src/components/layout/Shell";
import { Assistant } from "../src/features/assistant/Assistant";
const state = vi.hoisted(() => ({
  role: "Administrator",
  permissions: [
    "catalog.read",
    "loan.self",
    "user.manage",
    "budget.manage",
    "budget.read",
    "resource.manage",
    "vendor.manage",
    "acquisition.manage",
    "report.read",
    "event.read",
    "loan.manage",
  ],
}));
vi.mock("../src/features/auth/Auth", () => ({
  useAuth: () => ({
    user: {
      user_id: "U",
      name: "Asha Rao",
      role: state.role,
      department_id: "CS",
      permissions: state.permissions,
    },
    logout: async () => {},
  }),
}));
vi.mock("../src/components/charts/Utilization", () => ({
  Utilization: ({ used }: { used: number }) => <p>{used}% of budget used</p>,
}));
const money = { minor_units: 10000, currency: "INR", formatted: "INR 100.00" };
const resource = {
  resource_id: "B001",
  resource_type: "Book",
  title: "Effective C++",
  authors: ["Scott Meyers"],
  publisher: "Addison-Wesley",
  publication_year: 2020,
  price: money,
  currency: "INR",
  unit_price_minor: 10000,
  holdings: 5,
  lent: 1,
  isbn: "978",
  issn: "",
  vendors: [],
};
function renderPage(node: ReactNode, path = "/") {
  return render(
    <QueryClientProvider
      client={
        new QueryClient({ defaultOptions: { queries: { retry: false } } })
      }
    >
      <MemoryRouter initialEntries={[path]}>
        <Routes>
          <Route path="*" element={node} />
        </Routes>
      </MemoryRouter>
    </QueryClientProvider>,
  );
}
beforeEach(() => {
  state.role = "Administrator";
  state.permissions = [
    "catalog.read",
    "loan.self",
    "user.manage",
    "budget.manage",
    "budget.read",
    "resource.manage",
    "vendor.manage",
    "acquisition.manage",
    "report.read",
    "event.read",
    "loan.manage",
  ];
  vi.stubGlobal("matchMedia", () => ({
    matches: false,
    addEventListener: () => {},
    removeEventListener: () => {},
  }));
  vi.stubGlobal(
    "fetch",
    vi.fn(async (input: RequestInfo | URL) => {
      const path = String(input);
      let data: unknown = [];
      if (path.includes("/auth/me"))
        data = { user: { user_id: "U" }, csrf_token: "csrf" };
      else if (path.includes("/resources?"))
        data = { items: [resource], next_cursor: null, limit: 12, offset: 0 };
      else if (path.includes("/resources/")) data = resource;
      else if (path.includes("/reports/budget"))
        data = {
          total: money,
          spent: money,
          remaining: money,
          percentage_used: 25,
          categories: [{ category: "Book", units_used: 1 }],
        };
      else if (path.includes("/reports/")) data = { approved: 2, borrowed: 3 };
      else if (path.includes("/users?"))
        data = [
          {
            user_id: "U",
            name: "Asha Rao",
            email: "asha@example.invalid",
            role: "Administrator",
            active: true,
            department_id: "CS",
          },
        ];
      else if (path.includes("/vendors"))
        data = [
          {
            vendor_id: "V",
            name: "Academic Press",
            rating: 4.5,
            estimated_delivery_days: 2,
          },
        ];
      else if (path.includes("/events"))
        data = {
          items: [{ event_id: 1, actor: "U", event_type: "Borrow" }],
          next_cursor: null,
        };
      else if (path.includes("/acquisitions?"))
        data = [
          {
            order_id: 1,
            title: "Effective C++",
            quantity: 1,
            approved: true,
            status: 0,
            requested_department: "CS",
          },
        ];
      else if (path.includes("/loans"))
        data = [
          {
            loan_id: "L1",
            resource_id: "B001",
            borrower_id: "U",
            quantity: 2,
            returned_quantity: 0,
            due_at: "2099-01-01",
            status: 0,
          },
        ];
      else if (path.includes("/assistant/"))
        data = {
          message: "Authoritative results",
          tool_results: [],
          confirmations: [],
        };
      return { ok: true, json: async () => data, headers: new Headers() };
    }),
  );
});
describe("Feature workflows", () => {
  it("catalog renders actual API data and switches views", async () => {
    renderPage(<Catalog />);
    await screen.findByText("Scott Meyers");
    fireEvent.click(screen.getByLabelText("List view"));
    expect(
      screen.getByText("Scott Meyers").closest(".resource-list"),
    ).not.toBeNull();
    fireEvent.click(screen.getByText("Filters"));
    expect(screen.getByLabelText("Author")).toBeVisible();
    fireEvent.change(screen.getByLabelText("Author"), {
      target: { value: "Meyers" },
    });
    fireEvent.click(screen.getByText("Clear filters"));
  });
  it("resource dialog exposes complete editing metadata", async () => {
    render(
      <QueryClientProvider client={new QueryClient()}>
        <MemoryRouter initialEntries={["/catalog/B001"]}>
          <Routes>
            <Route path="/catalog/:id" element={<ResourceDetail />} />
          </Routes>
        </MemoryRouter>
      </QueryClientProvider>,
    );
    await screen.findByRole("heading", { name: "Effective C++" });
    fireEvent.click(screen.getByText("Edit resource"));
    expect(screen.queryByLabelText("Digital rights management")).toBeNull();
    expect(screen.getByLabelText("Binding")).toHaveValue("0");
    expect(screen.getByLabelText("ISBN")).toHaveValue("978");
  });
  it("loan history filter and partial return form", async () => {
    renderPage(<Loans />);
    await screen.findByText("L1");
    fireEvent.click(screen.getByText("Returned"));
    expect(screen.getByText("No matching loans on this page")).toBeVisible();
    fireEvent.click(screen.getByText("Active loans"));
    fireEvent.click(screen.getByText("Return resource"));
    expect(screen.getByLabelText("Copies to return")).toHaveAttribute(
      "max",
      "2",
    );
  });
  it("budgets show backend financial values and quota form", async () => {
    renderPage(<Budgets />);
    await screen.findByText("25% of budget used");
    expect(screen.getAllByText("INR 100.00")).toHaveLength(3);
    fireEvent.click(screen.getByText("Configure a category quota"));
    expect(screen.getByLabelText("Maximum units")).toBeVisible();
  });
  it("vendors show ratings and functional offer form", async () => {
    renderPage(<Vendors />);
    await screen.findByText("Academic Press");
    expect(screen.getByText("Rating 4.5 / 5")).toBeVisible();
    expect(screen.getByLabelText("Price")).toBeVisible();
  });
  it("admin manages access using a dialog", async () => {
    renderPage(<Users />);
    await screen.findByText("Asha Rao");
    fireEvent.click(screen.getByText("Manage"));
    expect(screen.getByRole("dialog")).toHaveAccessibleName("Manage Asha Rao");
    expect(screen.getByLabelText("Account status")).toHaveValue("true");
  });
  it("reports render two structured sources", async () => {
    renderPage(<Reports />);
    await screen.findByText("Lending & resource utilization");
    expect(screen.getByText("Acquisition outcomes")).toBeVisible();
  });
  it("events use cursor feed", async () => {
    renderPage(<Events />);
    await screen.findByText("Borrow");
    expect(fetch).toHaveBeenCalledWith(
      expect.stringContaining("/events/feed"),
      expect.anything(),
    );
    expect(screen.getByText("Next")).toBeDisabled();
  });
  it("acquisitions show persisted history", async () => {
    renderPage(<Acquisitions />);
    await screen.findByRole("heading", { name: "Effective C++" });
    expect(screen.getByLabelText("Vendor strategy")).toHaveValue("Cheapest");
  });
  it("dashboard presents personal and financial state", async () => {
    renderPage(<Dashboard />);
    await screen.findByText("25% of budget used");
    expect(screen.getByText("Welcome back, Asha.")).toBeVisible();
  });
  it("student navigation hides privileged links", () => {
    state.role = "Student";
    state.permissions = ["catalog.read", "loan.self"];
    renderPage(<Shell />);
    expect(screen.queryByText("Administration")).toBeNull();
    expect(screen.getByText("Lending")).toBeVisible();
  });
  it("assistant sends a prompt and renders server response", async () => {
    renderPage(<Assistant />);
    fireEvent.change(screen.getByLabelText("Message Libra"), {
      target: { value: "Find books by Scott Meyers" },
    });
    fireEvent.click(screen.getByLabelText("Send message"));
    await screen.findByText("Authoritative results");
  });
});
