import { useState } from "react";
import { Button } from "../../components/ui/button";
import { useQuery } from "@tanstack/react-query";
import { api } from "../../services/api";
import type { Vendor } from "../../types/api";
import {
  Loading,
  ErrorState,
  Empty,
  Status,
} from "../../components/feedback/States";
import { ActionForm } from "../../components/ui/ActionForm";
export function Vendors() {
  const [offset, setOffset] = useState(0);
  const q = useQuery({
    queryKey: ["vendors", offset],
    queryFn: () => api<Vendor[]>("/vendors?limit=50&offset=" + offset),
  });
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">TRUSTED CONNECTIONS</p>
          <h1>Partners in the collection.</h1>
          <p className="muted">
            Compare quality and delivery. Let the core select the right offer.
          </p>
        </div>
      </div>
      {q.isPending ? (
        <Loading />
      ) : q.error ? (
        <ErrorState error={q.error} />
      ) : !q.data.length ? (
        <Empty title="No vendors yet" />
      ) : (
        <div className="loan-grid">
          {q.data.map((v) => (
            <article className="panel" key={v.vendor_id}>
              <span className="eyebrow">{v.vendor_id}</span>
              <h2>{v.name}</h2>
              <Status tone="success">Rating {v.rating} / 5</Status>
              <p>{v.estimated_delivery_days} day estimated delivery</p>
              <details>
                <summary>Edit vendor metadata</summary>
                <ActionForm
                  title="Update vendor"
                  path={"/vendors/" + encodeURIComponent(v.vendor_id)}
                  method="PUT"
                  keys={["vendors", "resource", "catalog"]}
                  fields={[
                    {
                      name: "name",
                      label: "Vendor name",
                      value: v.name,
                      required: true,
                    },
                    {
                      name: "rating",
                      label: "Rating",
                      type: "number",
                      value: v.rating,
                      min: 0,
                      max: 5,
                      required: true,
                    },
                    {
                      name: "estimated_delivery_days",
                      label: "Delivery days",
                      type: "number",
                      value: v.estimated_delivery_days,
                      min: 0,
                      required: true,
                    },
                  ]}
                />
              </details>
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
          Previous vendors
        </Button>
        <Button
          variant="outline"
          disabled={(q.data?.length ?? 0) < 50}
          onClick={() => setOffset(offset + 50)}
        >
          Next vendors
        </Button>
      </div>
      <section className="panel">
        <h2>Create or update a resource offer</h2>
        <p className="muted">
          Existing vendor metadata must match. Prices are validated against the
          resource currency.
        </p>
        <ActionForm
          title="Save vendor offer"
          path="/vendor-offers"
          method="PUT"
          keys={["vendors", "resource", "catalog"]}
          fields={[
            { name: "resource_id", label: "Resource ID", required: true },
            { name: "vendor_id", label: "Vendor ID", required: true },
            { name: "name", label: "Vendor name", required: true },
            {
              name: "price_minor",
              label: "Price",
              type: "money",
              min: 0,
              required: true,
            },
            {
              name: "currency",
              label: "Currency",
              value: "INR",
              required: true,
            },
            {
              name: "rating",
              label: "Rating (0–5)",
              type: "number",
              min: 0,
              max: 5,
              required: true,
            },
            {
              name: "estimated_delivery_days",
              label: "Estimated delivery days",
              type: "number",
              min: 0,
              required: true,
            },
          ]}
        />
      </section>
    </>
  );
}
