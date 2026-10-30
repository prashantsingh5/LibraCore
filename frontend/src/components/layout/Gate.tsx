import type { ReactNode } from "react";
import { Link } from "react-router-dom";
import { ShieldCheck } from "lucide-react";
import { useAuth } from "../../features/auth/Auth";
export function Gate({
  permission,
  children,
}: {
  permission: string;
  children: ReactNode;
}) {
  const { user } = useAuth();
  return user?.permissions.includes(permission) ? (
    children
  ) : (
    <section className="restricted panel">
      <ShieldCheck size={52} />
      <p className="eyebrow">403 · YOUR ACCESS BOUNDARY</p>
      <h1>Access is restricted</h1>
      <p>
        Your current role does not have access to this workspace. Contact your
        library administrator if your responsibilities have changed.
      </p>
      <Link className="button" to="/">
        Return to workspace
      </Link>
    </section>
  );
}
