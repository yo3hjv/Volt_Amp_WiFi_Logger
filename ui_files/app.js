(() => {
  const el = (id) => document.getElementById(id);

  const connPill = el('connPill');
  const ledLog = el('ledLog');
  const ledPulse = el('ledPulse');
  const timeBar = el('timeBar');

  const valV = el('valV');
  const valA = el('valA');
  const valmW = el('valmW');
  const valmWh = el('valmWh');
  const valT = el('valT');

  const inpInterval = el('inpInterval');
  const btnStart = el('btnStart');
  const btnStop = el('btnStop');

  const stopImax = el('stopImax');
  const stopImin = el('stopImin');
  const stopVmax = el('stopVmax');
  const stopVmin = el('stopVmin');
  const stopT = el('stopT');

  const calV = el('calV');
  const calI = el('calI');
  const btnCal = el('btnCal');

  const plot = el('plot');
  const ctx = plot.getContext('2d');
  const plotScale = el('plotScale');

  let ws;
  let wsTimer;
  let isLogging = false;
  let lastPulseId = 0;

  // Plot buffers (points from plot_point)
  const maxPoints = 900; // enough for wide screens; we map to canvas width
  const points = [];

  // Autoscale up and down (user thresholds)
  let scaleV = 3;
  let scaleA = 0.1; // A

  // User thresholds
  const vScaleSteps = [3, 6, 10, 15, 26, 35];
  const aScaleSteps = [0.1, 0.2, 0.5, 1.0, 1.5, 3.0];

  function nextFromSteps(x, steps, fallbackMax) {
    if (!isFinite(x) || x <= 0) return steps[0];
    for (const s of steps) {
      if (x <= s) return s;
    }
    return fallbackMax;
  }

  function formatElapsed(sec) {
    if (!isFinite(sec)) return '--';
    const s = Math.max(0, Math.floor(sec));
    const hh = String(Math.floor(s / 3600)).padStart(2, '0');
    const mm = String(Math.floor((s % 3600) / 60)).padStart(2, '0');
    const ss = String(s % 60).padStart(2, '0');
    return `${hh}:${mm}:${ss}`;
  }

  function ledSetOn(node, on) {
    node.classList.toggle('on', !!on);
  }

  function pulseBlink() {
    ledPulse.classList.add('pulse');
    setTimeout(() => ledPulse.classList.remove('pulse'), 160);
  }

  function drawGrid() {
    const w = plot.width;
    const h = plot.height;
    ctx.clearRect(0, 0, w, h);

    // dotted every 10px, solid every 100px
    ctx.save();
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, w, h);

    // dotted
    ctx.strokeStyle = 'rgba(128,128,128,0.5)';
    ctx.lineWidth = 1;
    for (let x = 0; x <= w; x += 10) {
      if (x % 100 === 0) continue;
      for (let y = 0; y <= h; y += 10) {
        if (y % 100 === 0) continue;
        ctx.fillStyle = 'rgba(128,128,128,0.25)';
        ctx.fillRect(x, y, 1, 1);
      }
    }

    // solid every 100px
    ctx.strokeStyle = 'rgba(128,128,128,0.35)';
    ctx.setLineDash([]);
    for (let x = 0; x <= w; x += 100) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, h);
      ctx.stroke();
    }
    for (let y = 0; y <= h; y += 100) {
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(w, y);
      ctx.stroke();
    }

    ctx.restore();
  }

  function drawPlot() {
    const w = plot.width;
    const h = plot.height;

    drawGrid();

    // scale labels
    plotScale.textContent = `V: 0..${scaleV.toFixed(3)} | A: 0..${scaleA.toFixed(3)}`;

    ctx.save();
    ctx.font = '12px system-ui, -apple-system, Segoe UI, Roboto, Arial';
    ctx.fillStyle = 'rgba(0,0,0,0.75)';
    ctx.textBaseline = 'top';
    ctx.textAlign = 'left';
    ctx.fillText(`${scaleV.toFixed(3)} V`, 6, 6);
    ctx.textAlign = 'right';
    ctx.fillText(`${scaleA.toFixed(3)} A`, w - 6, 6);
    ctx.restore();

    if (points.length === 0) return;

    // map points to x by order (one point per pixel)
    const visibleCount = Math.min(points.length, w);
    const start = points.length - visibleCount;

    // Voltage (blue)
    ctx.strokeStyle = '#2a84ff';
    ctx.lineWidth = 2;
    ctx.beginPath();
    for (let i = 0; i < visibleCount; i++) {
      const p = points[start + i];
      const x = i;
      const y = h - Math.max(0, Math.min(1, p.V / scaleV)) * h;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();

    // Current (red)
    ctx.strokeStyle = '#ff3b3b';
    ctx.lineWidth = 2;
    ctx.beginPath();
    for (let i = 0; i < visibleCount; i++) {
      const p = points[start + i];
      const x = i;
      const y = h - Math.max(0, Math.min(1, p.A / scaleA)) * h;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  function maybeScaleUpFromPoint(p) {
    const v = Math.max(0, Number(p.V) || 0);
    const a = Math.max(0, Number(p.A) || 0);

    const desiredV = nextFromSteps(v, vScaleSteps, vScaleSteps[vScaleSteps.length - 1]);
    const desiredA = nextFromSteps(a, aScaleSteps, aScaleSteps[aScaleSteps.length - 1]);

    if (desiredV > scaleV) scaleV = desiredV;
    if (desiredA > scaleA) scaleA = desiredA;

    // Allow scale down with hysteresis
    if (desiredV < 0.6 * scaleV) scaleV = desiredV;
    if (desiredA < 0.6 * scaleA) scaleA = desiredA;
  }

  function maybeScaleFromVisible() {
    const w = plot.width;
    const visibleCount = Math.min(points.length, w);
    if (visibleCount <= 0) return;
    const start = points.length - visibleCount;

    let maxV = 0;
    let maxA = 0;
    for (let i = 0; i < visibleCount; i++) {
      const p = points[start + i];
      const v = Math.max(0, Number(p.V) || 0);
      const a = Math.max(0, Number(p.A) || 0);
      if (v > maxV) maxV = v;
      if (a > maxA) maxA = a;
    }

    const desiredV = nextFromSteps(maxV, vScaleSteps, vScaleSteps[vScaleSteps.length - 1]);
    const desiredA = nextFromSteps(maxA, aScaleSteps, aScaleSteps[aScaleSteps.length - 1]);

    if (desiredV > scaleV) scaleV = desiredV;
    else if (desiredV < 0.6 * scaleV) scaleV = desiredV;

    if (desiredA > scaleA) scaleA = desiredA;
    else if (desiredA < 0.6 * scaleA) scaleA = desiredA;
  }

  function send(obj) {
    if (!ws || ws.readyState !== 1) return;
    ws.send(JSON.stringify(obj));
  }

  function connectWs() {
    const proto = location.protocol === 'https:' ? 'wss' : 'ws';
    ws = new WebSocket(`${proto}://${location.host}/ws`);

    ws.onopen = () => {
      connPill.textContent = 'WS: connected';
      connPill.style.color = '#9ff5c4';
    };

    ws.onclose = () => {
      connPill.textContent = 'WS: disconnected';
      connPill.style.color = '';
      ledSetOn(ledLog, false);
      if (wsTimer) clearTimeout(wsTimer);
      wsTimer = setTimeout(connectWs, 1000);
    };

    ws.onerror = () => {
      // ignore
    };

    ws.onmessage = (ev) => {
      let msg;
      try { msg = JSON.parse(ev.data); } catch { return; }

      if (msg.type === 'status') {
        isLogging = !!msg.logging;
        ledSetOn(ledLog, isLogging);
        if (typeof msg.pulse_id === 'number') {
          if (msg.pulse_id !== lastPulseId) {
            lastPulseId = msg.pulse_id;
            pulseBlink();
          }
        }
      }

      if (msg.type === 'telemetry') {
        valV.textContent = (msg.V ?? 0).toFixed(3) + ' V';
        valA.textContent = (msg.A ?? 0).toFixed(4) + ' A';
        valmW.textContent = (msg.mW ?? 0).toFixed(1) + ' mW';
        valmWh.textContent = (msg.mWh ?? 0).toFixed(6) + ' mWh';
        valT.textContent = formatElapsed(msg.elapsed_s ?? 0);

        if (timeBar) {
          const d = (msg.date ?? '').toString();
          const t = (msg.time ?? '').toString();
          if (d.length && t.length) timeBar.textContent = `${d}  ${t}`;
          else timeBar.textContent = '--';
        }

        const usedPct = Number(msg.spiffs_used_pct ?? NaN);
        if (isFinite(usedPct)) {
          if (usedPct >= 75.0) {
            connPill.classList.add('warn');
            if (!connPill.textContent.includes('SPIFFS')) {
              connPill.textContent = connPill.textContent + ` | SPIFFS ${usedPct.toFixed(0)}%`;
            }
          } else {
            connPill.classList.remove('warn');
          }
        }

        if (typeof msg.pulse_id === 'number') {
          if (msg.pulse_id !== lastPulseId) {
            lastPulseId = msg.pulse_id;
            pulseBlink();
          }
        }
      }

      if (msg.type === 'plot_point') {
        const p = {
          t: Number(msg.t ?? 0),
          V: Number(msg.V ?? 0),
          A: Number(msg.A ?? 0),
        };
        points.push(p);
        while (points.length > maxPoints) points.shift();
        maybeScaleUpFromPoint(p);
        maybeScaleFromVisible();
        drawPlot();

        if (typeof msg.pulse_id === 'number') {
          if (msg.pulse_id !== lastPulseId) {
            lastPulseId = msg.pulse_id;
            pulseBlink();
          }
        }
      }
    };
  }

  btnStart.addEventListener('click', () => {
    const interval_ms = Math.max(50, Number(inpInterval.value || 1000));

    const sImax = stopImax.value.trim();
    const sImin = stopImin.value.trim();
    const sVmax = stopVmax.value.trim();
    const sVmin = stopVmin.value.trim();
    const sT = stopT.value.trim();

    send({
      type: 'start_log',
      interval_ms,
      stop_imax_en: sImax.length > 0,
      stop_imin_en: sImin.length > 0,
      stop_vmax_en: sVmax.length > 0,
      stop_vmin_en: sVmin.length > 0,
      stop_t_en: sT.length > 0,
      stop_imax_mA: sImax.length ? Number(sImax) : 0,
      stop_imin_mA: sImin.length ? Number(sImin) : 0,
      stop_vmax_V: sVmax.length ? Number(sVmax) : 0,
      stop_vmin_V: sVmin.length ? Number(sVmin) : 0,
      stop_t_s: sT.length ? Number(sT) : 0,
    });

    points.length = 0;
    scaleV = vScaleSteps[0];
    scaleA = aScaleSteps[0];
    drawPlot();
  });

  btnStop.addEventListener('click', () => {
    send({ type: 'stop_log' });
  });

  btnCal.addEventListener('click', () => {
    const vStr = (calV.value || '').trim();
    const iStr = (calI.value || '').trim();
    const msg = { type: 'cal' };
    if (vStr.length) msg.v_ext = Number(vStr);
    if (iStr.length) msg.i_ext = Number(iStr);
    send(msg);
  });

  // Init
  drawPlot();
  connectWs();
})();
