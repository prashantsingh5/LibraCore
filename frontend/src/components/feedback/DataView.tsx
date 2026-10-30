import {
  money,
  date,
  enumLabel,
  roleLabel,
  loanStatus,
  orderStatus,
} from "../../lib/presentation";
import { label } from "../../lib/utils";
// Structured semantic output: no JSON dumps and no HTML supplied by the model.
export function DataView({ value }: { value: unknown }) {
  if (value === null || value === undefined)
    return <span className="muted">—</span>;
  if (typeof value !== "object")
    return (
      <span>
        {typeof value === "boolean" ? (value ? "Yes" : "No") : String(value)}
      </span>
    );
  if (Array.isArray(value))
    return (
      <div className="data-list">
        {value.length ? (
          value.map((v, i) => (
            <div className="data-item" key={i}>
              <DataView value={v} />
            </div>
          ))
        ) : (
          <span className="muted">No records</span>
        )}
      </div>
    );
  const obj = value as Record<string, unknown>;
  if (typeof obj.formatted === "string")
    return <strong>{obj.formatted}</strong>;
  return (
    <dl className="data-grid">
      {Object.entries(obj)
        .filter(([k]) => !["token", "request_id", "csrf_token"].includes(k))
        .map(([k, v]) => (
          <div key={k}>
            <dt>{label(k.replace(/_minor$/, ""))}</dt>
            <dd>
              {k.endsWith("_minor") ? (
                money(v, String(obj.currency ?? "INR"))
              ) : k.endsWith("_utilization") || k === "percentage_used" ? (
                v == null ? (
                  "Not limited"
                ) : (
                  String(v) + "%"
                )
              ) : k.endsWith("_at") ? (
                date(v, true)
              ) : k === "role" ? (
                roleLabel(String(v))
              ) : k === "status" || k === "current_status" ? (
                obj.loan_id ? (
                  loanStatus(obj)
                ) : (
                  orderStatus(obj)
                )
              ) : [
                  "binding",
                  "format",
                  "license",
                  "category",
                  "event_type",
                  "record_type",
                ].includes(k) ? (
                enumLabel(k, v)
              ) : (
                <DataView value={v} />
              )}
            </dd>
          </div>
        ))}
    </dl>
  );
}
