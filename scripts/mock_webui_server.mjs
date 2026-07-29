// 독립 Web UI 원본을 mock API로 로컬 확인하는 개발 서버
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '..');
const webUiPath = path.join(repoRoot, 'web', 'web_ui.html');

const nagModes = {
  1: { mode: 1, modeStr: 'MODE 1', modeSummary: '고정 +1.80Nm 에코. DAS/조향각 조건 없음.', requiresContext: false },
  2: { mode: 2, modeStr: 'MODE 2', modeSummary: '4개 토크를 200ms 간격으로 1초 순환하고 1.5초 휴지.', requiresContext: false },
  3: { mode: 3, modeStr: 'MODE 3', modeSummary: '최신 원본에서 제거된 레거시 조건부 상태기계.', requiresContext: true },
};

const scenarios = new Set(['normal', 'a_warn', 'ap_block', 'bus_off', 'bus_err', 'no_frames']);
const cli = parseArgs(process.argv.slice(2));

function defaultSignalObserverSignals() {
  return [];
}

function defaultToggles() {
  return {
    a_channel_tx: true,
    summon_unlock_enabled: true,
    tsllc_enabled: true,
    nag_killer: true,
    nag_ap_only: true,
    a_spi_8mhz: false,
    a_mcp_oneshot: false,
    a_tx_guard: false,
  };
}

const state = {
  bootMs: Date.now(),
  scenario: scenarios.has(cli.scenario) ? cli.scenario : 'normal',
  theme: 'dark',
  nagMode: 2,
  logHead: 0,
  logs: [],
  rec: false,
  recStartMs: 0,
  samples: 24,
  diagStartedMs: 0,
  diagLogHead: 0,
  userMarkerActive: false,
  userMarkerCount: 0,
  userMarkerLastMs: 0,
  userMarkerLastDetail: 0,
  observerResetMs: 0,
  observerRuntime: true,
  observerFrozenElapsedMs: 0,
  observerSignals: defaultSignalObserverSignals(),
  toggles: defaultToggles(),
};

function resetMockNvsState() {
  state.theme = 'dark';
  state.nagMode = 2;
  state.rec = false;
  state.recStartMs = 0;
  state.samples = 24;
  state.userMarkerActive = false;
  state.userMarkerCount = 0;
  state.userMarkerLastMs = 0;
  state.userMarkerLastDetail = 0;
  state.observerResetMs = nowMs();
  state.observerRuntime = true;
  state.observerFrozenElapsedMs = 0;
  state.observerSignals = defaultSignalObserverSignals();
  state.toggles = defaultToggles();
}

let cachedHtml = '';
let cachedMtimeMs = 0;

function parseArgs(args) {
  const out = { host: '127.0.0.1', port: 8787, scenario: 'normal', explicitPort: false };
  for (let i = 0; i < args.length; i++) {
    const arg = args[i];
    if (arg === '--host' && args[i + 1]) out.host = args[++i];
    else if (arg === '--port' && args[i + 1]) {
      out.port = Number(args[++i]) || out.port;
      out.explicitPort = true;
    } else if (arg === '--scenario' && args[i + 1]) out.scenario = args[++i];
    else if (arg === '--help' || arg === '-h') {
      console.log('Usage: node scripts/mock_webui_server.mjs [--host 127.0.0.1] [--port 8787] [--scenario normal|a_warn|ap_block|bus_off|bus_err|no_frames]');
      process.exit(0);
    }
  }
  return out;
}

function getWebUiHtml() {
  const stat = fs.statSync(webUiPath);
  if (cachedHtml && stat.mtimeMs === cachedMtimeMs) return cachedHtml;

  cachedHtml = fs.readFileSync(webUiPath, 'utf8');
  cachedMtimeMs = stat.mtimeMs;
  return cachedHtml;
}

function nowMs() {
  return Date.now() - state.bootMs;
}

function elapsedSeconds() {
  return Math.floor(nowMs() / 1000);
}

function nagModeInfo() {
  return nagModes[state.nagMode] || nagModes[2];
}

function pushLog(msg) {
  state.logHead += 1;
  state.logs.push({ head: state.logHead, ts: Date.now(), msg });
  if (state.logs.length > 100) state.logs.shift();
}

function clampNagMode(value) {
  const mode = Number(value);
  return mode >= 1 && mode <= 3 ? mode : 2;
}

function send(res, status, contentType, body) {
  const bytes = Buffer.byteLength(body);
  res.writeHead(status, {
    'content-type': contentType,
    'content-length': bytes,
    'cache-control': 'no-store',
  });
  res.end(body);
}

function sendJson(res, obj, status = 200) {
  send(res, status, 'application/json; charset=utf-8', JSON.stringify(obj));
}

function sendText(res, text, status = 200, contentType = 'text/plain; charset=utf-8') {
  send(res, status, contentType, text);
}

function readBody(req, limitBytes = 1024 * 1024) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    let total = 0;
    req.on('data', (chunk) => {
      total += chunk.length;
      if (total > limitBytes) {
        reject(new Error('request body too large'));
        req.destroy();
        return;
      }
      chunks.push(chunk);
    });
    req.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')));
    req.on('error', reject);
  });
}

