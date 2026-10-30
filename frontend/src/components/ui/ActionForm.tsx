import { toMinor, roleLabel } from "../../lib/presentation";
import { DepartmentSelect } from "./DepartmentSelect";
import { ResultCard } from "../feedback/ResultCard";
import { useState, type ReactNode, type FormEvent } from "react";
import { useQueryClient } from "@tanstack/react-query";
import { api, requestId } from "../../services/api";
import { Button } from "./button";
import { Modal } from "./dialog";
import { ErrorState } from "../feedback/States";
import { DataView } from "../feedback/DataView";
export interface FormField {
  name: string;
  label: string;
  type?: string;
  value?: string | number;
  options?: (string | { value: string; label: string })[];
  disabled?: boolean;
  required?: boolean;
  min?: number;
  max?: number;
  hint?: string;
}
export function ActionForm({
  title,
  path,
  method = "POST",
  fields,
  extra = {},
  keys,
  confirm = false,
  onSuccess,
  children,
  onValueChange,
}: {
  title: string;
  path: string;
  method?: string;
  fields: FormField[];
  extra?: Record<string, unknown>;
  keys: string[];
  confirm?: boolean;
  onSuccess?: (result: unknown) => void | Promise<void>;
  children?: ReactNode;
  onValueChange?: (name: string, value: string) => void;
}) {
  const [fieldErrors, setFieldErrors] = useState<Record<string, string>>({});
  const [error, setError] = useState<Error | null>(null);
  const [pending, setPending] = useState(false);
  const [submitted, setSubmitted] = useState<Record<string, unknown>>({});
  const [result, setResult] = useState<unknown>(null);
  const [proposal, setProposal] = useState<Record<string, unknown> | null>(
    null,
  );
  const qc = useQueryClient();
  async function send(body: Record<string, unknown>) {
    setPending(true);
    setError(null);
    try {
      const r = await api<unknown>(path, body, method);
      setSubmitted(body);
      setResult(r);
      setProposal(null);
      for (const key of keys) await qc.invalidateQueries({ queryKey: [key] });
      await onSuccess?.(r);
    } catch (e) {
      setError(e as Error);
    } finally {
      setPending(false);
    }
  }
  function submit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault();
    const form = e.currentTarget;
    const data = new FormData(form);
    setFieldErrors({});
    setError(null);
    e.currentTarget
      .querySelectorAll<HTMLInputElement>("input")
      .forEach((x) => x.setCustomValidity(""));
    const body: Record<string, unknown> = { ...extra };
    for (const f of fields) {
      const v = String(f.disabled ? (f.value ?? "") : (data.get(f.name) ?? ""));
      if (v === "" && !f.required) continue;
      if (f.type === "money") {
        try {
          body[f.name] = toMinor(v);
        } catch (e) {
          setError(e as Error);
          setFieldErrors({ [f.name]: (e as Error).message });
          const input = form.elements.namedItem(
            f.name,
          ) as HTMLInputElement | null;
          input?.setCustomValidity((e as Error).message);
          input?.reportValidity();
          return;
        }
        continue;
      }
      body[f.name] =
        f.type === "number"
          ? Number(v)
          : f.type === "authors"
            ? v
                .split(",")
                .map((x) => x.trim())
                .filter(Boolean)
            : f.type === "boolean"
              ? v === "true"
              : v;
    }
    if (extra.request_id === "auto") body.request_id = requestId();
    if (confirm) setProposal(body);
    else void send(body);
  }
  return (
    <>
      <form className="action-form" onSubmit={submit}>
        <div className="form-grid">
          {fields.map((f) =>
            f.type === "department" ? (
              <DepartmentSelect
                key={f.name}
                name={f.name}
                label={f.label}
                defaultValue={String(f.value ?? "")}
                required={f.required}
              />
            ) : (
              <label key={f.name}>
                {f.label}
                {f.options ? (
                  <select
                    name={f.name}
                    aria-label={f.label}
                    defaultValue={f.value}
                    required={f.required}
                    disabled={f.disabled}
                    onChange={(e) => onValueChange?.(f.name, e.target.value)}
                  >
                    {f.options.map((v) => (
                      <option
                        key={typeof v === "string" ? v : v.value}
                        value={typeof v === "string" ? v : v.value}
                      >
                        {typeof v === "string" ? roleLabel(v) : v.label}
                      </option>
                    ))}
                  </select>
                ) : (
                  <input
                    name={f.name}
                    aria-label={f.label}
                    type={
                      f.type === "authors" || f.type === "money"
                        ? "text"
                        : (f.type ?? "text")
                    }
                    inputMode={f.type === "money" ? "decimal" : undefined}
                    readOnly={f.disabled}
                    onInput={(e) =>
                      (e.target as HTMLInputElement).setCustomValidity("")
                    }
                    defaultValue={f.value}
                    required={f.required}
                    aria-invalid={!!fieldErrors[f.name]}
                    aria-describedby={
                      fieldErrors[f.name] ? f.name + "-error" : undefined
                    }
                    min={f.min}
                    max={f.max}
                    step={f.type === "number" ? "any" : undefined}
                    autoComplete={
                      f.type === "password" ? "new-password" : undefined
                    }
                  />
                )}
                {f.hint && <small className="muted">{f.hint}</small>}
                {fieldErrors[f.name] && (
                  <small id={f.name + "-error"} role="alert">
                    {fieldErrors[f.name]}
                  </small>
                )}
              </label>
            ),
          )}
        </div>
        {children}
        {error && <ErrorState error={error} />}
        <Button type="submit" disabled={pending}>
          {pending ? "Working…" : title}
        </Button>
      </form>
      {result !== null && (
        <div className="action-result" role="status">
          <ResultCard operation={path} value={result} context={submitted} />
        </div>
      )}
      <Modal
        open={proposal !== null}
        onOpenChange={(v) => {
          if (!v && !pending) setProposal(null);
        }}
        title={title + "?"}
        description="Review these details. The application will validate the current state before completing your request."
      >
        <DataView value={proposal} />
        {error && <ErrorState error={error} />}
        <div className="button-row">
          <Button
            variant="outline"
            disabled={pending}
            onClick={() => setProposal(null)}
          >
            Keep editing
          </Button>
          <Button
            disabled={pending}
            onClick={() => proposal && void send(proposal)}
          >
            {pending ? "Completing…" : "Confirm " + title.toLowerCase()}
          </Button>
        </div>
      </Modal>
    </>
  );
}
