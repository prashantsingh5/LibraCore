import { roleLabel } from "../../lib/presentation";
import { useQuery } from "@tanstack/react-query";
import { useState } from "react";
import { api } from "../../services/api";
import { Loading, ErrorState, Status } from "../../components/feedback/States";
import { ActionForm } from "../../components/ui/ActionForm";
import { Button } from "../../components/ui/button";
import { Modal } from "../../components/ui/dialog";
import { DataView } from "../../components/feedback/DataView";
const roles = [
  "Student",
  "Faculty",
  "Librarian",
  "DepartmentManager",
  "Administrator",
];
interface Account {
  user_id: string;
  name: string;
  email: string;
  role: string;
  department_id: string;
  active: boolean;
}
export function Users() {
  const [selected, setSelected] = useState<Account | null>(null);
  const [offset, setOffset] = useState(0);
  const q = useQuery({
    queryKey: ["users", offset],
    queryFn: () => api<Account[]>("/users?limit=50&offset=" + offset),
  });
  const [before, setBefore] = useState(0);
  const security = useQuery({
    queryKey: ["security-events", before],
    queryFn: () =>
      api<
        {
          event_id: number;
          user_id: string;
          action: string;
          occurred_at: string;
        }[]
      >("/security-events?before=" + before),
  });
  const departments = useQuery({
    queryKey: ["departments"],
    queryFn: () => api<unknown[]>("/departments"),
  });
  const health = useQuery({
    queryKey: ["health"],
    queryFn: async () => {
      const r = await fetch("/ready");
      if (!r.ok) throw new Error("Service is not ready");
      return r.json() as Promise<Record<string, unknown>>;
    },
  });
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">PEOPLE & ACCESS</p>
          <h1>A well-connected institution.</h1>
          <p className="muted">
            Manage access thoughtfully. Changes revoke existing sessions.
          </p>
        </div>
      </div>
      <div className="two-column">
        <section className="panel">
          <h2>System readiness</h2>
          {health.error ? (
            <ErrorState error={health.error} />
          ) : (
            <DataView value={health.data} />
          )}
        </section>
        <section className="panel">
          <h2>Departments</h2>
          {departments.error ? (
            <ErrorState error={departments.error} />
          ) : (
            <DataView value={departments.data} />
          )}
          <details>
            <summary>Add department</summary>
            <ActionForm
              title="Create department"
              path="/departments"
              keys={["departments"]}
              fields={[
                {
                  name: "department_id",
                  label: "Department ID",
                  required: true,
                },
                { name: "name", label: "Department name", required: true },
              ]}
            />
          </details>
        </section>
      </div>
      <section className="panel">
        <h2>Institutional accounts</h2>
        {q.isPending ? (
          <Loading />
        ) : q.error ? (
          <ErrorState error={q.error} />
        ) : (
          q.data.map((u) => (
            <div className="row-link" key={u.user_id}>
              <span className="avatar">{u.name.slice(0, 2)}</span>
              <div>
                <strong>{u.name}</strong>
                <small>
                  {u.email} · {roleLabel(u.role)}
                  {u.email.endsWith("@example.invalid")
                    ? " · Legacy / seed account"
                    : u.user_id.startsWith("DEMO-")
                      ? " · Demo account"
                      : ""}
                </small>
              </div>
              <Status tone={u.active ? "success" : "warning"}>
                {u.active ? "Active" : "Inactive"}
              </Status>
              <Button variant="outline" onClick={() => setSelected(u)}>
                Manage
              </Button>
            </div>
          ))
        )}
        <div className="pagination">
          <Button
            variant="outline"
            disabled={offset === 0}
            onClick={() => setOffset(Math.max(0, offset - 50))}
          >
            Previous users
          </Button>
          <Button
            variant="outline"
            disabled={(q.data?.length ?? 0) < 50}
            onClick={() => setOffset(offset + 50)}
          >
            Next users
          </Button>
        </div>
        <details>
          <summary>Create user</summary>
          <ActionForm
            title="Create user"
            path="/users"
            keys={["users"]}
            fields={[
              {
                name: "user_id",
                label: "User ID",
                required: true,
                hint: "Unique institutional reference, e.g. STU-2026-001. It cannot be changed later.",
              },
              { name: "name", label: "Full name", required: true },
              {
                name: "email",
                label: "Institutional email",
                type: "email",
                required: true,
              },
              { name: "role", label: "Role", options: roles, value: "Student" },
              {
                name: "department_id",
                label: "Department",
                type: "department",
              },
            ]}
          />
          <p className="small muted">
            After creating the account, select Manage to provision its initial
            password.
          </p>
        </details>
      </section>
      <section className="panel">
        <h2>Authentication audit</h2>
        {security.isPending ? (
          <Loading />
        ) : security.error ? (
          <ErrorState error={security.error} />
        ) : (
          <DataView value={security.data} />
        )}
        <div className="pagination">
          <Button
            variant="outline"
            disabled={before === 0}
            onClick={() => setBefore(0)}
          >
            Latest events
          </Button>
          <Button
            variant="outline"
            disabled={(security.data?.length ?? 0) < 50}
            onClick={() => setBefore(security.data?.at(-1)?.event_id ?? 0)}
          >
            Older events
          </Button>
        </div>
      </section>
      <Modal
        open={!!selected}
        onOpenChange={(v) => {
          if (!v) setSelected(null);
        }}
        title={"Manage " + selected?.name}
        description="Role, department and credential changes invalidate all of this user’s sessions."
      >
        {selected && (
          <>
            <ActionForm
              title="Update access"
              path={
                "/users/" + encodeURIComponent(selected.user_id) + "/access"
              }
              method="PUT"
              keys={["users"]}
              confirm
              fields={[
                {
                  name: "role",
                  label: "Role",
                  options: roles,
                  value: selected.role,
                },
                {
                  name: "department_id",
                  label: "Department",
                  type: "department",
                  value: selected.department_id ?? "",
                },
                {
                  name: "active",
                  label: "Account status",
                  type: "boolean",
                  options: [
                    { value: "true", label: "Active" },
                    { value: "false", label: "Inactive" },
                  ],
                  value: String(selected.active),
                },
              ]}
            />
            <hr />
            <ActionForm
              title="Reset credentials"
              path={
                "/users/" +
                encodeURIComponent(selected.user_id) +
                "/credentials"
              }
              keys={[]}
              fields={[
                {
                  name: "password",
                  label: "New password (12–128 characters)",
                  type: "password",
                  required: true,
                },
              ]}
            />
          </>
        )}
      </Modal>
    </>
  );
}