async function readJsonBody(req) {
  const body = await readBody(req);
  if (!body.trim()) return {};
  try {
    return JSON.parse(body);
  } catch {
    return {};
  }
}

function boolFromBody(body, fallback) {
  if (typeof body.enabled === 'boolean') return body.enabled;
  if (body.enabled === 1 || body.enabled === '1' || body.enabled === 'true') return true;
  if (body.enabled === 0 || body.enabled === '0' || body.enabled === 'false') return false;
  return fallback;
}

function tickCounts() {
  const t = elapsedSeconds();
  const noFrames = state.scenario === 'no_frames';
  const bBusOff = state.scenario === 'bus_off';
  const bBusErr = state.scenario === 'bus_err';
  const aFrames = noFrames ? 0 : 1200 + t * 6;
  const bFrames = noFrames ? 0 : 180000 + t * 100;
  const b880 = noFrames ? 0 : 12000 + t * 100;
  const b923 = noFrames ? 0 : 320 + t * 2;
  const b297 = noFrames ? 0 : 14000 + t * 100;
  const injects = state.scenario === 'ap_block' || bBusOff || noFrames ? 0 : 210 + t * 3;
  const echo = noFrames ? 0 : 4200 + injects;
  return {
    t,
    noFrames,
    bBusOff,
    bBusErr,
    aFrames,
    bFrames,
    b880,
    b923,
    b297,
    injects,
    echo,
  };
}

function feature(enabled, build = true, supported = true) {
  return { supported, enabled, build_enabled: build };
}

function normalizeObserverChannel(value) {
  const text = String(value || 'A').toUpperCase();
  if (text === 'B' || text === 'CH') return 'B';
  if (text === 'BOTH' || text === 'A+B' || text === 'AB') return 'A+B';
  return 'A';
}

function normalizeObserverSignal(entry) {
  const frameId = Number(entry.frame_id ?? entry.id);
  const startBit = Number(entry.start_bit ?? entry.startBit);
  const length = Number(entry.length);
  if (!Number.isFinite(frameId) || frameId < 0 || frameId > 0x7ff) throw new Error('invalid frame_id');
  const byteOrder = String(entry.byte_order ?? entry.byteOrder ?? 'little').toLowerCase();
  if (!['little', 'big', 'intel', 'motorola'].includes(byteOrder)) throw new Error('byte_order must be little or big');
  const normalizedByteOrder = (byteOrder === 'big' || byteOrder === 'motorola') ? 'big' : 'little';
  if (!Number.isFinite(startBit) || !Number.isFinite(length) || startBit < 0 || startBit > 63 || length <= 0 || length > 32 || (normalizedByteOrder !== 'big' && startBit + length > 64)) {
    throw new Error('invalid bit layout');
  }
  return {
    name: String(entry.name || 'signal').slice(0, 39),
    enabled: entry.enabled !== false,
    channel: normalizeObserverChannel(entry.channel),
    frame_id: frameId,
    byte_order: normalizedByteOrder,
    start_bit: startBit,
    length,
    idle: Number(entry.idle ?? 0) || 0,
  };
}

function observerAFilterFits(signals) {
  const ids = new Set([280, 390, 921, 1016, 1021]);
  for (const sig of signals) {
    if (!sig.enabled || !sig.channel.includes('A')) continue;
    ids.add(sig.frame_id);
    if (ids.size > 6) return false;
  }
  return true;
}

function observerStatusJson(counts, uptimeMs) {
  const elapsed = state.observerRuntime ? Math.max(0, uptimeMs - state.observerResetMs) : state.observerFrozenElapsedMs;
  const sampleMs = state.observerRuntime ? uptimeMs : state.observerResetMs + elapsed;
  const tick = Math.floor(elapsed / 500);
  const noFrames = counts.noFrames;
  const signals = state.observerSignals.map((sig, idx) => {
    const baseRate = sig.channel === 'B' ? 100 : sig.channel === 'A+B' ? 20 : 6;
    const frameCount = noFrames ? 0 : Math.floor((elapsed / 1000) * baseRate) + idx * 3;
    let raw = 0;
    if (!noFrames && sig.enabled) {
      if (sig.name === 'SCCM_turnIndicatorStalkStatus') raw = Math.floor(elapsed / 3500) % 3 === 1 ? 1 : Math.floor(elapsed / 3500) % 3 === 2 ? 2 : 0;
      else raw = (tick + idx) % (1 << Math.min(sig.length, 4));
    }
    const active = raw !== (sig.idle || 0);
    const burstCount = noFrames ? 0 : Math.floor(elapsed / 3500) + (active ? 1 : 0);
    const currentRunFrames = active ? Math.floor((elapsed % 3500) / (sig.channel === 'B' ? 10 : 167)) + 1 : 0;
    const lastRunFrames = active ? Math.max(0, Math.floor(3500 / (sig.channel === 'B' ? 10 : 167)) - 3) : Math.floor((elapsed % 3500) / (sig.channel === 'B' ? 10 : 167));
    return {
      ...sig,
      channel: sig.channel,
      frame_hex: `0x${sig.frame_id.toString(16).toUpperCase().padStart(3, '0')}`,
      seen: frameCount > 0,
      active,
      raw,
      prev_raw: Math.max(0, raw - 1),
      frame_count: frameCount,
      active_frame_count: active ? Math.floor(frameCount / 2) : Math.floor(frameCount / 3),
      change_count: noFrames ? 0 : Math.floor(elapsed / 1500),
      burst_count: burstCount,
      current_run_frames: currentRunFrames,
      last_run_frames: lastRunFrames,
      max_run_frames: Math.max(currentRunFrames, lastRunFrames),
      first_seen_ms: frameCount > 0 ? state.observerResetMs : 0,
      last_seen_ms: frameCount > 0 ? sampleMs : 0,
      last_change_ms: frameCount > 0 ? Math.max(state.observerResetMs, sampleMs - 400) : 0,
      age_ms: frameCount > 0 ? Math.max(0, uptimeMs - sampleMs) : 0,
    };
  });
  return {
    enabled: state.observerRuntime,
    max_signals: 10,
    max_a_filter_ids: 6,
    event_count: Math.min(256, Math.floor(elapsed / 1500)),
    event_capacity: 256,
    event_overwritten: 0,
    count: signals.length,
    signals,
  };
}

