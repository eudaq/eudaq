<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>HIDRA Run Monitoring</title>
<meta name="viewport" content="width=device-width, initial-scale=1">

<style>
:root {
  --bg: #07111f;
  --panel: #101c2d;
  --panel2: #14243a;
  --text: #eaf2ff;
  --muted: #8fa6c2;
  --ok: #27d17f;
  --warn: #ffbf47;
  --bad: #ff5c7a;
  --blue: #48a6ff;
  --border: rgba(255,255,255,.09);
}

* { box-sizing: border-box; }

body {
  margin: 0;
  background: radial-gradient(circle at top, #102744, var(--bg));
  color: var(--text);
  font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}

header {
  padding: 22px 28px;
  border-bottom: 1px solid var(--border);
  display: flex;
  justify-content: space-between;
  gap: 16px;
  align-items: center;
}

h1 {
  margin: 0;
  font-size: 28px;
  letter-spacing: .5px;
}

.status-dot {
  display: inline-block;
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background: var(--bad);
  box-shadow: 0 0 18px var(--bad);
  margin-right: 8px;
}

.status-dot.live {
  background: var(--ok);
  box-shadow: 0 0 18px var(--ok);
}

.status-dot.warn {
  background: var(--warn);
  box-shadow: 0 0 18px var(--warn);
}

.status-dot.bad {
  background: var(--bad);
  box-shadow: 0 0 18px var(--bad);
}

.led-metric {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-top: 8px;
}

.status-led {
  flex: 0 0 auto;
  width: 24px;
  height: 24px;
  border-radius: 50%;
  background: var(--bad);
  box-shadow: 0 0 22px var(--bad);
}

.status-led.ok {
  background: var(--ok);
  box-shadow: 0 0 22px var(--ok);
}

.status-led.bad {
  background: var(--bad);
  box-shadow: 0 0 22px var(--bad);
}

.sub {
  color: var(--muted);
  font-size: 14px;
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 14px;
}

button {
  color: var(--text);
  background: rgba(255,255,255,.075);
  border: 1px solid var(--border);
  border-radius: 8px;
  padding: 7px 10px;
  font: inherit;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

button:hover {
  background: rgba(255,255,255,.12);
}

main {
  padding: 24px;
  display: grid;
  gap: 20px;
}

.cards {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 16px;
}

.card, .device {
  background: linear-gradient(180deg, var(--panel), var(--panel2));
  border: 1px solid var(--border);
  border-radius: 18px;
  padding: 18px;
  box-shadow: 0 14px 35px rgba(0,0,0,.22);
}

.metric-label {
  color: var(--muted);
  font-size: 13px;
  text-transform: uppercase;
  letter-spacing: .08em;
}

.metric-value {
  font-size: 30px;
  font-weight: 800;
  margin-top: 6px;
  overflow-wrap: anywhere;
}

.metric-value.time {
  font-size: 22px;
}

.devices {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
  gap: 16px;
}

.device-head {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  align-items: start;
  margin-bottom: 14px;
}

.device-name {
  font-size: 20px;
  font-weight: 800;
}

.badge {
  padding: 5px 10px;
  border-radius: 999px;
  font-weight: 700;
  font-size: 13px;
  background: rgba(255,255,255,.08);
}

.badge.started { color: var(--ok); }
.badge.warn { color: var(--warn); }
.badge.bad { color: var(--bad); }

.tags {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
  gap: 10px;
}

.tag {
  background: rgba(255,255,255,.055);
  border: 1px solid var(--border);
  border-radius: 12px;
  padding: 10px;
}

.tag-k {
  color: var(--muted);
  font-size: 12px;
}

.tag-v {
  font-size: 20px;
  font-weight: 750;
  margin-top: 3px;
  overflow-wrap: anywhere;
}

.footer {
  color: var(--muted);
  font-size: 13px;
}

.run-history {
  display: none;
  background: rgba(16,28,45,.88);
  border: 1px solid var(--border);
  border-radius: 8px;
  overflow: hidden;
}

.run-history.open {
  display: block;
}

.run-history-head {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
  padding: 7px 10px;
  border-bottom: 1px solid var(--border);
}

.run-history-title {
  font-size: 13px;
  font-weight: 800;
}

.run-history-count {
  color: var(--muted);
  font-size: 12px;
}

.run-table-wrap {
  overflow-x: auto;
}

.run-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 12px;
  line-height: 1.2;
}

.run-table th,
.run-table td {
  padding: 5px 8px;
  border-bottom: 1px solid rgba(255,255,255,.055);
  text-align: left;
  white-space: nowrap;
}

.run-table th {
  color: var(--muted);
  font-size: 11px;
  font-weight: 750;
  text-transform: uppercase;
  letter-spacing: .06em;
  background: rgba(255,255,255,.035);
}

.run-table tbody tr:hover {
  background: rgba(255,255,255,.045);
}

.run-table .num {
  text-align: right;
  font-variant-numeric: tabular-nums;
}

.run-table .run-num,
.run-table .event-num {
  font-size: 15px;
  font-weight: 850;
}

.run-table .event-num {
  color: var(--text);
}

.run-table .bad {
  color: var(--bad);
  font-weight: 750;
}

.run-table .ok {
  color: var(--ok);
  font-weight: 750;
}

.run-table .started {
  color: var(--ok);
  font-weight: 750;
}

.run-table .warn {
  color: var(--warn);
  font-weight: 750;
}

.run-table .devices-cell {
  min-width: 420px;
  white-space: normal;
}

.history-device {
  display: flex;
  gap: 5px;
  align-items: baseline;
  flex-wrap: wrap;
  padding: 1px 0;
}

.history-device + .history-device {
  border-top: 1px solid rgba(255,255,255,.045);
}

.history-device-name {
  font-weight: 800;
}

.history-tag {
  color: var(--muted);
  font-size: 11px;
}

.history-tag {
  background: rgba(255,255,255,.055);
  border: 1px solid rgba(255,255,255,.065);
  border-radius: 5px;
  padding: 1px 4px;
}

.history-tag.ok {
  color: var(--ok);
}

.history-tag.bad {
  color: var(--bad);
}

.error {
  color: var(--bad);
  font-weight: 700;
}

.stale {
  color: var(--warn);
}

.stopped {
  color: var(--bad);
}
</style>
</head>

<body>
<header>
  <div>
    <h1><span id="liveDot" class="status-dot"></span>HIDRA Run Monitoring</h1>
    <div class="sub" id="subtitle">Waiting for data...</div>
  </div>
  <div class="header-actions">
    <button id="historyToggle" type="button" aria-expanded="false">Runs</button>
    <div class="sub" id="clock"></div>
  </div>
</header>

<main>
  <section class="cards">
    <div class="card">
      <div class="metric-label">Run</div>
      <div class="metric-value" id="run">—</div>
    </div>

    <div class="card">
      <div class="metric-label">Start time</div>
      <div class="metric-value time" id="startTime">—</div>
    </div>

    <div class="card">
      <div class="metric-label">Stop time</div>
      <div class="metric-value time" id="stopTime">—</div>
    </div>

    <div class="card">
      <div class="metric-label">Devices</div>
      <div class="metric-value" id="deviceCount">—</div>
    </div>

    <div class="card">
      <div class="metric-label">Events on disk</div>
      <div class="metric-value" id="totalEvents">—</div>
    </div>

    <div class="card">
      <div class="metric-label">BoardsMon</div>
      <div class="led-metric">
        <span id="boardsMonLed" class="status-led bad"></span>
        <div class="metric-value" id="boardsMon">—</div>
      </div>
    </div>

    <div class="card">
      <div class="metric-label">DAQ age</div>
      <div class="metric-value" id="age">—</div>
    </div>
  </section>

  <section class="devices" id="devices"></section>

  <section class="run-history" id="runHistory">
    <div class="run-history-head">
      <div class="run-history-title">Run snapshots</div>
      <div class="run-history-count" id="runHistoryCount">0 runs</div>
    </div>
    <div class="run-table-wrap">
      <table class="run-table">
        <thead>
          <tr>
            <th>Run</th>
            <th>Start</th>
            <th>Last snapshot</th>
            <th>Duration</th>
            <th class="num">Events</th>
            <th>Devices and tags</th>
          </tr>
        </thead>
        <tbody id="runHistoryBody">
          <tr><td colspan="6">Waiting for data...</td></tr>
        </tbody>
      </table>
    </div>
  </section>

  <div class="footer" id="footer"></div>
</main>

<script>
const API = "api.php";
const POLL_MS = 1000;
const HIDDEN_TAGS = new Set(["MonitorEventN"]);
let historyVisible = false;

function nsToDate(ns) {
  if (!ns) return null;
  return new Date(Number(BigInt(ns) / 1000000n));
}

function fmtAge(seconds) {
  if (!Number.isFinite(seconds)) return "—";
  if (seconds < 1) return "<1 s";
  if (seconds < 60) return `${seconds.toFixed(0)} s`;
  return `${(seconds / 60).toFixed(1)} min`;
}

function fmtDuration(ms) {
  if (!Number.isFinite(ms) || ms < 0) return "—";

  const totalSeconds = Math.round(ms / 1000);
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;

  if (hours > 0) return `${hours}h ${minutes}m ${seconds}s`;
  if (minutes > 0) return `${minutes}m ${seconds}s`;
  return `${seconds}s`;
}

function stateClass(state) {
  const s = String(state || "").toLowerCase();

  if (s.includes("started") || s.includes("running")) return "started";
  if (
    s.includes("error") ||
    s.includes("failed") ||
    s.includes("dead") ||
    s.includes("stopped")
  ) return "bad";

  if (s.includes("warn")) return "warn";

  return "";
}

function tagValue(tags, key) {
  return tags && tags[key] !== undefined ? tags[key] : null;
}

function findTagValue(devices, deviceName, keys) {
  const preferredTags = devices[deviceName]?.tags;
  for (const key of keys) {
    const value = tagValue(preferredTags, key);
    if (value !== null) return value;
  }

  for (const device of Object.values(devices)) {
    for (const key of keys) {
      const value = tagValue(device.tags, key);
      if (value !== null) return value;
    }
  }

  return null;
}

function isOkStatus(status) {
  return String(status).trim().toUpperCase() === "OK";
}

function updateBoardsMonStatus(devices) {
  const boardsMon = findTagValue(devices, "HidraFERS2Producer", ["BoardsMon", "BoardStatus"]);
  const status = boardsMon === null ? "—" : String(boardsMon);
  const isOk = isOkStatus(status);

  const valueEl = document.getElementById("boardsMon");
  valueEl.textContent = status;
  valueEl.classList.toggle("stopped", !isOk);

  const led = document.getElementById("boardsMonLed");
  led.classList.toggle("ok", isOk);
  led.classList.toggle("bad", !isOk);
}

function eventsOnDisk(devices) {
  return Number(findTagValue(devices, "HidraDataCollector", ["EventsOnDisk"])) || 0;
}

function escapeHtml(x) {
  return String(x)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function tagStatusClass(key, value) {
  const text = String(value).trim().toLowerCase();
  const name = String(key).trim().toLowerCase();

  if (text === "ok" || text === "true" || text === "ready") return "ok";
  if (
    text.includes("error") ||
    text.includes("fail") ||
    text.includes("dead") ||
    text.includes("bad") ||
    text.includes("not ok") ||
    text === "false"
  ) return "bad";
  if (text.includes("warn")) return "warn";
  if (name.includes("status") && text && text !== "ok") return "bad";

  return "";
}

function renderDeviceTagSummary(devices) {
  const names = Object.keys(devices).sort();

  if (!names.length) {
    return `<span class="sub">No devices</span>`;
  }

  return names.map(name => {
    const dev = devices[name] || {};
    const tags = Object.entries(dev.tags || {})
      .filter(([key]) => !HIDDEN_TAGS.has(key))
      .sort(([a], [b]) => a.localeCompare(b));

    const tagHtml = tags.map(([key, value]) => {
      const klass = tagStatusClass(key, value);
      const label = `${key}=${value}`;
      return `<span class="history-tag ${klass}">${escapeHtml(label)}</span>`;
    }).join("");

    return `
      <div class="history-device">
        <span class="history-device-name">${escapeHtml(name)}</span>
        ${tagHtml || `<span class="history-tag">no tags</span>`}
      </div>
    `;
  }).join("");
}

function summarizeRun(item) {
  const entry = item.entry || {};
  const devices = entry.devices || {};
  const lastDate = nsToDate(entry.time_unix_ns);
  const startDate = item.run_start_unix_ns ? nsToDate(item.run_start_unix_ns) : null;

  return {
    run: entry.run ?? "—",
    start: startDate ? startDate.toLocaleString() : "—",
    last: lastDate ? lastDate.toLocaleString() : "—",
    duration: startDate && lastDate ? fmtDuration(lastDate - startDate) : "—",
    events: eventsOnDisk(devices),
    devicesHtml: renderDeviceTagSummary(devices),
  };
}

function renderRunHistory(runs) {
  const body = document.getElementById("runHistoryBody");
  const count = document.getElementById("runHistoryCount");
  const list = Array.isArray(runs) ? runs : [];

  count.textContent = `${list.length} run${list.length === 1 ? "" : "s"}`;

  if (!list.length) {
    body.innerHTML = `<tr><td colspan="6">No JSONL files found</td></tr>`;
    return;
  }

  body.innerHTML = list.map(item => {
    const row = summarizeRun(item);

    return `
      <tr>
        <td class="num run-num">${escapeHtml(row.run)}</td>
        <td>${escapeHtml(row.start)}</td>
        <td>${escapeHtml(row.last)}</td>
        <td>${escapeHtml(row.duration)}</td>
        <td class="num event-num">${escapeHtml(row.events)}</td>
        <td class="devices-cell">${row.devicesHtml}</td>
      </tr>
    `;
  }).join("");
}

function computeGlobalStatus(deviceNames, devices, now, daqAgeSec) {
  if (!Number.isFinite(daqAgeSec) || daqAgeSec > 10) {
    return "bad";
  }

  let status = "live";

  for (const name of deviceNames) {
    const dev = devices[name];
    const state = String(dev.state || "").toLowerCase();

    const lastUpdate = nsToDate(dev.last_update_unix_ns);
    const devAge = lastUpdate ? (now - lastUpdate) / 1000 : Infinity;

    if (
      state.includes("stopped") ||
      state.includes("error") ||
      state.includes("failed") ||
      state.includes("dead") ||
      devAge > 15
    ) {
      return "bad";
    }

    if (devAge > 5 || state.includes("warn")) {
      status = "warn";
    }
  }

  return status;
}

function render(data) {
  if (!data.ok) {
    throw new Error(data.error || "API error");
  }

  const entry = data.entry;
  renderRunHistory(data.runs);

  if (!entry) {
    document.getElementById("subtitle").innerHTML =
      `<span class="error">No valid JSON line found</span>`;
    return;
  }

  const devices = entry.devices || {};
  const deviceNames = Object.keys(devices);

  const run = entry.run ?? "—";
  const daqDate = nsToDate(entry.time_unix_ns);
  const now = new Date();
  const ageSec = daqDate ? (now - daqDate) / 1000 : NaN;

  const startDate = data.run_start_unix_ns
    ? nsToDate(data.run_start_unix_ns)
    : null;

  const anyStopped = deviceNames.some(name =>
    String(devices[name].state || "").toLowerCase().includes("stopped")
  );

  const stopDate = anyStopped ? nsToDate(entry.time_unix_ns) : null;

  document.getElementById("run").textContent = run;
  document.getElementById("deviceCount").textContent = deviceNames.length;
  document.getElementById("startTime").textContent =
    startDate ? startDate.toLocaleString() : "—";

  const stopTimeEl = document.getElementById("stopTime");
  stopTimeEl.textContent = stopDate ? stopDate.toLocaleString() : "running";
  stopTimeEl.classList.toggle("stopped", Boolean(stopDate));

  const ageEl = document.getElementById("age");
  ageEl.textContent = fmtAge(ageSec);
  ageEl.className = ageSec > 10 ? "metric-value stale" : "metric-value";

 
    let totalEvents = 0;
    /*
  for (const name of deviceNames) {
    const ev = Number(tagValue(devices[name].tags, "EventN"));
    if (Number.isFinite(ev)) {
      totalEvents += ev;
    }
  }
    */
    totalEvents = eventsOnDisk(devices);

  document.getElementById("totalEvents").textContent = totalEvents;
  updateBoardsMonStatus(devices);

  const globalStatus = computeGlobalStatus(deviceNames, devices, now, ageSec);
  const dot = document.getElementById("liveDot");
  dot.classList.remove("live", "warn", "bad");
  dot.classList.add(globalStatus);

  document.getElementById("subtitle").textContent =
    `File: ${data.file} · Last DAQ update: ${daqDate ? daqDate.toLocaleString() : "unknown"}`;

  const container = document.getElementById("devices");
  container.innerHTML = "";

  for (const name of deviceNames.sort()) {
    const dev = devices[name];
    const tags = Object.fromEntries(
    	  Object.entries(dev.tags || {}).filter(([key, value]) => !HIDDEN_TAGS.has(key)));
	  
    const lastUpdate = nsToDate(dev.last_update_unix_ns);
    const devAge = lastUpdate ? (now - lastUpdate) / 1000 : NaN;

    const card = document.createElement("article");
    card.className = "device";

    const tagHtml = Object.entries(tags).map(([k, v]) => `
      <div class="tag">
        <div class="tag-k">${escapeHtml(k)}</div>
        <div class="tag-v">${escapeHtml(v)}</div>
      </div>
    `).join("");

    card.innerHTML = `
      <div class="device-head">
        <div>
          <div class="device-name">${escapeHtml(name)}</div>
          <div class="sub">
            last update: ${lastUpdate ? lastUpdate.toLocaleTimeString() : "unknown"}
            · age ${fmtAge(devAge)}
          </div>
        </div>
        <div class="badge ${stateClass(dev.state)}">${escapeHtml(dev.state || "Unknown")}</div>
      </div>
      <div class="tags">
        ${tagHtml || `<div class="sub">No tags</div>`}
      </div>
    `;

    container.appendChild(card);
  }

  document.getElementById("footer").textContent =
    `File size: ${data.file_size} bytes · Server time: ${new Date(data.server_time * 1000).toLocaleString()}`;
}

async function poll() {
  try {
    const res = await fetch(API, { cache: "no-store" });
    const data = await res.json();
    render(data);
  } catch (err) {
    document.getElementById("subtitle").innerHTML =
      `<span class="error">${escapeHtml(err.message)}</span>`;

    const dot = document.getElementById("liveDot");
    dot.classList.remove("live", "warn");
    dot.classList.add("bad");
  }
}

setInterval(() => {
  document.getElementById("clock").textContent = new Date().toLocaleString();
}, 500);

document.getElementById("historyToggle").addEventListener("click", () => {
  historyVisible = !historyVisible;
  document.getElementById("runHistory").classList.toggle("open", historyVisible);
  document.getElementById("historyToggle").setAttribute("aria-expanded", String(historyVisible));
});

poll();
setInterval(poll, POLL_MS);
</script>
</body>
</html>
