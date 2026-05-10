// embedded 웹 UI를 mock API로 로컬 확인하는 개발 서버
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '..');
const webUiPath = path.join(repoRoot, 'include', 'web', 'web_ui.h');

const profiles = [
  {
    smartProfile: 0,
    profileLabel: '기본',
    profileSummary: '현재 검증 기준. 700/400ms 타이밍을 유지하고 조건이 맞는 동안 연속 관찰 주입.',
    state1GraceMs: 500,
    state2DelayMs: 700,
    strongDelayMs: 400,
    strongRampMs: 500,
    state2MildMinRawDelta: 50,
    state2MildMaxRawDelta: 150,
    state2MildMinNm: 0.5,
    state2MildMaxNm: 1.5,
    state2BurstMs: 0,
    state2PauseMs: 0,
    strongBurstMs: 0,
    strongPauseMs: 0,
  },
  {
    smartProfile: 1,
    profileLabel: 'A안',
    profileSummary: '초기 grace를 줄이고 짧은 burst 후 쉬는 구간을 둔다.',
    state1GraceMs: 150,
    state2DelayMs: 700,
    strongDelayMs: 400,
    strongRampMs: 500,
    state2MildMinRawDelta: 50,
    state2MildMaxRawDelta: 150,
    state2MildMinNm: 0.5,
    state2MildMaxNm: 1.5,
    state2BurstMs: 250,
    state2PauseMs: 750,
    strongBurstMs: 500,
    strongPauseMs: 1000,
  },
  {
    smartProfile: 2,
    profileLabel: 'B안',
    profileSummary: '가장 보수적. state1 주입을 없애고 더 짧게 반응한 뒤 길게 관찰한다.',
    state1GraceMs: 0,
    state2DelayMs: 900,
    strongDelayMs: 600,
    strongRampMs: 500,
    state2MildMinRawDelta: 50,
    state2MildMaxRawDelta: 150,
    state2MildMinNm: 0.5,
    state2MildMaxNm: 1.5,
    state2BurstMs: 150,
    state2PauseMs: 1350,
    strongBurstMs: 300,
    strongPauseMs: 1700,
  },
  {
    smartProfile: 3,
    profileLabel: 'C안',
    profileSummary: '1차 delay+torque 후보. state2는 600ms로 앞당기고 mild 상한을 1.7Nm까지 올리며 strong은 400ms/2.10Nm 유지.',
    state1GraceMs: 500,
    state2DelayMs: 600,
    strongDelayMs: 400,
    strongRampMs: 500,
    state2MildMinRawDelta: 50,
    state2MildMaxRawDelta: 170,
    state2MildMinNm: 0.5,
    state2MildMaxNm: 1.7,
    state2BurstMs: 0,
    state2PauseMs: 0,
    strongBurstMs: 0,
    strongPauseMs: 0,
  },
];

const scenarios = new Set(['normal', 'ap_block', 'bus_off', 'bus_err', 'no_frames']);
const cli = parseArgs(process.argv.slice(2));

const state = {
  bootMs: Date.now(),
  scenario: scenarios.has(cli.scenario) ? cli.scenario : 'normal',
  theme: 'dark',
  smartProfile: 0,
  logHead: 0,
  logs: [],
  rec: false,
  recStartMs: 0,
  samples: 24,
  diagStartedMs: 0,
  diagLogHead: 0,
  toggles: {
    a_channel_tx: true,
    ui_ulc_stalk_confirm_enabled: true,
    ui_alc_off_highway_enable_enabled: true,
    enhanced_autopilot: true,
    tsllc_enabled: true,
    nag_killer: true,
    enable_print: true,
    a_spi_8mhz: false,
    a_mcp_oneshot: false,
    a_tx_guard: false,
    singleShotTx: false,
    busOffStopSkip: false,
  },
};

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
      console.log('Usage: node scripts/mock_webui_server.mjs [--host 127.0.0.1] [--port 8787] [--scenario normal|ap_block|bus_off|bus_err|no_frames]');
      process.exit(0);
    }
  }
  return out;
}

