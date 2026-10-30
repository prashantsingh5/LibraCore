import { Gate } from "../components/layout/Gate";
import React from "react";
import ReactDOM from "react-dom/client";
import { BrowserRouter, Routes, Route, Navigate } from "react-router-dom";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { AuthProvider } from "../features/auth/Auth";
import { Shell } from "../components/layout/Shell";
import { Dashboard } from "../pages/Dashboard";
import { Catalog } from "../features/catalog/Catalog";
import { ResourceDetail } from "../features/catalog/ResourceDetail";
import { Loans } from "../features/loans/Loans";
import { Acquisitions } from "../features/acquisitions/Acquisitions";
import { Budgets } from "../features/budgets/Budgets";
import { Vendors } from "../features/vendors/Vendors";
import { Reports } from "../features/reports/Reports";
import { Events } from "../features/events/Events";
import { Users } from "../features/users/Users";
import { Assistant } from "../features/assistant/Assistant";
import { AccountSettings } from "../features/auth/Settings";
import "../styles/index.css";
const client = new QueryClient({
  defaultOptions: {
    queries: { staleTime: 30000, retry: false, refetchOnWindowFocus: false },
    mutations: { retry: false },
  },
});
ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <QueryClientProvider client={client}>
      <BrowserRouter>
        <AuthProvider>
          <Routes>
            <Route element={<Shell />}>
              <Route index element={<Dashboard />} />
              <Route path="catalog" element={<Catalog />} />
              <Route path="catalog/:id" element={<ResourceDetail />} />
              <Route path="loans" element={<Loans />} />
              <Route
                path="acquisitions"
                element={
                  <Gate permission="acquisition.manage">
                    <Acquisitions />
                  </Gate>
                }
              />
              <Route
                path="budgets"
                element={
                  <Gate permission="budget.read">
                    <Budgets />
                  </Gate>
                }
              />
              <Route
                path="vendors"
                element={
                  <Gate permission="vendor.manage">
                    <Vendors />
                  </Gate>
                }
              />
              <Route
                path="reports"
                element={
                  <Gate permission="report.read">
                    <Reports />
                  </Gate>
                }
              />
              <Route
                path="events"
                element={
                  <Gate permission="event.read">
                    <Events />
                  </Gate>
                }
              />
              <Route
                path="users"
                element={
                  <Gate permission="user.manage">
                    <Users />
                  </Gate>
                }
              />
              <Route
                path="administration"
                element={
                  <Gate permission="user.manage">
                    <Users />
                  </Gate>
                }
              />
              <Route path="assistant" element={<Assistant />} />
              <Route path="settings" element={<AccountSettings />} />
              <Route path="*" element={<Navigate to="/" replace />} />
            </Route>
          </Routes>
        </AuthProvider>
      </BrowserRouter>
    </QueryClientProvider>
  </React.StrictMode>,
);