function statusJson(url) {
  const c = tickCounts();
  const logSince = Number(url.searchParams.get('log_since') || 0);
  const twaiStateCode = c.bBusOff ? 2 : 1;
  const fresh = !c.noFrames;
  const uptimeMs = nowMs();
  const logs = state.logs.filter((entry) => entry.head > logSince).map((entry) => ({ msg: entry.msg, ts: entry.ts }));

  return {
    isa_speed_chime_suppress: false,
    emergency_vehicle_detection: false,
    summon_unlock_enabled: state.toggles.summon_unlock_enabled,
    nag_killer: state.toggles.nag_killer,
    a_channel_tx: state.toggles.a_channel_tx,
    tsllc_enabled: state.toggles.tsllc_enabled,
    nag_ap_only: state.toggles.nag_ap_only,
    theme: state.theme,
    uptime_ms: uptimeMs,
    uptime_s: Math.floor(uptimeMs / 1000),
    firmware_version: 'mock-1.2.0',
    firmware_build_id: 'MOCK-LOCAL-WEBUI',
    firmware_build_short: 'MOCK-LOCAL',
    firmware_build_env: 'mock_webui',
    firmware_build_at: new Date(state.bootMs).toISOString(),
    firmware_git_sha: 'local',
    firmware_git_branch: 'mock',
    firmware_source_hash: 'mock',
    firmware_git_dirty: true,
    hw_handler: 'HW3',
    user_marker_count: state.userMarkerCount,
    user_marker_log_count: state.userMarkerCount,
    user_marker_last_ms: state.userMarkerLastMs,
    user_marker_last_detail: state.userMarkerLastDetail,
    user_marker_last_detail_text: userMarkerDetailName(state.userMarkerLastDetail),
    user_marker_active: state.userMarkerActive,
    summon_unlock: {
      enabled: state.toggles.summon_unlock_enabled,
      active: state.toggles.a_channel_tx && state.toggles.summon_unlock_enabled,
      tx_master: state.toggles.a_channel_tx,
      gate: state.scenario !== 'ap_block',
      ap: state.scenario !== 'ap_block',
      parked: state.scenario !== 'ap_block',
      summon: false,
      aca: false,
      spr: false,
      block_reason: !state.toggles.summon_unlock_enabled ? 'DISABLED' :
        !state.toggles.a_channel_tx ? 'A_TX_OFF' :
        state.scenario === 'ap_block' ? 'PARK-,SUMMON-' : 'NONE',
      last_280_age_ms: c.noFrames ? 9000 : 20,
      parked_timeout_ms: 5000,
      rx280: c.noFrames ? 0 : Math.floor(c.aFrames / 5),
      rx390: c.noFrames ? 0 : Math.floor(c.aFrames / 7),
      rx921: c.noFrames ? 0 : Math.floor(c.aFrames / 6),
      rx1016: c.noFrames ? 0 : Math.floor(c.aFrames / 2),
      rxMux1: c.noFrames || state.scenario === 'ap_block' ? 0 : Math.floor(c.aFrames / 3),
      txOk: c.noFrames || state.scenario === 'ap_block' ? 0 : Math.floor(c.aFrames / 3),
      txFail: 0,
      blocked: state.scenario === 'ap_block' ? Math.floor(c.aFrames / 3) : 0,
      wake_count: c.noFrames ? 0 : 2,
      wake_waiting_tx: false,
      wake_to_tx_ms: c.noFrames ? 0 : 184,
      last_wake_rx_ms: c.noFrames ? 0 : Math.max(0, uptimeMs - 5000),
      canState: 1,
      uptimeS: Math.floor(uptimeMs / 1000),
      hardware: 'HW3',
      enable_bit: 46,
    },
    tsllc: {
      enabled: state.toggles.tsllc_enabled,
      tx_master: state.toggles.a_channel_tx,
      guard: false,
      inject_ready: state.toggles.tsllc_enabled && state.toggles.a_channel_tx,
      txOk: c.noFrames ? 0 : Math.floor(c.aFrames / 6),
      txFail: 0,
      last_tx_age_ms: c.noFrames ? 0 : 24,
    },
    log_head: state.logHead,
    logs,
    web_health: {
      status_req_count: c.t + 1,
      status_last_age_ms: 0,
      status_last_duration_ms: 1,
      status_max_duration_ms: 3,
      nag_stats_req_count: c.t + 1,
      nag_stats_last_age_ms: 0,
      nag_stats_last_duration_ms: 1,
      nag_stats_max_duration_ms: 3,
      logs_bundle_req_count: 0,
      logs_bundle_last_age_ms: 0,
      logs_bundle_last_duration_ms: 0,
      logs_bundle_max_duration_ms: 0,
      free_heap: 220000,
      min_free_heap: 180000,
      ap_station_count: 1,
      ap_station_change_count: 1,
      ap_station_last_change_age_ms: uptimeMs,
    },
    ota_pending_state: 0,
    ota_pending_verify: false,
    ota_rollback_confirm_pending: false,
    ota_recovery_mode: false,
    can_boot_allowed: true,
    can_boot_block_reason: '',
    ota_confirm_remaining_ms: 0,
    ota_confirm_window_ms: 60000,
    ota_rollback_remaining_ms: 0,
    ota_rollback_window_ms: 60000,
    ota_current_label: 'mock_ota_0',
    ota_fallback_label: 'mock_ota_1',
    features: {
      isa_speed_chime_suppress: feature(false, false, false),
      emergency_vehicle_detection: feature(false, false, false),
      summon_unlock: feature(state.toggles.summon_unlock_enabled),
      nag_killer: feature(state.toggles.nag_killer),
      nag_ap_only: feature(state.toggles.nag_ap_only),
      a_channel_tx: feature(state.toggles.a_channel_tx),
      tsllc_enabled: feature(state.toggles.tsllc_enabled),
      a_spi_8mhz: feature(state.toggles.a_spi_8mhz),
      a_mcp_oneshot: feature(state.toggles.a_mcp_oneshot),
      a_tx_guard: feature(state.toggles.a_tx_guard),
      ota: feature(false),
    },
    channels: {
      a_channel: {
        frames_received: c.aFrames,
        frame_hz: c.noFrames ? 0 : 6.0,
        frames_1016: c.noFrames ? 0 : Math.floor(c.aFrames / 2),
        id_1016_period_ms: c.noFrames ? 0 : 333,
        frames_1021: c.aFrames,
        id_1021_period_ms: c.noFrames ? 0 : 167,
        frames_280: c.noFrames ? 0 : Math.floor(c.aFrames / 5),
        frames_390: c.noFrames ? 0 : Math.floor(c.aFrames / 7),
        frames_921: c.noFrames ? 0 : Math.floor(c.aFrames / 6),
        summon_unlock_modified: c.noFrames ? 0 : Math.floor(c.aFrames / 3),
        summon_tx_ok: c.noFrames || state.scenario === 'ap_block' ? 0 : Math.floor(c.aFrames / 3),
        summon_tx_fail: 0,
        summon_blocked: state.scenario === 'ap_block' ? Math.floor(c.aFrames / 3) : 0,
        tsllc_modified: c.noFrames ? 0 : Math.floor(c.aFrames / 6),
        tsllc_tx_ok: c.noFrames ? 0 : Math.floor(c.aFrames / 6),
        tsllc_tx_fail: 0,
        last_frame_id: c.noFrames ? 0 : 1021,
        last_loop_ms: uptimeMs,
        core_id: 0,
        spi_freq_hz: state.toggles.a_spi_8mhz ? 8000000 : 10000000,
        spi_requested_hz: state.toggles.a_spi_8mhz ? 8000000 : 10000000,
        spi_reboot_required: false,
        mcp_one_shot: state.toggles.a_mcp_oneshot,
        channel_tx_enabled: state.toggles.a_channel_tx,
        tx_guard_enabled: state.toggles.a_tx_guard,
        mcp_eflg: state.scenario === 'a_warn' ? 0x40 : 0,
        mcp_eflg_peak: 0x40,
        mcp_txbo_count: 0,
        mcp_recovery_attempt: 0,
        mcp_recovery_success: 0,
        mcp_recovery_fail: 0,
        mcp_busoff_since_ms: 0,
        mcp_last_recovery_ms: 0,
        tx_ok: c.noFrames ? 0 : Math.floor(c.aFrames / 2),
        tx_fail: c.noFrames ? 0 : 1,
        tx_fail_window: 0,
        tx_fail_window_peak: c.noFrames ? 0 : 1,
        tx_fail_guard_threshold: 2,
        tec: 0,
        rec: 0,
        tec_peak: 0,
        merrf: 0,
        rx_ovr: 1,
        rec_peak: 0,
        last_frame_rx_ms: uptimeMs,
        last_tx_ms: uptimeMs,
        eflg_event_count: state.scenario === 'a_warn' ? 1 : 0,
        wake_count: c.noFrames ? 0 : 2,
        last_wake_rx_ms: c.noFrames ? 0 : Math.max(0, uptimeMs - 5000),
        wake_waiting_summon_tx: false,
        wake_to_summon_tx_ms: c.noFrames ? 0 : 184,
        tx_guard_active: false,
        tx_guard_remaining_ms: 0,
        tx_guard_count: 0,
        tx_guard_skip: 0,
        tx_guard_reason: 'NONE',
        driver_initialized: true,
        driver_ok: true,
        connected: fresh,
        fresh,
        task_alive: true,
        frame_age_ms: fresh ? 20 : 9000,
        loop_age_ms: 10,
        health_state: state.scenario === 'a_warn' ? 'RX_OVERRUN' : c.noFrames ? 'NO_FRAMES' : 'OK',
        health_reason: state.scenario === 'a_warn' ? 'MCP2515 수신 버퍼 오버런' : c.noFrames ? '최근 2초간 수신 프레임 없음' : '정상 수신 중',
        health_level: state.scenario === 'a_warn' || c.noFrames ? 1 : 0,
        eflg_rx1_overrun: false,
        eflg_rx0_overrun: state.scenario === 'a_warn',
        eflg_tx_bus_off: false,
        eflg_tx_passive: false,
        eflg_rx_passive: false,
        eflg_tx_warning: false,
        eflg_rx_warning: false,
        eflg_warning: false,
      },
      b_channel: {
        frames_received: c.bFrames,
        frame_hz: c.noFrames ? 0 : 100.0,
        filtered_hz: c.noFrames ? 0 : 112.0,
        frames_880: c.b880,
        frames_target: c.b880,
        frames_921: 0,
        frames_923: c.b923,
        frames_297: c.b297,
        target_id: 880,
        id_880_period_ms: c.noFrames ? 0 : 10,
        id_target_period_ms: c.noFrames ? 0 : 10,
        id_921_period_ms: 0,
        id_923_period_ms: c.noFrames ? 0 : 500,
        id_297_period_ms: c.noFrames ? 0 : 10,
        das_hands_state: state.scenario === 'normal' ? 3 : 2,
        das_source_id: c.noFrames ? 0 : 923,
        last_das_status_rx_ms: uptimeMs,
        nag_mode: state.nagMode,
        nag_mode_name: nagModeInfo().modeStr,
        nag_ap_only: state.toggles.nag_ap_only,
        nag_ap_active: state.scenario !== 'ap_block',
        echo_count: c.echo,
        echo_drop_late: 0,
        skip_runtime_or_inactive: 0,
        skip_ap_state: state.scenario === 'ap_block' ? 2500 + c.t * 100 : 0,
        skip_hands_on: 0,
        skip_das_state: 0,
        twai_state_code: twaiStateCode,
        last_frame_id: c.noFrames ? 0 : 880,
        last_frame_rx_ms: uptimeMs,
        last_loop_ms: uptimeMs,
        core_id: 1,
        busoff_count: c.bBusOff ? 1 : 0,
        recovery_attempt_count: c.bBusOff ? 1 : 0,
        recovery_success_count: 0,
        recovery_fail_count: 0,
        last_busoff_ms: c.bBusOff ? Math.max(0, uptimeMs - 5000) : 0,
        last_recovery_start_ms: 0,
        last_recovery_done_ms: 0,
        last_recovery_duration_ms: 0,
        twai_rx_err_peak: 0,
        twai_tx_err_peak: c.bBusOff ? 255 : 0,
        twai_rx_err_now: 0,
        twai_tx_err_now: c.bBusOff ? 255 : 0,
        arb_lost: c.noFrames ? 0 : 120 + c.t,
        bus_error: c.bBusErr ? 21 : 0,
        tx_failed: c.bBusOff ? 3 : 0,
        rx_missed: 0,
        driver_initialized: true,
        driver_ok: true,
        driver_install_err: 0,
        driver_start_err: 0,
        can_task_created: true,
        connected: fresh && !c.bBusOff,
        fresh,
        task_alive: true,
        frame_age_ms: fresh ? 8 : 9000,
        loop_age_ms: 8,
        health_state: c.bBusOff ? 'BUS_OFF' : c.noFrames ? 'NO_FRAMES' : c.bBusErr ? 'ERROR_WARNING' : 'RUNNING',
        health_reason: c.bBusOff ? 'TWAI BUS-OFF' : c.noFrames ? '최근 2초간 수신 프레임 없음' : c.bBusErr ? 'TWAI 버스 오류 증가' : '정상 수신 중',
        health_level: c.bBusOff ? 2 : c.noFrames || c.bBusErr ? 1 : 0,
      },
    },
    signal_observer: observerStatusJson(c, uptimeMs),
    can: {
      state: c.bBusOff ? 'BUS_OFF' : 'RUNNING',
      state_channel: 'B',
      rx_errors: 0,
      tx_errors: c.bBusOff ? 255 : 0,
      bus_errors: c.bBusErr ? 21 : 0,
      rx_missed: 0,
      rx_queued: 0,
      frames_received: c.aFrames + c.bFrames,
      frames_received_a: c.aFrames,
      frames_received_b: c.bFrames,
      frames_sent: Math.floor(c.aFrames / 3) + c.echo,
      frames_sent_a: Math.floor(c.aFrames / 3),
      frames_sent_b: c.echo,
    },
  };
}

