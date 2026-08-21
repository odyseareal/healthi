CREATE TABLE `daily_metrics` (
	`id` integer PRIMARY KEY AUTOINCREMENT NOT NULL,
	`recorded_at` text NOT NULL,
	`steps` integer DEFAULT 0 NOT NULL,
	`distance_km` real DEFAULT 0 NOT NULL,
	`cadence` real DEFAULT 0 NOT NULL,
	`flights` integer DEFAULT 0 NOT NULL,
	`active_calories` real DEFAULT 0 NOT NULL,
	`sleep_minutes` integer DEFAULT 0 NOT NULL
);
--> statement-breakpoint
CREATE TABLE `food_logs` (
	`id` integer PRIMARY KEY AUTOINCREMENT NOT NULL,
	`name` text NOT NULL,
	`calories` integer NOT NULL,
	`protein` real DEFAULT 0 NOT NULL,
	`logged_at` text NOT NULL
);
--> statement-breakpoint
CREATE TABLE `mood_logs` (
	`id` integer PRIMARY KEY AUTOINCREMENT NOT NULL,
	`score` integer NOT NULL,
	`note` text DEFAULT '' NOT NULL,
	`logged_at` text NOT NULL
);
--> statement-breakpoint
CREATE TABLE `workout_logs` (
	`id` integer PRIMARY KEY AUTOINCREMENT NOT NULL,
	`exercise` text NOT NULL,
	`sets` integer NOT NULL,
	`reps` integer NOT NULL,
	`calories` real DEFAULT 0 NOT NULL,
	`duration_seconds` integer DEFAULT 0 NOT NULL,
	`logged_at` text NOT NULL
);
