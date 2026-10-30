import {
  createContext,
  useContext,
  useEffect,
  useState,
  type ReactNode,
  type FormEvent,
} from "react";
import { useQueryClient } from "@tanstack/react-query";
import { BookOpen, ArrowUpRight, Layers, ShieldCheck } from "lucide-react";
import { api, setCsrf } from "../../services/api";
import type { User } from "../../types/api";
import { Button } from "../../components/ui/button";
import { ErrorState, Loading } from "../../components/feedback/States";
const Context = createContext<{
  user: User | null;
  login: (e: string, p: string) => Promise<void>;
  logout: () => Promise<void>;
}>({ user: null, login: async () => {}, logout: async () => {} });
export const useAuth = () => useContext(Context);
export function AuthProvider({ children }: { children: ReactNode }) {
  const [user, setUser] = useState<User | null>(null);
  const [loading, setLoading] = useState(true);
  const qc = useQueryClient();
  useEffect(() => {
    api<{ user: User; csrf_token: string }>("/auth/me")
      .then((r) => {
        setUser(r.user);
        setCsrf(r.csrf_token);
      })
      .catch(() => {})
      .finally(() => setLoading(false));
  }, []);
  const login = async (email: string, password: string) => {
    const r = await api<{ user: User; csrf_token: string }>("/auth/login", {
      email,
      password,
    });
    qc.clear();
    setCsrf(r.csrf_token);
    setUser(r.user);
  };
  const logout = async () => {
    await api("/auth/logout", {});
    qc.clear();
    setCsrf("");
    setUser(null);
  };
  return (
    <Context.Provider value={{ user, login, logout }}>
      {loading ? <Loading /> : user ? children : <Login />}
    </Context.Provider>
  );
}
function Login() {
  const { login } = useAuth();
  const [error, setError] = useState<Error | null>(null);
  const [pending, setPending] = useState(false);
  async function submit(e: FormEvent<HTMLFormElement>) {
    e.preventDefault();
    const f = new FormData(e.currentTarget);
    setPending(true);
    setError(null);
    try {
      await login(String(f.get("email")), String(f.get("password")));
    } catch (e) {
      setError(e as Error);
    } finally {
      setPending(false);
    }
  }
  return (
    <main className="login">
      <section className="login-story">
        <a href="/" className="wordmark">
          <BookOpen /> LibraCore<span>03</span>
        </a>
        <div className="story-content">
          <p className="eyebrow">THE KNOWLEDGE COMMONS</p>
          <h1>
            A place for
            <br />
            every next
            <br />
            <em>discovery.</em>
          </h1>
          <p>
            Your collections, people and possibilities.
            <br />
            Connected in one thoughtful workspace.
          </p>
          <p className="story-caption">A thousand paths. One place to begin.</p>
        </div>
        <footer>
          <span>Your institutional knowledge, connected</span>
          <ShieldCheck size={18} />
        </footer>
      </section>
      <section className="login-form">
        <div>
          <span className="brand-icon">
            <Layers />
          </span>
          <p className="eyebrow">WELCOME TO LIBRACORE</p>
          <h2>
            Your next chapter
            <br />
            starts here.
          </h2>
          <p className="muted">Sign in to your institutional workspace.</p>
          <form onSubmit={submit}>
            <label>
              Institutional email
              <input
                name="email"
                type="email"
                autoComplete="username"
                required
                placeholder="you@institution.edu"
              />
            </label>
            <label>
              Password
              <input
                name="password"
                type="password"
                autoComplete="current-password"
                required
                maxLength={128}
              />
            </label>
            {error && <ErrorState error={error} />}
            <Button disabled={pending} type="submit">
              {pending ? "Signing in…" : "Enter workspace"}
              <ArrowUpRight size={18} />
            </Button>
          </form>
          <p className="small muted">
            Need access? Contact your library administrator.
          </p>
        </div>
        <footer>
          LIBRACORE V3 <span>Secure. Considered. Connected.</span>
        </footer>
      </section>
    </main>
  );
}