function nagConfigJson() {
  return {
    ...nagModeInfo(),
    apOnly: state.toggles.nag_ap_only,
    defaultMode: state.nagMode === 2,
    targetId: 880,
    torqueMinNm: -1.8,
    torqueMaxNm: 1.8,
    contextFreshMs: 1000,
    steeringMaxAbsDeg: 5.0,
  };
}

function nagStatsJson() {
  const c = tickCounts();
  const apBlocked = state.scenario === 'ap_block';
  const busOff = state.scenario === 'bus_off';
  const noFrames = state.scenario === 'no_frames';
  const canState = busOff ? 'bus_off' : 'running';
  return {
    rx: c.b880,
    echo: c.echo,
    txFail: busOff ? 3 : 0,
    latUs: noFrames ? 0 : 180,
    ho: apBlocked ? 0 : 1,
    torqueNm: apBlocked ? -0.23 : 0.62,
    busoffCount: busOff ? 1 : 0,
    dasHandsState: apBlocked ? 2 : 3,
    dasSourceId: noFrames ? 0 : 923,
    frames921: 0,
    frames923: c.b923,
    tecNow: busOff ? 255 : 0,
    recNow: 0,
    tecPeak: busOff ? 255 : 0,
    canState,
    uptimeS: elapsedSeconds(),
    ...nagModeInfo(),
    apOnly: state.toggles.nag_ap_only,
    apActive: !apBlocked,
    targetId: 880,
    dasApState: apBlocked ? 1 : 3,
    steerAngleDeg: apBlocked ? -0.6 : 1.4,
    steeringValid: !noFrames,
    frames297: c.b297,
    modeBPhase: apBlocked ? 0 : 6,
    modeBInjects: c.injects,
    modeBLastNm: apBlocked ? 0 : 1.05,
    modeBStateAgeMs: apBlocked ? 65535 : 420,
    modeBPhaseAgeMs: apBlocked ? 65535 : 180,
    modeBFirstEchoDelayMs: apBlocked ? 0 : (state.nagMode === 3 ? 1000 : 1),
    boSoftMode: true,
    boSoftFallback: 0,
    singleShotTx: state.toggles.singleShotTx,
    busOffStopSkip: state.toggles.busOffStopSkip,
    nagFiredNoDas: 0,
    skipApState: apBlocked ? 2500 + c.t * 100 : 0,
    echoDropLate: 0,
    nagLastDecision: apBlocked ? 9 : 1,
    nagLastDecisionText: apBlocked ? 'AP_BLOCK' : 'ECHO',
    last880AgeMs: noFrames ? 9000 : 8,
    last921AgeMs: 0,
    last923AgeMs: noFrames ? 9000 : 315,
    lastDasStatusAgeMs: noFrames ? 9000 : 315,
    last297AgeMs: noFrames ? 9000 : 7,
    lastEchoAgeMs: apBlocked ? 25000 : 8,
  };
}