function getWebUiHtml() {
  const stat = fs.statSync(webUiPath);
  if (cachedHtml && stat.mtimeMs === cachedMtimeMs) return cachedHtml;

  const source = fs.readFileSync(webUiPath, 'utf8');
  const match = source.match(/const char WEB_UI_HTML\[\]\s*=\s*R"rawliteral\(([\s\S]*?)\)rawliteral";/);
  if (!match) throw new Error(`WEB_UI_HTML raw literal not found in ${webUiPath}`);
  cachedHtml = match[1];
  cachedMtimeMs = stat.mtimeMs;
  return cachedHtml;
}

function nowMs() {
  return Date.now() - state.bootMs;
}

function elapsedSeconds() {
  return Math.floor(nowMs() / 1000);
}

function profile() {
  return profiles[state.smartProfile] || profiles[0];
}

function pushLog(msg) {
  state.logHead += 1;
  state.logs.push({ head: state.logHead, ts: Date.now(), msg });
  if (state.logs.length > 100) state.logs.shift();
}

function clampProfile(value) {
  const p = Number(value);
  return p >= 0 && p <= 3 ? p : 0;
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

function statusJson(url) {
  const c = tickCounts();
  const logSince = Number(url.searchParams.get('log_since') || 0);
  const twaiStateCode = c.bBusOff ? 2 : 1;
  const fresh = !c.noFrames;
  const uptimeMs = nowMs();
  const logs = state.logs.filter((entry) => entry.head > logSince).map((entry) => ({ msg: entry.msg, ts: entry.ts }));

  return {
    fsd_enabled: true,
    isa_speed_chime_suppress: false,
    emergency_vehicle_detection: false,
    enhanced_autopilot: state.toggles.a_channel_tx && state.toggles.enhanced_autopilot,
    ui_ulc_stalk_confirm_enabled: state.toggles.a_channel_tx && state.toggles.ui_ulc_stalk_confirm_enabled,
    ui_alc_off_highway_enable_enabled: state.toggles.a_channel_tx && state.toggles.ui_alc_off_highway_enable_enabled,
    nag_killer: state.toggles.nag_killer,
    a_channel_tx: state.toggles.a_channel_tx,
    tsllc_enabled: state.toggles.a_channel_tx && state.toggles.tsllc_enabled,
    enable_print: state.toggles.enable_print,
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
    ota_confirm_remaining_ms: 0,
    ota_confirm_window_ms: 60000,
    ota_rollback_remaining_ms: 0,
    ota_rollback_window_ms: 60000,
    ota_current_label: 'mock_ota_0',
    ota_fallback_label: 'mock_ota_1',
    features: {
      isa_speed_chime_suppress: feature(false, false, false),
      emergency_vehicle_detection: feature(false, false, false),
      enhanced_autopilot: feature(state.toggles.enhanced_autopilot),
      ui_ulc_stalk_confirm: feature(state.toggles.ui_ulc_stalk_confirm_enabled),
      ui_alc_off_highway_enable: feature(state.toggles.ui_alc_off_highway_enable_enabled),
      nag_killer: feature(state.toggles.nag_killer),
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
        ulc_stalk_confirm_modified: c.noFrames ? 0 : Math.floor(c.aFrames / 7),
        ulc_stalk_confirm_skipped: c.noFrames ? 0 : Math.floor(c.aFrames / 5),
        alc_off_highway_modified: c.noFrames ? 0 : Math.floor(c.aFrames / 6),
        alc_off_highway_skipped: c.noFrames ? 0 : Math.floor(c.aFrames / 4),
        eap_modified: c.noFrames ? 0 : Math.floor(c.aFrames / 3),
        last_frame_id: c.noFrames ? 0 : 1021,
        last_update_ms: uptimeMs,
        last_loop_ms: uptimeMs,
        core_id: 0,
        spi_freq_hz: state.toggles.a_spi_8mhz ? 8000000 : 10000000,
        spi_requested_hz: state.toggles.a_spi_8mhz ? 8000000 : 10000000,
        spi_reboot_required: false,
        mcp_one_shot: state.toggles.a_mcp_oneshot,
        channel_tx_enabled: state.toggles.a_channel_tx,
        ui_ulc_stalk_confirm_enabled: state.toggles.a_channel_tx && state.toggles.ui_ulc_stalk_confirm_enabled,
        ui_alc_off_highway_enable_enabled: state.toggles.a_channel_tx && state.toggles.ui_alc_off_highway_enable_enabled,
        tx_guard_enabled: state.toggles.a_tx_guard,
        mcp_eflg: 0,
        mcp_eflg_peak: 0x40,
        mcp_txbo_count: 0,
        mcp_recovery_attempt: 0,
        mcp_recovery_success: 0,
        mcp_recovery_fail: 0,
        mcp_busoff_since_ms: 0,
        mcp_last_recovery_ms: 0,
        tx_ok: c.noFrames ? 0 : Math.floor(c.aFrames / 2),
        tx_fail: c.noFrames ? 0 : 1,
        tec: 0,
        rec: 0,
        tec_peak: 0,
        merrf: 0,
        rx_ovr: 1,
        rec_peak: 0,
        last_frame_rx_ms: uptimeMs,
        last_tx_ms: uptimeMs,
        eflg_event_count: 0,
        tx_guard_active: false,
        tx_guard_remaining_ms: 0,
        tx_guard_count: 0,
        tx_guard_skip: 0,
        tx_guard_reason: 'NONE',
        driver_ok: true,
        connected: fresh,
        fresh,
        task_alive: true,
        frame_age_ms: fresh ? 20 : 9000,
        loop_age_ms: 10,
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
        nag_mode: 1,
        smart_profile: state.smartProfile,
        echo_count: c.echo,
        echo_drop_late: 0,
        skip_runtime_or_inactive: 0,
        skip_ap_state: state.scenario === 'ap_block' ? 2500 + c.t * 100 : 0,
        skip_hands_on: 0,
        skip_das_state: 0,
        twai_state_code: twaiStateCode,
        last_frame_id: c.noFrames ? 0 : 880,
        last_update_ms: uptimeMs,
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
      },
    },
    can: {
      state: c.bBusOff ? 'BUS_OFF' : 'RUNNING',
      rx_errors: 0,
      tx_errors: c.bBusOff ? 255 : 0,
      bus_errors: c.bBusErr ? 21 : 0,
      rx_missed: 0,
      rx_queued: 0,
      frames_received: c.aFrames + c.bFrames,
      frames_received_a: c.aFrames,
      frames_received_b: c.bFrames,
      frames_sent: Math.floor(c.aFrames / 3) + c.echo,
    },
  };
}

