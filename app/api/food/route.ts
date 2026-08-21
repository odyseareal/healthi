import { getDb } from "../../../db";
import { foodLogs } from "../../../db/schema";

export async function POST(request: Request) {
  try {
    const payload = await request.json() as { name?: string; calories?: number; protein?: number };
    const name = payload.name?.trim();
    const calories = Number(payload.calories);
    const protein = Number(payload.protein ?? 0);
    if (!name || !Number.isFinite(calories) || calories <= 0 || !Number.isFinite(protein) || protein < 0)
      return Response.json({ error: "Valid food, calories and protein are required" }, { status: 400 });
    const [food] = await getDb().insert(foodLogs).values({ name, calories: Math.round(calories), protein, loggedAt: new Date().toISOString() }).returning();
    return Response.json({ food }, { status: 201 });
  } catch (error) {
    return Response.json({ error: error instanceof Error ? error.message : "Unable to log food" }, { status: 500 });
  }
}