function busOffLogJson() {
  if (state.scenario !== 'bus_off') return { count: 0, events: [] };
  return {
    count: 1,
    events: [
      { seq: 1, ts: Math.max(0, nowMs() - 5000), tec: 255, rec: 0, dur_ms: 0, since_ms: 0, ok: false },
    ],
  };
}

function timeseriesStatusJson() {
  return {
    rec: state.rec,
    start_ms: state.recStartMs,
    elapsed_ms: state.rec ? Math.max(0, nowMs() - state.recStartMs) : 0,
    samples: state.samples,
  };
}

function canDiagLogJson() {
  if (!state.diagStartedMs) return { state: 0, head: state.diagLogHead, complete_head: 0, lines: [] };
  const age = Date.now() - state.diagStartedMs;
  const lines = [
    'mock: A채널 MCP2515 프레임 수신 정상',
    `mock: B채널 TWAI 상태 ${state.scenario === 'bus_off' ? 'BUS-OFF' : 'RUNNING'}`,
    `mock: Nag mode ${nagModeInfo().modeStr}`,
    `mock: scenario=${state.scenario}`,
  ];
  const visible = Math.min(lines.length, Math.floor(age / 500) + 1);
  const since = state.diagLogHead;
  const nextLines = lines.slice(since, visible).map((msg, idx) => ({ msg, ts: nowMs() + idx }));
  state.diagLogHead = Math.max(state.diagLogHead, visible);
  return {
    state: visible >= lines.length ? 2 : 1,
    head: state.diagLogHead,
    complete_head: visible >= lines.length ? lines.length : state.diagLogHead,
    lines: nextLines,
  };
}

