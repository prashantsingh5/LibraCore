export interface User {
  user_id: string;
  name: string;
  role: string;
  department_id: string;
  permissions: string[];
}
export interface Money {
  minor_units: number;
  currency: string;
  formatted: string;
}
export interface Resource {
  resource_id: string;
  resource_type: string;
  title: string;
  publisher: string;
  publication_year: number;
  authors: string[];
  price: Money;
  currency: string;
  unit_price_minor: number;
  holdings: number;
  lent: number;
  isbn: string;
  issn: string;
  vendors?: Vendor[];
  [key: string]: unknown;
}
export interface Vendor {
  vendor_id: string;
  name: string;
  rating: number;
  estimated_delivery_days: number;
  price_minor?: number;
  currency?: string;
}
export interface Loan {
  title?: string;
  returned_at?: string;
  loan_id: string;
  resource_id: string;
  borrower_id: string;
  quantity: number;
  returned_quantity: number;
  due_at: string;
  borrowed_at: string;
  status: number;
}
export interface Order {
  cancellation_order_id?: number;
  record_type?: number;
  order_id: number;
  requested_resource_id: string;
  requested_department: string;
  title: string;
  quantity: number;
  approved: boolean;
  status: number;
  current_status?: number;
  reason: string;
  cost?: Money;
  post_tax_minor?: number;
  currency: string;
  vendor: string;
}
export interface Budget {
  fiscal_year?: number;
  department_id?: string;
  total: Money;
  spent: Money;
  remaining: Money;
  percentage_used: number;
  categories: Record<string, unknown>[];
}
export interface Proposal {
  token: string;
  action: string;
  parameters: Record<string, unknown>;
  expires_in_seconds: number;
}
export interface AssistantReply {
  message: string;
  tool_results: {
    operation: string;
    result: Record<string, unknown> | unknown[];
  }[];
  confirmations: Proposal[];
}
