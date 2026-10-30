import type { ReactNode } from "react";
import { Inbox, AlertCircle } from "lucide-react";
export function Loading() {
  return (
    <div role="status" aria-label="Loading" className="skeleton-grid">
      {[1, 2, 3].map((i) => (
        <div className="skeleton" key={i} />
      ))}
    </div>
  );
}
export function ErrorState({ error }: { error: Error }) {
  return (
    <div className="notice error" role="alert">
      <AlertCircle size={20} />
      <span>{error.message}</span>
    </div>
  );
}
export function Empty({
  title = "Nothing here yet",
  children,
}: {
  title?: string;
  children?: ReactNode;
}) {
  return (
    <div className="empty">
      <Inbox size={36} />
      <h3>{title}</h3>
      <p>{children ?? "Your next action will appear here."}</p>
    </div>
  );
}
export function Status({
  children,
  tone = "info",
}: {
  children: ReactNode;
  tone?: string;
}) {
  return <span className={"badge " + tone}>{children}</span>;
}
