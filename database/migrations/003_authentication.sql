-- Additive V3 upgrade: credentials deliberately separate from V2 public user records.
CREATE TABLE auth_credentials (
 user_id text PRIMARY KEY REFERENCES users(user_id),
 password_hash text NOT NULL,
 changed_at timestamptz NOT NULL DEFAULT now()
);
CREATE TABLE auth_sessions (
 session_id uuid PRIMARY KEY,
 user_id text NOT NULL REFERENCES users(user_id),
 token_hash text NOT NULL UNIQUE CHECK(length(token_hash)=64),
 csrf_hash text NOT NULL CHECK(length(csrf_hash)=64),
 created_at timestamptz NOT NULL DEFAULT now(),
 expires_at timestamptz NOT NULL,
 last_used_at timestamptz NOT NULL DEFAULT now(),
 revoked_at timestamptz,
 CHECK(expires_at>created_at)
);
CREATE INDEX auth_sessions_user ON auth_sessions(user_id) WHERE revoked_at IS NULL;
CREATE INDEX auth_sessions_expiry ON auth_sessions(expires_at);
CREATE TABLE security_events (
 event_id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
 user_id text REFERENCES users(user_id),
 action text NOT NULL,
 occurred_at timestamptz NOT NULL DEFAULT now()
);
CREATE INDEX security_events_recent ON security_events(event_id DESC);
