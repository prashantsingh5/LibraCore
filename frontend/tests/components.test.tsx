import { describe, it, expect, vi, afterEach } from "vitest";
import { render, screen, fireEvent, waitFor } from "@testing-library/react";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { Button } from "../src/components/ui/button";
import {
  Empty,
  ErrorState,
  Loading,
  Status,
} from "../src/components/feedback/States";
import { DataView } from "../src/components/feedback/DataView";
import { Modal } from "../src/components/ui/dialog";
import { ActionForm } from "../src/components/ui/ActionForm";
import { Confirmation } from "../src/features/assistant/Assistant";
import { NAV } from "../src/components/layout/Shell";
import { api, setCsrf } from "../src/services/api";
afterEach(() => {
  vi.unstubAllGlobals();
  setCsrf("");
});
describe("Accessible feedback", () => {
  it("has a live loading status", () => {
    render(<Loading />);
    expect(screen.getByRole("status")).toHaveAccessibleName("Loading");
  });
  it("has a meaningful empty state", () => {
    render(<Empty title="No active loans">Explore the catalog</Empty>);
    expect(screen.getByText("Explore the catalog")).toBeVisible();
  });
  it("announces errors", () => {
    render(<ErrorState error={new Error("Permission denied")} />);
    expect(screen.getByRole("alert")).toHaveTextContent("Permission denied");
  });
  it("does not rely on color", () => {
    render(<Status tone="warning">Overdue</Status>);
    expect(screen.getByText("Overdue")).toBeVisible();
  });
  it("disables pending controls", () => {
    render(<Button disabled>Submit</Button>);
    expect(screen.getByRole("button")).toBeDisabled();
  });
  it("redacts token fields in structured cards", () => {
    render(
      <DataView
        value={{
          token: "SECRET",
          cost: { formatted: "INR 100.00" },
          approved: true,
        }}
      />,
    );
    expect(screen.queryByText("SECRET")).toBeNull();
    expect(screen.getByText("INR 100.00")).toBeVisible();
  });
  it("renders lists and null safely", () => {
    render(<DataView value={[{ resource_id: "B001", value: null }]} />);
    expect(screen.getByText("B001")).toBeVisible();
  });
  it("dialog is named and described", () => {
    render(
      <Modal
        open
        onOpenChange={() => {}}
        title="Return book"
        description="Review your return"
      >
        <p>Quantity 1</p>
      </Modal>,
    );
    expect(screen.getByRole("dialog")).toHaveAccessibleName("Return book");
  });
});
describe("Server state and confirmation", () => {
  it("requires confirmation before sending a mutation", async () => {
    const fetcher = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => ({ approved: true }),
      headers: new Headers(),
    });
    vi.stubGlobal("fetch", fetcher);
    render(
      <QueryClientProvider client={new QueryClient()}>
        <ActionForm
          title="Purchase"
          path="/acquisitions"
          confirm
          keys={[]}
          fields={[
            {
              name: "quantity",
              label: "Quantity",
              type: "number",
              value: 1,
              required: true,
            },
          ]}
        />
      </QueryClientProvider>,
    );
    fireEvent.click(screen.getByRole("button", { name: "Purchase" }));
    expect(fetcher).not.toHaveBeenCalled();
    fireEvent.click(screen.getByRole("button", { name: "Confirm purchase" }));
    await waitFor(() => expect(fetcher).toHaveBeenCalledTimes(1));
  });
  it("assistant confirmation cannot be replayed from the card", async () => {
    const fetcher = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => ({ approved: true }),
      headers: new Headers(),
    });
    vi.stubGlobal("fetch", fetcher);
    render(
      <Confirmation
        proposal={{
          token: "opaque",
          action: "submit_purchase",
          parameters: { quantity: 1 },
          expires_in_seconds: 300,
        }}
        onConfirmed={async () => {}}
      />,
    );
    fireEvent.click(screen.getByRole("button", { name: "Confirm action" }));
    await screen.findByText("Action completed.");
    expect(screen.queryByRole("button", { name: "Confirm action" })).toBeNull();
    expect(fetcher).toHaveBeenCalledTimes(1);
  });
  it("dismiss does not execute", () => {
    const fetcher = vi.fn();
    vi.stubGlobal("fetch", fetcher);
    render(
      <Confirmation
        proposal={{
          token: "t",
          action: "cancel_purchase",
          parameters: {},
          expires_in_seconds: 300,
        }}
        onConfirmed={async () => {}}
      />,
    );
    fireEvent.click(screen.getByText("Dismiss request"));
    expect(fetcher).not.toHaveBeenCalled();
  });
  it("typed client sends cookie mode and csrf", async () => {
    const f = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => ({ ok: true }),
      headers: new Headers(),
    });
    vi.stubGlobal("fetch", f);
    setCsrf("anti-forgery");
    await api("/loans", { quantity: 1 });
    expect(f.mock.calls[0][1].credentials).toBe("same-origin");
    expect(f.mock.calls[0][1].headers["X-CSRF-Token"]).toBe("anti-forgery");
  });
  it("client preserves structured errors", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: false,
        status: 403,
        json: async () => ({
          error: { code: "FORBIDDEN", message: "Not permitted" },
        }),
        headers: new Headers(),
      }),
    );
    await expect(api("/users")).rejects.toMatchObject({
      status: 403,
      code: "FORBIDDEN",
    });
  });
  it("navigation assigns explicit permissions", () => {
    expect(NAV.find((x) => x.path === "/users")?.permission).toBe(
      "user.manage",
    );
    expect(NAV.find((x) => x.path === "/budgets")?.permission).toBe(
      "budget.read",
    );
  });
});
