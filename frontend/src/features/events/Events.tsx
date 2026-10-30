import { useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { api } from "../../services/api";
import { Loading, ErrorState, Empty } from "../../components/feedback/States";
import { DataView } from "../../components/feedback/DataView";
import { Button } from "../../components/ui/button";
export function Events() {
  const [cursor, setCursor] = useState(0);
  const q = useQuery({
    queryKey: ["events", cursor],
    queryFn: () =>
      api<{ items: unknown[]; next_cursor: number | null }>(
        "/events/feed?limit=20&before=" + cursor,
      ),
  });
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">A TRUSTWORTHY RECORD</p>
          <h1>The story of your library.</h1>
          <p className="muted">
            Purchases, lending and returns. Every domain event, in context.
          </p>
        </div>
      </div>
      {q.isPending ? (
        <Loading />
      ) : q.error ? (
        <ErrorState error={q.error} />
      ) : q.data.items.length ? (
        <section className="panel timeline">
          <DataView value={q.data.items} />
        </section>
      ) : (
        <Empty title="No recent events" />
      )}
      <div className="pagination">
        <Button
          variant="outline"
          disabled={cursor === 0}
          onClick={() => setCursor(0)}
        >
          Latest
        </Button>
        <Button
          variant="outline"
          disabled={(q.data?.items.length ?? 0) < 20}
          onClick={() => setCursor(q.data?.next_cursor ?? 0)}
        >
          Next
        </Button>
      </div>
    </>
  );
}
