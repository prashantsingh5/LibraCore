import { date, roleLabel, greetingName } from "../lib/presentation";
import { useQuery } from "@tanstack/react-query";
import { Link } from "react-router-dom";
import { ArrowUpRight, BookOpen, Clock3, Compass, Wallet } from "lucide-react";
import { useAuth } from "../features/auth/Auth";
import { api } from "../services/api";
import type { Loan, Budget, Resource } from "../types/api";
import {
  Loading,
  ErrorState,
  Empty,
  Status,
} from "../components/feedback/States";
import { Utilization } from "../components/charts/Utilization";
import { DataView } from "../components/feedback/DataView";
import { Button } from "../components/ui/button";
export function Dashboard() {
  const { user } = useAuth();
  const loans = useQuery({
    queryKey: ["loans", user?.user_id],
    queryFn: () =>
      api<Loan[]>("/users/" + encodeURIComponent(user!.user_id) + "/loans"),
  });
  const catalog = useQuery({
    queryKey: ["catalog", "featured"],
    queryFn: () => api<{ items: Resource[] }>("/resources?limit=3"),
  });
  const finance = user!.permissions.includes("budget.read");
  const budget = useQuery({
    queryKey: ["budget", user?.department_id],
    enabled: finance,
    queryFn: () =>
      api<Budget>(
        user!.role === "DepartmentManager"
          ? "/reports/departments/" + encodeURIComponent(user!.department_id)
          : "/reports/budget",
      ),
  });
  const activity = useQuery({
    queryKey: ["events", "dashboard", user?.role, user?.department_id],
    enabled: finance,
    queryFn: () =>
      api<unknown[]>(
        user!.permissions.includes("event.read")
          ? "/events?limit=5"
          : "/acquisitions?limit=5&department_id=" +
              encodeURIComponent(user!.department_id),
      ),
  });
  const active = loans.data?.filter((x) => x.status === 0) ?? [];
  const overdue = active.filter((x) => new Date(x.due_at) < new Date());
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">YOUR KNOWLEDGE, CONNECTED</p>
          <h1>Welcome back, {greetingName(user?.name ?? "")}.</h1>
          <p className="muted">
            A little perspective on your library. A clear path to what’s next.
          </p>
        </div>
        <Status tone="success">
          {user?.department_id || "Library workspace"}
        </Status>
      </div>
      <section className="hero-banner">
        <div>
          <span className="eyebrow">ROOM FOR YOUR NEXT IDEA</span>
          <h2>
            Great work begins
            <br />
            with a new perspective.
          </h2>
          <p>
            Explore the collection. Find the resource that moves you forward.
          </p>
          <Button asChild>
            <Link to="/catalog">
              Explore catalog <ArrowUpRight size={17} />
            </Link>
          </Button>
        </div>
        <div className="hero-mark" aria-hidden="true">
          <BookOpen />
          <span>
            THE
            <br />
            KNOWLEDGE
            <br />
            COMMONS
          </span>
        </div>
      </section>
      <section className="stat-grid">
        <div className="stat">
          <BookOpen />
          <span>Your active loans</span>
          <strong>{loans.isPending ? "—" : active.length}</strong>
          <small>Resources in your hands</small>
        </div>
        <div className="stat">
          <Clock3 />
          <span>Overdue returns</span>
          <strong>{loans.isPending ? "—" : overdue.length}</strong>
          <small>
            {overdue.length
              ? "Visit Lending to return a resource"
              : "A fresh start for your next read"}
          </small>
        </div>
        <div className="stat">
          <Compass />
          <span>Your workspace</span>
          <strong className="role-value">{roleLabel(user?.role ?? "")}</strong>
          <small>Access tailored to your role</small>
        </div>
        {finance && (
          <div className="stat">
            <Wallet />
            <span>Budget remaining</span>
            <strong className="money-value">
              {budget.data?.remaining.formatted ?? "—"}
            </strong>
            <small>Authoritative financial position</small>
          </div>
        )}
      </section>
      <div className="two-column">
        <section className="panel">
          <div className="section-heading">
            <h2>On your reading desk</h2>
            <Link to="/loans">
              View loans <ArrowUpRight size={15} />
            </Link>
          </div>
          {loans.isPending ? (
            <Loading />
          ) : loans.error ? (
            <ErrorState error={loans.error} />
          ) : !active.length ? (
            <Empty title="Your desk is ready">
              Browse the catalog and make room for a new idea.
            </Empty>
          ) : (
            active.slice(0, 5).map((l) => (
              <Link
                className="row-link"
                key={l.loan_id}
                to={"/catalog/" + encodeURIComponent(l.resource_id)}
              >
                <span className="resource-mini">
                  <BookOpen size={20} />
                </span>
                <div>
                  <strong>{l.title || l.resource_id}</strong>
                  <small>Due {date(l.due_at)}</small>
                </div>
                <Status
                  tone={new Date(l.due_at) < new Date() ? "warning" : "success"}
                >
                  {new Date(l.due_at) < new Date() ? "Overdue" : "Active"}
                </Status>
              </Link>
            ))
          )}
        </section>
        <section className="panel">
          <div className="section-heading">
            <h2>{finance ? "Budget at a glance" : "Your next chapter"}</h2>
          </div>
          {finance ? (
            budget.error ? (
              <ErrorState error={budget.error} />
            ) : budget.data ? (
              <>
                <Utilization used={budget.data.percentage_used} />
                <Link to="/budgets">Explore budget details →</Link>
              </>
            ) : (
              <Loading />
            )
          ) : (
            <div className="quick-actions">
              <Link to="/catalog">
                <BookOpen />
                Discover a resource
                <ArrowUpRight />
              </Link>
              <Link to="/assistant">
                <Compass />
                Find it with Libra
                <ArrowUpRight />
              </Link>
              <Link to="/loans">
                <Clock3 />
                Review your due dates
                <ArrowUpRight />
              </Link>
            </div>
          )}
        </section>
      </div>
      {finance && (
        <section className="panel">
          <div className="section-heading">
            <h2>
              {user!.permissions.includes("event.read")
                ? "Recent library activity"
                : "Recent department acquisitions"}
            </h2>
          </div>
          {activity.isPending ? (
            <Loading />
          ) : activity.error ? (
            <ErrorState error={activity.error} />
          ) : (
            <DataView value={activity.data} />
          )}
        </section>
      )}
      <section>
        <div className="section-heading">
          <h2>From the collection</h2>
          <Link to="/catalog">
            Browse all <ArrowUpRight size={15} />
          </Link>
        </div>
        <div className="resource-grid">
          {catalog.error ? (
            <ErrorState error={catalog.error} />
          ) : (
            catalog.data?.items.map((r, i) => (
              <Link
                to={"/catalog/" + encodeURIComponent(r.resource_id)}
                className="resource-card"
                key={r.resource_id}
              >
                <div className={"cover cover-" + i}>
                  <span>{r.resource_type}</span>
                  <strong>{r.title}</strong>
                  <BookOpen size={28} />
                </div>
                <div className="card-body">
                  <h3>{r.title}</h3>
                  <p>{r.authors.join(", ") || r.publisher}</p>
                  <Status tone={r.holdings > r.lent ? "success" : "warning"}>
                    {r.holdings > r.lent
                      ? "Available"
                      : "Currently unavailable"}
                  </Status>
                </div>
              </Link>
            ))
          )}
        </div>
      </section>
    </>
  );
}