function nagConfigJson() {
  return {
    mode: 1,
    modeStr: 'SMART',
    ...profile(),
    targetId: 880,
    hoRatePct: 100,
    torque: [
      { b2: 8, b3: 0xB6, nm: 1.8 },
      { b2: 8, b3: 0x98, nm: 1.5 },
      { b2: 7, b3: 0x6C, nm: -1.5 },
      { b2: 7, b3: 0x4E, nm: -1.8 },
    ],
  };
}

function nagStatsJson() {
  const c = tickCounts();
  const p = profile();
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
    mode: 1,
    modeStr: 'SMART',
    ...p,
    targetId: 880,
    dasApState: apBlocked ? 1 : 3,
    steerAngleDeg: apBlocked ? -0.6 : 1.4,
    frames297: c.b297,
    modeBPhase: apBlocked ? 0 : 6,
    modeBInjects: c.injects,
    modeBLastNm: apBlocked ? 0 : 1.05,
    modeBStateAgeMs: apBlocked ? 65535 : 420,
    modeBPhaseAgeMs: apBlocked ? 65535 : 180,
    modeBFirstEchoDelayMs: apBlocked ? 0 : p.state2DelayMs,
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
  if (!state.diagStartedMs) return { state: 0, head: state.diagLogHead, lines: [] };
  const age = Date.now() - state.diagStartedMs;
  const lines = [
    'mock: A채널 MCP2515 프레임 수신 정상',
    `mock: B채널 TWAI 상태 ${state.scenario === 'bus_off' ? 'BUS-OFF' : 'RUNNING'}`,
    `mock: Smart profile ${profile().profileLabel}`,
    `mock: scenario=${state.scenario}`,
  ];
  const visible = Math.min(lines.length, Math.floor(age / 500) + 1);
  const since = state.diagLogHead;
  const nextLines = lines.slice(since, visible).map((msg, idx) => ({ msg, ts: nowMs() + idx }));
  state.diagLogHead = Math.max(state.diagLogHead, visible);
  return { state: visible >= lines.length ? 2 : 1, head: state.diagLogHead, lines: nextLines };
}

