import { desc } from "drizzle-orm";
import { getDb } from "../../../db";
import { dailyMetrics, foodLogs, moodLogs, workoutLogs } from "../../../db/schema";

export async function GET() {
  try {
    const db = getDb();
    const [metrics, foods, workouts, moods] = await Promise.all([
      db.select().from(dailyMetrics).orderBy(desc(dailyMetrics.recordedAt)).limit(7),
      db.select().from(foodLogs).orderBy(desc(foodLogs.loggedAt)).limit(20),
      db.select().from(workoutLogs).orderBy(desc(workoutLogs.loggedAt)).limit(12),
      db.select().from(moodLogs).orderBy(desc(moodLogs.loggedAt)).limit(7),
    ]);
    const moodByDate = new Map(moods.map((item) => [item.loggedAt.slice(0, 10), item.score]));
    const days = metrics.reverse().map((item) => ({
      label: new Date(item.recordedAt).toLocaleDateString("en-AU", { weekday: "short" }).toUpperCase(),
      steps: item.steps,
      calories: item.activeCalories,
      mood: moodByDate.get(item.recordedAt.slice(0, 10)) ?? 3,
    }));
    return Response.json({ days, foods, workouts });
  } catch (error) {
    return Response.json({ error: error instanceof Error ? error.message : "Unable to load dashboard" }, { status: 500 });
  }
}
