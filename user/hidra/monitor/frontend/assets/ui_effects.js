/* Ambient UI effects for the dashboard, loaded automatically from assets/.
 *
 * A short canvas particle animation ("shower"). It is idle by default —
 * just one pending timer and a click listener, no CPU until it runs — and
 * removes itself when done. The overlay has pointer-events disabled so it
 * never blocks the UI.
 *
 * Triggers:
 *   - a triple-click on the title (always available), and
 *   - optionally, once a day at a configured local hour. That automatic
 *     trigger is read from the `#ui-effects-config` element's
 *     `data-shower-hour` attribute (set from config.yaml's
 *     `ui_effects.shower_hour`); an empty value disables it.
 */
(function () {
  "use strict";

  var PALETTE = ["#cba6f7", "#89b4fa", "#a6e3a1", "#f9e2af", "#f38ba8", "#94e2d5"];
  var active = false;

  // ── Trigger: triple-click the title ────────────────────────────────
  // Delegated on document so it works regardless of when Dash renders the
  // (client-side) title element.
  var clicks = 0;
  var clickTimer = null;
  document.addEventListener("click", function (e) {
    if (!e.target.closest || !e.target.closest("#app-title")) return;
    clicks += 1;
    clearTimeout(clickTimer);
    if (clicks >= 3) {
      clicks = 0;
      shower();
      return;
    }
    clickTimer = setTimeout(function () { clicks = 0; }, 1200);
  });

  // ── Trigger: once a day at a configured local hour ─────────────────
  // The hour comes from config.yaml via the #ui-effects-config element.
  // An empty / out-of-range value disables the automatic trigger.
  function readShowerHour(el) {
    var v = el.getAttribute("data-shower-hour");
    if (v === null || v === "" || v === "null") return null;
    var h = parseInt(v, 10);
    return isNaN(h) || h < 0 || h > 23 ? null : h;
  }

  // Recompute the delay each time (instead of a fixed 24h interval) so it
  // stays aligned across DST changes. Only fires while a page is open.
  function msUntilNextHour(hour) {
    var now = new Date();
    var next = new Date(now.getFullYear(), now.getMonth(), now.getDate(), hour, 0, 0, 0);
    if (next <= now) next.setDate(next.getDate() + 1);
    return next - now;
  }

  function startScheduler(el) {
    var hour = readShowerHour(el);
    if (hour === null) return; // automatic trigger disabled in config
    (function scheduleDaily() {
      setTimeout(function () {
        shower();
        scheduleDaily();
      }, msUntilNextHour(hour));
    })();
  }

  // Dash renders the layout (and thus #ui-effects-config) client-side
  // after this asset loads, so wait for the element before scheduling.
  (function waitForConfig() {
    var el = document.getElementById("ui-effects-config");
    if (el) {
      startScheduler(el);
      return;
    }
    var obs = new MutationObserver(function () {
      var found = document.getElementById("ui-effects-config");
      if (found) {
        obs.disconnect();
        startScheduler(found);
      }
    });
    obs.observe(document.documentElement, { childList: true, subtree: true });
  })();

  function toast(text) {
    var el = document.createElement("div");
    el.textContent = text;
    Object.assign(el.style, {
      position: "fixed", top: "16%", left: "50%", transform: "translateX(-50%)",
      padding: "10px 18px", borderRadius: "10px",
      background: "rgba(30,30,46,0.92)", color: "#cdd6f4",
      border: "1px solid #cba6f7", boxShadow: "0 0 22px rgba(203,166,247,0.45)",
      fontFamily: "monospace", fontSize: "15px", zIndex: 99999,
      opacity: "0", transition: "opacity 0.4s ease", pointerEvents: "none",
    });
    document.body.appendChild(el);
    requestAnimationFrame(function () { el.style.opacity = "1"; });
    setTimeout(function () { el.style.opacity = "0"; }, 2600);
    setTimeout(function () { el.remove(); }, 3200);
  }

  function shower() {
    if (active) return;
    active = true;
    toast("🌌  cosmic-ray shower detected!");

    var canvas = document.createElement("canvas");
    Object.assign(canvas.style, {
      position: "fixed", inset: "0", zIndex: 99998, pointerEvents: "none",
    });
    document.body.appendChild(canvas);
    var ctx = canvas.getContext("2d");
    var dpr = window.devicePixelRatio || 1;

    function resize() {
      canvas.width = window.innerWidth * dpr;
      canvas.height = window.innerHeight * dpr;
    }
    resize();
    window.addEventListener("resize", resize);

    var parts = [];
    var t0 = performance.now();
    var SPAWN_MS = 3500;
    var TOTAL_MS = 6500;

    function spawn() {
      parts.push({
        x: Math.random() * canvas.width,
        y: -10 * dpr,
        vx: (Math.random() - 0.5) * 1.2 * dpr,
        vy: (1.5 + Math.random() * 3) * dpr,
        r: (1.5 + Math.random() * 3) * dpr,
        c: PALETTE[(Math.random() * PALETTE.length) | 0],
        life: 1,
      });
    }

    function frame(now) {
      var t = now - t0;
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      if (t < SPAWN_MS) {
        for (var s = 0; s < 4; s++) spawn();
      }
      for (var i = 0; i < parts.length; i++) {
        var p = parts[i];
        p.x += p.vx;
        p.y += p.vy;
        p.vy += 0.03 * dpr;
        if (p.y > canvas.height + 20 * dpr) p.life = 0;
        ctx.globalAlpha = Math.max(0, p.life);
        ctx.fillStyle = p.c;
        ctx.shadowColor = p.c;
        ctx.shadowBlur = 12 * dpr;
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.r, 0, Math.PI * 2);
        ctx.fill();
      }
      for (var j = parts.length - 1; j >= 0; j--) {
        if (parts[j].life <= 0) parts.splice(j, 1);
      }
      if (t < TOTAL_MS && (t < SPAWN_MS || parts.length)) {
        requestAnimationFrame(frame);
      } else {
        window.removeEventListener("resize", resize);
        canvas.remove();
        active = false;
      }
    }
    requestAnimationFrame(frame);
  }
})();
