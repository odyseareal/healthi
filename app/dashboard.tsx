"use client";

import { FormEvent, useEffect, useState } from "react";

type DayPoint = { label: string; steps: number; calories: number; mood: number };
type FoodLog = { id: number; name: string; calories: number; protein: number; loggedAt: string };
type Workout = { id: number; exercise: string; sets: number; reps: number; calories: number; durationSeconds: number; loggedAt: string };
type SerialPortLike = { open(options: { baudRate: number }): Promise<void>; readable: ReadableStream<Uint8Array> };

const demoDays: DayPoint[] = [
  { label: "MON", steps: 6840, calories: 1840, mood: 3 },
  { label: "TUE", steps: 9220, calories: 2130, mood: 4 },
  { label: "WED", steps: 7560, calories: 1980, mood: 4 },
  { label: "THU", steps: 11040, calories: 2260, mood: 5 },
  { label: "FRI", steps: 8430, calories: 2075, mood: 4 },
  { label: "SAT", steps: 12780, calories: 2410, mood: 5 },
  { label: "SUN", steps: 9472, calories: 2184, mood: 4 },
];

const demoFoods: FoodLog[] = [
  { id: -1, name: "Greek yoghurt + berries", calories: 284, protein: 22, loggedAt: "08:10" },
  { id: -2, name: "Chicken rice bowl", calories: 618, protein: 46, loggedAt: "12:42" },
  { id: -3, name: "Banana", calories: 105, protein: 1, loggedAt: "15:25" },
];

const demoWorkouts: Workout[] = [
  { id: -1, exercise: "Push-ups", sets: 4, reps: 12, calories: 42, durationSeconds: 520, loggedAt: "Today · 7:14 AM" },
  { id: -2, exercise: "Squats", sets: 3, reps: 15, calories: 68, durationSeconds: 690, loggedAt: "Yesterday · 5:36 PM" },
  { id: -3, exercise: "Pull-ups", sets: 5, reps: 6, calories: 55, durationSeconds: 610, loggedAt: "19 Aug · 4:20 PM" },
];

const moodFaces = ["◔", "◑", "●", "◕", "✦"];
const moodLabels = ["Very low", "Low", "Okay", "Good", "Great"];

function formatDuration(seconds: number) {
  const minutes = Math.floor(seconds / 60);
  return `${minutes}:${String(seconds % 60).padStart(2, "0")}`;
}

function MetricCard({ label, value, unit, delta, tone }: { label: string; value: string; unit: string; delta: string; tone: string }) {
  return <article className={`metric-card ${tone}`}><div className="metric-top"><span>{label}</span><span className="mini-pulse" /></div><div className="metric-value">{value}<small>{unit}</small></div><p>{delta}</p></article>;
}

function StepsChart({ days }: { days: DayPoint[] }) {
  const max = Math.max(...days.map((d) => d.steps), 12000);
  const points = days.map((d, i) => `${i * 100},${92 - d.steps / max * 74}`).join(" ");
  return <div className="chart-wrap" aria-label="Seven-day step chart"><svg viewBox="0 0 600 112" role="img"><defs><linearGradient id="stepFill" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stopColor="#b7ff4a" stopOpacity=".32" /><stop offset="100%" stopColor="#b7ff4a" stopOpacity="0" /></linearGradient></defs>{[18, 42, 66, 90].map((y) => <line key={y} x1="0" x2="600" y1={y} y2={y} className="grid-line" />)}<polygon points={`0,100 ${points} 600,100`} fill="url(#stepFill)" /><polyline points={points} className="step-line" />{days.map((d, i) => <circle key={d.label} cx={i * 100} cy={92 - d.steps / max * 74} r="4" className="step-dot" />)}</svg><div className="chart-labels">{days.map((d) => <span key={d.label}>{d.label}</span>)}</div></div>;
}

