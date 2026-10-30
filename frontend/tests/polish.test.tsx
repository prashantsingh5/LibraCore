import { describe, it, expect, vi, beforeEach } from "vitest";
import { render, screen, fireEvent, waitFor } from "@testing-library/react";
import { MemoryRouter } from "react-router-dom";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import type { ReactNode } from "react";
import {
  money,
  toMinor,
  date,
  roleLabel,
  greetingName,
  loanStatus,
} from "../src/lib/presentation";
import { ResourceForm } from "../src/features/catalog/ResourceForm";
import { ResourceCover } from "../src/components/ui/ResourceCover";
import { Budgets } from "../src/features/budgets/Budgets";
import { Gate } from "../src/components/layout/Gate";
import { DataView } from "../src/components/feedback/DataView";
import { ResultCard } from "../src/components/feedback/ResultCard";
import { Catalog } from "../src/features/catalog/Catalog";
import { ActionForm } from "../src/components/ui/ActionForm";
const state = vi.hoisted(() => ({
  role: "Administrator",
  permissions: [
    "catalog.read",
    "resource.manage",
    "budget.read",
    "budget.manage",
    "user.manage",
  ],
}));
vi.mock("../src/features/auth/Auth", () => ({
  useAuth: () => ({
    user: {
      user_id: "U",
      name: "Dr. Meera Shah",
      role: state.role,
      department_id: "CS",
      permissions: state.permissions,
    },
  }),
}));
function show(child: ReactNode) {
  return render(
    <QueryClientProvider
      client={
        new QueryClient({ defaultOptions: { queries: { retry: false } } })
      }
    >
      <MemoryRouter>{child}</MemoryRouter>
    </QueryClientProvider>,
  );
}
beforeEach(() => {
  vi.stubGlobal(
    "ResizeObserver",
    class {
      observe() {}
      unobserve() {}
      disconnect() {}
    },
  );
  state.role = "Administrator";
  state.permissions = [
    "catalog.read",
    "resource.manage",
    "budget.read",
    "budget.manage",
    "user.manage",
  ];
  vi.stubGlobal(
    "fetch",
    vi.fn(async (input: RequestInfo | URL) => ({
      ok: true,
      headers: new Headers(),
      json: async () =>
        String(input).includes("/departments?")
          ? [{ department_id: "CS", name: "Computer Science" }]
          : String(input).includes("/resources?")
            ? { items: [], limit: 12, offset: 0, next_cursor: null }
            : {
                total: { formatted: "INR 100.00" },
                spent: { formatted: "INR 10.00" },
                remaining: { formatted: "INR 90.00" },
                percentage_used: 10,
                categories: [],
              },
    })),
  );
});
describe("Presentation invariants", () => {
  it("converts currency strings exactly without financial arithmetic", () => {
    expect(toMinor("12.34")).toBe(1234);
    expect(toMinor("0.29")).toBe(29);
    expect(() => toMinor("1.001")).toThrow();
    expect(() => toMinor("1e3")).toThrow();
    expect(() => toMinor("9999999999999999")).toThrow();
    expect(money({ formatted: "INR 12.34" })).toBe("INR 12.34");
  });
  it("formats unambiguous dates, names, roles and partial returns", () => {
    expect(date("2026-11-12T12:00:00Z")).toBe("12 Nov 2026");
    expect(roleLabel("DepartmentManager")).toBe("Department Manager");
    expect(greetingName("Dr. Meera Shah")).toBe("Meera");
    expect(loanStatus({ status: 0, returned_quantity: 1 })).toBe(
      "Partially returned",
    );
  });
  it("humanizes enums, percentages and money", () => {
    show(
      <DataView
        value={{
          binding: 0,
          event_type: 2,
          unit_price_minor: 10000,
          currency: "INR",
          spend_utilization: 0.9,
        }}
      />,
    );
    expect(screen.getByText("Paperback")).toBeVisible();
    expect(screen.getByText("Borrow")).toBeVisible();
    expect(screen.getByText("0.9%")).toBeVisible();
    expect(screen.queryByText("10000")).toBeNull();
  });
  it("links cancellation to its immutable record", () => {
    show(
      <ResultCard
        operation="get_order"
        value={{
          order_id: 10,
          title: "Effective C++",
          current_status: 3,
          approved: true,
          cancellation_order_id: 11,
          quantity: 1,
          post_tax_minor: 9000,
          currency: "INR",
        }}
      />,
    );
    expect(screen.getAllByText("Cancelled").length).toBeGreaterThan(0);
    expect(screen.getByText(/Cancelled by record #11/)).toBeVisible();
  });
  it("renders focused vendor success", () => {
    show(
      <ResultCard
        operation="/vendor-offers"
        value={{
          resource_id: "B001",
          title: "Effective C++",
          vendors: [
            { name: "Economy Books", price_minor: 9000, currency: "INR" },
          ],
        }}
      />,
    );
    expect(screen.getByText("Vendor offer updated")).toBeVisible();
    expect(screen.queryByText("9000")).toBeNull();
  });
  it("falls back to bundled cover if media is missing", () => {
    show(<ResourceCover id="B" title="A title" />);
    const img = screen.getByAltText("A title cover");
    fireEvent.error(img);
    expect(img).toHaveAttribute("src", "/art/default-cover.svg");
  });
  it("sends exact ID through general search", async () => {
    show(<Catalog />);
    fireEvent.change(screen.getByLabelText("Search catalog"), {
      target: { value: "TEST-LIB-001" },
    });
    fireEvent.click(screen.getByRole("button", { name: "Search" }));
    await waitFor(() =>
      expect(fetch).toHaveBeenCalledWith(
        expect.stringContaining("q=TEST-LIB-001"),
        expect.anything(),
      ),
    );
  });
});
describe("Type-aware forms", () => {
  it.each([
    ["Book", "Binding", "Narrator"],
    ["Journal", "ISSN", "Binding"],
    ["EBook", "EBook format", "University"],
    ["AudioBook", "Narrator", "ISBN"],
    ["Thesis", "University", "License"],
    ["ElectronicResource", "License", "Binding"],
  ])("shows only %s fields", (kind, present, absent) => {
    show(<ResourceForm />);
    fireEvent.change(screen.getByLabelText("Resource type"), {
      target: { value: kind },
    });
    expect(screen.getByLabelText(present)).toBeVisible();
    expect(screen.queryByLabelText(absent)).toBeNull();
    expect(screen.queryByText(/minor units/i)).toBeNull();
  });
  it("sends initial holdings and exact price on creation", async () => {
    show(<ResourceForm />);
    fireEvent.change(screen.getByLabelText(/Resource ID/), {
      target: { value: "NEW-1" },
    });
    fireEvent.change(screen.getByLabelText("Title"), {
      target: { value: "New title" },
    });
    fireEvent.change(screen.getByLabelText(/^Price/), {
      target: { value: "19.95" },
    });
    fireEvent.change(screen.getByLabelText(/Initial holdings/), {
      target: { value: "3" },
    });
    fireEvent.click(screen.getByRole("button", { name: "Create resource" }));
    await waitFor(() => expect(fetch).toHaveBeenCalled());
    const options = vi.mocked(fetch).mock.calls[0][1];
    const body = JSON.parse(String(options?.body));
    expect(body.unit_price_minor).toBe(1995);
    expect(body.initial_holdings).toBe(3);
    expect(body).not.toHaveProperty("narrator");
  });
  it("defers budget lookup until explicit submit", async () => {
    show(<Budgets />);
    await screen.findByRole("option", { name: "Computer Science" });
    const initial = vi.mocked(fetch).mock.calls.length;
    fireEvent.change(screen.getByLabelText("Department"), {
      target: { value: "CS" },
    });
    expect(fetch).toHaveBeenCalledTimes(initial);
    fireEvent.click(screen.getByRole("button", { name: "View budget" }));
    await waitFor(() =>
      expect(fetch).toHaveBeenCalledWith(
        "/api/v1/reports/departments/CS",
        expect.anything(),
      ),
    );
  });
  it("does not expose manager department switching", async () => {
    state.role = "DepartmentManager";
    show(<Budgets />);
    await screen.findByText("Allocation in perspective");
    expect(screen.queryByLabelText("Department")).toBeNull();
    expect(fetch).toHaveBeenCalledWith(
      "/api/v1/reports/departments/CS",
      expect.anything(),
    );
  });
  it("uses a valid department selector in account creation", async () => {
    show(
      <ActionForm
        title="Create"
        path="/users"
        keys={[]}
        fields={[
          { name: "department_id", label: "Department", type: "department" },
        ]}
      />,
    );
    await screen.findByRole("option", { name: "Computer Science" });
    expect(screen.getByLabelText("Department").tagName).toBe("SELECT");
  });
});
describe("Consistent role gates", () => {
  it.each([
    "Student",
    "Faculty",
    "Librarian",
    "DepartmentManager",
    "Administrator",
  ])("%s cannot bypass administration route", (role) => {
    state.role = role;
    state.permissions =
      role === "Administrator" ? ["user.manage"] : ["catalog.read"];
    show(
      <Gate permission="user.manage">
        <h1>Private accounts</h1>
      </Gate>,
    );
    expect(screen.queryByText("Private accounts") !== null).toBe(
      role === "Administrator",
    );
    if (role !== "Administrator") {
      expect(screen.getByText("Access is restricted")).toBeVisible();
      expect(
        screen.getByRole("link", { name: "Return to workspace" }),
      ).toHaveAttribute("href", "/");
    }
  });
});
describe("Cover form requests", () => {
  it("uploads an optional file after resource creation without adding it to domain JSON", async () => {
    const resource = {
      resource_id: "NEW-1",
      resource_type: "Book",
      title: "New title",
      authors: [],
      publisher: "",
      publication_year: 2026,
      price: { formatted: "INR 19.95" },
      currency: "INR",
      unit_price_minor: 1995,
      holdings: 3,
      lent: 0,
      isbn: "",
      issn: "",
    };
    vi.stubGlobal(
      "fetch",
      vi.fn(async () => ({
        ok: true,
        headers: new Headers(),
        json: async () => resource,
      })),
    );
    show(<ResourceForm />);
    fireEvent.change(screen.getByLabelText("Resource ID"), {
      target: { value: "NEW-1" },
    });
    fireEvent.change(screen.getByLabelText("Title"), {
      target: { value: "New title" },
    });
    fireEvent.change(screen.getByLabelText("Price"), {
      target: { value: "19.95" },
    });
    const file = new File(["fixture"], "cover.png", { type: "image/png" });
    fireEvent.change(screen.getByLabelText("Upload book cover"), {
      target: { files: [file] },
    });
    fireEvent.click(screen.getByRole("button", { name: "Create resource" }));
    await screen.findByText("Cover updated.");
    expect(fetch).toHaveBeenCalledTimes(2);
    const calls = vi.mocked(fetch).mock.calls;
    expect(calls[1][0]).toBe("/api/v1/resources/NEW-1/cover");
    expect(calls[1][1]?.body).toBe(file);
    expect(JSON.parse(String(calls[0][1]?.body))).not.toHaveProperty("cover");
  });
  it("retains common input when changing resource type", () => {
    show(<ResourceForm />);
    fireEvent.change(screen.getByLabelText("Title"), {
      target: { value: "Keep my title" },
    });
    fireEvent.change(screen.getByLabelText("Resource type"), {
      target: { value: "Journal" },
    });
    expect(screen.getByLabelText("Title")).toHaveValue("Keep my title");
  });
});