function logsBundleText() {
  const c = tickCounts();
  const obs = observerStatusJson(c, nowMs());
  return [
    '=== CanMod Mock 통합 로그 ===',
    `Generated: ${new Date().toISOString()}`,
    'Firmware: mock-1.2.0',
    `Scenario: ${state.scenario}`,
    `NagMode: ${nagModeInfo().modeStr}`,
    '',
    '=== [1] 런타임 로그 ===',
    `[mock] A RX=${c.aFrames} B RX=${c.bFrames} Echo=${c.echo}`,
    `[mock] SUMMON enabled=${state.toggles.summon_unlock_enabled ? 1 : 0} gate=${state.scenario === 'ap_block' ? 0 : 1}`,
    `[mock] NAG decision=${state.scenario === 'ap_block' ? 'AP_BLOCK' : 'ECHO'}`,
    ...obs.signals.map((sig) => `[mock] OBS ${sig.name},${sig.channel},${sig.frame_hex},raw=${sig.raw},frames=${sig.frame_count},active=${sig.active_frame_count},bursts=${sig.burst_count},run=${sig.current_run_frames}/${sig.last_run_frames}/${sig.max_run_frames}`),
    '',
    '=== [2] BUS-OFF 이벤트 로그 ===',
    state.scenario === 'bus_off' ? 'seq,timestamp_ms,tec,rec,recovered\n1,0,255,0,0' : '(BUS-OFF 없음)',
  ].join('\n');
}