export default function Dashboard() {
  const [tab, setTab] = useState("Overview");
  const [days, setDays] = useState(demoDays);
  const [foods, setFoods] = useState(demoFoods);
  const [workouts, setWorkouts] = useState(demoWorkouts);
  const [foodOpen, setFoodOpen] = useState(false);
  const [moodOpen, setMoodOpen] = useState(false);
  const [mood, setMood] = useState(4);
  const [syncing, setSyncing] = useState(false);
  const [deviceConnected, setDeviceConnected] = useState(false);
  const [toast, setToast] = useState("");

  useEffect(() => {
    fetch("/api/dashboard").then((response) => response.ok ? response.json() : null).then((data) => {
      if (!data) return;
      if (data.days?.length) setDays(data.days);
      if (data.foods?.length) setFoods(data.foods);
      if (data.workouts?.length) setWorkouts(data.workouts);
    }).catch(() => undefined);
  }, []);

  const averageSteps = Math.round(days.reduce((sum, day) => sum + day.steps, 0) / days.length);
  const today = days[days.length - 1];
  const foodCalories = foods.reduce((sum, food) => sum + food.calories, 0);
  const protein = foods.reduce((sum, food) => sum + food.protein, 0);
  const showToast = (message: string) => { setToast(message); window.setTimeout(() => setToast(""), 2600); };

  const saveFood = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    const form = new FormData(event.currentTarget);
    const entry = { name: String(form.get("name")), calories: Number(form.get("calories")), protein: Number(form.get("protein")) };
    setFoods((current) => [{ id: Date.now(), ...entry, loggedAt: "Just now" }, ...current.filter((item) => item.id > 0)]);
    setFoodOpen(false); showToast("Food added to today");
    await fetch("/api/food", { method: "POST", headers: { "content-type": "application/json" }, body: JSON.stringify(entry) }).catch(() => undefined);
  };
  const saveMood = async () => { setMoodOpen(false); showToast(`Mood saved: ${moodLabels[mood - 1]}`); await fetch("/api/mood", { method: "POST", headers: { "content-type": "application/json" }, body: JSON.stringify({ score: mood }) }).catch(() => undefined); };
  const syncNow = () => { setSyncing(true); window.setTimeout(() => { setSyncing(false); showToast("Arduino data is up to date"); }, 1200); };
  const readSerial = async (port: SerialPortLike) => {
    const reader = port.readable.getReader();
    const decoder = new TextDecoder();
    let buffer = "";
    let lastUploadMs = 0;
    let lastWorkoutKey = "";
    try {
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split(/\r?\n/); buffer = lines.pop() ?? "";
        for (const line of lines) {
          let packet: Record<string, string | number> = {};
          try { packet = JSON.parse(line); }
          catch {
            line.split(",").forEach((part) => { const [key, raw] = part.trim().split("="); if (key && raw) packet[key] = Number.isNaN(Number(raw)) ? raw : Number(raw); });
            packet.activeCalories = Number(packet.calories ?? 0);
          }
          const workoutKey = packet.workoutComplete ?
            `${packet.exercise}-${packet.sets}-${packet.reps}-${packet.durationSeconds}` : "";
          const completedWorkoutIsNew = Boolean(workoutKey && workoutKey !== lastWorkoutKey);
          if (Object.keys(packet).length > 1 &&
              (Date.now() - lastUploadMs >= 30000 || completedWorkoutIsNew)) {
            await fetch("/api/arduino", { method: "POST", headers: { "content-type": "application/json" }, body: JSON.stringify(packet) });
            lastUploadMs = Date.now();
            if (completedWorkoutIsNew) lastWorkoutKey = workoutKey;
          }
        }
      }
    } finally { reader.releaseLock(); setDeviceConnected(false); }
  };
  const connectArduino = async () => {
    const serial = (navigator as unknown as { serial?: { requestPort(): Promise<SerialPortLike> } }).serial;
    if (!serial) { showToast("Use Chrome or Edge for USB connection"); return; }
    try {
      const port = await serial.requestPort();
      await port.open({ baudRate: 115200 });
      setDeviceConnected(true); showToast("Arduino connected at 115200 baud");
      void readSerial(port);
    } catch { showToast("Connection cancelled or unavailable"); }
  };

  return <main>
    <aside className="sidebar">
      <a className="brand" href="#top"><span className="brand-mark">H</span><strong>Healthi</strong></a>
      <nav aria-label="Main navigation">{["Overview", "Progress", "Nutrition", "Mind", "Device"].map((item) => <button key={item} onClick={() => setTab(item)} className={tab === item ? "active" : ""}><span>{({ Overview: "⌁", Progress: "↗", Nutrition: "◒", Mind: "◎", Device: "⌗" } as Record<string, string>)[item]}</span>{item}</button>)}</nav>
      <div className="device-mini"><span className="status-dot" /><div><strong>ARDUINO R4</strong><small>{deviceConnected ? "USB connected · live" : "Ready for USB sync"}</small></div></div>
      <button className="profile"><span>CG</span><div><strong>Chengcheng</strong><small>Student athlete</small></div><b>•••</b></button>
    </aside>
    <section className="shell" id="top">
      <header className="topbar"><div><p className="eyebrow">FRIDAY · 21 AUGUST</p><h1>{tab === "Overview" ? <>MOVE WITH <em>INTENT.</em></> : tab.toUpperCase()}</h1></div><div className="header-actions"><button className="icon-button" aria-label="Notifications">◌<i /></button><button className="primary-action" onClick={() => setFoodOpen(true)}>＋ LOG FOOD</button></div></header>

      {(tab === "Overview" || tab === "Progress") && <><section className="metrics-grid"><MetricCard label="STEPS TODAY" value={today.steps.toLocaleString()} unit="steps" delta="↑ 12% vs daily average" tone="lime" /><MetricCard label="ACTIVE ENERGY" value="486" unit="kcal" delta="72% of 675 kcal goal" tone="violet" /><MetricCard label="DISTANCE" value="6.83" unit="km" delta="Pace 9:14 /km · 108 spm" tone="blue" /><MetricCard label="FLIGHTS CLIMBED" value="14" unit="flights" delta="↑ 4 above weekly average" tone="coral" /></section>
      <section className="dashboard-grid"><article className="panel chart-panel"><div className="panel-heading"><div><span className="section-kicker">LAST 7 DAYS</span><h2>Daily movement</h2></div><div className="average"><small>DAILY AVG</small><strong>{averageSteps.toLocaleString()}</strong></div></div><StepsChart days={days} /><div className="chart-summary"><span><b>68%</b> goal consistency</span><span><b>+18%</b> from last week</span><span><b>12,780</b> best day</span></div></article>
      <article className="panel recovery-panel"><div className="panel-heading"><div><span className="section-kicker">TODAY&apos;S READINESS</span><h2>Recovery score</h2></div><span className="score-chip">GOOD</span></div><div className="recovery-ring"><div><strong>82</strong><span>/100</span></div></div><div className="recovery-bars"><label><span>Sleep quality <b>86%</b></span><i><u style={{ width: "86%" }} /></i></label><label><span>Mood balance <b>80%</b></span><i><u style={{ width: "80%" }} /></i></label><label><span>Activity load <b>77%</b></span><i><u style={{ width: "77%" }} /></i></label></div><button className="text-button" onClick={() => setMoodOpen(true)}>CHECK IN NOW ↗</button></article></section>
      <section className="lower-grid"><article className="panel workout-panel"><div className="panel-heading"><div><span className="section-kicker">RECENT ACTIVITY</span><h2>Workout log</h2></div><button className="text-button" onClick={() => setTab("Progress")}>VIEW ALL ↗</button></div><div className="workout-list">{workouts.map((workout) => <div className="workout-row" key={workout.id}><span className="workout-icon">{workout.exercise === "Squats" ? "◇" : workout.exercise === "Pull-ups" ? "⌁" : "↟"}</span><div><strong>{workout.exercise}</strong><small>{workout.loggedAt}</small></div><p><b>{workout.sets} × {workout.reps}</b><small>SETS × REPS</small></p><p><b>{formatDuration(workout.durationSeconds)}</b><small>ACTIVE</small></p><p><b>{workout.calories}</b><small>KCAL</small></p></div>)}</div></article><article className="panel sync-panel"><div className="orb"><span /></div><span className="section-kicker">LIVE SENSOR</span><h2>{syncing ? "Reading your tracker…" : "Everything is synced."}</h2><p>Steps, workouts, sleep and stair data transfer from your Arduino dashboard.</p><button onClick={syncNow}>{syncing ? "SYNCING…" : "SYNC NOW"}</button></article></section></>}

      {tab === "Nutrition" && <section className="feature-page nutrition-page"><div className="nutrition-hero"><div><span className="section-kicker">TODAY&apos;S INTAKE</span><h2>{foodCalories.toLocaleString()} <small>/ 2,400 kcal</small></h2><p>{Math.max(0, 2400 - foodCalories)} kcal remaining</p></div><div className="macro-ring" style={{ "--progress": `${Math.min(100, foodCalories / 24)}%` } as React.CSSProperties}><span>{Math.round(foodCalories / 24)}%</span></div></div><div className="nutrition-grid"><article className="panel food-list"><div className="panel-heading"><div><span className="section-kicker">MEALS</span><h2>Food log</h2></div><button className="primary-action" onClick={() => setFoodOpen(true)}>＋ ADD</button></div>{foods.map((food) => <div className="food-row" key={food.id}><span>◒</span><div><strong>{food.name}</strong><small>{food.loggedAt} · {food.protein} g protein</small></div><b>{food.calories}<small> kcal</small></b></div>)}</article><article className="panel macro-card"><span className="section-kicker">MACRONUTRIENTS</span><h2>Daily balance</h2><div className="macro-stat"><span>Protein</span><b>{protein} / 130 g</b><i><u style={{ width: `${Math.min(100, protein / 1.3)}%` }} /></i></div><div className="macro-stat"><span>Carbohydrates</span><b>214 / 300 g</b><i><u style={{ width: "71%" }} /></i></div><div className="macro-stat"><span>Fats</span><b>58 / 80 g</b><i><u style={{ width: "72%" }} /></i></div></article></div></section>}

      {tab === "Mind" && <section className="feature-page mind-page"><article className="mood-hero"><span className="section-kicker">MENTAL WELLBEING</span><h2>How are you feeling?</h2><p>A quick check-in helps identify patterns between mood, sleep and activity.</p><div className="mood-picker">{moodFaces.map((face, index) => <button key={face} className={mood === index + 1 ? "selected" : ""} onClick={() => setMood(index + 1)}><span>{face}</span><small>{moodLabels[index]}</small></button>)}</div><button className="primary-action" onClick={saveMood}>SAVE CHECK-IN</button></article><div className="mind-grid"><article className="panel"><span className="section-kicker">7-DAY TREND</span><h2>Mood average: 4.1</h2><div className="mood-trend">{days.map((day) => <div key={day.label}><i style={{ height: `${day.mood * 16}%` }} /><span>{day.label}</span></div>)}</div></article><article className="panel insight-card"><span>✦</span><h2>Your strongest days follow better sleep.</h2><p>On days after 8+ hours of sleep, your mood score is approximately 18% higher.</p><small>PERSONAL INSIGHT · NOT MEDICAL ADVICE</small></article></div></section>}

      {tab === "Device" && <section className="feature-page device-page"><article className="device-hero"><div className="device-visual"><span>H</span><i /></div><div><span className="live-badge"><i /> {deviceConnected ? "USB CONNECTED" : "READY TO CONNECT"}</span><h2>Arduino UNO R4 WiFi</h2><p>{deviceConnected ? "Live serial upload · 115200 baud" : "Connect by USB in Chrome or Edge"}</p><div className="device-stats"><span><b>19</b> data fields</span><span><b>{deviceConnected ? "LIVE" : "—"}</b> sync health</span><span><b>5 V</b> USB connection</span></div><div className="device-actions"><button className="primary-action" onClick={connectArduino}>{deviceConnected ? "DEVICE CONNECTED" : "CONNECT ARDUINO"}</button><a className="download-action" href="/healthi-standalone.html" download>DOWNLOAD STANDALONE</a></div></div></article><div className="device-grid"><article className="panel"><span className="section-kicker">DATA ENDPOINT</span><h2>Send readings to Healthi</h2><p className="muted">Connect by USB, or make a JSON POST request after a workout or every few minutes.</p><code>POST /api/arduino</code><pre>{`{\n  "steps": 9472,\n  "distanceKm": 6.83,\n  "cadence": 108,\n  "flights": 14,\n  "exercise": "pushups",\n  "sets": 4, "reps": 12\n}`}</pre></article><article className="panel sensor-map"><span className="section-kicker">LATEST READING</span><h2>Sensor health</h2>{[["MPU6050", "Motion + steps"], ["HC-SR04", "Exercise reps"], ["BMP180", "Flights climbed"], ["Joystick", "Menu + mood"]].map(([name, role]) => <div key={name}><i /><span><b>{name}</b><small>{role}</small></span><strong>{deviceConnected ? "LIVE" : "READY"}</strong></div>)}</article></div></section>}
    </section>
    {foodOpen && <div className="modal-backdrop" onMouseDown={() => setFoodOpen(false)}><form className="modal" onSubmit={saveFood} onMouseDown={(event) => event.stopPropagation()}><button type="button" className="modal-close" onClick={() => setFoodOpen(false)}>×</button><span className="section-kicker">NUTRITION</span><h2>Log food</h2><label>Food or meal<input name="name" required placeholder="e.g. Chicken rice bowl" /></label><div className="form-grid"><label>Calories<input name="calories" required type="number" min="1" placeholder="520" /></label><label>Protein (g)<input name="protein" required type="number" min="0" placeholder="35" /></label></div><button className="primary-action" type="submit">ADD TO TODAY</button></form></div>}
    {moodOpen && <div className="modal-backdrop" onMouseDown={() => setMoodOpen(false)}><div className="modal mood-modal" onMouseDown={(event) => event.stopPropagation()}><button className="modal-close" onClick={() => setMoodOpen(false)}>×</button><span className="section-kicker">QUICK CHECK-IN</span><h2>How do you feel?</h2><div className="mood-picker compact">{moodFaces.map((face, index) => <button key={face} className={mood === index + 1 ? "selected" : ""} onClick={() => setMood(index + 1)}><span>{face}</span><small>{index + 1}</small></button>)}</div><p className="mood-result">{moodLabels[mood - 1]}</p><button className="primary-action" onClick={saveMood}>SAVE CHECK-IN</button></div></div>}
    {toast && <div className="toast"><span>✓</span>{toast}</div>}
  </main>;
}
