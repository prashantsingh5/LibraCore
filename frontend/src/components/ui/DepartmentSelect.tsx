import { useQuery } from "@tanstack/react-query";
import { api } from "../../services/api";
export function DepartmentSelect({
  name = "department_id",
  value,
  onChange,
  defaultValue = "",
  label = "Department",
  required = false,
}: {
  name?: string;
  value?: string;
  onChange?: (v: string) => void;
  defaultValue?: string;
  label?: string;
  required?: boolean;
}) {
  const q = useQuery({
    queryKey: ["department-options"],
    queryFn: async () => {
      const all: { department_id: string; name: string }[] = [];
      for (let offset = 0; ; offset += 200) {
        const page = await api<{ department_id: string; name: string }[]>(
          "/departments?limit=200&offset=" + offset,
        );
        all.push(...page);
        if (page.length < 200) return all;
      }
    },
  });
  return (
    <label>
      {label}
      <select
        name={name}
        value={value}
        defaultValue={value === undefined ? defaultValue : undefined}
        onChange={(e) => onChange?.(e.target.value)}
        required={required}
        disabled={q.isPending}
      >
        <option value="">Central library / no department</option>
        {q.data?.map((d) => (
          <option key={d.department_id} value={d.department_id}>
            {d.name}
          </option>
        ))}
      </select>
      {q.error && (
        <small role="alert">Departments could not load. Please refresh.</small>
      )}
    </label>
  );
}
