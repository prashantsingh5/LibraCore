-- Presentation metadata only: existing resources need no backfill.
CREATE TABLE resource_covers (
 resource_id text PRIMARY KEY REFERENCES resources(resource_id) ON DELETE CASCADE,
 storage_key text NOT NULL CHECK (storage_key ~ '^[a-f0-9]{32}\.webp$'),
 updated_at timestamptz NOT NULL DEFAULT now()
);
