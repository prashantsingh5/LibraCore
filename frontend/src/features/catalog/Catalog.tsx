import { ResourceCover } from "../../components/ui/ResourceCover";
import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { Link, useSearchParams } from "react-router-dom";
import { Grid2X2, List, Search, SlidersHorizontal, Plus } from "lucide-react";
import { api } from "../../services/api";
import type { Resource } from "../../types/api";
import { useAuth } from "../auth/Auth";
import {
  Loading,
  ErrorState,
  Empty,
  Status,
} from "../../components/feedback/States";
import { Button } from "../../components/ui/button";
import { Modal } from "../../components/ui/dialog";
import { ResourceForm } from "./ResourceForm";
export function Catalog() {
  const [params, setParams] = useSearchParams();
  const [view, setView] = useState("cards");
  const [advanced, setAdvanced] = useState(false);
  const [create, setCreate] = useState(false);
  const { user } = useAuth();
  const q = new URLSearchParams(params);
  q.set("limit", "12");
  const data = useQuery({
    queryKey: ["catalog", q.toString()],
    queryFn: () =>
      api<{ items: Resource[]; next_cursor: string | null }>("/resources?" + q),
  });
  function change(k: string, v: string) {
    const p = new URLSearchParams(params);
    if (v) p.set(k, v);
    else p.delete(k);
    p.delete("after");
    p.delete("offset");
    setParams(p, { replace: true });
  }
  const offset = Number(params.get("offset") ?? 0);
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">THE COLLECTION</p>
          <h1>Find your next perspective.</h1>
          <p className="muted">
            Books, journals and digital resources. One connected catalog.
          </p>
        </div>
        {user?.permissions.includes("resource.manage") && (
          <Button onClick={() => setCreate(true)}>
            <Plus size={18} />
            Add resource
          </Button>
        )}
      </div>
      <section className="filter-panel">
        <form
          className="search-form"
          onSubmit={(e) => {
            e.preventDefault();
            change("q", String(new FormData(e.currentTarget).get("q") ?? ""));
          }}
        >
          <Search />
          <input
            aria-label="Search catalog"
            name="q"
            defaultValue={params.get("q") ?? ""}
            placeholder="Title, author, publisher, ISBN or resource ID"
          />
          <Button type="submit">Search</Button>
        </form>
        <div className="filter-row">
          <Button
            variant="outline"
            onClick={() => setAdvanced(!advanced)}
            aria-expanded={advanced}
          >
            <SlidersHorizontal size={16} />
            Filters
          </Button>
          <select
            aria-label="Category"
            value={params.get("category") ?? ""}
            onChange={(e) => change("category", e.target.value)}
          >
            <option value="">All categories</option>
            <option value="0">Books & theses</option>
            <option value="1">Electronic resources & audio</option>
            <option value="2">Journals</option>
            <option value="3">EBooks</option>
          </select>
          <select
            aria-label="Sort resources"
            value={params.get("sort") ?? "id"}
            onChange={(e) => change("sort", e.target.value)}
          >
            <option value="id">Resource ID</option>
            <option value="title_asc">Title A–Z</option>
            <option value="title_desc">Title Z–A</option>
            <option value="year_desc">Newest first</option>
            <option value="year_asc">Oldest first</option>
          </select>
          <label className="checkbox">
            <input
              type="checkbox"
              checked={params.get("available_only") === "true"}
              onChange={(e) =>
                change("available_only", e.target.checked ? "true" : "")
              }
            />
            Available now
          </label>
          <Button variant="ghost" onClick={() => setParams({})}>
            Clear filters
          </Button>
          <div className="view-toggle">
            <button
              aria-label="Card view"
              aria-pressed={view === "cards"}
              onClick={() => setView("cards")}
            >
              <Grid2X2 size={18} />
            </button>
            <button
              aria-label="List view"
              aria-pressed={view === "list"}
              onClick={() => setView("list")}
            >
              <List size={18} />
            </button>
          </div>
        </div>
        {advanced && (
          <div className="form-grid advanced">
            {[
              ["author", "Author"],
              ["from_year", "From year"],
              ["to_year", "To year"],
              ["identifier", "ISBN / ISSN"],
            ].map(([key, label]) => (
              <label key={key}>
                {label}
                <input
                  type={key.includes("year") ? "number" : "text"}
                  value={params.get(key) ?? ""}
                  onChange={(e) => change(key, e.target.value)}
                />
              </label>
            ))}
          </div>
        )}
      </section>
      <div className="section-heading">
        <p className="muted">
          {data.data
            ? `${data.data.items.length} resources on this page`
            : "Exploring the collection…"}
        </p>
        <span className="small muted">
          Carefully cataloged. Ready to discover.
        </span>
      </div>
      {data.isPending ? (
        <Loading />
      ) : data.error ? (
        <ErrorState error={data.error} />
      ) : !data.data.items.length ? (
        <Empty title="No resources found">
          Try another title or clear your filters.
        </Empty>
      ) : (
        <div className={view === "cards" ? "resource-grid" : "resource-list"}>
          {data.data.items.map((r) => (
            <Link
              to={"/catalog/" + encodeURIComponent(r.resource_id)}
              className="resource-card"
              key={r.resource_id}
            >
              <ResourceCover
                id={r.resource_id}
                title={r.title}
                type={r.resource_type}
              />
              <div className="card-body">
                <span className="eyebrow">
                  {r.resource_type} · {r.publication_year}
                </span>
                <h3>{r.title}</h3>
                <p>{r.authors.join(", ") || r.publisher}</p>
                <div className="card-bottom">
                  <Status tone={r.holdings > r.lent ? "success" : "warning"}>
                    {r.holdings > r.lent ? "Available" : "Unavailable"}
                  </Status>
                  <span>{r.price.formatted}</span>
                </div>
              </div>
            </Link>
          ))}
        </div>
      )}
      <div className="pagination">
        <Button
          variant="outline"
          disabled={!params.get("after") && offset === 0}
          onClick={() => {
            const p = new URLSearchParams(params);
            p.delete("after");
            p.set("offset", String(Math.max(0, offset - 12)));
            setParams(p);
          }}
        >
          Previous / first
        </Button>
        <Button
          variant="outline"
          disabled={(data.data?.items.length ?? 0) < 12}
          onClick={() => {
            const p = new URLSearchParams(params);
            if (
              !params.get("q") &&
              (params.get("sort") ?? "id") === "id" &&
              data.data?.next_cursor
            ) {
              p.set("after", data.data.next_cursor);
              p.delete("offset");
            } else p.set("offset", String(offset + 12));
            setParams(p);
          }}
        >
          Next page →
        </Button>
      </div>
      <Modal
        open={create}
        onOpenChange={setCreate}
        title="Add to the collection"
        description="Add the details that help readers discover this resource."
      >
        <ResourceForm />
      </Modal>
    </>
  );
}
