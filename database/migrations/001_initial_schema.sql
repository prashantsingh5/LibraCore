CREATE TABLE app_state (
 id integer PRIMARY KEY CHECK(id=1), next_order integer NOT NULL CHECK(next_order>0),
 next_loan bigint NOT NULL CHECK(next_loan>0), print_tax integer NOT NULL CHECK(print_tax>=0),
 electronic_tax integer NOT NULL CHECK(electronic_tax>=0), loan_days integer NOT NULL CHECK(loan_days BETWEEN 1 AND 36500)
);
INSERT INTO app_state VALUES(1,1,1,0,0,14);
CREATE TABLE resources (
 resource_id text PRIMARY KEY CHECK(btrim(resource_id)<>''),
 resource_type text NOT NULL CHECK(resource_type IN ('Book','Journal','ElectronicResource','EBook','AudioBook','Thesis')),
 category integer NOT NULL CHECK(category BETWEEN 0 AND 3), title text NOT NULL CHECK(title<>''), publisher text NOT NULL,
 publication_year integer NOT NULL, currency text NOT NULL CHECK(currency ~ '^[A-Z]{3}$'),
 unit_price_minor bigint NOT NULL CHECK(unit_price_minor>=0), holdings integer NOT NULL DEFAULT 0 CHECK(holdings>=0),
 lent integer NOT NULL DEFAULT 0 CHECK(lent>=0 AND lent<=holdings),
 created_at timestamptz NOT NULL DEFAULT now(), updated_at timestamptz NOT NULL DEFAULT now(),
 UNIQUE(resource_id,currency)
);
-- A typed one-to-one metadata row avoids JSON financial state and redundant per-subclass tables.
CREATE TABLE resource_details (
 resource_id text PRIMARY KEY REFERENCES resources ON DELETE CASCADE,
 isbn text NOT NULL DEFAULT '', issn text NOT NULL DEFAULT '', edition integer NOT NULL DEFAULT 1 CHECK(edition>=1),
 binding integer NOT NULL DEFAULT 0 CHECK(binding IN(0,1)), issues_per_year integer NOT NULL DEFAULT 1,
 subscription_years integer NOT NULL DEFAULT 1 CHECK(subscription_years>=1), access_url text NOT NULL DEFAULT '',
 license integer NOT NULL DEFAULT 1 CHECK(license IN(0,1)), platform_fee_minor bigint NOT NULL DEFAULT 0 CHECK(platform_fee_minor>=0),
 platform_currency text NOT NULL DEFAULT 'INR' CHECK(platform_currency ~ '^[A-Z]{3}$'),
 format integer NOT NULL DEFAULT 0 CHECK(format BETWEEN 0 AND 2), drm boolean NOT NULL DEFAULT false,
 narrator text NOT NULL DEFAULT '', duration_minutes integer NOT NULL DEFAULT 1 CHECK(duration_minutes>0),
 university text NOT NULL DEFAULT '', degree text NOT NULL DEFAULT '', supervisor text NOT NULL DEFAULT ''
);
CREATE TABLE authors(author_id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY, name text NOT NULL UNIQUE);
CREATE TABLE resource_authors(resource_id text REFERENCES resources ON DELETE CASCADE, author_id bigint REFERENCES authors,
 position integer NOT NULL CHECK(position>=0), PRIMARY KEY(resource_id,position));
CREATE TABLE vendors(vendor_id text PRIMARY KEY CHECK(btrim(vendor_id)<>''), name text NOT NULL CHECK(btrim(name)<>''),
 rating double precision NOT NULL CHECK(rating>=0 AND rating<=5), estimated_delivery_days integer NOT NULL CHECK(estimated_delivery_days>=0));
CREATE TABLE vendor_offers(resource_id text NOT NULL, vendor_id text REFERENCES vendors, price_minor bigint NOT NULL CHECK(price_minor>=0),
 currency text NOT NULL, PRIMARY KEY(resource_id,vendor_id), FOREIGN KEY(resource_id,currency) REFERENCES resources(resource_id,currency) ON DELETE CASCADE);
CREATE TABLE departments(department_id text PRIMARY KEY CHECK(btrim(department_id)<>''), name text NOT NULL UNIQUE,
 active boolean NOT NULL DEFAULT true, created_at timestamptz NOT NULL DEFAULT now());
CREATE TABLE users(user_id text PRIMARY KEY CHECK(btrim(user_id)<>''), name text NOT NULL CHECK(btrim(name)<>''), email text NOT NULL UNIQUE,
 role text NOT NULL CHECK(role IN('Student','Faculty','Librarian','DepartmentManager','Administrator')),
 department_id text REFERENCES departments, active boolean NOT NULL DEFAULT true, created_at timestamptz NOT NULL DEFAULT now());
