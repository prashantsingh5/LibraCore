import { ResultCard } from "../../components/feedback/ResultCard";
import { roleLabel } from "../../lib/presentation";
import { useState, type FormEvent } from "react";
import { useQueryClient } from "@tanstack/react-query";
import { Sparkles, ArrowUp, ShieldCheck } from "lucide-react";
import { api } from "../../services/api";
import type { AssistantReply, Proposal } from "../../types/api";
import { Button } from "../../components/ui/button";
import { DataView } from "../../components/feedback/DataView";
import { ErrorState, Status } from "../../components/feedback/States";
import { useAuth } from "../auth/Auth";
interface Message {
  id: string;
  prompt: string;
  reply?: AssistantReply;
  error?: Error;
}
export function Assistant() {
  const { user } = useAuth();
  const [messages, setMessages] = useState<Message[]>([]);
  const [text, setText] = useState("");
  const [pending, setPending] = useState(false);
  const qc = useQueryClient();
  async function submit(e: FormEvent) {
    e.preventDefault();
    if (!text.trim()) return;
    const id = crypto.randomUUID();
    const prompt = text;
    setMessages((m) => [...m, { id, prompt }]);
    setText("");
    setPending(true);
    try {
      const reply = await api<AssistantReply>("/assistant/message", {
        message: prompt,
        session_id: "browser",
      });
      setMessages((m) => m.map((x) => (x.id === id ? { ...x, reply } : x)));
    } catch (e) {
      setMessages((m) =>
        m.map((x) => (x.id === id ? { ...x, error: e as Error } : x)),
      );
    } finally {
      setPending(false);
    }
  }
  async function confirmed() {
    for (const key of [
      "orders",
      "budget",
      "loans",
      "events",
      "catalog",
      "resource",
    ])
      await qc.invalidateQueries({ queryKey: [key] });
  }
  return (
    <>
      <div className="page-heading">
        <div>
          <p className="eyebrow">A LITTLE GUIDANCE, A LOT OF POSSIBILITY</p>
          <h1>Meet Libra.</h1>
          <p className="muted">
            Find resources, explore information and act with confidence.
          </p>
        </div>
        <Status tone="info">{roleLabel(user?.role ?? "")} access</Status>
      </div>
      <section className="assistant-workspace">
        <div className="assistant-intro">
          <span className="assistant-orb">
            <Sparkles size={28} />
          </span>
          <h2>What would you like to discover?</h2>
          <p className="muted">
            I use your permitted tools. The library core provides every result.
          </p>
          <div className="prompt-grid">
            {[
              "Find books by Scott Meyers",
              "How many copies of B001 are available?",
              "Can Computer Science buy 1 copies of B001?",
            ].map((p) => (
              <button key={p} onClick={() => setText(p)}>
                {p}
                <ArrowUp size={16} />
              </button>
            ))}
          </div>
        </div>
        <div className="conversation" aria-live="polite">
          {messages.map((m) => (
            <article className="exchange" key={m.id}>
              <div className="user-message">{m.prompt}</div>
              <div className="assistant-message">
                <span className="eyebrow">
                  <Sparkles size={14} /> LIBRA
                </span>
                {m.error ? (
                  <ErrorState error={m.error} />
                ) : m.reply ? (
                  <>
                    <p>{m.reply.message}</p>
                    {m.reply.tool_results.map((r, i) => (
                      <div className="tool-card" key={i}>
                        <Status tone="success">
                          Tool completed · {r.operation.replaceAll("_", " ")}
                        </Status>
                        <ResultCard operation={r.operation} value={r.result} />
                      </div>
                    ))}
                    {m.reply.confirmations.map((p) => (
                      <Confirmation
                        key={p.token}
                        proposal={p}
                        onConfirmed={confirmed}
                      />
                    ))}
                  </>
                ) : (
                  <p role="status" className="muted">
                    Consulting your available tools…
                  </p>
                )}
              </div>
            </article>
          ))}
        </div>
        <form className="assistant-compose" onSubmit={submit}>
          <label className="sr-only" htmlFor="assistant-input">
            Message Libra
          </label>
          <textarea
            id="assistant-input"
            value={text}
            onChange={(e) => setText(e.target.value)}
            placeholder="Ask about your collection…"
            maxLength={8000}
            rows={2}
          />
          <Button
            disabled={pending || !text.trim()}
            type="submit"
            aria-label="Send message"
          >
            <ArrowUp size={20} />
          </Button>
        </form>
        <p className="assistant-footnote">
          <ShieldCheck size={14} />
          Intent from AI. Decisions from the core. Permissions from your role.
        </p>
      </section>
    </>
  );
}
export function Confirmation({
  proposal,
  onConfirmed,
}: {
  proposal: Proposal;
  onConfirmed: () => Promise<void>;
}) {
  const [state, setState] = useState("pending");
  const [result, setResult] = useState<unknown>(null);
  const [error, setError] = useState<Error | null>(null);
  async function confirm() {
    setState("working");
    try {
      setResult(
        await api("/assistant/confirm", {
          token: proposal.token,
          session_id: "browser",
        }),
      );
      setState("done");
      await onConfirmed();
    } catch (e) {
      setError(e as Error);
      setState("failed");
    }
  }
  return (
    <div className="confirmation-card">
      <Status tone="warning">Human approval required</Status>
      <h3>{proposal.action.replaceAll("_", " ")}</h3>
      <p className="muted">
        Review the exact request. It expires after{" "}
        {proposal.expires_in_seconds / 60} minutes. No action has been executed.
      </p>
      <DataView value={proposal.parameters} />
      {error && <ErrorState error={error} />}
      {result !== null && (
        <ResultCard operation={proposal.action} value={result} />
      )}
      {state === "pending" && (
        <div className="button-row">
          <Button variant="outline" onClick={() => setState("dismissed")}>
            Dismiss request
          </Button>
          <Button onClick={() => void confirm()}>Confirm action</Button>
        </div>
      )}
      {state !== "pending" && (
        <p role="status">
          {state === "done"
            ? "Action completed."
            : state === "dismissed"
              ? "Request dismissed."
              : state === "working"
                ? "Executing once…"
                : "Request could not complete. Ask Libra for a new proposal."}
        </p>
      )}
    </div>
  );
}
