import { date, loanStatus } from "../../lib/presentation";
import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { Link } from "react-router-dom";
import { api } from "../../services/api";
import type { Loan } from "../../types/api";
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
export function Loans() {
  const { user } = useAuth();
  const [all, setAll] = useState(false);
  const [filter, setFilter] = useState("active");
  const [offset, setOffset] = useState(0);
  const [selected, setSelected] = useState<Loan | null>(null);
  const q = useQuery({
    queryKey: ["loans", user?.user_id, all, offset],
    queryFn: () =>
      api<Loan[]>(
        (all
          ? "/loans"
          : "/users/" + encodeURIComponent(user!.user_id) + "/loans") +
          "?limit=50&offset=" +
          offset,
      ),
  });
  const items =
    q.data?.filter(
      (l) =>
        filter === "all" ||
        (filter === "active" ? l.status === 0 : l.status === 1),
    ) ?? [];
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">IN CIRCULATION</p>
          <h1>{all ? "Lending operations" : "Your reading desk"}</h1>
          <p className="muted">
            Keep track of what’s with you, and what comes next.
          </p>
        </div>
        <Button asChild variant="outline">
          <Link to="/catalog">Explore catalog →</Link>
        </Button>
      </div>
      <div className="tabs">
        {["active", "returned", "all"].map((v) => (
          <button
            className={v === filter ? "active" : ""}
            key={v}
            onClick={() => setFilter(v)}
          >
            {v === "all"
              ? "History"
              : v === "active"
                ? "Active loans"
                : "Returned"}
          </button>
        ))}
        {user?.permissions.includes("loan.manage") && (
          <label className="checkbox">
            <input
              type="checkbox"
              checked={all}
              onChange={(e) => {
                setAll(e.target.checked);
                setOffset(0);
              }}
            />
            All borrowers
          </label>
        )}
      </div>
      {q.isPending ? (
        <Loading />
      ) : q.error ? (
        <ErrorState error={q.error} />
      ) : !items.length ? (
        <Empty title="No matching loans on this page">
          Your lending activity will appear here.
        </Empty>
      ) : (
        <div className="loan-grid">
          {items.map((l) => (
            <article className="panel loan-card" key={l.loan_id}>
              <div className="section-heading">
                <span className="eyebrow">{l.loan_id}</span>
                <Status
                  tone={
                    l.status === 1
                      ? "info"
                      : new Date(l.due_at) < new Date()
                        ? "warning"
                        : "success"
                  }
                >
                  {l.status === 1
                    ? "Returned"
                    : new Date(l.due_at) < new Date()
                      ? "Overdue"
                      : loanStatus(l)}
                </Status>
              </div>
              <h2>
                <Link to={"/catalog/" + encodeURIComponent(l.resource_id)}>
                  {l.title || l.resource_id}
                </Link>
              </h2>
              <p className="muted">{l.resource_id}</p>
              <dl>
                <dt>Due date</dt>
                <dd>{date(l.due_at)}</dd>
                <dt>Copies returned</dt>
                <dd>
                  {l.returned_quantity} / {l.quantity}
                </dd>
                {all && (
                  <>
                    <dt>Borrower</dt>
                    <dd>{l.borrower_id}</dd>
                  </>
                )}
              </dl>
              {l.status === 0 && (
                <Button variant="outline" onClick={() => setSelected(l)}>
                  Return resource
                </Button>
              )}
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
          disabled={(q.data?.length ?? 0) < 50}
          onClick={() => setOffset(offset + 50)}
        >
          Next
        </Button>
      </div>
      <Modal
        open={!!selected}
        onOpenChange={(v) => {
          if (!v) setSelected(null);
        }}
        title="Return to the commons"
        description="You can return all copies or make a partial return."
      >
        {selected && (
          <ActionForm
            title="Return resource"
            path={"/loans/" + encodeURIComponent(selected.loan_id) + "/return"}
            confirm
            keys={["loans", "resource", "catalog"]}
            extra={{ loan_id: selected.loan_id, request_id: "auto" }}
            fields={[
              {
                name: "quantity",
                label: "Copies to return",
                type: "number",
                min: 1,
                max: selected.quantity - selected.returned_quantity,
                value: 1,
                required: true,
              },
            ]}
          />
        )}
      </Modal>
    </>
  );
}
