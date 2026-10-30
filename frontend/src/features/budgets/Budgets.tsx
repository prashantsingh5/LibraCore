import { DepartmentSelect } from "../../components/ui/DepartmentSelect";
import { Button } from "../../components/ui/button";
import { categories } from "../../lib/presentation";
import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { api } from "../../services/api";
import type { Budget } from "../../types/api";
import { useAuth } from "../auth/Auth";
import { Loading, ErrorState } from "../../components/feedback/States";
import { DataView } from "../../components/feedback/DataView";
import { Utilization } from "../../components/charts/Utilization";
import { ActionForm } from "../../components/ui/ActionForm";
export function Budgets() {
  const { user } = useAuth();
  const [department, setDepartment] = useState(
    user!.role === "DepartmentManager" ? user!.department_id : "",
  );
  const [selection, setSelection] = useState(department);
  const q = useQuery({
    queryKey: ["budget", department],
    queryFn: () =>
      api<Budget>(
        department
          ? "/reports/departments/" + encodeURIComponent(department)
          : "/reports/budget",
      ),
  });
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">RESOURCES, RESPONSIBLY</p>
          <h1>Every allocation has a purpose.</h1>
          <p className="muted">
            A clear view of spending, capacity and room to grow.
          </p>
        </div>
      </div>
      {user!.role !== "DepartmentManager" && (
        <form
          className="department-filter"
          onSubmit={(e) => {
            e.preventDefault();
            setDepartment(selection);
          }}
        >
          <DepartmentSelect value={selection} onChange={setSelection} />
          <Button type="submit">View budget</Button>
        </form>
      )}
      {q.isPending ? (
        <Loading />
      ) : q.error ? (
        <ErrorState error={q.error} />
      ) : (
        <>
          <section className="stat-grid">
            {[
              { name: "Allocated", money: q.data.total },
              { name: "Spent", money: q.data.spent },
              { name: "Remaining", money: q.data.remaining },
            ].map((x) => (
              <div className="stat" key={x.name}>
                <span>{x.name}</span>
                <strong className="money-value">{x.money.formatted}</strong>
                <small>Reported by the domain engine</small>
              </div>
            ))}
          </section>
          <section className="panel">
            <h2>Allocation in perspective</h2>
            <p className="muted">
              {department || "Central library"} · Fiscal year{" "}
              {q.data.fiscal_year ?? "as configured"}
            </p>
            <Utilization used={q.data.percentage_used} />
          </section>
          <section className="panel">
            <h2>Category quotas & usage</h2>
            <DataView value={q.data.categories} />
          </section>
        </>
      )}
      {user?.permissions.includes("budget.manage") && (
        <section className="panel">
          <h2>Budget configuration</h2>
          <details>
            <summary>Configure a category quota</summary>
            <ActionForm
              title="Save quota"
              path="/budgets/quota"
              method="PUT"
              keys={["budget"]}
              extra={{ department_id: department }}
              fields={[
                {
                  name: "category",
                  label: "Category",
                  options: categories.map((label, value) => ({
                    value: String(value),
                    label,
                  })),
                  type: "number",
                  min: 0,
                  max: 3,
                  value: 0,
                  required: true,
                },
                {
                  name: "max_units",
                  label: "Maximum units",
                  type: "number",
                  min: 0,
                  required: true,
                },
                {
                  name: "max_spend",
                  label: "Maximum spend",
                  type: "money",
                  min: 0,
                  required: true,
                },
                {
                  name: "max_titles",
                  label: "Maximum distinct titles",
                  type: "number",
                  min: 0,
                },
              ]}
            />
          </details>
          <details>
            <summary>Create a budget</summary>
            <ActionForm
              title="Create budget"
              path="/budgets"
              keys={["budget"]}
              extra={{ department_id: department }}
              fields={[
                { name: "budget_id", label: "Budget ID", required: true },
                {
                  name: "fiscal_year",
                  label: "Fiscal year",
                  type: "number",
                  value: 2026,
                  required: true,
                },
                {
                  name: "currency",
                  label: "Currency code",
                  value: "INR",
                  required: true,
                },
                {
                  name: "total_minor",
                  label: "Allocation",
                  type: "money",
                  min: 0,
                  required: true,
                },
              ]}
            />
          </details>
          <details>
            <summary>Roll over to a new fiscal year</summary>
            <ActionForm
              title="Roll over budget"
              path="/budgets/rollover"
              confirm
              keys={["budget"]}
              extra={{ department_id: department, request_id: "auto" }}
              fields={[
                {
                  name: "fiscal_year",
                  label: "New fiscal year",
                  type: "number",
                  required: true,
                },
                {
                  name: "new_allocation",
                  label: "New allocation",
                  type: "money",
                  min: 0,
                  required: true,
                },
                {
                  name: "carry_percent",
                  label: "Carry forward percent",
                  type: "number",
                  min: 0,
                  max: 100,
                  value: 0,
                  required: true,
                },
              ]}
            />
          </details>
        </section>
      )}
    </>
  );
}
