import { ResultCard } from "../../components/feedback/ResultCard";
import { orderStatus } from "../../lib/presentation";
import { useState, type FormEvent } from "react";
import { useQuery, useQueryClient } from "@tanstack/react-query";
import { api, requestId } from "../../services/api";
import type { Order, Resource } from "../../types/api";
import { useAuth } from "../auth/Auth";
import {
  Loading,
  ErrorState,
  Empty,
  Status,
} from "../../components/feedback/States";
import { Button } from "../../components/ui/button";
import { Modal } from "../../components/ui/dialog";
import { ActionForm } from "../../components/ui/ActionForm";
import { DataView } from "../../components/feedback/DataView";
export function Acquisitions() {
  const { user } = useAuth();
  const qc = useQueryClient();
  const department =
    user!.role === "DepartmentManager" ? user!.department_id : "";
  const [offset, setOffset] = useState(0);
  const [search, setSearch] = useState("");
  const resources = useQuery({
    queryKey: ["catalog", "acquisition-picker", search],
    queryFn: () =>
      api<{ items: Resource[] }>(
        "/resources?limit=20&title=" + encodeURIComponent(search),
      ),
  });
  const orders = useQuery({
    queryKey: ["orders", department, offset],
    queryFn: () =>
      api<Order[]>(
        "/acquisitions?department_id=" +
          encodeURIComponent(department) +
          "&offset=" +
          offset,
      ),
  });
  const [quote, setQuote] = useState<unknown>(null);
  const [proposal, setProposal] = useState<Record<string, unknown> | null>(
    null,
  );
  const [error, setError] = useState<Error | null>(null);
  const [pending, setPending] = useState(false);
  const [cancel, setCancel] = useState<Order | null>(null);
  const [result, setResult] = useState<Order | null>(null);
  async function review(e: FormEvent<HTMLFormElement>) {
    e.preventDefault();
    const f = new FormData(e.currentTarget);
    const p = {
      resource_id: String(f.get("resource_id")),
      quantity: Number(f.get("quantity")),
      department_id: department || String(f.get("department_id") ?? ""),
      vendor_selection: String(f.get("vendor_selection")),
    };
    setPending(true);
    setError(null);
    try {
      setQuote(await api("/acquisitions/check", p));
      setProposal({ ...p, request_id: requestId() });
    } catch (e) {
      setError(e as Error);
    } finally {
      setPending(false);
    }
  }
  async function submit() {
    if (!proposal) return;
    setPending(true);
    setError(null);
    try {
      setResult(await api<Order>("/acquisitions", proposal));
      setProposal(null);
      for (const key of ["orders", "budget", "catalog", "events"])
        await qc.invalidateQueries({ queryKey: [key] });
    } catch (e) {
      setError(e as Error);
    } finally {
      setPending(false);
    }
  }
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">BUILD THE COLLECTION</p>
          <h1>Considered acquisitions.</h1>
          <p className="muted">
            Review a quote. Understand the impact. Make an informed decision.
          </p>
        </div>
      </div>
      <section className="panel">
        <h2>Plan a purchase</h2>
        <label>
          Find a resource by title
          <input
            value={search}
            onChange={(e) => setSearch(e.target.value)}
            placeholder="Filter the resource suggestions"
          />
        </label>
        <form className="action-form" onSubmit={review}>
          <div className="form-grid">
            <label>
              Resource ID
              <input
                name="resource_id"
                required
                placeholder="B001"
                list="acquisition-resources"
              />
              <datalist id="acquisition-resources">
                {resources.data?.items.map((r) => (
                  <option key={r.resource_id} value={r.resource_id}>
                    {r.title}
                  </option>
                ))}
              </datalist>
            </label>
            <label>
              Quantity
              <input
                name="quantity"
                type="number"
                min={1}
                step={1}
                defaultValue={1}
                required
              />
            </label>
            <label>
              Department
              <input
                name="department_id"
                defaultValue={department}
                readOnly={!!department}
                placeholder="Blank for central budget"
              />
            </label>
            <label>
              Vendor strategy
              <select name="vendor_selection">
                <option>Cheapest</option>
                <option>Fastest</option>
                <option>HighestRated</option>
              </select>
            </label>
          </div>
          {error && <ErrorState error={error} />}
          <Button disabled={pending}>Get quote & check eligibility</Button>
        </form>
      </section>
      {result && (
        <section className="panel" role="status">
          <h2>{result.approved ? "Purchase approved" : "Purchase rejected"}</h2>
          <ResultCard operation="submit_purchase" value={result} />
        </section>
      )}
      <div className="section-heading">
        <h2>Acquisition history</h2>
        <span className="muted small">Latest records first</span>
      </div>
      {orders.isPending ? (
        <Loading />
      ) : orders.error ? (
        <ErrorState error={orders.error} />
      ) : !orders.data.length ? (
        <Empty title="No purchase history" />
      ) : (
        <div className="loan-grid">
          {orders.data.map((o) => (
            <article className="panel" key={o.order_id}>
              <div className="section-heading">
                <span className="eyebrow">ORDER {o.order_id}</span>
                <Status tone={o.approved ? "success" : "warning"}>
                  {orderStatus(o)}
                </Status>
              </div>
              <h3>{o.title || o.requested_resource_id}</h3>
              {o.cancellation_order_id && (
                <p>
                  Cancelled by record #{o.cancellation_order_id}. See order
                  history for the refund.
                </p>
              )}
              <p>
                {o.quantity} units ·{" "}
                {o.requested_department || "Central library"}
              </p>
              <p className="muted">{o.reason}</p>
              <Button variant="outline" onClick={() => setCancel(o)}>
                View order & cancellation
              </Button>
            </article>
          ))}
        </div>
      )}
      <div className="pagination">
        <Button
          variant="outline"
          disabled={offset === 0}
          onClick={() => setOffset(Math.max(0, offset - 50))}
        >
          Previous
        </Button>
        <Button
          variant="outline"
          disabled={(orders.data?.length ?? 0) < 50}
          onClick={() => setOffset(offset + 50)}
        >
          Next
        </Button>
      </div>
      <Modal
        open={!!proposal}
        onOpenChange={(v) => {
          if (!v && !pending) setProposal(null);
        }}
        title="Review your acquisition"
        description="This quote comes from C++. Availability, budgets and quotas are checked again when you submit."
      >
        <ResultCard operation="check_purchase" value={quote} />
        <DataView value={proposal} />
        {error && <ErrorState error={error} />}
        <div className="button-row">
          <Button
            variant="outline"
            onClick={() => setProposal(null)}
            disabled={pending}
          >
            Keep editing
          </Button>
          <Button onClick={() => void submit()} disabled={pending}>
            {pending ? "Submitting…" : "Confirm purchase"}
          </Button>
        </div>
      </Modal>
      <Modal
        open={!!cancel}
        onOpenChange={(v) => {
          if (!v) setCancel(null);
        }}
        title={"Order " + cancel?.order_id}
        description="Cancellation is validated against holdings, fiscal year and prior cancellation history."
      >
        {cancel && <OrderDetails order={cancel} />}
      </Modal>
    </>
  );
}
function OrderDetails({ order }: { order: Order }) {
  const q = useQuery({
    queryKey: ["orders", order.order_id],
    queryFn: () => api<Order>("/acquisitions/" + order.order_id),
  });
  return (
    <>
      {q.data && <ResultCard operation="get_order" value={q.data} />}
      {q.data && (q.data.current_status ?? q.data.status) === 1 && (
        <ActionForm
          title="Cancel purchase"
          path={"/acquisitions/" + order.order_id + "/cancel"}
          confirm
          keys={["orders", "budget", "catalog", "events"]}
          fields={[]}
          extra={{ order_id: order.order_id, request_id: "auto" }}
        />
      )}
    </>
  );
}
