import { integer, real, sqliteTable, text } from "drizzle-orm/sqlite-core";

export const dailyMetrics = sqliteTable("daily_metrics", {
  id: integer("id").primaryKey({ autoIncrement: true }),
  recordedAt: text("recorded_at").notNull(),
  steps: integer("steps").notNull().default(0),
  distanceKm: real("distance_km").notNull().default(0),
  cadence: real("cadence").notNull().default(0),
  flights: integer("flights").notNull().default(0),
  activeCalories: real("active_calories").notNull().default(0),
  sleepMinutes: integer("sleep_minutes").notNull().default(0),
});

export const workoutLogs = sqliteTable("workout_logs", {
  id: integer("id").primaryKey({ autoIncrement: true }),
  exercise: text("exercise").notNull(),
  sets: integer("sets").notNull(),
  reps: integer("reps").notNull(),
  calories: real("calories").notNull().default(0),
  durationSeconds: integer("duration_seconds").notNull().default(0),
  loggedAt: text("logged_at").notNull(),
});

export const foodLogs = sqliteTable("food_logs", {
  id: integer("id").primaryKey({ autoIncrement: true }),
  name: text("name").notNull(),
  calories: integer("calories").notNull(),
  protein: real("protein").notNull().default(0),
  loggedAt: text("logged_at").notNull(),
});

export const moodLogs = sqliteTable("mood_logs", {
  id: integer("id").primaryKey({ autoIncrement: true }),
  score: integer("score").notNull(),
  note: text("note").notNull().default(""),
  loggedAt: text("logged_at").notNull(),
});
