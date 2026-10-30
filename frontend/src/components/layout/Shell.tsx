import { roleLabel } from "../../lib/presentation";
import { useState, useEffect, useRef } from "react";
import { NavLink, Outlet, useLocation, Link } from "react-router-dom";
import {
  BookOpen,
  LayoutDashboard,
  Library,
  Repeat,
  ShoppingBag,
  Wallet,
  Truck,
  ChartNoAxesCombined,
  Users,
  Activity,
  Sparkles,
  Menu,
  LogOut,
  Search,
  Sun,
  Settings,
} from "lucide-react";
import { useAuth } from "../../features/auth/Auth";
import { Button } from "../ui/button";
export const NAV = [
  { path: "/", label: "Overview", icon: LayoutDashboard, permission: "" },
  {
    path: "/catalog",
    label: "Catalog",
    icon: Library,
    permission: "catalog.read",
  },
  { path: "/loans", label: "Lending", icon: Repeat, permission: "loan.self" },
  {
    path: "/acquisitions",
    label: "Acquisitions",
    icon: ShoppingBag,
    permission: "acquisition.manage",
  },
  {
    path: "/budgets",
    label: "Budgets",
    icon: Wallet,
    permission: "budget.read",
  },
  {
    path: "/vendors",
    label: "Vendors",
    icon: Truck,
    permission: "vendor.manage",
  },
  {
    path: "/reports",
    label: "Reports",
    icon: ChartNoAxesCombined,
    permission: "report.read",
  },
  {
    path: "/events",
    label: "Activity",
    icon: Activity,
    permission: "event.read",
  },
  {
    path: "/users",
    label: "Administration",
    icon: Users,
    permission: "user.manage",
  },
  { path: "/assistant", label: "Assistant", icon: Sparkles, permission: "" },
  { path: "/settings", label: "Settings", icon: Settings, permission: "" },
];
export function Shell() {
  const { user, logout } = useAuth();
  const [open, setOpen] = useState(false);
  const [mobile, setMobile] = useState(
    () => matchMedia("(max-width: 800px)").matches,
  );
  const menuButton = useRef<HTMLButtonElement>(null);
  const sidebar = useRef<HTMLElement>(null);
  useEffect(() => {
    const media = matchMedia("(max-width: 800px)");
    const update = () => setMobile(media.matches);
    media.addEventListener("change", update);
    return () => media.removeEventListener("change", update);
  }, []);
  useEffect(() => {
    if (!mobile || !open) return;
    sidebar.current?.querySelector<HTMLElement>("a")?.focus();
    const escape = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        setOpen(false);
        menuButton.current?.focus();
      }
    };
    document.addEventListener("keydown", escape);
    return () => document.removeEventListener("keydown", escape);
  }, [mobile, open]);
  const [theme, setTheme] = useState(localStorage.getItem("theme") ?? "system");
  const location = useLocation();
  const [error, setError] = useState("");
  useEffect(() => {
    localStorage.setItem("theme", theme);
    const media = matchMedia("(prefers-color-scheme: dark)");
    const apply = () =>
      (document.documentElement.dataset.theme =
        theme === "system" ? (media.matches ? "dark" : "light") : theme);
    apply();
    media.addEventListener("change", apply);
    return () => media.removeEventListener("change", apply);
  }, [theme]);
  return (
    <div className="workspace">
      <a className="skip-link" href="#main">
        Skip to content
      </a>
      <aside
        ref={sidebar}
        id="workspace-navigation"
        inert={mobile && !open}
        className={open ? "sidebar open" : "sidebar"}
      >
        <Link className="wordmark" to="/">
          <BookOpen />
          LibraCore<span>03</span>
        </Link>
        <p className="nav-caption">YOUR WORKSPACE</p>
        <nav aria-label="Main navigation">
          {NAV.filter(
            (n) => !n.permission || user?.permissions.includes(n.permission),
          ).map((n) => (
            <NavLink
              key={n.path}
              to={n.path}
              end={n.path === "/"}
              onClick={() => setOpen(false)}
            >
              <n.icon size={19} />
              {n.label}
              {n.path === "/assistant" && <span className="nav-ai" aria-hidden="true">AI</span>}
            </NavLink>
          ))}
        </nav>
        <div className="sidebar-note">
          <span className="online-dot" /> Connected knowledge
          <br />
          <small>LibraCore · Version 3.0</small>
        </div>
        <div className="profile">
          <span className="avatar">{user?.name.slice(0, 2).toUpperCase()}</span>
          <div>
            <strong>{user?.name}</strong>
            <small>{roleLabel(user?.role ?? "")}</small>
          </div>
          <button
            className="icon-button"
            title="Sign out"
            onClick={() => logout().catch((e) => setError(e.message))}
          >
            <LogOut size={17} />
          </button>
        </div>
      </aside>
      {open && (
        <button
          className="scrim"
          aria-label="Close navigation"
          onClick={() => setOpen(false)}
        />
      )}
      <div className="workspace-main">
        <header className="topbar">
          <button
            ref={menuButton}
            className="icon-button mobile-menu"
            aria-label="Open navigation"
            aria-expanded={open}
            aria-controls="workspace-navigation"
            onClick={() => setOpen(true)}
          >
            <Menu />
          </button>
          <div className="breadcrumb">
            Workspace <span>/</span>{" "}
            <strong>
              {NAV.find((n) => n.path === (location.pathname === "/administration" ? "/users" : location.pathname))?.label ??
                "Resource details"}
            </strong>
          </div>
          <div className="top-actions">
            <Link to="/catalog" className="search-shortcut">
              <Search size={16} />
              Search the collection
            </Link>
            <label className="theme-select">
              <Sun size={17} />
              <select
                aria-label="Color theme"
                value={theme}
                onChange={(e) => setTheme(e.target.value)}
              >
                <option value="light">Light</option>
                <option value="dark">Dark</option>
                <option value="system">System</option>
              </select>
            </label>
            <Button asChild variant="outline">
              <Link to="/assistant">
                <Sparkles size={16} />
                Ask Libra
              </Link>
            </Button>
          </div>
        </header>
        <main id="main" className="content">
          {error && <p role="alert">{error}</p>}
          <Outlet />
        </main>
        <footer className="workspace-footer">
          A considered space for knowledge.<span>LibraCore V3</span>
        </footer>
      </div>
    </div>
  );
}
