import {
  ResponsiveContainer,
  BarChart,
  Bar,
  XAxis,
  YAxis,
  Tooltip,
} from "recharts";
export function Utilization({ used }: { used: number }) {
  const data = [
    { name: "Budget used", value: used },
    { name: "Remaining", value: Math.max(0, 100 - used) },
  ];
  return (
    <>
      <div className="chart" aria-hidden="true">
        <ResponsiveContainer width="100%" height={170}>
          <BarChart
            data={data}
            layout="vertical"
            margin={{ left: 12, right: 16 }}
          >
            <XAxis
              type="number"
              domain={[0, 100]}
              tickFormatter={(v) => v + "%"}
            />
            <YAxis type="category" dataKey="name" width={90} />
            <Tooltip formatter={(v) => v + "%"} />
            <Bar
              isAnimationActive={false}
              dataKey="value"
              fill="var(--accent)"
              radius={[0, 5, 5, 0]}
            />
          </BarChart>
        </ResponsiveContainer>
      </div>
      <p className="small muted">
        {used.toFixed(1)}% of the budget is used; {(100 - used).toFixed(1)}%
        remains.
      </p>
    </>
  );
}
