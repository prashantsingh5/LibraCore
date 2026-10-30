import { useState } from "react";
export function ResourceCover({
  id,
  title,
  type = "Book",
  version = 0,
}: {
  id: string;
  title: string;
  type?: string;
  version?: number;
}) {
  const [failed, setFailed] = useState(false);
  return (
    <div className="resource-cover" data-kind={type}>
      <img
        key={id + version}
        src={
          !failed && type === "Book"
            ? "/api/v1/resources/" +
              encodeURIComponent(id) +
              "/cover?v=" +
              version
            : "/art/default-cover.svg"
        }
        onError={() => setFailed(true)}
        alt={title + " cover"}
        loading="lazy"
      />
      <div className="cover-caption">
        <span>{type}</span>
        <strong>{title}</strong>
      </div>
    </div>
  );
}
