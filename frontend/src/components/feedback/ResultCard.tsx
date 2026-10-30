import { Link } from "react-router-dom";
import { money, date, loanStatus, orderStatus } from "../../lib/presentation";
import { DataView } from "./DataView";
import { ResourceCover } from "../ui/ResourceCover";
import { Status } from "./States";
type RecordValue = Record<string, unknown>;
const obj = (v: unknown): RecordValue =>
  v && typeof v === "object" && !Array.isArray(v) ? (v as RecordValue) : {};
export function TechnicalDetails({ value }: { value: unknown }) {
  return (
    <details className="technical">
      <summary>Technical details</summary>
      <DataView value={value} />
    </details>
  );
}
export function ResultCard({
  operation,
  value,
  context = {},
}: {
  operation: string;
  value: unknown;
  context?: RecordValue;
}) {
  const r = obj(value);
  if (r.operation && r.result)
    return <ResultCard operation={String(r.operation)} value={r.result} />;
  const currency = String(r.currency ?? obj(r.cost).currency ?? "INR");
  if (operation === "search_resources")
    return (
      <div className="assistant-results">
        {(Array.isArray(r.items) ? r.items : []).map((item, i) => {
          const v = obj(item);
          return (
            <Link
              className="search-result"
              key={i}
              to={"/catalog/" + encodeURIComponent(String(v.resource_id))}
            >
              <ResourceCover
                id={String(v.resource_id)}
                title={String(v.title)}
                type={String(v.resource_type)}
              />
              <div>
                <h3>{String(v.title)}</h3>
                <p>{Array.isArray(v.authors) ? v.authors.join(", ") : ""}</p>
                <p>
                  {String(v.resource_type)} ·{" "}
                  {Number(v.holdings) - Number(v.lent)} available
                </p>
                <strong>{money(v.price)}</strong>
              </div>
            </Link>
          );
        })}
      </div>
    );
  if (operation.includes("availability"))
    return (
      <section className="result-summary">
        <h3>Available in your library</h3>
        <div className="stat-grid">
          {["holdings", "borrowed", "available"].map((k) => (
            <div key={k}>
              <span>
                {k === "holdings"
                  ? "Total holdings"
                  : k === "borrowed"
                    ? "Currently borrowed"
                    : "Available now"}
              </span>
              <strong>{String(r[k] ?? 0)}</strong>
            </div>
          ))}
        </div>
      </section>
    );
  if (operation.includes("check") || operation === "quote_purchase") {
    const vendor = obj(r.selected_vendor);
    return (
      <section className="result-summary">
        <Status tone={r.allowed === false ? "warning" : "success"}>
          {r.allowed === false ? "Purchase not eligible" : "Purchase eligible"}
        </Status>
        <h3>{String(r.title ?? "Purchase review")}</h3>
        {r.quantity !== undefined && <p>Quantity: {String(r.quantity)}</p>}
        <p>{String(r.reason || "All budget and quota checks passed.")}</p>
        {!!vendor.name && (
          <p>
            {String(vendor.name)} ·{" "}
            {money(
              vendor.price ?? vendor.price_minor,
              String(vendor.currency ?? currency),
            )}{" "}
            ·{" "}
            {String(
              vendor.estimated_delivery_days ?? vendor.deliveryDays ?? "—",
            )}{" "}
            days delivery
          </p>
        )}
        <div className="impact-row">
          <div>
            Before
            <strong>{money(obj(r.budget_before).remaining, currency)}</strong>
          </div>
          <div>
            Cost<strong>{money(r.cost, currency)}</strong>
          </div>
          <div>
            After
            <strong>
              {r.budget_after
                ? money(obj(r.budget_after).remaining, currency)
                : "Not available"}
            </strong>
          </div>
        </div>
        <details>
          <summary>View detailed quota calculations</summary>
          <DataView value={r} />
        </details>
      </section>
    );
  }
  if (r.loan_id) {
    return (
      <section className="result-summary">
        <Status tone="success">
          {operation.includes("return")
            ? "Resource returned successfully"
            : "Loan recorded successfully"}
        </Status>
        <h3>{String(r.title ?? r.resource_id)}</h3>
        <p>
          Loan {String(r.loan_id)} · {loanStatus(r)}
        </p>
        <p>
          {operation.includes("return")
            ? "Total returned: " + String(r.returned_quantity)
            : "Borrowed: " + String(r.quantity)}{" "}
          copies
        </p>
        <p>
          {operation.includes("return")
            ? r.returned_at
              ? "Returned on: " + date(r.returned_at, true)
              : "Partial return recorded; this loan remains active."
            : "Due: " + date(r.due_at)}
        </p>
        <TechnicalDetails value={r} />
      </section>
    );
  }
  if (r.order_id) {
    return (
      <section className="result-summary">
        <Status tone={r.approved === false ? "warning" : "success"}>
          {operation.includes("cancel")
            ? "Cancellation recorded"
            : orderStatus(r)}
        </Status>
        <h3>
          {String(r.title ?? "Order")} · Order {String(r.order_id)}
        </h3>
        <p>
          {String(r.quantity)} copies ·{" "}
          {money(r.cost ?? r.post_tax_minor, currency)}
        </p>
        {!!r.reason && <p>{String(r.reason)}</p>}
        {!!r.cancellation_order_id && (
          <p>
            Cancelled by record #{String(r.cancellation_order_id)}. Original
            purchase preserved in history.
          </p>
        )}
        <TechnicalDetails value={r} />
      </section>
    );
  }
  if (operation === "/vendor-offers") {
    return (
      <section className="result-summary">
        <Status tone="success">Vendor offer updated</Status>
        <h3>
          {String(r.title)} · {String(r.resource_id)}
        </h3>
        {(Array.isArray(r.vendors) ? r.vendors : [])
          .filter(
            (v) => !context.vendor_id || obj(v).vendor_id === context.vendor_id,
          )
          .map((v, i) => {
            const offer = obj(v);
            return (
              <div key={i}>
                <p>
                  {String(offer.name ?? offer.vendor_id)} ·{" "}
                  {String(offer.vendor_id ?? "")}
                </p>
                <p>
                  Price:{" "}
                  {money(
                    offer.price ?? offer.price_minor,
                    String(offer.currency ?? currency),
                  )}
                </p>
                <p>
                  Rating: {String(offer.rating ?? "—")} / 5 · Delivery:{" "}
                  {String(offer.estimated_delivery_days ?? "—")} days
                </p>
              </div>
            );
          })}
      </section>
    );
  }
  if (r.total && r.remaining)
    return (
      <section className="result-summary">
        <h3>Budget overview</h3>
        <div className="impact-row">
          {["total", "spent", "remaining"].map((k) => (
            <div key={k}>
              {k}
              <strong>{money(r[k])}</strong>
            </div>
          ))}
        </div>
        <p>{String(r.percentage_used ?? 0)}% of budget used</p>
        <TechnicalDetails value={r} />
      </section>
    );
  if (operation.includes("/resources"))
    return (
      <section className="result-summary">
        <p className="success-message">Resource saved successfully.</p>
        <h3>{String(r.title ?? "Resource")}</h3>
        <p>
          {String(r.resource_id ?? "")} · {money(r.price, currency)}
        </p>
        <TechnicalDetails value={r} />
      </section>
    );
  if (r.ok || r.deleted)
    return (
      <p className="success-message" role="status">
        {String(
          r.message ??
            (operation.includes("/access")
              ? "Access updated successfully."
              : operation.includes("credentials")
                ? "Credentials updated successfully."
                : "Changes saved successfully."),
        )}
      </p>
    );
  return (
    <>
      <p className="success-message">
        {operation === "/users"
          ? "Account created successfully."
          : operation.includes("resources")
            ? "Resource saved successfully."
            : "Completed successfully."}
      </p>
      <DataView value={value} />
    </>
  );
}
