import { getDb } from "../../../db";
import { dailyMetrics, workoutLogs } from "../../../db/schema";

type ArduinoPayload = {
  steps?: number; distanceKm?: number; cadence?: number; flights?: number;
  activeCalories?: number; sleepMinutes?: number; exercise?: string;
  sets?: number; reps?: number; workoutCalories?: number; durationSeconds?: number;
  workoutComplete?: boolean;
};

const finite = (value: unknown, fallback = 0) => Number.isFinite(Number(value)) ? Number(value) : fallback;

export async function POST(request: Request) {
  try {
    const payload = await request.json() as ArduinoPayload;
    const now = new Date().toISOString();
    const db = getDb();
    const statements = [db.insert(dailyMetrics).values({
      recordedAt: now,
      steps: Math.max(0, Math.round(finite(payload.steps))),
      distanceKm: Math.max(0, finite(payload.distanceKm)),
      cadence: Math.max(0, finite(payload.cadence)),
      flights: Math.max(0, Math.round(finite(payload.flights))),
      activeCalories: Math.max(0, finite(payload.activeCalories)),
      sleepMinutes: Math.max(0, Math.round(finite(payload.sleepMinutes))),
    })];
    if (payload.workoutComplete && payload.exercise && finite(payload.sets) > 0 && finite(payload.reps) > 0) {
      statements.push(db.insert(workoutLogs).values({
        exercise: payload.exercise,
        sets: Math.round(finite(payload.sets)),
        reps: Math.round(finite(payload.reps)),
        calories: Math.max(0, finite(payload.workoutCalories)),
        durationSeconds: Math.max(0, Math.round(finite(payload.durationSeconds))),
        loggedAt: now,
      }));
    }
    await db.batch(statements as [typeof statements[0], ...typeof statements]);
    return Response.json({ accepted: true, recordedAt: now }, { status: 201 });
  } catch (error) {
    return Response.json({ error: error instanceof Error ? error.message : "Arduino upload failed" }, { status: 500 });
  }
}

export function OPTIONS() {
  return new Response(null, { status: 204, headers: { "access-control-allow-origin": "*", "access-control-allow-methods": "POST, OPTIONS", "access-control-allow-headers": "content-type" } });
}