async function handlePost(req, res, url) {
  if (url.pathname === '/api/ota') {
    await readBody(req);
    sendJson(res, { ok: true, mock: true });
    return;
  }

  const body = await readJsonBody(req);
  const toggleRoutes = new Map([
    ['/api/a-channel-tx', 'a_channel_tx'],
    ['/api/summon-unlock', 'summon_unlock_enabled'],
    ['/api/tsllc', 'tsllc_enabled'],
    ['/api/nag-killer', 'nag_killer'],
    ['/api/nag-ap-only', 'nag_ap_only'],
    ['/api/a-spi-8mhz', 'a_spi_8mhz'],
    ['/api/a-oneshot', 'a_mcp_oneshot'],
    ['/api/a-tx-guard', 'a_tx_guard'],
  ]);

  if (toggleRoutes.has(url.pathname)) {
    const key = toggleRoutes.get(url.pathname);
    state.toggles[key] = boolFromBody(body, state.toggles[key]);
    if ((key === 'summon_unlock_enabled' || key === 'tsllc_enabled') && state.toggles[key]) {
      state.toggles.a_channel_tx = true;
    }
    pushLog(`[mock] ${key}: ${state.toggles[key] ? 'ON' : 'OFF'}`);
    sendJson(res, { ok: true });
    return;
  }

  if (url.pathname === '/api/signal-observer/config') {
    const rawSignals = Array.isArray(body) ? body : body.signals;
    if (!Array.isArray(rawSignals) || rawSignals.length === 0) {
      sendJson(res, { ok: false, error: 'signals array required' }, 400);
      return;
    }
    try {
      const signals = rawSignals.slice(0, 10).map(normalizeObserverSignal);
      if (!observerAFilterFits(signals)) {
        sendJson(res, { ok: false, error: 'too many A-channel IDs for MCP2515 filters' }, 400);
        return;
      }
      state.observerSignals = signals;
      state.observerResetMs = nowMs();
      state.observerRuntime = true;
      state.observerFrozenElapsedMs = 0;
      pushLog(`[mock] signal_observer loaded ${signals.length} signals`);
      sendJson(res, { ok: true, count: signals.length });
    } catch (error) {
      sendJson(res, { ok: false, error: error.message }, 400);
    }
    return;
  }

  if (url.pathname === '/api/signal-observer/reset') {
    state.observerResetMs = nowMs();
    state.observerFrozenElapsedMs = 0;
    pushLog('[mock] signal_observer counters reset');
    sendJson(res, { ok: true });
    return;
  }

  if (url.pathname === '/api/signal-observer/capture') {
    const enabled = boolFromBody(body, state.observerRuntime);
    if (enabled) {
      state.observerResetMs = nowMs();
      state.observerFrozenElapsedMs = 0;
      state.observerRuntime = true;
      pushLog('[mock] signal_observer capture started');
    } else {
      state.observerFrozenElapsedMs = Math.max(0, nowMs() - state.observerResetMs);
      state.observerRuntime = false;
      pushLog('[mock] signal_observer capture stopped');
    }
    sendJson(res, { ok: true, enabled: state.observerRuntime });
    return;
  }

  if (url.pathname === '/api/set-theme') {
    state.theme = body.theme === 'light' ? 'light' : 'dark';
    sendJson(res, { ok: true, theme: state.theme });
    return;
  }

  if (url.pathname === '/api/nag-mode') {
    state.nagMode = clampNagMode(url.searchParams.get('m'));
    pushLog(`[mock] Nag 모드 -> ${nagModeInfo().modeStr}`);
    sendJson(res, nagConfigJson());
    return;
  }

  if (url.pathname === '/api/nag-update') {
    sendJson(res, nagConfigJson());
    return;
  }

  if (url.pathname === '/api/twai-ss-tx') {
    state.toggles.singleShotTx = url.searchParams.get('v') === '1';
    sendText(res, 'OK');
    return;
  }

  if (url.pathname === '/api/twai-busoff-stop') {
    state.toggles.busOffStopSkip = url.searchParams.get('v') === '1';
    sendText(res, 'OK');
    return;
  }

  if (url.pathname === '/api/busoff-cooldown') {
    const ms = Number(body.ms) || 1000;
    sendJson(res, { ok: true, ms });
    return;
  }

  if (url.pathname === '/api/timeseries/reset') {
    state.samples = 0;
    state.recStartMs = 0;
    state.rec = false;
    sendJson(res, { ok: true });
    return;
  }

  if (url.pathname === '/api/timeseries/rec') {
    state.rec = !!body.start;
    state.recStartMs = state.rec ? nowMs() : state.recStartMs;
    sendJson(res, { ok: true, ...timeseriesStatusJson() });
    return;
  }

  if (url.pathname === '/api/user-marker') {
    const requestedType = url.searchParams.get('type') || 'toggle';
    const activeBefore = state.userMarkerActive;
    let detail = state.userMarkerActive ? 2 : 1;
    if (requestedType === 'start') detail = 1;
    else if (requestedType === 'end') detail = 2;
    state.userMarkerActive = detail === 1;
    if (detail === 2 && activeBefore) state.userMarkerCount += 1;
    state.userMarkerLastMs = nowMs();
    state.userMarkerLastDetail = detail;
    const detailText = userMarkerDetailName(detail);
    pushLog(`[mock] marker ${detailText}`);
    sendJson(res, {
      ok: true,
      timestamp_ms: state.userMarkerLastMs,
      count: state.userMarkerCount,
      log_count: state.userMarkerCount,
      active: state.userMarkerActive,
      detail,
      detail_text: detailText,
    });
    return;
  }

  if (url.pathname === '/api/can-diag/start') {
    state.diagStartedMs = Date.now();
    state.diagLogHead = 0;
    sendJson(res, { ok: true, msg: 'mock CAN 진단 시작됨' });
    return;
  }

  if (url.pathname === '/api/time') {
    sendJson(res, { ok: true, ms: Number(url.searchParams.get('ms') || 0) });
    return;
  }

  if (url.pathname === '/api/reboot') {
    pushLog('[mock] reboot requested');
    sendJson(res, { ok: true, restarting: true, mock: true });
    return;
  }

  if (url.pathname === '/api/nvs-reset') {
    resetMockNvsState();
    pushLog('[mock] nvs reset requested');
    sendJson(res, { ok: true, erased: true, restarting: true, mock: true });
    return;
  }

  if (url.pathname === '/api/ota-confirm' || url.pathname === '/api/ota-recovery-confirm') {
    sendJson(res, { ok: true, mock: true });
    return;
  }

  if (url.pathname === '/api/ota-rollback' || url.pathname === '/api/ota-enter-recovery') {
    sendJson(res, { ok: true, mock: true, restarting: true });
    return;
  }

  sendJson(res, { ok: false, error: `unhandled mock POST ${url.pathname}` }, 404);
}