CREATE TABLE budgets(budget_id text PRIMARY KEY, department_id text UNIQUE REFERENCES departments,
 fiscal_year integer NOT NULL, currency text NOT NULL CHECK(currency ~ '^[A-Z]{3}$'),
 total_minor bigint NOT NULL CHECK(total_minor>=0), spent_minor bigint NOT NULL DEFAULT 0 CHECK(spent_minor>=0 AND spent_minor<=total_minor));
CREATE UNIQUE INDEX one_default_budget ON budgets ((department_id IS NULL)) WHERE department_id IS NULL;
CREATE TABLE budget_quotas(budget_id text REFERENCES budgets, category integer CHECK(category BETWEEN 0 AND 3),
 max_units integer NOT NULL CHECK(max_units>=0), max_spend bigint NOT NULL CHECK(max_spend>=0), max_titles integer CHECK(max_titles>=0), PRIMARY KEY(budget_id,category));
CREATE TABLE budget_usage(budget_id text REFERENCES budgets, category integer CHECK(category BETWEEN 0 AND 3),
 units integer NOT NULL CHECK(units>=0), spent bigint NOT NULL CHECK(spent>=0), PRIMARY KEY(budget_id,category));
CREATE TABLE budget_title_usage(budget_id text REFERENCES budgets, category integer CHECK(category BETWEEN 0 AND 3),
 resource_id text REFERENCES resources, units integer NOT NULL CHECK(units>0), PRIMARY KEY(budget_id,category,resource_id));
CREATE TABLE loans(loan_id text PRIMARY KEY, resource_id text NOT NULL REFERENCES resources, borrower_id text NOT NULL REFERENCES users,
 quantity integer NOT NULL CHECK(quantity>0), returned_quantity integer NOT NULL CHECK(returned_quantity BETWEEN 0 AND quantity),
 borrowed_at timestamptz NOT NULL, due_at timestamptz NOT NULL CHECK(due_at>borrowed_at), returned_at timestamptz,
 status integer NOT NULL CHECK(status IN(0,1)), CHECK(returned_at IS NULL OR returned_at>=borrowed_at),
 CHECK((status=0 AND returned_quantity<quantity AND returned_at IS NULL) OR (status=1 AND returned_quantity=quantity AND returned_at IS NOT NULL)));
CREATE TABLE purchase_records(order_id integer PRIMARY KEY CHECK(order_id>0), resource_id text REFERENCES resources,
 requested_resource_id text NOT NULL, title text NOT NULL, category integer NOT NULL CHECK(category BETWEEN 0 AND 3),
 quantity integer NOT NULL, pre_tax_minor bigint NOT NULL CHECK(pre_tax_minor>=0), post_tax_minor bigint NOT NULL CHECK(post_tax_minor>=0),
 currency text NOT NULL CHECK(currency ~ '^[A-Z]{3}$'), approved boolean NOT NULL, reason text NOT NULL,
 record_type integer NOT NULL CHECK(record_type IN(0,1)), related_order_id integer REFERENCES purchase_records,
 department_id text REFERENCES departments, requested_department text NOT NULL, vendor text NOT NULL,
 vendor_selection text NOT NULL CHECK(vendor_selection IN('Cheapest','Fastest','HighestRated')), fiscal_year integer, status integer NOT NULL CHECK(status BETWEEN 0 AND 3), vendor_snapshot jsonb, created_at timestamptz NOT NULL,
 CHECK(NOT approved OR quantity>0), CHECK((record_type=0 AND related_order_id IS NULL) OR (record_type=1 AND related_order_id IS NOT NULL)));
CREATE UNIQUE INDEX one_cancellation ON purchase_records(related_order_id) WHERE record_type=1;
CREATE TABLE acquisition_requests(request_id text PRIMARY KEY, operation text NOT NULL, parameters jsonb NOT NULL,
 result jsonb NOT NULL, created_at timestamptz NOT NULL DEFAULT now());
-- parameters/result are immutable idempotency receipts, not authoritative domain state.
CREATE TABLE audit_events(event_id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
 event_type integer NOT NULL CHECK(event_type BETWEEN 0 AND 3), resource_id text REFERENCES resources,
 quantity integer NOT NULL CHECK(quantity>0), actor text NOT NULL, user_id text REFERENCES users, department_id text REFERENCES departments, reference_id text NOT NULL,
 occurred_at timestamptz NOT NULL);
CREATE TABLE budget_rollovers(rollover_id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
 budget_id text NOT NULL REFERENCES budgets, from_year integer NOT NULL, to_year integer NOT NULL,
 old_total bigint NOT NULL, old_spent bigint NOT NULL, new_total bigint NOT NULL, currency text NOT NULL,
 created_at timestamptz NOT NULL DEFAULT now(), UNIQUE(budget_id,from_year));
