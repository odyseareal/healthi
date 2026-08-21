import { getDb } from "../../../db";
import { moodLogs } from "../../../db/schema";

export async function POST(request: Request) {
  try {
    const payload = await request.json() as { score?: number; note?: string };
    const score = Number(payload.score);
    if (!Number.isInteger(score) || score < 1 || score > 5)
      return Response.json({ error: "Mood score must be an integer from 1 to 5" }, { status: 400 });
    const [mood] = await getDb().insert(moodLogs).values({ score, note: payload.note?.trim() ?? "", loggedAt: new Date().toISOString() }).returning();
    return Response.json({ mood }, { status: 201 });
  } catch (error) {
    return Response.json({ error: error instanceof Error ? error.message : "Unable to save mood" }, { status: 500 });
  }
}
