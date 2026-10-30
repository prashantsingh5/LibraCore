import { useState } from "react";
import { useQueryClient } from "@tanstack/react-query";
import { ActionForm, type FormField } from "../../components/ui/ActionForm";
import { api } from "../../services/api";
import { fromMinor } from "../../lib/presentation";
import { ResourceCover } from "../../components/ui/ResourceCover";
import type { Resource } from "../../types/api";
export function ResourceForm({ resource }: { resource?: Resource }) {
  const [saved, setSaved] = useState<Resource | undefined>(resource);
  const [kind, setKind] = useState(resource?.resource_type ?? "Book");
  const [file, setFile] = useState<File | null>(null);
  const [revert, setRevert] = useState(false);
  const [coverMessage, setCoverMessage] = useState("");
  const [savedMessage, setSavedMessage] = useState("");
  const [version, setVersion] = useState(0);
  const qc = useQueryClient();
  const electronic = ["ElectronicResource", "EBook", "AudioBook"].includes(
    kind,
  );
  const options = (labels: string[]) =>
    labels.map((label, value) => ({ value: String(value), label }));
  const fields: FormField[] = [
    {
      name: "resource_type",
      label: "Resource type",
      options: [
        "Book",
        "Journal",
        "EBook",
        "AudioBook",
        "Thesis",
        "ElectronicResource",
      ],
      value: kind,
      disabled: !!saved,
      required: true,
    },
    {
      name: "resource_id",
      label: "Resource ID",
      required: true,
      disabled: !!saved,
      hint: "Unique catalog reference, e.g. BK-2026-001.",
    },
    { name: "title", label: "Title", required: true },
    { name: "publisher", label: "Publisher" },
    {
      name: "publication_year",
      label: "Publication year",
      type: "number",
      value: new Date().getFullYear(),
      required: true,
    },
    {
      name: "currency",
      label: "Currency code",
      value: "INR",
      disabled: !!saved,
      required: true,
    },
    {
      name: "unit_price_minor",
      label: "Price",
      type: "money",
      required: true,
      hint: "Normal currency amount, for example 125.50.",
    },
    ...(!saved
      ? [
          {
            name: "initial_holdings",
            label: electronic
              ? "Initial licensed seats"
              : "Initial holdings / copies",
            type: "number",
            min: 0,
            value: 0,
            hint: "Already-owned opening stock only. New purchases belong in Acquisitions and use your budget.",
          },
        ]
      : []),
    ...(["Book", "EBook"].includes(kind)
      ? [
          {
            name: "authors",
            label: "Authors (comma separated)",
            type: "authors",
          },
          { name: "isbn", label: "ISBN" },
        ]
      : []),
    ...(kind === "Book"
      ? [
          {
            name: "edition",
            label: "Edition",
            type: "number",
            value: 1,
            min: 1,
          },
          {
            name: "binding",
            label: "Binding",
            type: "number",
            options: options(["Paperback", "Hardcover"]),
            value: 0,
          },
        ]
      : []),
    ...(kind === "Journal"
      ? [
          { name: "issn", label: "ISSN" },
          {
            name: "issues_per_year",
            label: "Issues per year",
            type: "number",
            value: 1,
            min: 1,
          },
          {
            name: "subscription_years",
            label: "Subscription years",
            type: "number",
            value: 1,
            min: 1,
          },
        ]
      : []),
    ...(electronic
      ? [
          { name: "access_url", label: "Electronic access URL", type: "url" },
          {
            name: "license",
            label: "License",
            type: "number",
            options: options(["Perpetual", "Annual subscription"]),
            value: 1,
          },
          {
            name: "platform_fee_minor",
            label: "Platform fee",
            type: "money",
            value: "0.00",
          },
          {
            name: "platform_currency",
            label: "Platform fee currency",
            value: saved?.currency ?? "INR",
          },
        ]
      : []),
    ...(kind === "EBook"
      ? [
          {
            name: "format",
            label: "EBook format",
            type: "number",
            options: options(["PDF", "EPUB", "HTML"]),
            value: 0,
          },
          {
            name: "drm",
            label: "Digital rights management",
            type: "boolean",
            options: [
              { value: "false", label: "No" },
              { value: "true", label: "Yes" },
            ],
            value: "false",
          },
        ]
      : []),
    ...(kind === "AudioBook"
      ? [
          { name: "narrator", label: "Narrator" },
          {
            name: "duration_minutes",
            label: "Duration (minutes)",
            type: "number",
            min: 1,
            value: 90,
          },
        ]
      : []),
    ...(kind === "Thesis"
      ? [
          { name: "university", label: "University" },
          { name: "degree", label: "Degree" },
          { name: "supervisor", label: "Supervisor" },
        ]
      : []),
  ];
  async function afterSave(result: unknown) {
    const r = result as Resource;
    setSaved(r);
    setSavedMessage("Resource saved successfully.");
    setCoverMessage("");
    if (kind === "Book" && (file || revert)) {
      try {
        await api(
          "/resources/" + encodeURIComponent(r.resource_id) + "/cover",
          file ?? undefined,
          file ? "PUT" : "DELETE",
        );
        setVersion((v) => v + 1);
        setFile(null);
        setRevert(false);
        setCoverMessage(file ? "Cover updated." : "Default cover restored.");
        await qc.invalidateQueries({ queryKey: ["resource"] });
      } catch (e) {
        setCoverMessage(
          "Resource saved, but cover was not changed: " +
            (e as Error).message +
            " Select the cover again and save to retry.",
        );
      }
    }
  }
  return (
    <>
      <ActionForm
        key={saved ? "edit" : "new"}
        title={saved ? "Save resource" : "Create resource"}
        path={
          "/resources" +
          (saved ? "/" + encodeURIComponent(saved.resource_id) : "")
        }
        method={saved ? "PUT" : "POST"}
        keys={["catalog", "resource"]}
        onSuccess={afterSave}
        onValueChange={(name, value) => {
          if (name === "resource_type") setKind(value);
        }}
        extra={saved ? { resource_type: kind } : {}}
        fields={fields.map((f) => ({
          ...f,
          value:
            saved && saved[f.name] !== undefined
              ? f.type === "money"
                ? fromMinor(saved[f.name])
                : f.name === "authors"
                  ? saved.authors.join(", ")
                  : String(saved[f.name])
              : f.value,
        }))}
      >
        {kind === "Book" && (
          <fieldset className="cover-editor">
            <legend>Book cover · optional</legend>
            {saved && (
              <ResourceCover
                id={saved.resource_id}
                title={saved.title}
                version={version}
              />
            )}
            <label>
              Upload book cover
              <input
                type="file"
                accept=".jpg,.jpeg,.png,.webp"
                onChange={(e) => {
                  setFile(e.target.files?.[0] ?? null);
                  setRevert(false);
                }}
              />
            </label>
            <small>
              JPG, PNG or WEBP, up to 5 MB. A library cover is used by default.
            </small>
            {saved && (
              <label className="checkbox">
                <input
                  type="checkbox"
                  checked={revert}
                  onChange={(e) => {
                    setRevert(e.target.checked);
                    setFile(null);
                  }}
                />
                Revert to default cover
              </label>
            )}
          </fieldset>
        )}
      </ActionForm>
      {savedMessage && (
        <p role="status" className="success-message">
          {savedMessage}
        </p>
      )}
      {coverMessage && <p role="status">{coverMessage}</p>}
    </>
  );
}
