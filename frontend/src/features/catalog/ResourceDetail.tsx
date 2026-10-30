import { ResourceCover } from "../../components/ui/ResourceCover";
import { enumLabel, money } from "../../lib/presentation";
import { useState } from "react";
import { useParams, Link } from "react-router-dom";
import { useQuery } from "@tanstack/react-query";
import { ArrowLeft } from "lucide-react";
import { api } from "../../services/api";
import type { Resource } from "../../types/api";
import { useAuth } from "../auth/Auth";
import { Loading, ErrorState, Status } from "../../components/feedback/States";
import { Button } from "../../components/ui/button";
import { Modal } from "../../components/ui/dialog";
import { ActionForm } from "../../components/ui/ActionForm";
import { DataView } from "../../components/feedback/DataView";
import { ResourceForm } from "./ResourceForm";
export function ResourceDetail() {
  const { id = "" } = useParams();
  const { user } = useAuth();
  const [edit, setEdit] = useState(false);
  const q = useQuery({
    queryKey: ["resource", id],
    queryFn: () => api<Resource>("/resources/" + encodeURIComponent(id)),
  });
  if (q.isPending) return <Loading />;
  if (q.error) return <ErrorState error={q.error} />;
  const r = q.data;
  return (
    <>
      <Link className="back-link" to="/catalog">
        <ArrowLeft size={16} />
        Back to catalog
      </Link>
      <div className="resource-detail">
        <ResourceCover
          id={r.resource_id}
          title={r.title}
          type={r.resource_type}
          version={q.dataUpdatedAt}
        />
        <div>
          <p className="eyebrow">
            {r.resource_type} · {r.publication_year}
          </p>
          <h1>{r.title}</h1>
          <p className="lead muted">{r.authors.join(", ") || r.publisher}</p>
          <Status tone={r.holdings > r.lent ? "success" : "warning"}>
            {r.holdings - r.lent} available · {r.holdings} holdings
          </Status>
          <p className="detail-price">{r.price.formatted}</p>
          <p className="muted">
            Published by {r.publisher}. Resource {r.resource_id}.
          </p>
          {user?.permissions.includes("resource.manage") && (
            <Button variant="outline" onClick={() => setEdit(true)}>
              Edit resource
            </Button>
          )}
        </div>
      </div>
      <section className="stat-grid availability">
        {[
          ["Total holdings", r.holdings],
          ["Currently borrowed", r.lent],
          ["Available now", r.holdings - r.lent],
        ].map(([label, value]) => (
          <div className="stat" key={label}>
            <span>{label}</span>
            <strong>{value}</strong>
          </div>
        ))}
      </section>
      <div className="two-column">
        <section className="panel">
          <h2>Borrow this resource</h2>
          <p className="muted">
            The library assigns your due date and validates availability.
          </p>
          <ActionForm
            title="Borrow resource"
            path="/loans"
            keys={["loans", "resource", "catalog"]}
            confirm
            extra={{
              resource_id: id,
              borrower_id: user!.user_id,
              request_id: "auto",
            }}
            fields={[
              {
                name: "quantity",
                label: "Copies or seats",
                type: "number",
                value: 1,
                min: 1,
                required: true,
              },
            ]}
          />
        </section>
        <section className="panel">
          <h2>Resource details</h2>
          <dl className="data-grid">
            {["Book", "EBook"].includes(r.resource_type) && (
              <>
                <div>
                  <dt>Authors</dt>
                  <dd>{r.authors.join(", ") || "—"}</dd>
                </div>
                <div>
                  <dt>ISBN</dt>
                  <dd>{r.isbn || "—"}</dd>
                </div>
              </>
            )}
            {(r.resource_type === "Book"
              ? ["binding", "edition"]
              : r.resource_type === "Journal"
                ? ["issn", "issues_per_year", "subscription_years"]
                : r.resource_type === "Thesis"
                  ? ["university", "degree", "supervisor"]
                  : r.resource_type === "AudioBook"
                    ? ["narrator", "duration_minutes", "license"]
                    : r.resource_type === "EBook"
                      ? ["format", "drm", "license"]
                      : ["license"]
            ).map((k) => (
              <div key={k}>
                <dt>{k.replaceAll("_", " ")}</dt>
                <dd>
                  {typeof r[k] === "boolean"
                    ? r[k]
                      ? "Yes"
                      : "No"
                    : enumLabel(k, r[k])}
                </dd>
              </div>
            ))}
            {["EBook", "AudioBook", "ElectronicResource"].includes(
              r.resource_type,
            ) && (
              <>
                <div>
                  <dt>Access</dt>
                  <dd>{String(r.access_url || "Not provided")}</dd>
                </div>
                <div>
                  <dt>Platform fee</dt>
                  <dd>{money(r.platform_fee_minor, r.currency)}</dd>
                </div>
              </>
            )}
          </dl>
        </section>
      </div>
      {r.vendors && (
        <section className="panel">
          <h2>Vendor offers</h2>
          <DataView value={r.vendors} />
        </section>
      )}
      <Modal
        open={edit}
        onOpenChange={setEdit}
        title="Edit resource"
        description="Resource type and currency are immutable once referenced."
      >
        <ResourceForm resource={r} />
      </Modal>
    </>
  );
}
