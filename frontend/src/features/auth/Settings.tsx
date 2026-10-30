import { ActionForm } from "../../components/ui/ActionForm";
export function AccountSettings() {
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">YOUR WORKSPACE</p>
          <h1>Security & sessions.</h1>
          <p className="muted">
            Your password stays private. Your access stays in your control.
          </p>
        </div>
      </div>
      <section className="panel narrow">
        <h2>Change password</h2>
        <p className="muted">
          This signs out every session, including this one.
        </p>
        <ActionForm
          title="Change password"
          path="/auth/change-password"
          keys={[]}
          onSuccess={() => location.reload()}
          fields={[
            {
              name: "current_password",
              label: "Current password",
              type: "password",
              required: true,
            },
            {
              name: "new_password",
              label: "New password (12–128 characters)",
              type: "password",
              required: true,
            },
          ]}
        />
      </section>
      <section className="panel narrow">
        <h2>Sign out everywhere</h2>
        <ActionForm
          title="Sign out all sessions"
          path="/auth/logout-all"
          confirm
          keys={[]}
          fields={[]}
          onSuccess={() => location.reload()}
        />
      </section>
    </>
  );
}