function logsBundleText() {
  const c = tickCounts();
  return [
    '=== CanMod Mock 통합 로그 ===',
    `Generated: ${new Date().toISOString()}`,
    'Firmware: mock-1.2.0',
    `Scenario: ${state.scenario}`,
    `SmartProfile: ${profile().profileLabel}`,
    '',
    '=== [1] 런타임 로그 ===',
    `[mock] A RX=${c.aFrames} B RX=${c.bFrames} Echo=${c.echo}`,
    `[mock] UI_ulcStalkConfirm ON=${state.toggles.ui_ulc_stalk_confirm_enabled ? 1 : 0} modified=${c.noFrames ? 0 : Math.floor(c.aFrames / 7)} skip=${c.noFrames ? 0 : Math.floor(c.aFrames / 5)}`,
    `[mock] UI_alcOffHighwayEnable ON=${state.toggles.ui_alc_off_highway_enable_enabled ? 1 : 0} modified=${c.noFrames ? 0 : Math.floor(c.aFrames / 6)} skip=${c.noFrames ? 0 : Math.floor(c.aFrames / 4)}`,
    `[mock] NAG decision=${state.scenario === 'ap_block' ? 'AP_BLOCK' : 'ECHO'}`,
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
    ['/api/ui-ulc-stalk-confirm', 'ui_ulc_stalk_confirm_enabled'],
    ['/api/ui-alc-off-highway-enable', 'ui_alc_off_highway_enable_enabled'],
    ['/api/enhanced-autopilot', 'enhanced_autopilot'],
    ['/api/tsllc', 'tsllc_enabled'],
    ['/api/nag-killer', 'nag_killer'],
    ['/api/a-spi-8mhz', 'a_spi_8mhz'],
    ['/api/a-oneshot', 'a_mcp_oneshot'],
    ['/api/a-tx-guard', 'a_tx_guard'],
  ]);

  if (toggleRoutes.has(url.pathname)) {
    const key = toggleRoutes.get(url.pathname);
    state.toggles[key] = boolFromBody(body, state.toggles[key]);
    if ((key === 'enhanced_autopilot' || key === 'tsllc_enabled' ||
      key === 'ui_ulc_stalk_confirm_enabled' || key === 'ui_alc_off_highway_enable_enabled') && state.toggles[key]) {
      state.toggles.a_channel_tx = true;
    }
    pushLog(`[mock] ${key}: ${state.toggles[key] ? 'ON' : 'OFF'}`);
    sendJson(res, { ok: true });
    return;
  }

  if (url.pathname === '/api/set-theme') {
    state.theme = body.theme === 'light' ? 'light' : 'dark';
    sendJson(res, { ok: true, theme: state.theme });
    return;
  }

  if (url.pathname === '/api/nag-profile') {
    state.smartProfile = clampProfile(url.searchParams.get('p'));
    pushLog(`[mock] 스마트 프로파일 -> ${profile().profileLabel}`);
    sendJson(res, nagConfigJson());
    return;
  }

  if (url.pathname === '/api/nag-mode' || url.pathname === '/api/nag-update') {
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
    pushLog(`[mock] marker ${url.searchParams.get('type') || 'manual'}`);
    sendJson(res, { ok: true, timestamp_ms: nowMs() });
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

pushLog(`[mock] server boot scenario=${state.scenario}`);
start(cli.port);
