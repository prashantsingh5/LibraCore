import { useQuery } from "@tanstack/react-query";
import { api } from "../../services/api";
import { Loading, ErrorState } from "../../components/feedback/States";
import { DataView } from "../../components/feedback/DataView";
export function Reports() {
  const q = useQuery({
    queryKey: ["reports"],
    queryFn: async () => ({
      lending: await api("/reports/lending"),
      acquisitions: await api("/reports/acquisitions"),
    }),
  });
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">THE BIGGER PICTURE</p>
          <h1>Knowledge in motion.</h1>
          <p className="muted">
            System-wide · all-time acquisition history and current lending
            snapshot.
          </p>
        </div>
      </div>
      {q.isPending ? (
        <Loading />
      ) : q.error ? (
        <ErrorState error={q.error} />
      ) : (
        <div className="two-column">
          <section className="panel">
            <h2>Lending & resource utilization</h2>
            <DataView value={q.data.lending} />
          </section>
          <section className="panel">
            <h2>Acquisition outcomes</h2>
            <p className="muted">
              Approved purchase totals are historical gross values.
              Cancellations remain separate records; consult Budgets for current
              remaining allocations and fiscal-year usage.
            </p>
            <DataView value={q.data.acquisitions} />
          </section>
        </div>
      )}
    </>
  );
}