async function handleRequest(req, res) {
  const url = new URL(req.url || '/', `http://${req.headers.host || '127.0.0.1'}`);

  try {
    if (url.pathname === '/' || url.pathname === '/index.html') {
      const requestedScenario = url.searchParams.get('scenario');
      if (requestedScenario && scenarios.has(requestedScenario)) {
        state.scenario = requestedScenario;
        pushLog(`[mock] scenario -> ${state.scenario}`);
      }
      sendText(res, getWebUiHtml(), 200, 'text/html; charset=utf-8');
      return;
    }

    if (req.method === 'GET') {
      if (url.pathname === '/api/status') sendJson(res, statusJson(url));
      else if (url.pathname === '/api/nag-stats') sendJson(res, nagStatsJson());
      else if (url.pathname === '/api/nag-config') sendJson(res, nagConfigJson());
      else if (url.pathname === '/api/busoff-log') sendJson(res, busOffLogJson());
      else if (url.pathname === '/api/timeseries/status') sendJson(res, timeseriesStatusJson());
      else if (url.pathname === '/api/can-diag/log') sendJson(res, canDiagLogJson());
      else if (url.pathname === '/api/logs-bundle') sendText(res, logsBundleText(), 200, 'text/plain; charset=utf-8');
      else if (url.pathname === '/api/busoff-log-dl') sendText(res, 'seq,timestamp_ms,tec,rec,recovered\n', 200, 'text/csv; charset=utf-8');
      else sendJson(res, { ok: false, error: `unhandled mock GET ${url.pathname}` }, 404);
      return;
    }

    if (req.method === 'POST') {
      await handlePost(req, res, url);
      return;
    }

    if (req.method === 'DELETE' && url.pathname === '/api/busoff-log') {
      sendJson(res, { ok: true });
      return;
    }

    sendJson(res, { ok: false, error: `unhandled mock ${req.method} ${url.pathname}` }, 404);
  } catch (error) {
    sendJson(res, { ok: false, error: error.message }, 500);
  }
}

function start(port) {
  const server = http.createServer((req, res) => {
    handleRequest(req, res).catch((error) => sendJson(res, { ok: false, error: error.message }, 500));
  });

  server.on('error', (error) => {
    if (error.code === 'EADDRINUSE' && !cli.explicitPort && port < cli.port + 10) {
      start(port + 1);
      return;
    }
    console.error(error.message);
    process.exit(1);
  });

  server.listen(port, cli.host, () => {
    console.log(`TeslaCAN mock Web UI: http://${cli.host}:${port}/`);
    console.log(`Scenarios: ${Array.from(scenarios).join(', ')}`);
    console.log(`Current scenario: ${state.scenario}`);
    console.log('Example: /?scenario=ap_block');
  });

  process.on('SIGINT', () => {
    server.close(() => process.exit(0));
  });
}

function userMarkerDetailName(detail) {
  if (detail === 1) return 'USER_MARK_START';
  if (detail === 2) return 'USER_MARK_END';
  return 'NONE';
}

pushLog(`[mock] server boot scenario=${state.scenario}`);
start(cli.port);
