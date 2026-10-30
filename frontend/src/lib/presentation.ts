export const roleLabel = (role: string) =>
  role === "DepartmentManager" ? "Department Manager" : role;
export const greetingName = (name: string) =>
  name
    .trim()
    .replace(/^(dr\.?|prof\.?|mr\.?|mrs\.?|ms\.?)\s+/i, "")
    .split(/\s+/)[0]
    .replace(/[.,]+$/, "") || "reader";
export const categories = [
  "Book / Thesis",
  "Electronic / Audio",
  "Journal",
  "EBook",
];
export const eventLabels = ["Purchase", "Cancellation", "Borrow", "Return"];
export const date = (value: unknown, time = false) => {
  if (!value || Number.isNaN(new Date(String(value)).getTime())) return "—";
  return new Intl.DateTimeFormat("en-GB", {
    day: "2-digit",
    month: "short",
    year: "numeric",
    ...(time ? { hour: "numeric", minute: "2-digit", hour12: true } : {}),
  }).format(new Date(String(value)));
};
export function toMinor(value: string): number {
  if (!/^\d+(\.\d{1,2})?$/.test(value.trim()))
    throw new Error("Enter a positive amount with at most two decimal places.");
  const [whole, fraction = ""] = value.trim().split(".");
  const amount = BigInt(whole) * 100n + BigInt(fraction.padEnd(2, "0"));
  if (amount > BigInt(Number.MAX_SAFE_INTEGER))
    throw new Error("Amount exceeds the supported form limit.");
  return Number(amount);
}
export const fromMinor = (value: unknown) => {
  const n = Number(value ?? 0);
  return Number.isSafeInteger(n)
    ? `${Math.trunc(n / 100)}.${String(Math.abs(n % 100)).padStart(2, "0")}`
    : "0.00";
};
export function money(value: unknown, currency = "INR"): string {
  if (value && typeof value === "object") {
    const v = value as Record<string, unknown>;
    return typeof v.formatted === "string"
      ? v.formatted
      : money(v.minor_units, String(v.currency ?? currency));
  }
  const n = Number(value ?? 0);
  try {
    return new Intl.NumberFormat("en-IN", {
      style: "currency",
      currency,
      minimumFractionDigits: 2,
      maximumFractionDigits: 2,
    }).format(n / 100);
  } catch {
    return `${currency} ${fromMinor(n)}`;
  }
}
export const loanStatus = (v: {
  status?: number;
  returned_quantity?: number;
}) =>
  v.status === 1
    ? "Returned"
    : (v.returned_quantity ?? 0) > 0
      ? "Partially returned"
      : "Active";
export const orderStatus = (v: {
  current_status?: number;
  status?: number;
  record_type?: number;
}) =>
  v.record_type === 1
    ? "Cancellation"
    : ({ 0: "Pending", 1: "Approved", 2: "Rejected", 3: "Cancelled" }[
        v.current_status ?? v.status ?? 0
      ] ?? "Unknown");
export const enumLabel = (key: string, value: unknown) =>
  ({
    binding: ["Paperback", "Hardcover"],
    format: ["PDF", "EPUB", "HTML"],
    license: ["Perpetual", "Annual subscription"],
    category: categories,
    event_type: eventLabels,
    record_type: ["Purchase", "Cancellation"],
  })[key]?.[Number(value)] ?? String(value ?? "—");
