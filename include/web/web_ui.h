// include/web_ui.h
#pragma once

const char WEB_UI_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<link rel="icon" href="data:,">
<title>TeslaCAN</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{min-height:100%}

/* ── Theme variables ── */
:root,[data-theme="dark"]{
  --bg:#1a1a2e;--bg2:#0f0f23;--card:#16213e;
  --bd:#27304d;--row-sep:#1a1a2e;
  --tx:#e0e0e0;--tx2:#cfd5e6;--tx3:#7b84a3;--tx4:#6a7392;
  --acc:#00d4aa;--acc2:#3dba72;
  --err:#ff6b6b;--warn:#f5a623;
  --sw-off:#2e3755;--sw-thumb:#7a849f;
  --log-tx:#a8b2d8;--le-bd:#151528;
  --dot-off:#3e4a6a;
  --hint-bg:#0f0f23;
  --file-bg:#0f0f23;--file-tx:#cbd3f0;
  --btn-bg:#00d4aa;--btn-tx:#08111b;
  --btn-dis-bg:#2e3755;--btn-dis-tx:#7a849f;
  --emg-bg:#ef4444;--emg-bg2:#b91c1c;--emg-tx:#fff7f7;
  /* 별칭(이전 코드 호환): muted/txt/border/bad/ok */
  --muted:#7b84a3;--txt:#e0e0e0;--border:#27304d;--bad:#ff6b6b;--ok:#3dba72;
}
[data-theme="light"]{
  --bg:#f2f5fb;--bg2:#e8edf8;--card:#ffffff;
  --bd:#d4dded;--row-sep:#f2f5fb;
  --tx:#1a2035;--tx2:#2d3a5c;--tx3:#5a6890;--tx4:#8491b0;
  --acc:#0066cc;--acc2:#16a34a;
  --err:#dc2626;--warn:#c27700;
  --sw-off:#c8d0e3;--sw-thumb:#a0aec0;
  --log-tx:#374455;--le-bd:#dde4f0;
  --dot-off:#a0aec0;
  --hint-bg:#eef2ff;
  --file-bg:#eef2ff;--file-tx:#2d3a5c;
  --btn-bg:#0066cc;--btn-tx:#ffffff;
  --btn-dis-bg:#d4dded;--btn-dis-tx:#a0aec0;
  --emg-bg:#dc2626;--emg-bg2:#991b1b;--emg-tx:#ffffff;
  --muted:#5a6890;--txt:#1a2035;--border:#d4dded;--bad:#dc2626;--ok:#16a34a;
}

body{
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif;
  background:var(--bg);color:var(--tx);
  padding:16px;margin:0 auto;
  width:100%;max-width:540px;overflow-x:hidden;
  transition:background .25s,color .25s
}

/* ── Page header ── */
#page-hdr{
  display:flex;flex-direction:column;gap:8px;
  margin-bottom:16px
}
h1{font-size:1.55em;color:var(--acc);margin:0;font-weight:700}
.header-top{display:flex;align-items:flex-start;justify-content:space-between;gap:10px}
.title-wrap{min-width:0}
.title-row{display:flex;align-items:center;gap:8px;flex-wrap:wrap}
#hw-badge{
  display:inline-block;background:var(--acc);color:var(--btn-tx);
  font-size:.52em;font-weight:800;letter-spacing:.05em;
  padding:2px 8px;border-radius:6px;vertical-align:middle
}
#build-badge{
  display:inline-block;background:var(--btn-dis-bg);color:var(--tx2);
  font-size:.52em;font-weight:800;letter-spacing:0;
  padding:2px 8px;border-radius:6px;vertical-align:middle;
  max-width:190px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap
}
#ver-badge{
  color:var(--tx3);font-size:.74em;font-weight:600;
  line-height:1.25;max-width:100%;overflow-wrap:anywhere
}
#theme-btn{
  padding:6px 14px;border:1.5px solid var(--bd);border-radius:20px;
  background:var(--card);color:var(--tx3);font-size:.78em;
  cursor:pointer;font-family:inherit;font-weight:600;
  transition:all .2s;white-space:nowrap;flex-shrink:0
}
#theme-btn:hover{border-color:var(--acc);color:var(--acc)}
.header-actions{display:flex;gap:8px;align-items:center;justify-content:flex-start;flex-wrap:wrap}
#log-save-btn{
  display:inline-flex;align-items:center;justify-content:center;
  padding:6px 14px;border-radius:20px;border:none;background:#c0392b;color:#fff;
  font-size:.78em;font-weight:700;cursor:pointer;text-decoration:none;
  font-family:inherit;white-space:nowrap
}
.user-mark-wrap{display:inline-flex;align-items:center;gap:6px;white-space:nowrap}
.user-mark-btn{box-sizing:border-box;min-width:104px;padding:6px 12px;border-radius:20px;border:1.5px solid var(--bd);background:var(--card);color:var(--tx2);font-size:.78em;font-weight:800;cursor:pointer;font-family:inherit;white-space:nowrap;text-align:center;transition:background .18s,border-color .18s,color .18s,box-shadow .18s}
.user-mark-btn.active{background:#c0392b;border-color:#e74c3c;color:#fff;box-shadow:0 0 0 2px rgba(192,57,43,.22)}
.user-mark-btn:disabled{opacity:.65;cursor:wait}
.user-mark-count{font-size:.76em;color:var(--tx3);font-weight:700;min-width:26px;text-align:left}

/* ── Main navigation ── */
.main-tabs{display:grid;grid-template-columns:repeat(5,1fr);gap:8px;margin:0 0 14px}
.main-tab{min-height:42px;border:1.5px solid var(--bd);border-radius:12px;background:var(--card);color:var(--tx3);font-family:inherit;font-size:.86em;font-weight:800;cursor:pointer;transition:background .18s,border-color .18s,color .18s}
.main-tab.active{background:var(--btn-dis-bg);border-color:var(--tx4);color:var(--tx)}
.view{display:none}.view.active{display:block}

/* ── OTA sticky 배너 ── */
.ota-top-banner{
  position:sticky;top:0;z-index:999;
  margin:-16px -16px 12px -16px;
  padding:14px 16px;
  border-bottom:2px solid;
  border-radius:0;
}
.ota-top-banner.ota-confirm{background:#0d2318;border-color:var(--acc2)}
.ota-top-banner.ota-rollback{background:#0d1a2e;border-color:var(--warn)}
.ota-top-banner-title{font-size:1em;font-weight:700;margin-bottom:5px}
.ota-top-banner.ota-confirm .ota-top-banner-title{color:var(--acc2)}
.ota-top-banner.ota-rollback .ota-top-banner-title{color:var(--warn)}
.ota-top-banner-sub{font-size:.82em;color:var(--tx2);margin-bottom:3px}
.ota-top-banner-meta{font-size:.77em;color:var(--tx3);margin-bottom:4px}
.ota-top-banner-countdown{font-size:.9em;color:var(--warn);font-weight:700;margin-bottom:10px}
.ota-top-banner-btns{display:flex;gap:8px}
.ota-top-banner-btns .btn{margin-top:0;flex:1;padding:10px 8px;font-size:.88em}

/* ── Card ── */
.card{
  background:var(--card);border-radius:12px;
  padding:16px;margin-bottom:12px;
  box-shadow:0 2px 12px rgba(0,0,0,.09);
  transition:background .25s
}
.card h2{
  font-size:.88em;color:var(--tx3);font-weight:700;
  margin-bottom:10px;text-transform:uppercase;letter-spacing:.09em
}

/* ── Row ── */
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:9px 0}
.row+.row{border-top:1px solid var(--row-sep)}
.child-row{padding-left:16px}
.child2-row{padding-left:28px}
.labelWrap{display:flex;flex:1 1 auto;flex-direction:column;gap:2px;min-width:0}
.label{color:var(--tx2);font-size:1.0em;line-height:1.3;overflow-wrap:anywhere}
.meta{color:var(--tx3);font-size:.76em;line-height:1.35}
.val{flex:0 0 auto;font-weight:700;font-size:1.1em;text-align:right}
.on{color:var(--acc2)}.off{color:var(--dot-off)}

/* ── Toggle switch ── */
.sw{position:relative;flex:0 0 auto;width:48px;height:26px}
.sw input{opacity:0;width:0;height:0}
.sl{position:absolute;cursor:pointer;inset:0;background:var(--sw-off);border-radius:26px;transition:.22s ease}
.sl:before{content:"";position:absolute;height:20px;width:20px;left:3px;bottom:3px;
  background:var(--sw-thumb);border-radius:50%;transition:.22s ease}
input:checked+.sl{background:var(--acc)}
input:checked+.sl:before{transform:translateX(22px);background:#fff}
input:disabled+.sl{opacity:.45;cursor:not-allowed}

/* ── Dot ── */
.dot{width:8px;height:8px;border-radius:50%;display:inline-block}
.dot.on{background:var(--acc2);box-shadow:0 0 6px var(--acc2)}
.dot.off{background:var(--dot-off)}

/* ── Connection error ── */
.err{color:var(--err);text-align:center;font-size:.8em;padding:8px;display:none}

/* ── Channel state pill ── */
.ch-pill{display:inline-block;padding:2px 9px;border-radius:10px;font-size:.9em;font-weight:700}
.mux-dot{display:inline-flex;align-items:center;justify-content:center;width:28px;height:28px;
  border-radius:50%;font-size:.78em;font-weight:700;border:2px solid var(--bd);
  background:var(--dot-off);color:var(--tx3);transition:background .3s,color .3s}
.mux-dot.learned{background:#00d4aa;color:#08111b;border-color:#00d4aa}
.mux-dot.armed{background:#f5a623;color:#08111b;border-color:#f5a623}
.ch-ok{background:rgba(61,186,114,.12);color:#3dba72}
.ch-err{background:rgba(255,107,107,.12);color:#ff6b6b}
.ch-warn{background:rgba(245,166,35,.12);color:#f5a623}
.ch-dim{background:rgba(255,255,255,.06);color:var(--tx4)}

/* ── OTA ── */
.btn{
  width:100%;margin-top:10px;padding:12px 14px;
  border:0;border-radius:10px;
  background:var(--btn-bg);color:var(--btn-tx);
  font-weight:700;font-size:.95em;cursor:pointer;font-family:inherit;
  transition:opacity .18s
}
.btn:hover{opacity:.88}
.btn:disabled{background:var(--btn-dis-bg)!important;color:var(--btn-dis-tx)!important;cursor:not-allowed;opacity:1}
.file{
  width:100%;padding:12px;border:1.5px solid var(--bd);
  border-radius:10px;background:var(--file-bg);color:var(--file-tx);
  font-family:inherit;transition:border .2s
}
.hint{
  color:var(--tx3);font-size:.76em;line-height:1.5;
  background:var(--hint-bg);border-radius:8px;
  padding:10px 12px;margin-bottom:10px
}
#otaStatus{margin-top:10px;font-size:.8em;color:var(--log-tx);min-height:1.2em}

/* ── Log ── */
#log{
  background:var(--bg2);border-radius:8px;padding:10px;
  height:200px;overflow-y:auto;
  font-family:"SF Mono",Monaco,Consolas,monospace;
  font-size:.75em;color:var(--log-tx);
  transition:background .25s
}
.le{padding:3px 0;border-bottom:1px solid var(--le-bd);overflow-wrap:anywhere}
.ts{color:var(--tx4);margin-right:6px}

/* ── CAN Sniffer ── */
.ui-select{
  flex:0 0 140px;background:var(--bg2);border:1.5px solid var(--bd);border-radius:8px;
  padding:7px 10px;color:var(--tx);font-size:12px;font-family:inherit;
}
.ui-select:disabled{opacity:.45;cursor:not-allowed}
.s-ts{color:var(--tx4);font-size:10px;padding-top:1px}
.s-id{color:var(--acc);font-weight:700}
.s-data{color:var(--log-tx);word-break:break-all}
.s-name{color:var(--acc2);font-size:10px;margin-top:2px;font-weight:600}

.btn-row{display:flex;gap:8px}
.btn-action{
  flex:1;padding:10px 8px;border:1.5px solid var(--bd);border-radius:9px;
  background:transparent;font-family:inherit;font-size:12px;font-weight:600;
  cursor:pointer;color:var(--tx3);transition:all .18s;
  text-align:center;text-decoration:none;display:block
}
.btn-action:hover{border-color:var(--acc);color:var(--acc)}
.btn-action.rec-active{border-color:var(--err)!important;color:var(--err)!important}
.btn-dl{border-color:var(--acc2)!important;color:var(--acc2)!important}
.btn-dl:hover{background:var(--acc2)!important;color:#fff!important;border-color:var(--acc2)!important}
/* Nag v2 소형 버튼 */
.btn-sm{font:inherit;cursor:pointer;background:var(--card);color:var(--txt);border:1px solid var(--bd);border-radius:6px;padding:5px 10px;font-size:12px}
.btn-sm.primary{background:var(--acc);color:#0d1117;border-color:transparent;font-weight:600}
.btn-sm.danger{color:var(--err);border-color:#3a1f23}
.btn-sm:hover{filter:brightness(1.15)}
.mode-toggle{display:flex;align-items:center;gap:8px;flex-wrap:wrap;margin-bottom:10px}
.mode-label{font-size:12px;font-weight:700;color:var(--muted)}
.mode-toggle input{display:none}
.mode-track{position:relative;width:164px;height:34px;border-radius:999px;background:var(--bg2);border:1.5px solid var(--bd);cursor:pointer;display:grid;grid-template-columns:1fr 1fr;align-items:center;text-align:center;overflow:hidden}
.mode-track:before{content:"";position:absolute;top:3px;bottom:3px;left:3px;width:calc(50% - 3px);border-radius:999px;background:var(--acc);transition:transform .2s ease}
.mode-track span{position:relative;z-index:1;font-size:12px;font-weight:800;color:var(--tx3)}
.mode-track span:first-child{color:var(--btn-tx)}
.mode-toggle input:checked+.mode-track:before{transform:translateX(100%)}
.mode-toggle input:checked+.mode-track span:first-child{color:var(--tx3)}
.mode-toggle input:checked+.mode-track span:last-child{color:var(--btn-tx)}
.profile-mode-toggle{align-items:stretch;gap:6px}
.profile-current{font-size:12px;font-weight:700;color:var(--muted)}
.profile-seg{display:grid;grid-template-columns:repeat(5,1fr);gap:4px;min-width:0;max-width:520px;flex:1}
.profile-seg label{display:block}
.profile-seg input{display:none}
.profile-seg span{display:block;text-align:center;border:1.5px solid var(--bd);border-radius:6px;background:var(--bg2);padding:8px 6px;font-size:12px;font-weight:800;color:var(--tx3);cursor:pointer}
.profile-seg input:checked+span{background:var(--acc);border-color:transparent;color:var(--btn-tx)}
.profile-desc{font-size:11px;line-height:1.6;margin-bottom:8px;padding:5px 7px;background:rgba(0,0,0,.12);border-radius:5px;color:var(--muted)}
.experiment-seg{display:grid;grid-template-columns:repeat(auto-fit,minmax(82px,1fr));gap:5px;width:100%}
.experiment-seg label{display:block}
.experiment-seg input{display:none}
.experiment-seg span{display:block;text-align:center;border:1.5px solid var(--bd);border-radius:6px;background:var(--bg2);padding:8px 5px;font-size:11px;font-weight:800;color:var(--tx3);cursor:pointer;min-height:32px}
.experiment-seg input:checked+span{background:var(--acc);border-color:transparent;color:var(--btn-tx)}
.experiment-current{font-size:11px;color:var(--muted);line-height:1.45;margin-top:5px}
.observer-head{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:10px}
.observer-head h2{margin-bottom:0}
.observer-file-input{display:none}
.observer-file-btn{width:auto;min-width:92px;padding:9px 12px}
.observer-actions{display:grid;grid-template-columns:repeat(auto-fit,minmax(104px,1fr));gap:8px;margin-bottom:8px;align-items:stretch}
.observer-actions .btn-action{padding:9px 8px}
.observer-actions .btn-action:disabled{opacity:.48;cursor:not-allowed;border-color:var(--bd);color:var(--tx4)}
.observer-table-wrap{overflow:auto;max-height:360px;border:1px solid var(--bd);border-radius:8px;background:var(--bg2);scroll-behavior:smooth}
.observer-table{width:100%;min-width:850px;border-collapse:collapse;font-size:11px;font-variant-numeric:tabular-nums}
.observer-table th,.observer-table td{padding:7px 8px;border-bottom:1px solid var(--row-sep);text-align:left;white-space:nowrap}
.observer-table th{color:var(--tx3);font-size:10px;text-transform:uppercase;letter-spacing:.04em;background:rgba(0,0,0,.08)}
.observer-table td{color:var(--tx2)}
.observer-table tr:last-child td{border-bottom:0}
.observer-table .obs-name{font-weight:800;color:var(--tx);max-width:210px;overflow:hidden;text-overflow:ellipsis}
.observer-table .obs-active{color:var(--acc2);font-weight:800}
.observer-table .obs-idle{color:var(--tx4)}
.observer-status{min-height:16px;margin-bottom:8px}
/* Nag v2 stat 박스 */
.stat{background:var(--bg);border:1px solid var(--bd);border-radius:6px;padding:6px 8px}
.stat .k{font-size:10px;text-transform:uppercase;letter-spacing:.05em;color:var(--muted)}
.stat .v{font-size:15px;font-weight:600;margin-top:2px}

.btn-emergency{
  width:100%;padding:12px 14px;border:1px solid rgba(255,255,255,.2);border-radius:10px;
  background:linear-gradient(135deg,var(--emg-bg),var(--emg-bg2));color:var(--emg-tx);
  font-family:inherit;font-weight:800;font-size:.95em;letter-spacing:.01em;cursor:pointer;
  box-shadow:0 0 0 1px rgba(255,255,255,.08) inset,0 8px 20px rgba(185,28,28,.35);
  transition:transform .12s ease,filter .15s ease,box-shadow .15s ease;
}
.btn-emergency:hover{filter:brightness(1.07);transform:translateY(-1px);box-shadow:0 0 0 1px rgba(255,255,255,.15) inset,0 10px 24px rgba(185,28,28,.42)}
.btn-emergency:active{transform:translateY(0)}
.btn-emergency:disabled{opacity:.6;cursor:not-allowed;transform:none}

.btn-restore{
  width:100%;padding:12px 14px;border:1px solid rgba(255,255,255,.2);border-radius:10px;
  background:linear-gradient(135deg,var(--acc2),#1a8a4a);color:#fff;
  font-family:inherit;font-weight:800;font-size:.95em;letter-spacing:.01em;cursor:pointer;
  box-shadow:0 0 0 1px rgba(255,255,255,.08) inset,0 8px 20px rgba(22,163,74,.35);
  transition:transform .12s ease,filter .15s ease,box-shadow .15s ease;
}
.btn-restore:hover{filter:brightness(1.07);transform:translateY(-1px);box-shadow:0 0 0 1px rgba(255,255,255,.15) inset,0 10px 24px rgba(22,163,74,.42)}
.btn-restore:active{transform:translateY(0)}
.btn-restore:disabled{opacity:.6;cursor:not-allowed;transform:none}

@media(max-width:380px){
  body{padding:12px}.card{padding:14px}
  .label{font-size:.95em}.meta{font-size:.72em}
}

/* ── Collapsible cards ── */
.card>h2{display:flex;align-items:center}
.collapse-btn{
  background:none;border:1px solid var(--bd);border-radius:6px;
  color:var(--tx3);font-size:10px;padding:2px 6px;
  cursor:pointer;margin-left:auto;flex-shrink:0;
  font-family:inherit;line-height:1.4;transition:all .15s
}
.collapse-btn:hover{border-color:var(--acc);color:var(--acc)}
.card.collapsed>*:not(h2){display:none}
.card.collapsed>h2{margin-bottom:0}
/* ── Status strip ── */
#hdr-status{display:flex;align-items:center;gap:10px;flex-wrap:wrap;font-size:.82em;color:var(--tx3);margin-bottom:10px}
.chan-status{display:inline-flex;align-items:center;gap:5px;white-space:nowrap}
.sdot{width:7px;height:7px;border-radius:50%;flex-shrink:0;transition:background .4s,box-shadow .4s}
.dot-live,.dot-ok{background:var(--acc2);box-shadow:0 0 7px var(--acc2)}
.dot-warn{background:var(--warn);box-shadow:0 0 7px var(--warn)}
.dot-err{background:var(--err);box-shadow:0 0 7px var(--err)}
.dot-wait{background:var(--dot-off)}
/* ── FPS bar ── */
.fps-bars{display:flex;flex-direction:column;gap:4px;margin-bottom:14px}
.fps-row{display:flex;align-items:center;gap:6px}
.fps-lbl{font-size:9px;color:var(--tx4);letter-spacing:.5px;width:14px;text-align:right;flex-shrink:0}
.fps-bar{flex:1;height:3px;background:var(--bd);border-radius:2px;overflow:hidden}
.fps-fill{height:100%;border-radius:2px;transition:width .5s;width:0%}
.fps-fill-a{background:var(--acc2)}
.fps-fill-b{background:var(--acc)}
/* ── Stat grid ── */
.stat-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px;margin-bottom:12px}
.stat{background:var(--card);border-radius:10px;padding:10px 12px;min-width:0}
.stat-lbl{font-size:10px;color:var(--tx3);letter-spacing:.04em;margin-bottom:4px;line-height:1.25;min-height:2.5em;display:flex;align-items:flex-end}
.stat-val{font-size:14px;font-weight:600;color:var(--tx);line-height:1.25;overflow-wrap:anywhere}
.stat.primary .stat-val{font-size:17px;font-weight:800}
.main-stat-grid .stat{min-height:82px;text-align:center;display:flex;flex-direction:column;align-items:center;justify-content:center}
.main-stat-grid .stat-lbl{min-height:0;align-items:center;justify-content:center;font-size:11px;margin-bottom:7px;text-align:center}
.main-stat-grid .stat-val{font-size:19px;font-weight:900;text-align:center;font-variant-numeric:tabular-nums}
.main-stat-grid .stat-val.long{font-size:16px}.main-stat-grid .stat-val.xlong{font-size:14px}
.stat.subtle{background:var(--bg2)}
.main-guidance{border:1px solid var(--bd);border-radius:10px;background:var(--bg2);padding:11px 12px;margin:0 0 12px;min-width:0}
.main-guidance .k{font-size:10px;color:var(--tx3);letter-spacing:.05em;margin-bottom:5px;font-weight:700;text-transform:uppercase}
.main-guidance .m1{font-size:14px;font-weight:800;line-height:1.35;color:var(--tx);overflow-wrap:anywhere}
.main-guidance .m2{margin-top:4px;font-size:11px;line-height:1.45;color:var(--tx3);overflow-wrap:anywhere}
.main-guidance.ok .m1{color:var(--acc2)}
.main-guidance.warn .m1{color:var(--warn)}
.main-guidance.err .m1{color:var(--err)}
.main-guidance.dim .m1{color:var(--tx2)}
.guidance-state-row{display:flex;flex-wrap:wrap;gap:5px;margin-top:8px}
.guidance-chip{font-size:10px;font-weight:700;padding:3px 7px;border-radius:999px;border:1px solid var(--bd);background:var(--card);color:var(--tx3);line-height:1.3;white-space:nowrap}
.guidance-chip.active{border-color:var(--acc);background:rgba(0,212,170,.16);color:var(--tx)}
.guidance-chip.active.warn{border-color:var(--warn);background:rgba(245,166,35,.2)}
.guidance-chip.active.err{border-color:var(--err);background:rgba(239,68,68,.2)}
.guidance-actions{display:flex;justify-content:flex-end;margin-top:8px}
.guidance-detail-btn{width:auto;padding:6px 10px;border-radius:8px;border:1px solid var(--bd);background:var(--card);color:var(--tx2);font-size:11px;font-weight:700;cursor:pointer}
.guidance-detail-btn:hover{border-color:var(--acc);color:var(--acc)}
.guidance-detail-page{position:fixed;inset:0;background:rgba(0,0,0,.52);display:none;z-index:1200;align-items:flex-end;justify-content:center;padding:12px}
.guidance-detail-page.open{display:flex}
.guidance-detail-sheet{width:100%;max-width:520px;max-height:85vh;overflow:auto;background:var(--card);border:1px solid var(--bd);border-radius:14px;padding:14px}
.guidance-detail-head{display:flex;align-items:center;justify-content:space-between;gap:8px;margin-bottom:10px}
.guidance-detail-head h3{margin:0;font-size:14px;color:var(--tx)}
.guidance-close-btn{width:auto;padding:6px 10px;border-radius:8px;border:1px solid var(--bd);background:var(--bg2);color:var(--tx3);font-size:11px;font-weight:700;cursor:pointer}
.guidance-close-btn:hover{border-color:var(--acc);color:var(--acc)}
.guidance-detail-note{font-size:11px;line-height:1.45;color:var(--tx3);margin-bottom:10px}
.guidance-detail-list{display:grid;grid-template-columns:1fr;gap:6px}
.guidance-detail-item{border:1px solid var(--bd);border-radius:10px;background:var(--bg2);padding:8px 9px}
.guidance-detail-item .t{font-size:11px;font-weight:800;color:var(--tx)}
.guidance-detail-item .d{font-size:10px;line-height:1.4;color:var(--tx3);margin-top:3px}
.control-card>.row{border:1px solid var(--bd);border-radius:12px;padding:13px 14px;margin-bottom:10px;background:var(--bg2)}
.control-card>.row+.row{border-top:1px solid var(--bd)}
.control-card>.row.child-row{padding-left:14px}
.control-card>.row.parent-row{background:transparent;border-style:dashed;padding:10px 14px}
.control-card>.row .label{font-size:1.04em;font-weight:800;color:var(--tx)}
.control-card>.row .meta{font-size:.82em}
.profile-mini-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(92px,1fr));gap:5px;margin-bottom:6px}
.profile-mini-grid .stat{padding:8px 9px}
.profile-mini-grid .k{font-size:11px;color:var(--tx3);line-height:1.2;margin-bottom:4px}
.profile-mini-grid .v{font-size:14px;font-weight:900;line-height:1.35;color:var(--acc)}
.profile-mini-grid .burst-stat .k{font-size:10px}
.profile-mini-grid .burst-stat .v{font-size:12px}
.burst-lines{display:flex;flex-direction:column;gap:2px;white-space:normal}
.diag-id-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:6px;margin:6px 0 8px}
.diag-id-cell{border:1px solid var(--bd);border-radius:8px;background:var(--bg2);padding:7px 8px;min-width:0}
.diag-id-cell .k{font-size:10px;color:var(--tx3);line-height:1.2;margin-bottom:3px}
.diag-id-cell .v{font-size:13px;font-weight:800;color:var(--tx);overflow-wrap:anywhere}
.busoff-card table{min-width:520px}
.ota-status-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;margin-bottom:10px}
.ota-actions{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.ota-actions .btn-nvs-reset{grid-column:1/-1;background:#7f1d1d;color:#fff}
@media(max-width:520px){.diag-id-grid{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media(max-width:380px){.main-tabs{gap:6px}.main-tab{font-size:.8em;min-height:38px}.ota-actions{grid-template-columns:1fr}}
@media(max-width:520px){
  .header-actions{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
  #log-save-btn,.user-mark-wrap{width:100%;justify-content:center}
  .observer-actions{grid-template-columns:repeat(2,minmax(0,1fr))}
}
@media(max-width:420px){
  .observer-head{gap:8px}
  .observer-file-btn{padding:8px 10px;min-width:84px;font-size:11px}
}
/* ── 진단 화면: 항상 펼친 채널별 상태판 ─────────────────────── */
.diag-card{background:var(--card);border:1px solid var(--bd);border-radius:10px;padding:12px;margin-bottom:12px}
.diag-head{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:10px}
.diag-head h2{margin:0;font-size:1.05em;color:var(--tx)}
.diag-signal-grid{display:grid;grid-template-columns:1fr;gap:10px}
.diag-signal-group{border:1px solid var(--bd);border-radius:10px;background:var(--bg2);padding:10px;min-width:0}
.diag-channel-grid{display:grid;grid-template-columns:1fr;gap:12px;align-items:start}
.diag-title{font-weight:700;color:var(--tx)}
.diag-sum{font-size:11px;color:var(--tx3);margin-left:auto;text-align:right}
.diag-sum.warn{color:var(--warn);font-weight:600}
.diag-sum.err{color:var(--err);font-weight:700}
.diag-sum.ok{color:var(--acc2)}
.diag-row{display:flex;flex-wrap:wrap;align-items:center;gap:6px;margin-top:8px}
.diag-lbl{font-size:10px;color:var(--tx3);text-transform:uppercase;letter-spacing:.8px;min-width:42px}
.diag-help{font-size:11px;color:var(--tx3);line-height:1.55;margin-top:8px}
.diag-help b{color:var(--tx2)}
@media(max-width:760px){.diag-signal-grid,.diag-channel-grid{grid-template-columns:1fr}}
.chip{display:inline-block;font-size:11px;font-weight:600;padding:3px 8px;border-radius:12px;border:1px solid transparent;line-height:1.4;white-space:nowrap;font-family:monospace}
.chip.ok{background:rgba(76,175,80,.12);color:var(--acc2);border-color:rgba(76,175,80,.3)}
.chip.warn{background:rgba(230,126,34,.15);color:var(--warn);border-color:rgba(230,126,34,.4)}
.chip.err{background:rgba(244,67,54,.18);color:var(--err);border-color:rgba(244,67,54,.5);animation:chipPulse 1.5s infinite}
@keyframes chipPulse{50%{opacity:.55}}
.v-ok{color:var(--acc2)}.v-err{color:var(--err)}.v-acc{color:var(--acc)}.v-dim{color:var(--tx3)}.v-warn{color:var(--warn)}
</style>
</head>
<body>

<!-- OTA 확인 배너 (pending=2: 웹 OTA 신 FW 부팅, 1분 확인 창) - 페이지 최상단 sticky -->
<div id="otaConfirmBanner" class="ota-top-banner ota-confirm" style="display:none">
  <div class="ota-top-banner-title">&#x26A1; 펌웨어 업데이트 확인</div>
  <div class="ota-top-banner-sub">새 펌웨어가 설치되었습니다. 계속 사용하시겠습니까?</div>
  <div class="ota-top-banner-meta">현재: <span id="otaCurLabel">--</span> / 이전: <span id="otaFbLabel">--</span></div>
  <div class="ota-top-banner-countdown">남은 시간: <span id="otaConfirmCountdown">--</span> (초과 시 자동 롤백)</div>
  <div class="ota-top-banner-btns">
    <button class="btn" onclick="otaConfirmFw()" style="background:var(--acc2)">&#x2705; 이 펌웨어 사용</button>
    <button class="btn" onclick="otaRollbackFw()" style="background:var(--err)">&#x21A9; 이전으로 복구</button>
  </div>
</div>
<!-- OTA 복구완료 확인 배너 (pending=4) - 페이지 최상단 sticky -->
<div id="otaRollbackConfirmBanner" class="ota-top-banner ota-rollback" style="display:none">
  <div class="ota-top-banner-title">&#x1F504; 펌웨어 복구 완료</div>
  <div class="ota-top-banner-sub">이전 펌웨어로 복구되었습니다. 정상 동작을 확인해주세요.</div>
  <div class="ota-top-banner-countdown">남은 시간: <span id="otaRollbackCountdown">--</span> (초과 시 OTA 복구모드 진입)</div>
  <div class="ota-top-banner-btns">
    <button class="btn" onclick="otaRecoveryConfirmFw()" style="background:var(--acc2)">&#x2705; 복구 완료 확인</button>
    <button class="btn" onclick="otaEnterRecoveryFw()" style="background:var(--err)">&#x26A0; 복구모드 진입</button>
  </div>
</div>
<div id="page-hdr">
  <div class="header-top">
    <div class="title-wrap">
      <div class="title-row">
        <h1>TeslaCAN</h1>
        <span id="hw-badge" style="display:none"></span>
        <span id="build-badge" style="display:none"></span>
      </div>
      <span id="ver-badge" style="display:none"></span>
    </div>
    <button id="theme-btn" onclick="toggleTheme()">&#9728; Light</button>
  </div>
  <div class="header-actions">
    <a id="log-save-btn" href="/api/logs-bundle" data-download-url="/api/logs-bundle" download="canmod_logs.txt" onclick="return handleDownloadAction(event,this)">&#128190; 전체 저장</a>
    <span class="user-mark-wrap">
      <button id="userMarkBtn" class="user-mark-btn" onclick="markApWarning()">USER_MARK</button>
      <span id="userMarkCount" class="user-mark-count">0회</span>
    </span>
  </div>
</div>
<div class="main-tabs" role="tablist" aria-label="TeslaCAN 화면 선택">
  <button class="main-tab active" id="tab-main" role="tab" aria-selected="true" onclick="showView('main')">메인</button>
  <button class="main-tab" id="tab-control" role="tab" aria-selected="false" onclick="showView('control')">제어</button>
  <button class="main-tab" id="tab-diag" role="tab" aria-selected="false" onclick="showView('diag')">진단</button>
  <button class="main-tab" id="tab-ota" role="tab" aria-selected="false" onclick="showView('ota')">OTA</button>
  <button class="main-tab" id="tab-experiment" role="tab" aria-selected="false" onclick="showView('experiment')">실험</button>
</div>
<div id="connErr" class="err">Connection lost. Retrying...</div>

<section class="view active" id="view-main" role="tabpanel" aria-labelledby="tab-main">
<div id="hdr-status">
  <span class="chan-status"><span class="sdot dot-wait" id="sdot-a"></span><span id="hdr-a">A WAIT</span></span>
  <span class="chan-status"><span class="sdot dot-wait" id="sdot-b"></span><span id="hdr-b">B WAIT</span></span>
</div>
<div class="fps-bars">
  <div class="fps-row"><span class="fps-lbl">A</span><div class="fps-bar"><div class="fps-fill fps-fill-a" id="fps-fill-a"></div></div></div>
  <div class="fps-row"><span class="fps-lbl">B</span><div class="fps-bar"><div class="fps-fill fps-fill-b" id="fps-fill-b"></div></div></div>
</div>
<div class="stat-grid main-stat-grid">
  <div class="stat primary" title="A채널 MCP2515 상태와 수신 속도"><div class="stat-lbl">A 채널</div><div class="stat-val v-dim" id="s-main-a">--</div></div>
  <div class="stat primary" title="B채널 TWAI 상태와 880 수신 속도"><div class="stat-lbl">B 채널</div><div class="stat-val v-dim" id="s-main-b">--</div></div>
  <div class="stat" title="B채널 BUS-OFF 누적 발생 횟수. 0 유지가 정상"><div class="stat-lbl">BUS-OFF</div><div class="stat-val v-dim" id="s-main-busoff">0</div></div>
  <div class="stat primary" title="DAS_autopilotState. 값 뒤 @923은 수신 소스"><div class="stat-lbl">오토파일럿 상태</div><div class="stat-val v-dim" id="s-main-ap">--</div></div>
  <div class="stat primary" title="실제 핸들 토크 추정값. nag-stats torqueNm"><div class="stat-lbl">현재 핸들토크</div><div class="stat-val v-dim" id="s-main-torque">--</div></div>
  <div class="stat primary" title="SCCM 조향각도. ID 297 기준"><div class="stat-lbl">조향각</div><div class="stat-val v-dim" id="s-main-angle">--</div></div>
  <div class="stat" title="Smart Torque 상태 페이즈"><div class="stat-lbl">Smart Torque</div><div class="stat-val v-dim" id="s-main-phase">--</div></div>
  <div class="stat" title="Mode B 누적 토크 주입 횟수"><div class="stat-lbl">주입 횟수</div><div class="stat-val v-acc" id="s-main-inject">0</div></div>
  <div class="stat" title="현재 Smart Torque 프로파일"><div class="stat-lbl">프로파일</div><div class="stat-val v-dim" id="s-main-profile">--</div></div>
</div>
<div class="main-guidance dim" id="mainGuidanceCard">
  <div class="k">운전자 안내</div>
  <div class="m1" id="mainGuidancePrimary">상태 수신 대기 중입니다.</div>
  <div class="m2" id="mainGuidanceSecondary">주행 데이터를 확인 중입니다.</div>
  <div class="guidance-state-row" id="mainGuidanceStateRow">
    <span class="guidance-chip" id="gState-NONE">NONE</span>
    <span class="guidance-chip" id="gState-OFF">OFF</span>
    <span class="guidance-chip" id="gState-AP_BLOCK">AP_BLOCK</span>
    <span class="guidance-chip" id="gState-HANDS_ON">HANDS_ON</span>
    <span class="guidance-chip" id="gState-DAS_IDLE">DAS_IDLE</span>
    <span class="guidance-chip" id="gState-NO_880">NO_880</span>
    <span class="guidance-chip" id="gState-NO_921">NO_921</span>
    <span class="guidance-chip" id="gState-NO_ECHO">NO_ECHO</span>
    <span class="guidance-chip" id="gState-LATE_DROP">LATE_DROP</span>
    <span class="guidance-chip" id="gState-ECHO">ECHO</span>
  </div>
  <div class="guidance-actions">
    <button class="guidance-detail-btn" type="button" onclick="openGuidanceDetailPage()">상세보기</button>
  </div>
</div>
<div class="guidance-detail-page" id="guidanceDetailPage" onclick="closeGuidanceDetailPage(event)">
  <div class="guidance-detail-sheet" onclick="event.stopPropagation()">
    <div class="guidance-detail-head">
      <h3>Nag Killer 상태 상세</h3>
      <button class="guidance-close-btn" type="button" onclick="closeGuidanceDetailPage()">닫기</button>
    </div>
    <div class="guidance-detail-note">활성 상태는 메인 카드와 동일하게 강조됩니다.</div>
    <div class="guidance-state-row" id="guidanceDetailStateRow">
      <span class="guidance-chip" id="gDetailState-NONE">NONE</span>
      <span class="guidance-chip" id="gDetailState-OFF">OFF</span>
      <span class="guidance-chip" id="gDetailState-AP_BLOCK">AP_BLOCK</span>
      <span class="guidance-chip" id="gDetailState-HANDS_ON">HANDS_ON</span>
      <span class="guidance-chip" id="gDetailState-DAS_IDLE">DAS_IDLE</span>
      <span class="guidance-chip" id="gDetailState-NO_880">NO_880</span>
      <span class="guidance-chip" id="gDetailState-NO_921">NO_921</span>
      <span class="guidance-chip" id="gDetailState-NO_ECHO">NO_ECHO</span>
      <span class="guidance-chip" id="gDetailState-LATE_DROP">LATE_DROP</span>
      <span class="guidance-chip" id="gDetailState-ECHO">ECHO</span>
    </div>
    <div class="guidance-detail-list" style="margin-top:10px">
      <div class="guidance-detail-item"><div class="t">NONE</div><div class="d">초기 상태 또는 판정 정보 없음.</div></div>
      <div class="guidance-detail-item"><div class="t">OFF</div><div class="d">Nag Killer 런타임이 꺼져 있어 주입하지 않음.</div></div>
      <div class="guidance-detail-item"><div class="t">AP_BLOCK</div><div class="d">AP 상태가 3~6 구간이 아니어서 주입 차단.</div></div>
      <div class="guidance-detail-item"><div class="t">HANDS_ON</div><div class="d">실제 핸들 손 감지가 있어 주입 중단.</div></div>
      <div class="guidance-detail-item"><div class="t">DAS_IDLE</div><div class="d">DAS 경고 조건이 없어 주입 대기.</div></div>
      <div class="guidance-detail-item"><div class="t">NO_880</div><div class="d">NAG 타겟 880 프레임 미수신.</div></div>
      <div class="guidance-detail-item"><div class="t">NO_921</div><div class="d">DAS 상태 프레임 미수신.</div></div>
      <div class="guidance-detail-item"><div class="t">NO_ECHO</div><div class="d">조건이 아직 미충족이라 이번 구간 주입 없음.</div></div>
      <div class="guidance-detail-item"><div class="t">LATE_DROP</div><div class="d">주입 타이밍 창을 놓쳐 구간 드롭.</div></div>
      <div class="guidance-detail-item"><div class="t">ECHO</div><div class="d">가상 토크 echo 주입 중.</div></div>
    </div>
  </div>
</div>
<div class="card no-collapse">
  <h2>메인 상태</h2>
  <div class="row"><span class="label">Enhanced Autopilot</span><span class="val" id="eap">--</span></div>
  <div class="row"><span class="label">Autosteer Nag Killer</span><span class="val" id="nag">--</span></div>
  <div class="row"><span class="label">Uptime</span><span class="val" id="up">--</span></div>
  <div class="row"><button class="btn-emergency" id="emStopBtn" onclick="emergencyToggle()">즉시 기능해제</button></div>
</div>
</section>

<section class="view" id="view-control" role="tabpanel" aria-labelledby="tab-control">
<div class="card control-card no-collapse">
  <h2>제어</h2>
  <div class="row">
    <div class="labelWrap">
      <span class="label">Enhanced Autopilot</span>
      <span class="meta" id="metaEap">--</span>
    </div>
    <label class="sw"><input type="checkbox" id="tEap" onchange="togSwitch('/api/enhanced-autopilot',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">TSLLC (stop signs/lights)</span>
      <span class="meta" id="metaTsllc">Enable FSD stops control (stop signs/lights). TSLLC proven.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tTsllc" onchange="togSwitch('/api/tsllc',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">UI_ulcStalkConfirm</span>
      <span class="meta" id="metaUlcStalkConfirm">고속도로 NoA시 자동차선 변경 허용(스톡 컨펌완화). ON이면 bit1=0으로 유지.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tUlcStalkConfirm" onchange="togSwitch('/api/ui-ulc-stalk-confirm',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">UI_alcOffHighwayEnable</span>
      <span class="meta" id="metaAlcOffHighway">비고속도로 자동차선변경 허용. ON이면 bit56=1로 유지.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tAlcOffHighway" onchange="togSwitch('/api/ui-alc-off-highway-enable',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">Autosteer Nag Killer <span style="font-size:10px;color:var(--muted)">[B채널 Target ID / bus4]</span></span>
      <span class="meta" id="metaNag">--</span>
    </div>
    <label class="sw"><input type="checkbox" id="tNag" onchange="togSwitch('/api/nag-killer',this)"><span class="sl"></span></label>
  </div>
  <!-- Nag Killer 실시간 통계 + 스마트 프로파일 선택 -->
  <div class="row child-row" id="nagModePanel">
    <div style="width:100%">
      <div style="text-align:center;margin-bottom:6px">
        <span style="font-size:12px;color:var(--muted)">현재 프로파일 &gt;&gt;&nbsp;</span><span style="font-size:14px;font-weight:bold;color:var(--acc2)" id="nagProfileLabel">[--]</span>
      </div>
      <div class="profile-seg" role="radiogroup" aria-label="Nag Killer Smart Torque profile" style="margin-bottom:8px">
        <label><input type="radio" name="nagProfile" value="0" onchange="setNagProfile(0)"><span>기본</span></label>
        <label><input type="radio" name="nagProfile" value="1" onchange="setNagProfile(1)"><span>A안</span></label>
        <label><input type="radio" name="nagProfile" value="2" onchange="setNagProfile(2)"><span>B안</span></label>
        <label><input type="radio" name="nagProfile" value="3" onchange="setNagProfile(3)"><span>C안</span></label>
        <label><input type="radio" name="nagProfile" value="4" onchange="setNagProfile(4)"><span>D안</span></label>
      </div>
      <div id="nagProfileDesc" class="profile-desc">
        --
      </div>
      <div id="nagDescB" style="font-size:11px;line-height:1.6;margin-bottom:8px;padding:5px 7px;background:rgba(0,0,0,.12);border-radius:5px">
        <div style="color:var(--muted);margin-bottom:6px"><b>Smart Torque</b>: AP state 3-6 게이트 + HandsOnState별 조건부 토크 주입. 조건 불충족 시 버스 비개입.</div>
        <div class="profile-mini-grid">
          <div class="stat" title="HandsOnState=1 진입 후 기존 값을 유지하는 시간"><div class="k">state1 grace</div><div class="v" id="np_s1">--</div></div>
          <div class="stat" title="HandsOnState=2 진입 후 mild 주입 전 대기 시간"><div class="k">state2 delay</div><div class="v" id="np_s2">--</div></div>
          <div class="stat" title="HandsOnState=2 mild random-walk 토크 범위"><div class="k">state2 mild</div><div class="v" id="np_mild">--</div></div>
          <div class="stat" title="HandsOnState=3-5 진입 후 strong 주입 전 대기 시간"><div class="k">strong delay</div><div class="v" id="np_s3">--</div></div>
          <div class="stat burst-stat" title="burst/pause. 0/0이면 연속"><div class="k">burst / pause</div><div class="v" id="np_burst">--</div></div>
        </div>
        <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(100px,1fr));gap:5px;margin-top:6px">
          <div class="stat" title="DAS_autopilotState (ID 921/923 후보). 값 뒤 @923은 921이 아니라 923에서 읽은 상태"><div class="k">AP state</div><div class="v" id="ns_apst">--</div></div>
          <div class="stat" title="SCCM 조향각도 (ID 297). 방향 결정에 사용"><div class="k">steer angle</div><div class="v" id="ns_angle">--</div></div>
          <div class="stat" title="주입 페이즈: 0=idle 1=grace 2=delay 3=mild 4=sDelay 5=ramp 6=hold"><div class="k">phase</div><div class="v" id="ns_mbphase">--</div></div>
          <div class="stat" title="Mode B 누적 토크 주입 횟수"><div class="k">주입 횟수</div><div class="v" id="ns_mbinject">0</div></div>
          <div class="stat" title="Mode B 가장 최근 주입 토크 (Nm). 양수=우, 음수=좌"><div class="k">마지막 Nm</div><div class="v" id="ns_mbnm">--</div></div>
          <div class="stat" title="ID 297 SCCM 수신 프레임 수"><div class="k">rx (297)</div><div class="v" id="ns_rx297">0</div></div>
        </div>
      </div>
      <!-- 공통 실시간 Nag Stats -->
      <div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(110px,1fr));gap:6px;margin-bottom:6px">
        <div class="stat" title="차량이 보낸 880 프레임 수. ✅증가=차량 연결 정상"><div class="k">rx (880)</div><div class="v" id="ns_rx">0</div></div>
        <div class="stat" title="나그킬러가 발사한 에코 수. ✅증가=동작 중"><div class="k">echo sent</div><div class="v" id="ns_echo">0</div></div>
        <div class="stat" title="드라이버 레벨 TX 실패 누적. ❌증가=나쁨(큐 포화/BUS-OFF)"><div class="k">tx fail</div><div class="v" id="ns_fail">0</div></div>
        <div class="stat" title="TWAI 드라이버 상태. ✅running=정상 ❌bus_off=나쁨"><div class="k">can state</div><div class="v" id="ns_cs">--</div></div>
        <div class="stat" title="ID 921/923 DAS 핸즈온 판정. ✅1=스티어링감지(성공) ✅0/8=정상 ❌2=나그감지 ⚠️0xFF=미수신"><div class="k">DAS 921/923</div><div class="v" id="ns_das">--</div></div>
        <div class="stat" title="TX 에러 카운터 현재/피크. ✅0=최상 ⚠️≥96=경고 ❌≥128=에러패시브"><div class="k">TEC now/peak</div><div class="v" id="ns_tec">0 / 0</div></div>
      </div>
    </div>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">Enable Log</span>
      <span class="meta">Serial and web log polling</span>
    </div>
    <label class="sw"><input type="checkbox" id="tLog" checked onchange="togLog(this)"><span class="sl"></span></label>
  </div>
</div>
</section>

<section class="view" id="view-diag" role="tabpanel" aria-labelledby="tab-diag">

<!-- ═══ 진단 신호 패널 (옵션 B): A/B 채널 진단 카운터를 chip 형태로 압축 ═══
     ┌─ A: TX OK/Fail · TEC · MERRF · RX-OVR
     └─ B: ARB-LOST · BUS-ERR · TX-FAIL · RX-MISS
     0 = 녹색(ok), >0 = 주황(warn), 임계값 초과 = 빨강(err) -->
<div class="card diag-card no-collapse" id="diag-card">
  <div class="diag-head">
    <h2>🔬 진단 신호</h2>
    <span class="diag-sum" id="diag-sum">분석 대기…</span>
  </div>
  <div class="diag-signal-grid">
    <div class="diag-signal-group">
      <div class="diag-lbl">A채널</div>
      <div class="diag-row">
        <span class="chip ok" id="d-a-tx"     title="TX 결과 OK/Fail. Fail↑ → 송신 큐 포화 또는 하드 실패">TX 0/0</span>
        <span class="chip ok" id="d-a-tec"    title="TEC 현재/피크. ≥96 경고, ≥128 에러패시브">TEC 0/0</span>
        <span class="chip ok" id="d-a-merrf"  title="MERRF 누적. ↑ = ACK 부재 또는 동일 ID 충돌">MERRF 0</span>
        <span class="chip ok" id="d-a-rxovr"  title="RX 버퍼 오버런 누적. clear 후 재발 시 폴링 부족">RX-OVR 0</span>
        <span class="chip ok" id="d-a-rec"    title="REC 현재/피크. ↑ = 수신 에러(배선/종단 문제)">REC 0/0</span>
        <span class="chip ok" id="d-a-eflgev" title="EFLG 0→비제로 전환 횟수. 에러 발생 이벤트 빈도">EFLG-EV 0</span>
      </div>
      <div class="diag-help"><b>TX</b>: 송신 성공/실패 · <b>TEC/REC</b>: CAN 에러 카운터 · <b>MERRF/RX-OVR</b>: MCP2515 오류/수신 오버런 · <b>EFLG-EV</b>: EFLG 비정상 전환 횟수</div>
    </div>
    <div class="diag-signal-group">
      <div class="diag-lbl">B채널</div>
      <div class="diag-row">
        <span class="chip ok" id="d-b-arb"    title="Arbitration Lost. ↑ = 동일 ID 충돌">ARB 0</span>
        <span class="chip ok" id="d-b-err"    title="Bus Error 누적. ↑ = 배선/ACK 부재">BUS-ERR 0</span>
        <span class="chip ok" id="d-b-txf"    title="TX-FAILED 누적. Single-shot 충돌 회피 동작 검증">TX-FAIL 0</span>
        <span class="chip ok" id="d-b-rxm"    title="RX-MISSED 누적. ↑ = nagKillerTask 폴링 부족">RX-MISS 0</span>
      </div>
      <div class="diag-help"><b>ARB</b>: 중재 손실 · <b>BUS-ERR</b>: 버스 에러 누적 · <b>TX-FAIL</b>: TWAI 송신 실패 · <b>RX-MISS</b>: 수신 큐 누락</div>
    </div>
  </div>
</div>

<div class="diag-channel-grid">
<div class="card no-collapse">
  <h2><span id="aChTitle">&#x1F535; A 채널 (MCP2515)</span></h2>
  <div class="row"><span class="label">수신 속도</span><span class="val" id="aHz">-- Hz</span></div>
  <div class="row"><span class="label">ID 1016 Frame</span><span class="meta" style="font-size:11px">실제 CAN에서 들어오는 프레임</span></div>
  <div class="row"><span class="label">└ ULC Stalk Confirm 주입</span><span class="val" id="aUlcStalkConfirmMod">0</span></div>
  <div class="row"><div class="labelWrap"><span class="label">└ ULC Stalk Confirm 스킵</span><span class="meta">ON 상태에서 이미 bit1=0이라 송신하지 않음.</span></div><span class="val" id="aUlcStalkConfirmSkip">0</span></div>
  <div class="row"><span class="label">└ ALC Off Highway Enable 주입</span><span class="val" id="aAlcOffHighwayMod">0</span></div>
  <div class="row"><div class="labelWrap"><span class="label">└ ALC Off Highway Enable 스킵</span><span class="meta">ON 상태에서 이미 bit56=1이라 송신하지 않음.</span></div><span class="val" id="aAlcOffHighwaySkip">0</span></div>
  <div class="row"><span class="label">ID 1021 Frame</span><span class="meta" style="font-size:11px">실제 CAN에서 들어오는 프레임</span></div>
  <div class="row"><span class="label">└ EAP 주입 (누적)</span><span class="val" id="aEapMod">0</span></div>
  <div class="row"><span class="label">ID 1016 수신</span><span class="val" id="a1016Period">--</span></div>
  <div class="row"><span class="label">ID 1021 수신</span><span class="val" id="a1021Period">--</span></div>
  <div class="row"><span class="label">TX 마스터</span><span class="val" id="aTxMaster">--</span></div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">A 채널 TX 마스터</span>
      <span class="meta">OFF면 A채널 수정 프레임(1021/1016/293/3E9) 송신을 중단합니다.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tAChannelTx" onchange="togSwitch('/api/a-channel-tx',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">

      <span class="label">MCP2515 SPI 8MHz</span>
      <span class="meta" id="metaASpi8">ON=8MHz, OFF=10MHz · 재부팅 후 적용</span>
    </div>
    <label class="sw"><input type="checkbox" id="tASpi8" onchange="togSwitch('/api/a-spi-8mhz',this,'A채널 MCP2515 SPI 설정을 저장합니다. 적용하려면 재부팅이 필요합니다.')"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">MCP2515 One-Shot</span>
      <span class="meta" id="metaAOneShot">재전송 억제 모드</span>
    </div>
    <label class="sw"><input type="checkbox" id="tAOneShot" onchange="togSwitch('/api/a-oneshot',this,'A채널 MCP2515 모드를 변경합니다. 정차 상태에서만 바꾸는 것을 권장합니다.')"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">A TX Guard</span>
      <span class="meta" id="metaATxGuard">TEC/EFLG 상승 시 1021 주입 일시 보류</span>
    </div>
    <label class="sw"><input type="checkbox" id="tATxGuard" onchange="togSwitch('/api/a-tx-guard',this)"><span class="sl"></span></label>
  </div>
  <div class="row"><span class="label">자동 캡처 진행</span><span class="val" id="aAutoCap">-</span></div>
</div>

<div class="card no-collapse" id="bChCard">
  <h2 id="bChHdr"><span id="bChTitle">&#x1F534; B 채널 (TWAI)</span></h2>
  <div class="row"><span class="label">TWAI 상태</span><span class="val" id="bTwai">--</span></div>
  <div class="row"><span class="label">드라이버 오류 (install/start)</span><span class="val" id="bDrvErr">--</span></div>
  <div class="diag-id-grid" title="B채널 감시 ID별 수신 속도">
    <div class="diag-id-cell"><div class="k">ID 880</div><div class="v" id="bHz880">--</div></div>
    <div class="diag-id-cell"><div class="k">ID 921</div><div class="v" id="bHz921">--</div></div>
    <div class="diag-id-cell"><div class="k">ID 923</div><div class="v" id="bHz923">--</div></div>
    <div class="diag-id-cell"><div class="k">ID 297</div><div class="v" id="bHz297">--</div></div>
  </div>
  <div class="row"><span class="label" id="bTargetLbl">ID 880 (NAG)</span><span class="val" id="b880Info">0 / --</span></div>
  <div class="row"><span class="label">ID 921 (DAS)</span><span class="val" id="b921Info">0 / --</span></div>
  <div class="row"><span class="label">ID 923 (DAS 후보)</span><span class="val" id="b923Info">0 / --</span></div>
  <div class="row"><span class="label">ID 297 (SCCM)</span><span class="val" id="b297Info">0 / --</span></div>
  <div class="row"><span class="label">토크 모드</span><span class="val" id="bMode">--</span></div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">B 채널 TX 마스터</span>
      <span class="meta">Nag Killer 송신 마스터입니다. OFF면 B채널 echo 주입이 중단됩니다.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tBNagMaster" onchange="togSwitch('/api/nag-killer',this)"><span class="sl"></span></label>
  </div>
  <div class="row"><span class="label">에코 전송 (누적)</span><span class="val" id="bEcho">0</span></div>
  <div class="row"><span class="label">BUS-OFF 발생</span><span class="val" id="bBusOff">0</span></div>
  <div class="row"><span class="label">복구 (시도/성공/실패)</span><span class="val" id="bRecStat">0 / 0 / 0</span></div>
  <div class="row"><span class="label">최근 복구 소요</span><span class="val" id="bRecMs">-- ms</span></div>
  <div class="row"><span class="label">오류 피크 (RX/TX)</span><span class="val" id="bErrPeak">0 / 0</span></div>

  <div class="row" style="display:none;border-top:1px solid var(--bd);margin-top:6px;padding-top:8px">
    <div class="labelWrap">
      <span class="label">&#9201; BUS-OFF 쿨다운 (ms)</span>
      <span class="meta">300~10000ms, 기본 1000ms. 재부팅 후에도 유지됨</span>
    </div>
    <div style="display:flex;gap:6px;align-items:center">
      <input type="number" id="boCoolMs" min="300" max="10000" step="100" value="1000"
             style="width:80px;padding:4px 6px;background:var(--bg2);color:var(--tx);border:1px solid var(--bd);border-radius:4px;font-size:13px">
      <button onclick="applyBusCooldown()" style="padding:4px 10px;font-size:12px;background:var(--acc);color:#fff;border:none;border-radius:4px;cursor:pointer">적용</button>
    </div>
  </div>
  <div class="row" style="border-top:1px solid var(--bd);margin-top:6px;padding-top:8px">
    <div class="labelWrap">
      <span class="label">&#128260; BUS-OFF 복구 모드</span>
      <span class="meta" id="metaBoMode">Soft recovery 우선, 실패 시 Hard reinstall fallback</span>
      <span class="meta" id="metaBoModeCurrent">현재: Soft + Hard fallback (고정)</span>
      <span class="meta" id="boFallbackInfo"></span>
    </div>
    <span class="chip ok">FIXED</span>
  </div>
  <!-- [v4.4 실험 토글] Single Shot TX -->
  <div class="row" style="display:none;border-top:1px solid var(--bd);margin-top:4px;padding-top:6px"
       title="ON: echo TX 충돌 시 자동 재전송 금지(TEC 억제 기대) | OFF: 기본(TWAI 자동 재전송)">
    <div class="labelWrap">
      <span class="label">&#9889; Single Shot TX</span>
      <span class="meta">ON=충돌 시 재전송 금지(TEC억제 실험) | OFF=기본(자동재전송)</span>
      <span class="meta" id="metaSsTxCurrent">현재: OFF(기본)</span>
    </div>
    <label class="sw"><input type="checkbox" id="tSsTx" onchange="toggleSsTx(this)"><span class="sl"></span></label>
  </div>
  <!-- [v4.4 실험 토글] BUS-OFF stop skip -->
  <div class="row" style="display:none;border-top:1px solid var(--bd);margin-top:4px;padding-top:6px"
       title="ON: BUS-OFF 상태에서 twai_stop() 생략(v4.4 state machine 준수) | OFF: 기본">
    <div class="labelWrap">
      <span class="label">&#128683; BUS-OFF stop skip</span>
      <span class="meta">ON=BUS-OFF 시 stop() 생략(v4.4준수 실험) | OFF=기본</span>
      <span class="meta" id="metaBoStopCurrent">현재: OFF(기본)</span>
    </div>
    <label class="sw"><input type="checkbox" id="tBoStop" onchange="toggleBoStop(this)"><span class="sl"></span></label>
  </div>
</div>
</div>
<div class="card no-collapse busoff-card">
  <h2>BUS-OFF 이벤트</h2>
  <div class="row" style="justify-content:center;gap:8px;margin-top:0;flex-wrap:wrap">
    <button onclick="loadBusOffLog()" style="padding:5px 12px;font-size:12px;background:var(--acc);color:#fff;border:none;border-radius:4px;cursor:pointer">&#8635; 새로고침</button>
    <button onclick="clearBusOffLog()" style="padding:5px 12px;font-size:12px;background:#c0392b;color:#fff;border:none;border-radius:4px;cursor:pointer">&#128465; 로그 클리어</button>
    <a href="/api/busoff-log-dl" download="busoff_log.csv"
       style="padding:5px 12px;font-size:12px;background:var(--bg2);color:var(--tx);border:1px solid var(--bd);border-radius:4px;text-decoration:none">&#128229; CSV 다운로드</a>
    <span id="boLogMeta" style="font-size:11px;color:var(--tx4);align-self:center"></span>
  </div>
  <div style="overflow-x:auto">
    <table style="width:100%;border-collapse:collapse;font-size:12px">
      <thead>
        <tr style="background:var(--bg2);color:var(--tx4)">
          <th style="padding:4px 8px;text-align:left">#</th>
          <th style="padding:4px 8px;text-align:left">시각(s)</th>
          <th style="padding:4px 8px;text-align:left">TEC</th>
          <th style="padding:4px 8px;text-align:left">REC</th>
          <th style="padding:4px 8px;text-align:left">복구(ms)</th>
          <th style="padding:4px 8px;text-align:left">간격(s)</th>
          <th style="padding:4px 8px;text-align:left">결과</th>
        </tr>
      </thead>
      <tbody id="boLogBody"></tbody>
      </table>
    </div>
</div>
<!-- CAN 자가 진단 패널 -->
<div class="card no-collapse" id="diagCard">
  <h2>&#x1F52C; CAN 통신 자가 진단</h2>
  <div style="font-size:12px;color:var(--muted);margin-bottom:10px;line-height:1.6">
    버스 트래픽 &middot; 에코 동작 &middot; TEC/REC &middot; BUS-OFF 이력을 순서대로 체크합니다.
    차량 현장에서 컴퓨터 없이 이 페이지만으로 통신 이슈를 진단할 수 있습니다 (~19초 소요).
  </div>
  <div style="display:flex;gap:8px;align-items:center;margin-bottom:8px;flex-wrap:wrap">
    <button class="btn" id="diagBtn" onclick="startCanDiag()">CAN 진단</button>
    <button class="btn" onclick="resetTimeseries()">🗑 로그 초기화</button>
    <button class="btn" id="recBtn" onclick="toggleTimeseriesRec()">⏺ 기록 시작</button>
    <button class="btn user-mark-btn" id="markBtn" onclick="markApWarning()">USER_MARK</button>
    <a class="btn" href="/api/logs-bundle" data-download-url="/api/logs-bundle" download="canmod_logs.txt" onclick="return handleDownloadAction(event,this)" style="text-decoration:none;display:inline-flex;align-items:center">💾 전체 저장</a>
    <span id="recStatus" style="font-size:12px;color:var(--muted)"></span>
    <span id="diagStatus" style="font-size:12px;color:var(--muted)"></span>
  </div>
  <div id="diagLogWrap" style="display:none">
    <div id="diagLogEl" style="background:#0d1117;border:1px solid var(--border);border-radius:4px;padding:10px;max-height:340px;overflow-y:auto;font-family:monospace;font-size:11px;line-height:1.7;white-space:pre-wrap;color:#c9d1d9"></div>
  </div>
</div>

<div id="log"></div>
</section>

<section class="view" id="view-ota" role="tabpanel" aria-labelledby="tab-ota">
<div class="card no-collapse" id="otaCard">
  <h2>OTA 업데이트</h2>
  <div class="ota-status-grid">
    <div class="stat"><div class="stat-lbl">현재 파티션</div><div class="stat-val v-dim" id="otaPanelCurrent">--</div></div>
    <div class="stat"><div class="stat-lbl">이전 파티션</div><div class="stat-val v-dim" id="otaPanelFallback">--</div></div>
    <div class="stat"><div class="stat-lbl">현재 펌웨어</div><div class="stat-val v-dim" id="otaPanelCurrentFw">--</div></div>
    <div class="stat"><div class="stat-lbl">이전 펌웨어</div><div class="stat-val v-dim" id="otaPanelFallbackFw">--</div></div>
    <div class="stat"><div class="stat-lbl">현재 빌드 시각</div><div class="stat-val v-dim" id="otaPanelCurrentBuilt">--</div></div>
    <div class="stat"><div class="stat-lbl">이전 빌드 시각</div><div class="stat-val v-dim" id="otaPanelFallbackBuilt">--</div></div>
    <div class="stat"><div class="stat-lbl">업로드 시각</div><div class="stat-val v-dim" id="otaPanelUploadAt">--</div></div>
    <div class="stat"><div class="stat-lbl">확인 상태</div><div class="stat-val v-dim" id="otaPanelState">--</div></div>
    <div class="stat"><div class="stat-lbl">복구 모드</div><div class="stat-val v-dim" id="otaPanelRecovery">--</div></div>
  </div>
  <input class="file" type="file" id="otaFile" accept=".bin">
  <progress id="otaProgress" value="0" max="100" style="width:100%;height:8px;margin-top:10px;display:block"></progress>
  <div class="ota-actions">
    <button class="btn" id="otaBtn" onclick="uploadOta()">펌웨어 업로드</button>
    <button class="btn" onclick="rebootDevice()" style="background:var(--btn-dis-bg);color:var(--tx2)">보드 재부팅</button>
    <button class="btn btn-nvs-reset" id="nvsResetBtn" onclick="resetNvsAll()">모든 NVS 초기화</button>
  </div>
  <div id="otaStatus"></div>
</div>
</section>

<section class="view" id="view-experiment" role="tabpanel" aria-labelledby="tab-experiment">
<div class="card no-collapse">
  <h2>A채널 실험</h2>
  <div class="hint">차량 UI가 보내는 AP 보조 설정 비트를 복사 프레임에 덮어쓰는 실험 탭입니다. 기본값은 순정 유지이며, ON 또는 선택값을 고르면 A TX가 자동으로 켜집니다.</div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">UI_autoLaneChangeEnable</span>
      <span class="meta">ID 0x293 bit24-25. ON이면 자동 차선변경 비활성값(0)을 유지합니다.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tAutoLaneChange" onchange="togSwitch('/api/ui-auto-lane-change-enable',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">UI_autoTurnSignalMode</span>
      <span class="meta">ID 0x3FD bit52. ON이면 방향지시등 자동 취소 모드(1)를 약 50ms 펄스로 보냅니다.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tAutoTurnSignalMode" onchange="togSwitch('/api/ui-auto-turn-signal-mode',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">DAS_ulcConfirmationRequestActive</span>
      <span class="meta">ID 0x3E9 bit28. ON이면 ULC 확인 요청 활성값(1)을 세션 중 유지합니다.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tDasUlcConfirm" onchange="togSwitch('/api/das-ulc-confirmation-request',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">UI_ulcOffHighway</span>
      <span class="meta">ID 0x3F8 bit15. ON이면 ULC 고속도로 외 허용 비트(1)를 유지합니다.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tUlcOffHighway" onchange="togSwitch('/api/ui-ulc-off-highway',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">UI_alcOffHighwayEnable</span>
      <span class="meta">ID 0x3F8 bit56. 이미 구현된 비고속도로 ALC 허용 토글을 이 탭에도 연결합니다.</span>
    </div>
    <label class="sw"><input type="checkbox" id="tExpAlcOffHighway" onchange="togSwitch('/api/ui-alc-off-highway-enable',this)"><span class="sl"></span></label>
  </div>
  <div class="row" style="align-items:flex-start">
    <div class="labelWrap">
      <span class="label">UI_ulcSpeedConfig</span>
      <span class="meta">ID 0x3F8 bit50-51. ULC 속도 회복/가속 성향으로 추정됩니다.</span>
    </div>
  </div>
  <div class="row child-row" style="display:block">
    <div class="experiment-seg" role="radiogroup" aria-label="UI_ulcSpeedConfig">
      <label><input type="radio" name="ulcSpeedConfig" value="255" onchange="setExperimentValue('/api/ui-ulc-speed-config',255)"><span>순정</span></label>
      <label><input type="radio" name="ulcSpeedConfig" value="0" onchange="setExperimentValue('/api/ui-ulc-speed-config',0)"><span>DISABLED</span></label>
      <label><input type="radio" name="ulcSpeedConfig" value="1" onchange="setExperimentValue('/api/ui-ulc-speed-config',1)"><span>MILD</span></label>
      <label><input type="radio" name="ulcSpeedConfig" value="2" onchange="setExperimentValue('/api/ui-ulc-speed-config',2)"><span>AVERAGE</span></label>
      <label><input type="radio" name="ulcSpeedConfig" value="3" onchange="setExperimentValue('/api/ui-ulc-speed-config',3)"><span>MAD_MAX</span></label>
    </div>
    <div class="experiment-current" id="expSpeedCurrent">현재: --</div>
  </div>
  <div class="row" style="align-items:flex-start">
    <div class="labelWrap">
      <span class="label">UI_ulcBlindSpotConfig</span>
      <span class="meta">ID 0x3F8 bit52-53. 차선 변경 전 사각지대 차단 강도로 추정됩니다.</span>
    </div>
  </div>
  <div class="row child-row" style="display:block">
    <div class="experiment-seg" role="radiogroup" aria-label="UI_ulcBlindSpotConfig">
      <label><input type="radio" name="ulcBlindSpotConfig" value="255" onchange="setExperimentValue('/api/ui-ulc-blind-spot-config',255)"><span>순정</span></label>
      <label><input type="radio" name="ulcBlindSpotConfig" value="0" onchange="setExperimentValue('/api/ui-ulc-blind-spot-config',0)"><span>STANDARD</span></label>
      <label><input type="radio" name="ulcBlindSpotConfig" value="1" onchange="setExperimentValue('/api/ui-ulc-blind-spot-config',1)"><span>AGGRESSIVE</span></label>
      <label><input type="radio" name="ulcBlindSpotConfig" value="2" onchange="setExperimentValue('/api/ui-ulc-blind-spot-config',2)"><span>MAD_MAX</span></label>
    </div>
    <div class="experiment-current" id="expBlindCurrent">현재: --</div>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">실험 주입 카운터</span>
      <span class="meta" id="expInjectCounters">0x293/0x3F8/0x3E9 수신 대기 중</span>
    </div>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">실험 판정</span>
      <span class="meta" id="expJudgeLine">수신/주입 상태 대기 중</span>
    </div>
  </div>
</div>
<div class="card no-collapse">
  <div class="observer-head">
    <h2>신호 관찰기</h2>
    <button class="btn-action observer-file-btn" type="button" onclick="openSignalObserverFile()">파일 선택</button>
  </div>
  <div class="hint">프레임을 보내지 않고 수신값만 집계합니다. 관찰 채널은 A 또는 B 단독만 허용합니다. A채널은 기본 4개 ID(0x293/0x3F8/0x3FD/0x3E9)를 포함해 최대 6개 ID까지만 관찰합니다.</div>
  <input class="file observer-file-input" type="file" id="signalObserverFile" accept=".json,application/json" onchange="uploadSignalObserverConfig(this)">
  <div class="observer-actions">
    <button class="btn-action" onclick="resetSignalObserver()">카운터 리셋</button>
    <button class="btn-action" id="signalObserverStartBtn" onclick="setSignalObserverCapture(true)">시작</button>
    <button class="btn-action rec-active" id="signalObserverStopBtn" onclick="setSignalObserverCapture(false)">정지</button>
    <a class="btn-action btn-dl" href="/api/events-bundle" data-download-url="/api/events-bundle" download="canmod_events.txt" onclick="return handleDownloadAction(event,this)">이벤트 묶음 저장</a>
  </div>
  <div class="experiment-current observer-status" id="signalObserverStatus">기본 프리셋 대기 중</div>
  <div class="observer-table-wrap">
    <table class="observer-table" aria-label="신호 관찰기">
      <thead><tr><th>Signal</th><th>Ch</th><th>ID</th><th>Bit</th><th>Raw</th><th>Frames</th><th>Active</th><th>Change</th><th>Δ/s</th><th>Burst</th><th>Run</th><th>Age</th></tr></thead>
      <tbody id="signalObserverBody"><tr><td colspan="12" class="obs-idle">수신 대기 중</td></tr></tbody>
    </table>
  </div>
</div>
</section>
<script>
var logSince=0,errCount=0;
var baseUptimeMs=0,baseUptimeTs=0,detailPollingStarted=false;
var _activeView='main';
var _otaUploadInProgress=false;
var _otaReloadTimer=null;
var _backgroundNetworkPaused=false;
var _logsBundleResumeTimer=null;
var LOGS_BUNDLE_PAUSE_MS=180000;
var EVENTS_BUNDLE_PAUSE_MS=90000;
var DEFAULT_DOWNLOAD_PAUSE_MS=45000;
function isBackgroundNetworkPaused(){return _backgroundNetworkPaused;}
function pauseBackgroundNetwork(){_backgroundNetworkPaused=true;}
function resumeBackgroundNetwork(){_backgroundNetworkPaused=false;}
function appendQueryParam(url,key,value){
  return url+(url.indexOf('?')>=0?'&':'?')+key+'='+encodeURIComponent(value);
}
function scheduleLogsBundleNetworkResume(pauseMs){
  if(_logsBundleResumeTimer)clearTimeout(_logsBundleResumeTimer);
  _logsBundleResumeTimer=setTimeout(function(){
    _logsBundleResumeTimer=null;
    resumeBackgroundNetwork();
    poll();
    tickNagStats();
  },pauseMs||DEFAULT_DOWNLOAD_PAUSE_MS);
}
function handleDownloadAction(ev,el){
  if(!el&&ev&&ev.currentTarget)el=ev.currentTarget;
  if(!el)return true;
  var base=el.getAttribute('data-download-url')||el.getAttribute('href')||'/api/logs-bundle';
  var pauseMs=base.indexOf('/api/logs-bundle')===0?LOGS_BUNDLE_PAUSE_MS:(base.indexOf('/api/events-bundle')===0?EVENTS_BUNDLE_PAUSE_MS:DEFAULT_DOWNLOAD_PAUSE_MS);
  pauseBackgroundNetwork();
  scheduleLogsBundleNetworkResume(pauseMs);
  el.setAttribute('href',appendQueryParam(base,'dl',Date.now()));
  return true;
}
function showView(name){
  var views={main:1,control:1,diag:1,ota:1,experiment:1};
  if(!views[name])name='main';
  _activeView=name;
  document.querySelectorAll('.view').forEach(function(el){el.classList.toggle('active',el.id==='view-'+name);});
  document.querySelectorAll('.main-tab').forEach(function(el){
    var on=el.id==='tab-'+name;
    el.classList.toggle('active',on);
    el.setAttribute('aria-selected',on?'true':'false');
  });
  try{localStorage.setItem('tcan-view',name);}catch(e){}
  if(name==='diag')loadBusOffLog();
}
function fmt(s){
  var h=Math.floor(s/3600),m=Math.floor((s%3600)/60),sec=s%60;
  return h+':'+(m<10?'0':'')+m+':'+(sec<10?'0':'')+sec;
}
function setDot(el,on){
  if(!el)return;
  el.textContent='';
  var d=document.createElement('span');
  d.className='dot '+(on?'on':'off');
  el.appendChild(d);
  el.appendChild(document.createTextNode(' '+(on?'Active':'Off')));
  el.className='val '+(on?'on':'off');
}
async function togSwitch(path,el,confirmMsg){
  if(el.disabled)return;
  if(confirmMsg&&el.checked&&!confirm(confirmMsg)){el.checked=!el.checked;return;}
  try{
    var r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:el.checked})});
    if(!r.ok)throw new Error('toggle');
  }catch(e){el.checked=!el.checked;}
}
function setRadioValue(name,value){
  var items=document.querySelectorAll('input[name="'+name+'"]');
  for(var i=0;i<items.length;i++)items[i].checked=String(items[i].value)===String(value);
}
async function setExperimentValue(path,value){
  try{
    var r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({value:value})});
    if(!r.ok)throw new Error('value');
  }catch(e){poll();}
}
function htmlEscape(s){return String(s===undefined?'':s).replace(/[&<>"']/g,function(c){return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c];});}
var _signalObserverPrev={};
function signalObserverKey(s,i){return i+'|'+(s.name||'')+'|'+(s.channel||'')+'|'+n(s.frame_id)+'|'+n(s.start_bit)+'|'+n(s.length)+'|'+n(s.mux_start_bit)+'|'+n(s.mux_length)+'|'+n(s.mux_value);}
function signalObserverRateText(s,key,now){
  var cur={t:now,frame:n(s.frame_count),active:n(s.active_frame_count),change:n(s.change_count)};
  var prev=_signalObserverPrev[key];
  _signalObserverPrev[key]=cur;
  if(!prev||cur.t<=prev.t)return '--';
  var dt=(cur.t-prev.t)/1000;
  if(dt<=0)return '--';
  function r(a,b){var v=(a-b)/dt;if(v<0)v=0;return v>=100?String(Math.round(v)):(v>=10?v.toFixed(1):v.toFixed(2));}
  return r(cur.frame,prev.frame)+'/'+r(cur.active,prev.active)+'/'+r(cur.change,prev.change);
}
function updateSignalObserverCaptureUi(enabled){
  var start=document.getElementById('signalObserverStartBtn');
  var stop=document.getElementById('signalObserverStopBtn');
  if(start)start.disabled=!!enabled;
  if(stop)stop.disabled=!enabled;
}
function openSignalObserverFile(){
  var input=document.getElementById('signalObserverFile');
  if(input)input.click();
}
function renderSignalObserver(obs){
  var body=document.getElementById('signalObserverBody');
  var status=document.getElementById('signalObserverStatus');
  if(!body)return;
  var captureEnabled=!!(obs&&obs.enabled);
  updateSignalObserverCaptureUi(captureEnabled);
  if(!obs||!obs.signals||!obs.signals.length){
    body.innerHTML='<tr><td colspan="12" class="obs-idle">수신 대기 중</td></tr>';
    if(status)status.textContent='관찰 설정 없음';
    _signalObserverPrev={};
    return;
  }
  var rows=[];
  var now=Date.now();
  for(var i=0;i<obs.signals.length;i++){
    var s=obs.signals[i]||{};
    var bit=n(s.start_bit)+'+'+n(s.length);
    if(n(s.mux_length)>0)bit+=' m'+n(s.mux_value);
    var run=n(s.current_run_frames)+' / '+n(s.last_run_frames)+' / '+n(s.max_run_frames);
    var age=s.seen?n(s.age_ms)+'ms':'--';
    var key=signalObserverKey(s,i);
    var rate=signalObserverRateText(s,key,now);
    rows.push('<tr>'+[
      '<td class="obs-name" title="'+htmlEscape(s.name||'')+'">'+htmlEscape(s.name||'signal')+'</td>',
      '<td>'+htmlEscape(s.channel||'--')+'</td>',
      '<td>'+htmlEscape(s.frame_hex||('0x'+n(s.frame_id).toString(16).toUpperCase()))+'</td>',
      '<td>'+bit+'</td>',
      '<td class="'+(s.active?'obs-active':'obs-idle')+'">'+n(s.raw)+'</td>',
      '<td>'+n(s.frame_count)+'</td>',
      '<td>'+n(s.active_frame_count)+'</td>',
      '<td>'+n(s.change_count)+'</td>',
      '<td title="Frames/Active/Change per second">'+rate+'</td>',
      '<td>'+n(s.burst_count)+'</td>',
      '<td>'+run+'</td>',
      '<td>'+age+'</td>'
    ].join('')+'</tr>');
  }
  body.innerHTML=rows.join('');
  var eventText=obs.event_capacity?(' · 이벤트 '+n(obs.event_count||0)+'/'+n(obs.event_capacity)):'';
  if(status)status.textContent=(captureEnabled?'캡처 중':'캡처 정지')+' · 관찰 '+n(obs.count||obs.signals.length)+'개'+eventText+' · Ch=A/B 단독 · A필터 최대 '+n(obs.max_a_filter_ids||6)+' ID(기본 4 포함) · Δ/s=Frames/Active/Change · Run=현재/직전/최대 프레임';
}
async function setSignalObserverCapture(enabled){
  var status=document.getElementById('signalObserverStatus');
  if(status)status.textContent=enabled?'캡처 시작 중...':'캡처 정지 중...';
  updateSignalObserverCaptureUi(enabled);
  try{
    var r=await fetch('/api/signal-observer/capture',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:!!enabled})});
    if(!r.ok)throw new Error('capture');
    if(enabled)_signalObserverPrev={};
    if(status)status.textContent=enabled?'캡처 시작됨':'캡처 정지됨';
    poll();
  }catch(e){if(status)status.textContent='캡처 전환 실패';poll();}
}
async function resetSignalObserver(){
  var status=document.getElementById('signalObserverStatus');
  if(status)status.textContent='리셋 중...';
  try{
    var r=await fetch('/api/signal-observer/reset',{method:'POST'});
    if(!r.ok)throw new Error('reset');
    _signalObserverPrev={};
    if(status)status.textContent='카운터 리셋 완료';
    poll();
  }catch(e){if(status)status.textContent='리셋 실패';}
}
async function uploadSignalObserverConfig(input){
  if(!input||!input.files||!input.files.length)return;
  var status=document.getElementById('signalObserverStatus');
  var file=input.files[0];
  try{
    if(status)status.textContent='JSON 읽는 중...';
    var text=await file.text();
    var parsed=JSON.parse(text);
    var payload=Array.isArray(parsed)?{signals:parsed}:parsed;
    var r=await fetch('/api/signal-observer/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
    var resp=await r.text();
    if(!r.ok)throw new Error(resp||'config');
    _signalObserverPrev={};
    if(status)status.textContent='설정 적용 완료';
    poll();
  }catch(e){
    if(status)status.textContent='업로드 실패: '+(e&&e.message?e.message:'JSON 확인 필요');
  }finally{
    input.value='';
  }
}
async function emergencyToggle(){
  var targets=[
    {id:'tNag',   path:'/api/nag-killer'},
    {id:'tEap',   path:'/api/enhanced-autopilot'},
    {id:'tTsllc', path:'/api/tsllc'},
    {id:'tUlcStalkConfirm', path:'/api/ui-ulc-stalk-confirm'},
    {id:'tAlcOffHighway', path:'/api/ui-alc-off-highway-enable'},
    {id:'tExpAlcOffHighway', path:'/api/ui-alc-off-highway-enable'},
    {id:'tUlcOffHighway', path:'/api/ui-ulc-off-highway'},
    {id:'tAutoLaneChange', path:'/api/ui-auto-lane-change-enable'},
    {id:'tAutoTurnSignalMode', path:'/api/ui-auto-turn-signal-mode'},
    {path:'/api/a-channel-tx', force:true}
  ];
  var btn=document.getElementById('emStopBtn');
  if(btn){btn.disabled=true;btn.textContent='해제 중...';}
  for(var i=0;i<targets.length;i++){
    var el=document.getElementById(targets[i].id);
    if((el&&el.checked)||targets[i].force){
      if(el)el.checked=false;
      try{await fetch(targets[i].path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:false})});}catch(e){}
    }
  }
  try{await fetch('/api/ui-ulc-speed-config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({value:255})});}catch(e){}
  try{await fetch('/api/ui-ulc-blind-spot-config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({value:255})});}catch(e){}
  if(btn){btn.disabled=false;btn.textContent='즉시 기능해제';}
}
function toggleTheme(){
  var html=document.documentElement;
  var cur=html.getAttribute('data-theme')||'dark';
  var next=cur==='dark'?'light':'dark';
  html.setAttribute('data-theme',next);
  var btn=document.getElementById('theme-btn');
  if(btn)btn.textContent=next==='dark'?'\u2600 Light':'\ud83c\udf19 Dark';
  fetch('/api/set-theme',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({theme:next})}).catch(function(){});
}
var _nagProfile=0;
var _nagModePhaseLabels=['idle','grace','s2지연','s2mild','s3지연','ramp','hold'];
var _nagProfileFallback=[
  {label:'기본',summary:'현재 검증 기준. 700/400ms 타이밍을 유지하고 조건이 맞는 동안 연속 관찰 주입.',s1:500,s2:700,s3:400,mildMin:0.5,mildMax:1.5,s2b:0,s2p:0,s3b:0,s3p:0},
  {label:'A안',summary:'초기 grace를 줄이고 짧은 burst 후 쉬는 구간을 둔다.',s1:150,s2:700,s3:400,mildMin:0.5,mildMax:1.5,s2b:250,s2p:750,s3b:500,s3p:1000},
  {label:'B안',summary:'가장 보수적. state1 주입을 없애고 더 짧게 반응한 뒤 길게 관찰한다.',s1:0,s2:900,s3:600,mildMin:0.5,mildMax:1.5,s2b:150,s2p:1350,s3b:300,s3p:1700},
  {label:'C안',summary:'1차 delay+torque 후보. state2는 600ms로 앞당기고 mild 상한을 1.7Nm까지 올리며 strong은 400ms/2.10Nm 유지.',s1:500,s2:600,s3:400,mildMin:0.5,mildMax:1.7,s2b:0,s2p:0,s3b:0,s3p:0},
  {label:'D안',summary:'C안 + 직선 저조향각 sign hold 후보. 토크와 timing은 C안과 같고 방향만 1.5초 유지.',s1:500,s2:600,s3:400,mildMin:0.5,mildMax:1.7,s2b:0,s2p:0,s3b:0,s3p:0}
];
function msText(v){return (v===0||v>0)?(v+'ms'):'--';}
function nmRangeText(min,max){
  min=Number(min);max=Number(max);
  if(!isFinite(min)||!isFinite(max))return '--';
  return '±'+min.toFixed(1)+'-'+max.toFixed(1)+'Nm';
}
function formatProfileBadge(label){
  if(label==='기본')return '[기본]';
  if(label==='A안')return '[A모드]';
  if(label==='B안')return '[B모드]';
  if(label==='C안')return '[C모드]';
  if(label==='D안')return '[D모드]';
  return '['+label+']';
}
function updateNagProfileUi(d){
  var p=(d&&d.smartProfile!==undefined)?Number(d.smartProfile):_nagProfile;
  if(!(p>=0&&p<=4))p=0;
  _nagProfile=p;
  var fb=_nagProfileFallback[p];
  var radios=document.querySelectorAll('input[name="nagProfile"]');
  for(var i=0;i<radios.length;i++)radios[i].checked=Number(radios[i].value)===p;
  var profileRaw=(d&&d.profileLabel)||fb.label;
  var lbl=document.getElementById('nagProfileLabel');if(lbl)lbl.textContent=formatProfileBadge(profileRaw);
  var mainProfile=document.getElementById('s-main-profile');if(mainProfile){var profileText=(d&&d.profileLabel)||fb.label;mainProfile.textContent=profileText;mainProfile.className='stat-val v-acc';fitMainStat(mainProfile,profileText);}
  var desc=document.getElementById('nagProfileDesc');if(desc)desc.textContent=(d&&d.profileSummary)||fb.summary;
  var s1=(d&&d.state1GraceMs!==undefined)?d.state1GraceMs:fb.s1;
  var s2=(d&&d.state2DelayMs!==undefined)?d.state2DelayMs:fb.s2;
  var s3=(d&&d.strongDelayMs!==undefined)?d.strongDelayMs:fb.s3;
  var s2b=(d&&d.state2BurstMs!==undefined)?d.state2BurstMs:fb.s2b;
  var s2p=(d&&d.state2PauseMs!==undefined)?d.state2PauseMs:fb.s2p;
  var s3b=(d&&d.strongBurstMs!==undefined)?d.strongBurstMs:fb.s3b;
  var s3p=(d&&d.strongPauseMs!==undefined)?d.strongPauseMs:fb.s3p;
  var mildMin=(d&&d.state2MildMinNm!==undefined)?d.state2MildMinNm:fb.mildMin;
  var mildMax=(d&&d.state2MildMaxNm!==undefined)?d.state2MildMaxNm:fb.mildMax;
  var e=document.getElementById('np_s1');if(e)e.textContent=msText(s1);
  e=document.getElementById('np_s2');if(e)e.textContent=msText(s2);
  e=document.getElementById('np_mild');if(e)e.textContent=nmRangeText(mildMin,mildMax);
  e=document.getElementById('np_s3');if(e)e.textContent=msText(s3);
  e=document.getElementById('np_burst');if(e)e.innerHTML=(s2b||s3b)?('<span class="burst-lines"><span>S2 '+s2b+'/'+s2p+'ms</span><span>S3 '+s3b+'/'+s3p+'ms</span></span>'):'연속';
}
function setNagProfile(p){
  fetch('/api/nag-profile?p='+p,{method:'POST'}).then(function(r){return r.json();}).then(function(d){
    updateNagProfileUi(d);
  }).catch(function(){alert('프로파일 변경 실패');});
}
function tickNagStats(){
  if(isBackgroundNetworkPaused())return;
  fetch('/api/nag-stats').then(function(r){return r.json();}).then(function(d){
    var stateColors={'running':'var(--acc2)','bus_off':'var(--err)','recovering':'var(--warn)','init':'var(--tx3)'};
    var canStateText={'running':'RUNNING','bus_off':'BUS-OFF','recovering':'RECOVERING','init':'INIT'};
    var bTwai=document.getElementById('bTwai');
    if(bTwai){bTwai.textContent=canStateText[d.canState]||d.canState||'--';bTwai.style.color=stateColors[d.canState]||'';}
    var bMode=document.getElementById('bMode');if(bMode)bMode.textContent=d.torqueNm!==undefined?d.torqueNm.toFixed(1)+'Nm':'--';
    var bEcho=document.getElementById('bEcho');if(bEcho)bEcho.textContent=d.echo||0;
    var bBusOff=document.getElementById('bBusOff');if(bBusOff)bBusOff.textContent=d.busoffCount||0;
    if(d.boSoftMode!==undefined&&_boSoftMode!==!!d.boSoftMode){_boSoftMode=!!d.boSoftMode;updateBoModeUi();}
    var boFallbackInfo=document.getElementById('boFallbackInfo');if(boFallbackInfo)boFallbackInfo.textContent='Hard fallback '+(d.boSoftFallback||0)+'회';
    if(d.singleShotTx!==undefined&&_singleShotTx!==!!d.singleShotTx){_singleShotTx=!!d.singleShotTx;updateSsTxUi();}
    if(d.busOffStopSkip!==undefined&&_busOffStopSkip!==!!d.busOffStopSkip){_busOffStopSkip=!!d.busOffStopSkip;updateBoStopUi();}
    if(d.smartProfile!==undefined)updateNagProfileUi(d);
    // Smart Torque 상태 업데이트
    var eApst=document.getElementById('ns_apst');if(eApst){eApst.textContent=nagApStateText(d);eApst.title=nagApStateTitle(d)+' · 921='+n(d.frames921)+' 923='+n(d.frames923)+' last='+n(d.lastDasStatusAgeMs)+'ms';}
    var mainAp=document.getElementById('s-main-ap');if(mainAp){var mainApText=nagApStateMainText(d);mainAp.textContent=mainApText;mainAp.className=statusClass(apStateAllowsInject(d.dasApState)?'ok':'warn');mainAp.title=nagApStateMainTitle(d)+' · 921='+n(d.frames921)+' 923='+n(d.frames923)+' last='+n(d.lastDasStatusAgeMs)+'ms';fitMainStat(mainAp,mainApText);}
    var eAngle=document.getElementById('ns_angle');if(eAngle)eAngle.textContent=d.steerAngleDeg!==undefined?d.steerAngleDeg.toFixed(1)+'°':'--';
    var mainAngle=document.getElementById('s-main-angle');if(mainAngle){var mainAngleText=d.steerAngleDeg!==undefined?d.steerAngleDeg.toFixed(1)+'°':'--';mainAngle.textContent=mainAngleText;mainAngle.className='stat-val v-acc';fitMainStat(mainAngle,mainAngleText);}
    var eMbph=document.getElementById('ns_mbphase');
    if(eMbph){var ph=d.modeBPhase||0;eMbph.textContent=d.nagLastDecisionText==='AP_BLOCK'?'ap_block':(_nagModePhaseLabels[ph]||ph);}
    var mainPhase=document.getElementById('s-main-phase');if(mainPhase){var ph2=d.modeBPhase||0;var phaseText=d.nagLastDecisionText==='AP_BLOCK'?'ap_block':(_nagModePhaseLabels[ph2]||ph2);mainPhase.textContent=phaseText;mainPhase.className='stat-val '+(phaseText==='ap_block'?'v-warn':phaseText==='idle'?'v-dim':'v-acc');fitMainStat(mainPhase,phaseText);}
    var eMbInj=document.getElementById('ns_mbinject');if(eMbInj)eMbInj.textContent=d.modeBInjects||0;
    var mainInject=document.getElementById('s-main-inject');if(mainInject){var injectText=d.modeBInjects||0;mainInject.textContent=injectText;mainInject.className='stat-val v-acc';fitMainStat(mainInject,injectText);}
    var eMbNm=document.getElementById('ns_mbnm');if(eMbNm)eMbNm.textContent=d.modeBLastNm!==undefined?d.modeBLastNm.toFixed(2)+'Nm':'--';
    var mainTorque=document.getElementById('s-main-torque');if(mainTorque){var torqueText=d.torqueNm!==undefined?d.torqueNm.toFixed(2)+'Nm':'--';mainTorque.textContent=torqueText;mainTorque.className='stat-val '+(Math.abs(n(d.torqueNm))>2?'v-warn':'v-acc');fitMainStat(mainTorque,torqueText);}
    var eRx297=document.getElementById('ns_rx297');if(eRx297)eRx297.textContent=d.frames297||0;
    // nag-stats 공통 필드
    var nsRx=document.getElementById('ns_rx');if(nsRx)nsRx.textContent=d.rx||0;
    var nsEcho=document.getElementById('ns_echo');if(nsEcho)nsEcho.textContent=d.echo||0;
    var nsFail=document.getElementById('ns_fail');if(nsFail)nsFail.textContent=d.txFail||0;
    var nsCs=document.getElementById('ns_cs');if(nsCs){nsCs.textContent=d.canState||'--';var c={'running':'var(--ok)','bus_off':'var(--err)','recovering':'var(--warn)'};nsCs.style.color=c[d.canState]||'';}
    var nsDas=document.getElementById('ns_das');if(nsDas){nsDas.textContent=d.dasSourceId&&d.dasHandsState!==undefined?(d.dasHandsState+' @'+dasSourceText(d.dasSourceId)):'--';nsDas.title='921='+n(d.frames921)+' 923='+n(d.frames923)+' last='+n(d.lastDasStatusAgeMs)+'ms';}
    var nsTec=document.getElementById('ns_tec');if(nsTec)nsTec.textContent=(d.tecNow||0)+' / '+(d.tecPeak||0);
    updateMainGuidance(d);
  }).catch(function(){});
}
function loadBusOffLog(){
  fetch('/api/busoff-log').then(function(r){return r.json();}).then(function(d){
    var tbody=document.getElementById('boLogBody');
    var meta=document.getElementById('boLogMeta');
    if(meta)meta.textContent='BUS-OFF 이벤트: '+(d.count||0)+'건';
    if(!tbody)return;
    if(!d.events||d.events.length===0){
      tbody.innerHTML='<tr><td colspan="7" style="padding:10px;text-align:center;color:var(--muted)">BUS-OFF 이벤트 없음</td></tr>';
      return;
    }
    tbody.innerHTML=d.events.map(function(e){
      return '<tr style="border-bottom:1px solid var(--bd)">'
        +'<td style="padding:4px 8px">'+e.seq+'</td>'
        +'<td style="padding:4px 8px">'+(e.ts/1000).toFixed(1)+'s</td>'
        +'<td style="padding:4px 8px;color:'+(e.tec>=128?'var(--bad)':'var(--tx)')+'">'+e.tec+'</td>'
        +'<td style="padding:4px 8px">'+e.rec+'</td>'
        +'<td style="padding:4px 8px">'+e.dur_ms+'ms</td>'
        +'<td style="padding:4px 8px">'+(e.since_ms>0?(e.since_ms/1000).toFixed(1)+'s':'--')+'</td>'
        +'<td style="padding:4px 8px;color:'+(e.ok?'var(--ok)':'var(--bad)')+'">'+(e.ok?'&#x2705; 성공':'&#x274C; 실패')+'</td>'
        +'</tr>';
    }).join('');
  }).catch(function(){});
}
function n(v){v=Number(v);return isFinite(v)?v:0;}
function fitMainStat(el,text){
  if(!el)return;
  var len=String(text).length;
  el.classList.toggle('long',len>7);
  el.classList.toggle('xlong',len>11);
}
function setTxt(id,text,cls){
  var el=document.getElementById(id);if(!el)return;
  el.textContent=text;
  if(cls)el.className=cls;
  if(id.indexOf('s-main-')===0)fitMainStat(el,text);
}
function setColor(id,color){var el=document.getElementById(id);if(el)el.style.color=color||'';}
function setFill(id,hz){
  var el=document.getElementById(id);if(!el)return;
  var pct=Math.max(0,Math.min(100,n(hz)));
  el.style.width=pct+'%';
}
function setChip(id,text,level){
  var el=document.getElementById(id);if(!el)return;
  el.textContent=text;
  el.className='chip '+(level||'ok');
}
function twaiName(code){
  code=n(code);
  if(code===1)return 'RUNNING';
  if(code===2)return 'BUS-OFF';
  if(code===3)return 'RECOVERING';
  return 'INIT';
}
function bCardTitleText(b){
  var code=n(b.twai_state_code);
  var icon='\ud83d\udd35';
  if(!b.can_task_created||!b.driver_ok||code===2)icon='\ud83d\udd34';
  else if(code===3)icon='\ud83d\udfe1';
  else if(code===1)icon='\ud83d\udfe2';
  return icon+' B 채널 (TWAI)';
}
function aCardTitleText(a,aState){
  var level=(aState&&aState.level)||channelAState(a).level;
  var icon='\ud83d\udd35';
  if(level==='ok')icon='\ud83d\udfe2';
  else if(level==='warn')icon='\ud83d\udfe1';
  else if(level==='err')icon='\ud83d\udd34';
  return icon+' A 채널 (MCP2515)';
}
function dasSourceText(id){id=n(id);return id?id:'--';}
function apStateLabel(v){
  v=n(v);
  if(v===0)return 'DISABLED';
  if(v===1)return 'UNAVAILABLE';
  if(v===2)return 'AVAILABLE';
  if(v===3)return 'ACTIVE_NOMINAL';
  if(v===4)return 'ACTIVE_RESTRICTED';
  if(v===5)return 'ACTIVE_NAV';
  if(v===6)return 'ACTIVE_FSD';
  if(v===8)return 'ABORTING';
  if(v===9)return 'ABORTED';
  if(v===14)return 'FAULT';
  if(v===15)return 'SNA';
  return 'UNKNOWN';
}
function apStateAllowsInject(v){v=n(v);return v>=3&&v<=6;}
function apStateLabelKo(v){
  v=n(v);
  if(v===0)return '꺼짐';
  if(v===1)return '사용불가';
  if(v===2)return '대기중';
  if(v===3)return '활성(일반)';
  if(v===4)return '활성(제한)';
  if(v===5)return '활성(내비)';
  if(v===6)return '활성(FSD)';
  if(v===8)return '해제중';
  if(v===9)return '해제됨';
  if(v===14)return '오류';
  if(v===15)return '미수신';
  return '알수없음';
}
function nagApStateText(d){
  if(d.dasApState===undefined)return '--';
  var v=n(d.dasApState);
  var src=d.dasSourceId?' @'+d.dasSourceId:'';
  return v+' '+apStateLabel(v)+src;
}
function nagApStateMainText(d){
  if(d.dasApState===undefined)return '--';
  var v=n(d.dasApState);
  var src=d.dasSourceId?' @'+d.dasSourceId:'';
  var gate=apStateAllowsInject(v)?'주입가능':'주입대기';
  return v+' '+apStateLabelKo(v)+' '+gate+src;
}
function nagApStateTitle(d){
  if(d.dasApState===undefined)return 'DAS_autopilotState 미수신';
  var v=n(d.dasApState);
  var src=d.dasSourceId?d.dasSourceId:'--';
  return 'AP state '+v+' ('+apStateLabel(v)+') · inject '+(apStateAllowsInject(v)?'ALLOW':'BLOCK')+' · source '+src;
}
function nagApStateMainTitle(d){
  if(d.dasApState===undefined)return '오토파일럿 상태 미수신';
  var v=n(d.dasApState);
  var src=d.dasSourceId?d.dasSourceId:'--';
  var gate=apStateAllowsInject(v)?'나그 대응 동작 가능':'나그 대응 대기';
  return '오토파일럿 '+v+' ('+apStateLabelKo(v)+') · '+gate+' · source '+src;
}
function updateGuidanceStateHighlight(decision,level){
  var states=['NONE','OFF','AP_BLOCK','HANDS_ON','DAS_IDLE','NO_880','NO_921','NO_ECHO','LATE_DROP','ECHO'];
  var active=(states.indexOf(decision)>=0)?decision:'NONE';
  for(var i=0;i<states.length;i++){
    var name=states[i];
    var chip=document.getElementById('gState-'+name);
    var detailChip=document.getElementById('gDetailState-'+name);
    if(chip){
      chip.className='guidance-chip'+(name===active?' active':'');
      if(name===active&&level&&level!=='ok'&&level!=='dim')chip.className+=' '+level;
    }
    if(detailChip){
      detailChip.className='guidance-chip'+(name===active?' active':'');
      if(name===active&&level&&level!=='ok'&&level!=='dim')detailChip.className+=' '+level;
    }
  }
}
function openGuidanceDetailPage(){
  var page=document.getElementById('guidanceDetailPage');
  if(page)page.classList.add('open');
}
function closeGuidanceDetailPage(ev){
  if(ev&&ev.target&&ev.currentTarget&&ev.target!==ev.currentTarget)return;
  var page=document.getElementById('guidanceDetailPage');
  if(page)page.classList.remove('open');
}
function updateMainGuidance(d){
  var card=document.getElementById('mainGuidanceCard');
  var p=document.getElementById('mainGuidancePrimary');
  var s=document.getElementById('mainGuidanceSecondary');
  if(!card||!p||!s)return;

  var level='dim';
  var primary='상태 수신 대기 중입니다.';
  var secondary='주행 데이터를 확인 중입니다.';
  var decision=String((d&&d.nagLastDecisionText)||'');
  var ap=n(d&&d.dasApState);
  var warn=n(d&&d.dasHandsWarnLevel);
  var phase=n(d&&d.modeBPhase);
  var age=n(d&&d.lastDasStatusAgeMs);
  var nagToggle=document.getElementById('tNag');
  var nagOn=!!(nagToggle&&nagToggle.checked);
  var activeDecision='NONE';

  if(!nagOn){
    level='warn';
    activeDecision='OFF';
    primary='가상 토크 기능이 꺼져 있습니다.';
    secondary='필요 시 제어 탭에서 Autosteer Nag Killer를 켜세요.';
  } else if(!d||d.dasApState===undefined||age===0||age>3000){
    level='warn';
    activeDecision='NO_921';
    primary='상태 수신 대기 중입니다.';
    secondary='DAS 상태 프레임이 아직 안정적으로 들어오지 않습니다.';
  } else if(decision==='HANDS_ON'){
    level='warn';
    activeDecision='HANDS_ON';
    primary='현재 핸들을 잡고 있는 중입니다.';
    secondary='손 감지 중이라 가상 토크 주입을 멈춥니다.';
  } else if(decision==='ECHO'){
    level='ok';
    activeDecision='ECHO';
    var nm=Math.abs(Number(d.modeBLastNm||d.torqueNm||0));
    primary='현재 가상 토크 주입 중입니다.';
    secondary='현재 '+nm.toFixed(2)+'Nm 가상 토크를 주입 중입니다.';
  } else if(!apStateAllowsInject(ap)||decision==='AP_BLOCK'){
    level='warn';
    activeDecision='AP_BLOCK';
    primary='오토스티어 조건이 아니어서 대기 중입니다.';
    secondary='AP 상태가 주입 허용 구간(3-6)으로 바뀌면 대응합니다.';
  } else if(warn>=2||phase===2||phase===4){
    level='warn';
    activeDecision=(decision&&decision!=='NONE')?decision:'NO_ECHO';
    primary='핸들 경고가 감지되어 대응 준비 중입니다.';
    secondary='조건이 맞으면 가상 토크 주입으로 전환합니다.';
  } else {
    level='dim';
    activeDecision=(decision&&decision!=='NONE')?decision:'NO_ECHO';
    primary='현재 주입 대기 중입니다.';
    secondary='경고 발생 시에만 가상 토크 주입을 시작합니다.';
  }

  card.className='main-guidance '+level;
  p.textContent=primary;
  s.textContent=secondary;
  updateGuidanceStateHighlight(activeDecision,level);
}
function statusClass(level){return 'stat-val '+(level==='ok'?'v-ok':level==='warn'?'v-warn':level==='err'?'v-err':'v-dim');}
function dotStatusClass(level){return level==='ok'?'dot-ok':level==='warn'?'dot-warn':level==='err'?'dot-err':'dot-wait';}
function aRateLevel(hz){return n(hz)>0?'ok':'err';}
function b880RateLevel(hz){hz=n(hz);return hz>=80?'ok':hz>0?'warn':'err';}
function shortBuildId(id){
  id=String(id||'');
  var p=id.split('-');
  return p.length>=2?p[0]+'-'+p[1]:id;
}
function channelAState(a){
  if(!a.driver_ok)return {text:'INIT',level:'err'};
  if(n(a.mcp_eflg)&0x20)return {text:'BUS-OFF',level:'err'};
  if(!a.fresh)return {text:'NO FRAMES',level:'warn'};
  if(n(a.mcp_eflg)!==0)return {text:'WARN',level:'warn'};
  return {text:'OK',level:'ok'};
}
function channelBState(b){
  var code=n(b.twai_state_code);
  if(!b.can_task_created)return {text:'TASK OFF',level:'err'};
  if(!b.driver_ok)return {text:'DRIVER FAIL',level:'err'};
  if(code===2)return {text:'BUS-OFF',level:'err'};
  if(code===3)return {text:'RECOVERING',level:'warn'};
  if(!b.fresh)return {text:'NO FRAMES',level:'warn'};
  return {text:'OK',level:'ok'};
}
function fmtPeriod(ms){ms=n(ms);return ms>0?ms+' ms':'--';}
function fmtHzFromPeriod(ms){
  ms=n(ms);
  if(ms<=0)return '--';
  var hz=1000/ms;
  return hz>=10?hz.toFixed(0):hz.toFixed(1);
}
function setIdHz(id,periodMs){
  var text=fmtHzFromPeriod(periodMs);
  setTxt(id,text==='--'?'--':text+' Hz','v');
}
function updateChannelStatus(d){
  var ch=d.channels||{},a=ch.a_channel||{},b=ch.b_channel||{};
  var aState=channelAState(a),bState=channelBState(b);
  var aHz=n(a.frame_hz);
  var bHz=n(b.frame_hz);
  var aMainLevel=aState.level==='ok'?aRateLevel(aHz):aState.level;
  var bTwai=twaiName(b.twai_state_code);
  var bMainLevel=bState.level==='ok'?b880RateLevel(bHz):bState.level;
  setTxt('s-main-a',aHz.toFixed(1)+'Hz',statusClass(aMainLevel));
  setTxt('s-main-b',bHz.toFixed(1)+'Hz',statusClass(bMainLevel));
  setTxt('s-main-busoff',n(b.busoff_count),statusClass(n(b.busoff_count)?'err':'ok'));
  setTxt('s-can',aState.text,statusClass(aState.level));
  setTxt('s-bcan',bState.text,statusClass(bState.level));
  setTxt('s-a-eflg','0x'+('0'+n(a.mcp_eflg).toString(16).toUpperCase()).slice(-2),statusClass(n(a.mcp_eflg)?'warn':'ok'));
  setTxt('s-ahz',aHz.toFixed(1)+' Hz',statusClass(aRateLevel(aHz)));
  setTxt('s-arx',n(a.frames_received),'stat-val v-acc');
  setTxt('s-atx',n(a.eap_modified),'stat-val v-acc');
  setTxt('s-bhz',bHz.toFixed(1)+' Hz',statusClass(b880RateLevel(bHz)));
  setTxt('s-brx',n(b.frames_received),'stat-val v-acc');
  setTxt('s-btx',n(b.echo_count),'stat-val v-acc');
  setTxt('s-bdrop',n(b.echo_drop_late),'stat-val v-dim');
  setTxt('s-twai',twaiName(b.twai_state_code),statusClass(b.driver_ok?(n(b.twai_state_code)===2?'err':n(b.twai_state_code)===3?'warn':'ok'):'err'));
  setTxt('s-busoff',n(b.busoff_count),statusClass(n(b.busoff_count)?'err':'ok'));
  setFill('fps-fill-a',aHz);
  setFill('fps-fill-b',bHz);
  var sdotA=document.getElementById('sdot-a'),sdotB=document.getElementById('sdot-b');
  var hdrA=document.getElementById('hdr-a'),hdrB=document.getElementById('hdr-b');
  var aChTitle=document.getElementById('aChTitle');
  var bChTitle=document.getElementById('bChTitle');
  var bHdrText=bState.text==='OK'?bTwai:bState.text;
  if(sdotA)sdotA.className='sdot '+dotStatusClass(aMainLevel);
  if(sdotB)sdotB.className='sdot '+dotStatusClass(bMainLevel);
  if(hdrA)hdrA.textContent='A '+aState.text;
  if(hdrB)hdrB.textContent='B '+bHdrText;
  if(aChTitle)aChTitle.textContent=aCardTitleText(a,aState);
  if(bChTitle)bChTitle.textContent=bCardTitleText(b);
  setTxt('aHz',aHz.toFixed(1)+' Hz','val '+(aRateLevel(aHz)==='ok'?'on':'off'));
  setTxt('aUlcStalkConfirmMod',n(a.ulc_stalk_confirm_modified),'val');
  setTxt('aUlcStalkConfirmSkip',n(a.ulc_stalk_confirm_skipped),'val');
  setTxt('aAlcOffHighwayMod',n(a.alc_off_highway_modified),'val');
  setTxt('aAlcOffHighwaySkip',n(a.alc_off_highway_skipped),'val');
  var expCounters=document.getElementById('expInjectCounters');
  if(expCounters)expCounters.textContent='0x293 AutoLC '+n(a.auto_lane_change_modified)+'/'+n(a.auto_lane_change_skipped)+' · 0x3FD AutoTurn '+n(a.auto_turn_signal_mode_modified)+'/'+n(a.auto_turn_signal_mode_skipped)+' · 0x3E9 DASConfirm '+n(a.das_ulc_confirmation_modified)+'/'+n(a.das_ulc_confirmation_skipped)+' · 0x3F8 ULCOff '+n(a.ulc_off_highway_modified)+'/'+n(a.ulc_off_highway_skipped)+' · Speed '+n(a.ulc_speed_config_modified)+'/'+n(a.ulc_speed_config_skipped)+' · Blind '+n(a.ulc_blind_spot_config_modified)+'/'+n(a.ulc_blind_spot_config_skipped);
  var expJudge=document.getElementById('expJudgeLine');
  if(expJudge){
    function j(name,frames,on,mod,skip){
      if(!on)return n(mod)>0?name+' OFF·누적주입 '+n(mod):name+' OFF';
      if(!a.channel_tx_enabled)return name+' A-TX OFF';
      if(n(frames)===0)return name+' CAN 미수신';
      if(n(mod)>0)return name+' 주입됨 '+n(mod);
      if(n(skip)>0)return name+' 이미 목표값 '+n(skip);
      return name+' 수신 대기';
    }
    var speedOn=d.ui_ulc_speed_config_active;
    var blindOn=d.ui_ulc_blind_spot_config_active;
    var parts=[
      j('AutoLC',a.frames_293,d.ui_auto_lane_change_enable_enabled,a.auto_lane_change_modified,a.auto_lane_change_skipped),
      j('AutoTurn',a.frames_1021,d.ui_auto_turn_signal_mode_enabled,a.auto_turn_signal_mode_modified,a.auto_turn_signal_mode_skipped),
      j('DASConfirm',a.frames_1001,d.das_ulc_confirmation_request_enabled,a.das_ulc_confirmation_modified,a.das_ulc_confirmation_skipped),
      j('ULCOff',a.frames_1016,d.ui_ulc_off_highway_enabled,a.ulc_off_highway_modified,a.ulc_off_highway_skipped),
      j('Speed',a.frames_1016,speedOn,a.ulc_speed_config_modified,a.ulc_speed_config_skipped),
      j('Blind',a.frames_1016,blindOn,a.ulc_blind_spot_config_modified,a.ulc_blind_spot_config_skipped)
    ];
    expJudge.textContent=parts.join(' · ');
  }
  setTxt('aEapMod',n(a.eap_modified),'val');
  setTxt('a1016Period',n(a.frames_1016)+' / '+fmtPeriod(a.id_1016_period_ms),'val');
  setTxt('a1021Period',n(a.frames_1021)+' / '+fmtPeriod(a.id_1021_period_ms),'val');
  setTxt('aTxMaster',a.channel_tx_enabled?'ON':'OFF','val '+(a.channel_tx_enabled?'on':'off'));
  setTxt('aAutoCap',aState.text,'val '+(aState.level==='ok'?'on':'off'));
  setTxt('bTwai',twaiName(b.twai_state_code),'val');
  setColor('bTwai',b.driver_ok?(n(b.twai_state_code)===2?'var(--err)':n(b.twai_state_code)===3?'var(--warn)':'var(--acc2)'):'var(--err)');
  setTxt('bDrvErr',n(b.driver_install_err)+' / '+n(b.driver_start_err),'val '+(b.driver_ok?'on':'off'));
  setIdHz('bHz880',b.id_target_period_ms);
  setIdHz('bHz921',b.id_921_period_ms);
  setIdHz('bHz923',b.id_923_period_ms);
  setIdHz('bHz297',b.id_297_period_ms);
  var bl=document.getElementById('bTargetLbl');if(bl)bl.textContent='ID '+(b.target_id||880)+' (NAG)';
  setTxt('b880Info',n(b.frames_target)+' / '+fmtPeriod(b.id_target_period_ms),'val');
  setTxt('b921Info',n(b.frames_921)+' / '+fmtPeriod(b.id_921_period_ms),'val');
  setTxt('b923Info',n(b.frames_923)+' / '+fmtPeriod(b.id_923_period_ms),'val');
  setTxt('b297Info',n(b.frames_297)+' / '+fmtPeriod(b.id_297_period_ms),'val');
  setTxt('bEcho',n(b.echo_count),'val');
  setTxt('bBusOff',n(b.busoff_count),'val '+(n(b.busoff_count)?'off':'on'));
  setTxt('bRecStat',n(b.recovery_attempt_count)+' / '+n(b.recovery_success_count)+' / '+n(b.recovery_fail_count),'val');
  setTxt('bRecMs',n(b.last_recovery_duration_ms)+' ms','val');
  setTxt('bErrPeak',n(b.twai_rx_err_peak)+' / '+n(b.twai_tx_err_peak),'val');
  setChip('d-a-tx','TX '+n(a.tx_ok)+'/'+n(a.tx_fail),n(a.tx_fail)?'warn':'ok');
  var aTec=n(a.tec),aTecPeak=n(a.tec_peak);setChip('d-a-tec','TEC '+aTec+'/'+aTecPeak,aTec>=128||aTecPeak>=128?'err':aTec>=96||aTecPeak>=96?'warn':'ok');
  setChip('d-a-merrf','MERRF '+n(a.merrf),n(a.merrf)?'warn':'ok');
  setChip('d-a-rxovr','RX-OVR '+n(a.rx_ovr),n(a.rx_ovr)?'warn':'ok');
  setChip('d-a-rec','REC '+n(a.rec)+'/'+n(a.rec_peak),n(a.rec)>=128||n(a.rec_peak)>=128?'err':n(a.rec)||n(a.rec_peak)?'warn':'ok');
  setChip('d-a-eflgev','EFLG-EV '+n(a.eflg_event_count),n(a.eflg_event_count)?'warn':'ok');
  setChip('d-b-arb','ARB '+n(b.arb_lost),n(b.arb_lost)?'warn':'ok');
  setChip('d-b-err','BUS-ERR '+n(b.bus_error),n(b.bus_error)?'warn':'ok');
  setChip('d-b-txf','TX-FAIL '+n(b.tx_failed),n(b.tx_failed)?'warn':'ok');
  setChip('d-b-rxm','RX-MISS '+n(b.rx_missed),n(b.rx_missed)?'warn':'ok');
  var issues=[];if(aState.level!=='ok')issues.push('A '+aState.text);if(bState.level!=='ok')issues.push('B '+bState.text);if(a.fresh&&!a.task_alive)issues.push('A LOOP LAG');if(b.fresh&&!b.task_alive)issues.push('B LOOP LAG');
  var ds=document.getElementById('diag-sum');
  if(ds){ds.textContent=issues.length?issues.join(' · '):'A/B 정상';ds.className='diag-sum '+(issues.some(function(x){return x.indexOf('FAIL')>=0||x.indexOf('OFF')>=0;})?'err':issues.length?'warn':'ok');}
}
async function poll(){
  if(_otaUploadInProgress||isBackgroundNetworkPaused())return;
  try{
    var r=await fetch('/api/status?log_since='+logSince);
    if(!r.ok)throw new Error('status');
    var d=await r.json();
    var elFsd=document.getElementById('fsd');
    if(elFsd){elFsd.textContent=d.fsd_enabled?'활성':'비활성';elFsd.className='val '+(d.fsd_enabled?'on':'off');}
    var elEap=document.getElementById('eap');
    if(elEap){elEap.textContent=d.enhanced_autopilot?'활성':'비활성';elEap.className='val '+(d.enhanced_autopilot?'on':'off');}
    var elNag=document.getElementById('nag');
    if(elNag){elNag.textContent=d.nag_killer?'활성':'비활성';elNag.className='val '+(d.nag_killer?'on':'off');}
    var sEap=document.getElementById('s-eap');
    if(sEap){sEap.textContent=d.enhanced_autopilot?'ON':'OFF';sEap.className='stat-val '+(d.enhanced_autopilot?'v-ok':'v-err');}
    var sNag=document.getElementById('s-nag');
    if(sNag){sNag.textContent=d.nag_killer?'ON':'OFF';sNag.className='stat-val '+(d.nag_killer?'v-ok':'v-err');}
    var tEap=document.getElementById('tEap');if(tEap)tEap.checked=!!d.enhanced_autopilot;
    var tTsllc=document.getElementById('tTsllc');if(tTsllc)tTsllc.checked=!!d.tsllc_enabled;
    var tUlcStalkConfirm=document.getElementById('tUlcStalkConfirm');if(tUlcStalkConfirm)tUlcStalkConfirm.checked=!!d.ui_ulc_stalk_confirm_enabled;
    var tAlcOffHighway=document.getElementById('tAlcOffHighway');if(tAlcOffHighway)tAlcOffHighway.checked=!!d.ui_alc_off_highway_enable_enabled;
    var tExpAlcOffHighway=document.getElementById('tExpAlcOffHighway');if(tExpAlcOffHighway)tExpAlcOffHighway.checked=!!d.ui_alc_off_highway_enable_enabled;
    var tUlcOffHighway=document.getElementById('tUlcOffHighway');if(tUlcOffHighway)tUlcOffHighway.checked=!!d.ui_ulc_off_highway_enabled;
    var tAutoLaneChange=document.getElementById('tAutoLaneChange');if(tAutoLaneChange)tAutoLaneChange.checked=!!d.ui_auto_lane_change_enable_enabled;
    var tAutoTurnSignalMode=document.getElementById('tAutoTurnSignalMode');if(tAutoTurnSignalMode)tAutoTurnSignalMode.checked=!!d.ui_auto_turn_signal_mode_enabled;
    var tDasUlcConfirm=document.getElementById('tDasUlcConfirm');if(tDasUlcConfirm)tDasUlcConfirm.checked=!!d.das_ulc_confirmation_request_enabled;
    var tAChannelTx=document.getElementById('tAChannelTx');if(tAChannelTx)tAChannelTx.checked=!!d.a_channel_tx;
    setRadioValue('ulcSpeedConfig',d.ui_ulc_speed_config_value===undefined?255:d.ui_ulc_speed_config_value);
    setRadioValue('ulcBlindSpotConfig',d.ui_ulc_blind_spot_config_value===undefined?255:d.ui_ulc_blind_spot_config_value);
    var spCur=document.getElementById('expSpeedCurrent');if(spCur)spCur.textContent='현재: '+(d.ui_ulc_speed_config_name||'STOCK')+(d.ui_ulc_speed_config_active?' · 주입':' · 순정 유지');
    var bsCur=document.getElementById('expBlindCurrent');if(bsCur)bsCur.textContent='현재: '+(d.ui_ulc_blind_spot_config_name||'STOCK')+(d.ui_ulc_blind_spot_config_active?' · 주입':' · 순정 유지');
    var tNag=document.getElementById('tNag');if(tNag)tNag.checked=!!d.nag_killer;
    var tBNagMaster=document.getElementById('tBNagMaster');if(tBNagMaster)tBNagMaster.checked=!!d.nag_killer;
    var tLog=document.getElementById('tLog');if(tLog)tLog.checked=!!d.enable_print;
    if(d.features){
      var tASpi8=document.getElementById('tASpi8');if(tASpi8)tASpi8.checked=!!(d.features.a_spi_8mhz&&d.features.a_spi_8mhz.enabled);
      var tAOneShot=document.getElementById('tAOneShot');if(tAOneShot)tAOneShot.checked=!!(d.features.a_mcp_oneshot&&d.features.a_mcp_oneshot.enabled);
      var tATxGuard=document.getElementById('tATxGuard');if(tATxGuard)tATxGuard.checked=!!(d.features.a_tx_guard&&d.features.a_tx_guard.enabled);
    }
    if(d.uptime_ms&&baseUptimeTs===0){baseUptimeMs=d.uptime_ms;baseUptimeTs=Date.now();}
    var hwBadge=document.getElementById('hw-badge');
    if(hwBadge&&d.hw_handler){hwBadge.textContent=d.hw_handler;hwBadge.style.display='inline';}
    var buildBadge=document.getElementById('build-badge');
    var verBadge=document.getElementById('ver-badge');
    var buildLabel=d.firmware_build_short||shortBuildId(d.firmware_build_id);
    if(buildBadge&&buildLabel){buildBadge.textContent=buildLabel;buildBadge.style.display='inline-block';}
    if(verBadge&&d.firmware_version){
      verBadge.textContent='v'+d.firmware_version;
      verBadge.style.display='block';
    }
    if(d.theme&&d.theme!==document.documentElement.getAttribute('data-theme')){
      document.documentElement.setAttribute('data-theme',d.theme);
      var btn=document.getElementById('theme-btn');
      if(btn)btn.textContent=d.theme==='dark'?'\u2600 Light':'\ud83c\udf19 Dark';
    }
    updateChannelStatus(d);
    renderSignalObserver(d.signal_observer);
    updateUserMarkerUi(d);
    if(d.logs&&d.logs.length){
      var logEl=document.getElementById('log');
      if(logEl){
        for(var i=0;i<d.logs.length;i++){
          var le=document.createElement('div');le.className='le';
          var ts=document.createElement('span');ts.className='ts';
          ts.textContent='['+new Date(d.logs[i].ts).toLocaleTimeString()+'] ';
          le.appendChild(ts);le.appendChild(document.createTextNode(d.logs[i].msg));
          logEl.insertBefore(le,logEl.firstChild);
        }
        while(logEl.children.length>100)logEl.removeChild(logEl.lastChild);
      }
    }
    logSince=d.log_head||logSince;
    errCount=0;
    var ce=document.getElementById('connErr');if(ce)ce.style.display='none';
    if(typeof updateOtaConfirmBanner==='function')updateOtaConfirmBanner(d);
  }catch(e){
    errCount++;
    if(errCount>3){var ce2=document.getElementById('connErr');if(ce2)ce2.style.display='block';}
  }
}
function clearBusOffLog(){
  if(!confirm('BUS-OFF 로그를 모두 삭제하시겠습니까?'))return;
  fetch('/api/busoff-log',{method:'DELETE'}).then(function(){loadBusOffLog();}).catch(function(){alert('클리어 실패');});
}
function applyBusCooldown(){
  var ms=parseInt(document.getElementById('boCoolMs').value)||1000;
  fetch('/api/busoff-cooldown',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ms:ms})})
    .then(function(r){return r.json();}).then(function(d){
      if(d.ok)alert('쿨다운 '+d.ms+'ms 적용 완료');
    }).catch(function(){alert('적용 실패');});
}
// BUS-OFF 복구 모드: Soft recovery 우선 + Hard fallback 고정
var _boSoftMode=true;
function updateBoModeUi(){
  var sw=document.getElementById('tBoMode');
  if(sw){sw.checked=true;sw.disabled=true;}
  var meta=document.getElementById('metaBoModeCurrent');
  if(meta)meta.textContent='현재: Soft + Hard fallback (고정)';
}
function toggleBoMode(el){
  if(el)el.checked=true;
  updateBoModeUi();
}
// [v4.4 실험] Single Shot TX 토글
var _singleShotTx=false;
function updateSsTxUi(){
  var sw=document.getElementById('tSsTx');
  if(sw)sw.checked=!!_singleShotTx;
  var meta=document.getElementById('metaSsTxCurrent');
  if(meta)meta.textContent='현재: '+(_singleShotTx?'ON(재전송금지)':'OFF(기본)');
}
function toggleSsTx(el){
  var next=!!el.checked;
  fetch('/api/twai-ss-tx?v='+(next?1:0),{method:'POST'})
    .then(function(r){if(!r.ok)throw new Error('ss');return r.text();}).then(function(){
      _singleShotTx=next;
      updateSsTxUi();
    }).catch(function(){el.checked=!next;alert('전환 실패');});
}
// [v4.4 실험] BUS-OFF stop skip 토글
var _busOffStopSkip=false;
function updateBoStopUi(){
  var sw=document.getElementById('tBoStop');
  if(sw)sw.checked=!!_busOffStopSkip;
  var meta=document.getElementById('metaBoStopCurrent');
  if(meta)meta.textContent='현재: '+(_busOffStopSkip?'ON(v4.4준수)':'OFF(기본)');
}
function toggleBoStop(el){
  var next=!!el.checked;
  fetch('/api/twai-busoff-stop?v='+(next?1:0),{method:'POST'})
    .then(function(r){if(!r.ok)throw new Error('stop');return r.text();}).then(function(){
      _busOffStopSkip=next;
      updateBoStopUi();
    }).catch(function(){el.checked=!next;alert('전환 실패');});
}

// ── CAN 자가 진단 ──────────────────────────────────────────────────────────
var diagPollTimer=null,diagLogSince=0;
async function resetTimeseries(){
  if(!confirm('시계열/이벤트 로그와 기록 기준점을 초기화합니다. 드라이버 누적 상태와 토글 설정은 유지됩니다.'))return;
  var st=document.getElementById('diagStatus');
  try{var r=await fetch('/api/timeseries/reset',{method:'POST'});
    var d=await r.json();
    st.textContent=d.ok?'✓ 로그 초기화 완료':'초기화 실패';
    if(d.ok)updateUserMarkerUi({active:false,log_count:0,count:0});
    updateRecStatus();
  }catch(e){st.textContent='서버 오류';}
}
var recTimer=null;
function fmtElapsed(ms){
  var s=Math.floor(ms/1000);var h=Math.floor(s/3600);var m=Math.floor((s%3600)/60);s=s%60;
  return (h?h+':':'')+(m<10?'0':'')+m+':'+(s<10?'0':'')+s;
}
function updateUserMarkerUi(d){
  d=d||{};
  var hasActive=d.user_marker_active!==undefined||d.active!==undefined;
  var hasCount=d.user_marker_log_count!==undefined||d.log_count!==undefined||d.count!==undefined;
  var btns=[document.getElementById('userMarkBtn'),document.getElementById('markBtn')];
  var currentBtn=btns[0]||btns[1];
  var active=hasActive?!!(d.user_marker_active!==undefined?d.user_marker_active:d.active):!!(currentBtn&&currentBtn.classList&&currentBtn.classList.contains('active'));
  var count=hasCount?(d.user_marker_log_count!==undefined?d.user_marker_log_count:(d.log_count!==undefined?d.log_count:d.count)):null;
  for(var i=0;i<btns.length;i++){
    var btn=btns[i];
    if(!btn)continue;
    btn.disabled=false;
    btn.textContent='USER_MARK';
    btn.classList.toggle('active',active);
    btn.title=active?'경고 구간 기록 중. 다시 누르면 AP_WARNING_END가 기록됩니다.':'경고 첫 표시 순간에 누르면 AP_WARNING_START가 기록됩니다.';
  }
  var c=document.getElementById('userMarkCount');
  if(c&&count!==null)c.textContent=n(count)+'회';
}
async function updateRecStatus(){
  try{var r=await fetch('/api/timeseries/status');var d=await r.json();
    var btn=document.getElementById('recBtn');var st=document.getElementById('recStatus');
    if(d.rec){btn.textContent='⏹ 기록 정지';btn.style.background='#c0392b';
      st.textContent='🔴 REC '+fmtElapsed(d.elapsed_ms)+' · '+d.samples+'샘플';
    }else{btn.textContent='⏺ 기록 시작';btn.style.background='';
      st.textContent=d.start_ms?'⏹ 정지됨 · '+d.samples+'샘플':'';
    }
  }catch(e){}
}
async function toggleTimeseriesRec(){
  var btn=document.getElementById('recBtn');
  var starting=btn.textContent.indexOf('시작')>=0;
  if(starting&&!confirm('기록을 시작합니다. 기존 로그가 초기화됩니다.'))return;
  try{await fetch('/api/timeseries/rec',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({start:starting})});
    updateRecStatus();
    if(starting)updateUserMarkerUi({active:false,log_count:0,count:0});
    if(starting){if(!recTimer)recTimer=setInterval(updateRecStatus,1000);}
    else{if(recTimer){clearInterval(recTimer);recTimer=null;}}
  }catch(e){}
}
async function markApWarning(){
  var btn=document.getElementById('userMarkBtn')||document.getElementById('markBtn');
  var st=document.getElementById('diagStatus');
  var wasActive=btn&&btn.classList?btn.classList.contains('active'):false;
  var countEl=document.getElementById('userMarkCount');
  var prevCount=countEl?parseInt(countEl.textContent,10)||0:0;
  var nextCount=prevCount+(wasActive?1:0);
  updateUserMarkerUi({active:!wasActive,log_count:nextCount,count:nextCount});
  var btns=[document.getElementById('userMarkBtn'),document.getElementById('markBtn')];
  for(var i=0;i<btns.length;i++){if(btns[i])btns[i].disabled=true;}
  try{
    var r=await fetch('/api/user-marker?type=ap_warning',{method:'POST'});
    var d=await r.json();
    updateUserMarkerUi(d);
    if(st)st.textContent=d.ok?('✓ '+d.detail_text+' 기록됨 · '+d.timestamp_ms+'ms · '+d.log_count+'회'):'마커 실패';
  }catch(e){if(st)st.textContent='마커 서버 오류';updateUserMarkerUi({active:wasActive,log_count:prevCount,count:prevCount});}
}
// 페이지 로드 시 REC 상태 한 번 조회
setTimeout(function(){updateRecStatus();},1500);
async function startCanDiag(){
  var btn=document.getElementById('diagBtn');
  var logEl=document.getElementById('diagLogEl');
  var wrap=document.getElementById('diagLogWrap');
  var st=document.getElementById('diagStatus');
  btn.disabled=true;btn.textContent='진단 실행 중...';
  wrap.style.display='block';logEl.textContent='';diagLogSince=0;
  st.textContent='시작 요청 중...';
  try{var r=await fetch('/api/can-diag/start',{method:'POST'});
    var d=await r.json();
    st.textContent=d.msg||'진단 시작됨';
  }catch(e){st.textContent='서버 오류 — 재시도';}
  if(diagPollTimer)clearInterval(diagPollTimer);
  diagPollTimer=setInterval(pollDiagLog,800);
}
async function pollDiagLog(){
  try{
    var r=await fetch('/api/can-diag/log?since='+diagLogSince);
    var d=await r.json();
    var logEl=document.getElementById('diagLogEl');
    var st=document.getElementById('diagStatus');
    if(d.lines&&d.lines.length>0){
      d.lines.forEach(function(e){logEl.textContent+=e.msg+'\n';});
      logEl.scrollTop=logEl.scrollHeight;
      diagLogSince=d.head;
    }
    if(d.state===2){
      clearInterval(diagPollTimer);diagPollTimer=null;
      var btn=document.getElementById('diagBtn');
      btn.disabled=false;btn.textContent='다시 진단';
      st.textContent='✔ 진단 완료';
    }else if(d.state===1){
      st.textContent='실행 중 — 임시로 다른 탭 열어도 됩니다';
    }
  }catch(e){}
}
// ─────────────────────────────────────────────────────────────────────────────

function startDetailPolling(){
  if(detailPollingStarted)return;
  detailPollingStarted=true;
  // Nag 실시간 stats 폴링
  setInterval(tickNagStats,1000);
  tickNagStats();
}

function renderUptimeTick(){
  if(baseUptimeTs===0)return;
  var elapsed=Date.now()-baseUptimeTs;
  var seconds=Math.floor((baseUptimeMs+elapsed)/1000);
  var s=fmt(seconds);
  document.getElementById('up').textContent=s;
  var sUpEl=document.getElementById('s-up');
  if(sUpEl)sUpEl.textContent=s;
}

// ── OTA 확인/롤백 JS ──────────────────────────────────────────────────────────
var _otaConfirmTimer=null,_otaRollbackTimer=null;
var _otaConfirmDeadline=0,_otaRollbackDeadline=0;
function fmtCountdown(ms){
  if(ms<=0)return '0:00';
  var s=Math.ceil(ms/1000);var m=Math.floor(s/60);s=s%60;
  return m+':'+(s<10?'0':'')+s;
}
function otaShortDateTime(text){
  var s=String(text||'').trim();
  if(!s)return '--';
  var iso=s.match(/^(\d{4})-(\d{2})-(\d{2})[T ](\d{2}):(\d{2}):(\d{2})/);
  if(iso)return iso[1].slice(2)+'-'+iso[2]+'-'+iso[3]+' '+iso[4]+':'+iso[5]+':'+iso[6];
  var months={Jan:'01',Feb:'02',Mar:'03',Apr:'04',May:'05',Jun:'06',Jul:'07',Aug:'08',Sep:'09',Oct:'10',Nov:'11',Dec:'12'};
  var c=s.match(/^(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+(\d{1,2})\s+(\d{4})\s+(\d{2}):(\d{2}):(\d{2})/);
  if(c)return c[3].slice(2)+'-'+months[c[1]]+'-'+('0'+c[2]).slice(-2)+' '+c[4]+':'+c[5]+':'+c[6];
  return s.length>19?s.slice(0,19):s;
}
function otaBuildDateOnly(bid,builtAt){
  var m=String(bid||'').match(/FW\d+-(\d{2})(\d{2})(\d{2})\d{4}/);
  if(m)return m[1]+'-'+m[2]+'-'+m[3];
  var dt=otaShortDateTime(builtAt);
  return dt==='--'?'':dt.slice(0,8);
}
function otaFwBrief(ver,bid,builtAt){
  var v=ver||'--',d=otaBuildDateOnly(bid,builtAt);
  return d?v+' · '+d:v;
}
function otaSetText(id,text,cls){
  var el=document.getElementById(id);
  if(el){el.textContent=text||'--';el.className='stat-val '+(cls||'v-dim');}
}
function updateOtaConfirmBanner(d){
  var state=d.ota_pending_state||0;
  var stateText=state===2?'확인 대기':state===4?'복구 확인':state===5?'복구 모드':'정상';
  var stateLevel=state===0?'v-ok':state===2?'v-warn':state===4?'v-warn':'v-err';
  otaSetText('otaPanelCurrent',d.ota_current_label||'--','v-dim');
  otaSetText('otaPanelFallback',d.ota_fallback_label||'--','v-dim');
  otaSetText('otaPanelCurrentFw',otaFwBrief(d.ota_current_version||d.firmware_version,d.ota_current_build_id||d.firmware_build_id,d.ota_current_build_at||d.firmware_build_at),'v-acc');
  otaSetText('otaPanelFallbackFw',otaFwBrief(d.ota_fallback_version,d.ota_fallback_build_id,d.ota_fallback_build_at),d.ota_fallback_version?'v-warn':'v-dim');
  otaSetText('otaPanelCurrentBuilt',otaShortDateTime(d.ota_current_build_at||d.firmware_build_at),'v-dim');
  otaSetText('otaPanelFallbackBuilt',otaShortDateTime(d.ota_fallback_build_at),'v-dim');
  otaSetText('otaPanelUploadAt',d.ota_upload_at||'--',d.ota_upload_at?'v-warn':'v-dim');
  var st=document.getElementById('otaPanelState');if(st){st.textContent=stateText;st.className='stat-val '+stateLevel;}
  var rec=document.getElementById('otaPanelRecovery');if(rec){rec.textContent=d.ota_recovery_mode?'ON':'OFF';rec.className='stat-val '+(d.ota_recovery_mode?'v-err':'v-ok');}
  var cb=document.getElementById('otaConfirmBanner');
  var rb=document.getElementById('otaRollbackConfirmBanner');
  if(state===2&&cb){
    cb.style.display='block';if(rb)rb.style.display='none';
    var cl=document.getElementById('otaCurLabel');if(cl)cl.textContent=(d.ota_current_label||'--')+' · '+otaFwBrief(d.ota_current_version||d.firmware_version,d.ota_current_build_id||d.firmware_build_id,d.ota_current_build_at||d.firmware_build_at);
    var fl=document.getElementById('otaFbLabel');if(fl)fl.textContent=(d.ota_fallback_label||'--')+' · '+otaFwBrief(d.ota_fallback_version,d.ota_fallback_build_id,d.ota_fallback_build_at);
    _otaConfirmDeadline=Date.now()+(d.ota_confirm_remaining_ms||0);
    if(!_otaConfirmTimer)_otaConfirmTimer=setInterval(function(){
      var el=document.getElementById('otaConfirmCountdown');
      if(el)el.textContent=fmtCountdown(_otaConfirmDeadline-Date.now());
    },500);
  } else if(state===4&&rb){
    rb.style.display='block';if(cb)cb.style.display='none';
    _otaRollbackDeadline=Date.now()+(d.ota_rollback_remaining_ms||0);
    if(!_otaRollbackTimer)_otaRollbackTimer=setInterval(function(){
      var el=document.getElementById('otaRollbackCountdown');
      if(el)el.textContent=fmtCountdown(_otaRollbackDeadline-Date.now());
    },500);
  } else {
    if(cb)cb.style.display='none';if(rb)rb.style.display='none';
    if(_otaConfirmTimer){clearInterval(_otaConfirmTimer);_otaConfirmTimer=null;}
    if(_otaRollbackTimer){clearInterval(_otaRollbackTimer);_otaRollbackTimer=null;}
  }
}
function otaConfirmFw(){
  if(!confirm('이 펌웨어를 계속 사용합니다. 확인하시겠습니까?'))return;
  fetch('/api/ota-confirm',{method:'POST'}).then(function(r){return r.json();}).then(function(d){
    if(d.ok){var b=document.getElementById('otaConfirmBanner');if(b)b.style.display='none';}
  }).catch(function(){alert('요청 실패');});
}
function otaRollbackFw(){
  if(!confirm('이전 펌웨어로 되돌립니다. 기기가 재부팅됩니다.'))return;
  fetch('/api/ota-rollback',{method:'POST'}).catch(function(){});
}
function otaRecoveryConfirmFw(){
  if(!confirm('복구가 완료되었음을 확인합니다.'))return;
  fetch('/api/ota-recovery-confirm',{method:'POST'}).then(function(r){return r.json();}).then(function(d){
    if(d.ok){var b=document.getElementById('otaRollbackConfirmBanner');if(b)b.style.display='none';}
  }).catch(function(){alert('요청 실패');});
}
function otaEnterRecoveryFw(){
  if(!confirm('OTA 복구모드로 전환합니다. CAN 기능이 비활성화됩니다.'))return;
  fetch('/api/ota-enter-recovery',{method:'POST'}).catch(function(){});
}
function scheduleOtaReload(st){
  if(_otaReloadTimer)return;
  var started=Date.now();
  function waitReady(){
    fetch('/api/status?ota_reload='+Date.now(),{cache:'no-store'}).then(function(r){
      if(!r.ok)throw new Error('status');
      if(st)st.textContent='재연결됨. 새 페이지 로드 중...';
      location.replace('/?ota_reload='+Date.now());
    }).catch(function(){
      if(st)st.textContent='업로드 완료. 보드 재부팅 중... 재연결 대기 '+Math.floor((Date.now()-started)/1000)+'초';
      _otaReloadTimer=setTimeout(waitReady,2000);
    });
  }
  _otaReloadTimer=setTimeout(waitReady,9000);
}
function uploadOta(){
  var file=document.getElementById('otaFile').files[0];
  var st=document.getElementById('otaStatus');
  var btn=document.getElementById('otaBtn');
  var prog=document.getElementById('otaProgress');
  if(!file){if(st)st.textContent='펌웨어 파일을 선택하세요';return;}
  if(!confirm('펌웨어를 업로드합니다. 업로드 중 전원을 끄지 마세요.'))return;
  if(btn)btn.disabled=true;
  _otaUploadInProgress=true;
  if(st)st.textContent='업로드 중...';
  if(prog)prog.value=0;
  var xhr=new XMLHttpRequest();
  xhr.timeout=300000;
  xhr.open('POST','/api/ota');
  xhr.setRequestHeader('Content-Type','application/octet-stream');
  xhr.setRequestHeader('X-Upload-Epoch-Ms',String(Date.now()));
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable&&prog)prog.value=Math.round(e.loaded/e.total*100);
  };
  xhr.onload=function(){
    if(xhr.status>=200&&xhr.status<300){if(st)st.textContent='업로드 완료. 보드 재부팅 중...';if(prog)prog.value=100;scheduleOtaReload(st);}
    else{if(st)st.textContent='업로드 실패: '+xhr.responseText;if(btn)btn.disabled=false;}
  };
  xhr.onerror=function(){if(st)st.textContent='업로드 실패';if(btn)btn.disabled=false;};
  xhr.ontimeout=function(){if(st)st.textContent='업로드 실패: 시간 초과';if(btn)btn.disabled=false;};
  xhr.onabort=function(){if(st)st.textContent='업로드 취소됨';if(btn)btn.disabled=false;};
  xhr.onloadend=function(){if(!(xhr.status>=200&&xhr.status<300)){_otaUploadInProgress=false;if(btn)btn.disabled=false;}};
  xhr.send(file);
}
function rebootDevice(){
  if(!confirm('재부팅합니다.'))return;
  fetch('/api/reboot',{method:'POST'}).catch(function(){});
}
async function resetNvsAll(){
  var st=document.getElementById('otaStatus');
  var btn=document.getElementById('nvsResetBtn');
  if(!confirm('모든 저장 설정을 삭제하고 재부팅합니다. OTA 확인/롤백 정보도 삭제됩니다. WiFi/AP 비밀번호는 유지됩니다. 계속할까요?'))return;
  if(!confirm('정말 삭제할까요? 되돌릴 수 없습니다.'))return;
  if(btn)btn.disabled=true;
  if(st)st.textContent='NVS 초기화 중...';
  try{
    var r=await fetch('/api/nvs-reset',{method:'POST'});
    var text=await r.text();
    if(!r.ok)throw new Error(text||'NVS 초기화 실패');
    if(st)st.textContent='NVS 초기화 완료. 재부팅 중...';
  }catch(e){
    if(st)st.textContent='NVS 초기화 실패: '+(e&&e.message?e.message:'요청 실패');
    if(btn)btn.disabled=false;
  }
}

var STATUS_POLL_MS=1000;
setInterval(poll,STATUS_POLL_MS);poll();
setInterval(renderUptimeTick,250);
setTimeout(startDetailPolling,1000);

/* wall-clock 동기화: 페이지 로드 시 브라우저 epoch_ms를 디바이스에 POST.
 * 디바이스는 이를 baseline으로 저장해 logRing 타임스탬프를 ISO 형식으로 변환.
 * 페이지 재로드/새 클라이언트 접속 시마다 갱신 (드리프트 보정).
 */
(function syncDeviceTime(){
  try{
    fetch('/api/time?ms='+Date.now(), {method:'POST'}).catch(function(){});
  }catch(e){}
})();
setInterval(function(){
  try{ fetch('/api/time?ms='+Date.now(), {method:'POST'}).catch(function(){}); }catch(e){}
}, 60000); // 1분마다 재동기화 (디바이스 millis 드리프트 보정)


/* ── Collapsible cards ── */
(function(){
  document.querySelectorAll('.card').forEach(function(card){
    if(card.classList.contains('no-collapse'))return;
    var hdr=card.querySelector(':scope>h2');
    if(!hdr)return;
    var titleEl=hdr.tagName==='H2'?hdr:hdr.querySelector('h2');
    var title=titleEl?titleEl.textContent.trim().slice(0,40):'';
    var lsKey='tcan-card-'+title.replace(/[^\w\uAC00-\uD7A3]/g,'-').replace(/-+/g,'-').toLowerCase();
    try{
      var saved=localStorage.getItem(lsKey);
      if(saved==='0')card.classList.remove('collapsed');
      else if(saved==='1')card.classList.add('collapsed');
    }catch(e){}
    var btn=document.createElement('button');
    btn.className='collapse-btn';
    btn.textContent=card.classList.contains('collapsed')?'\u25BC':'\u25B2';
    btn.addEventListener('click',function(){
      var c=card.classList.toggle('collapsed');
      btn.textContent=c?'\u25BC':'\u25B2';
      try{localStorage.setItem(lsKey,c?'1':'0');}catch(e){}
      // B채널 카드 열릴 때 BUS-OFF 로그 자동 로드
      if(!c && card.id==='bChCard')loadBusOffLog();
    });
    hdr.appendChild(btn);
  });
})();
(function(){
  var saved='main';
  try{saved=localStorage.getItem('tcan-view')||'main';}catch(e){}
  showView(saved);
})();
</script>
</body>
</html>)rawliteral";

// ── OTA 복구모드 전용 최소 UI ─────────────────────────────────────────────────
// gOtaRecoveryModeActive==true 일 때 rootHandler 가 이 HTML 을 응답한다.
// CAN 기능 없이 OTA 업로드 + 재부팅 + 상태 확인만 제공한다.
const char WEB_RECOVERY_UI_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="ko" data-theme="dark">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>TeslaCAN — OTA 복구모드</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#1a1a2e;--card:#16213e;--bd:#27304d;--tx:#e0e0e0;--tx2:#cfd5e6;--tx3:#7b84a3;
  --acc:#00d4aa;--err:#ff6b6b;--warn:#f5a623;--ok:#3dba72;--btn:#00d4aa;--btn-tx:#08111b;}
body{background:var(--bg);color:var(--tx);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:24px 16px;}
h1{font-size:1.4em;color:var(--warn);margin-bottom:4px}
.sub{font-size:.85em;color:var(--tx3);margin-bottom:20px}
.card{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:20px;width:100%;max-width:480px;margin-bottom:16px}
.card h2{font-size:1em;margin-bottom:12px;color:var(--tx2)}
.btn{display:inline-flex;align-items:center;justify-content:center;padding:10px 20px;border-radius:8px;border:none;background:var(--btn);color:var(--btn-tx);font-size:.9em;font-weight:700;cursor:pointer;width:100%;margin-top:8px}
.btn:disabled{opacity:.5;cursor:not-allowed}
.btn.danger{background:var(--err);color:#fff}
input[type=file]{width:100%;padding:8px;background:#0f0f23;color:var(--tx);border:1px solid var(--bd);border-radius:6px;font-size:.85em;margin-top:8px}
.status{font-size:.82em;color:var(--tx3);margin-top:8px;min-height:1.2em}
.warn-box{background:#1a1200;border:1px solid var(--warn);border-radius:8px;padding:12px;font-size:.82em;color:var(--warn);margin-bottom:12px}
progress{width:100%;height:8px;border-radius:4px;accent-color:var(--acc);margin-top:6px;display:none}
.ota-status-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.stat{background:#0f0f23;border:1px solid var(--bd);border-radius:8px;padding:10px}
.stat-lbl{font-size:.72em;color:var(--tx3);margin-bottom:4px}
.stat-val{font-size:.84em;color:var(--tx2);word-break:break-word}
.v-dim{color:var(--tx3)}
.v-acc{color:var(--acc)}
.v-warn{color:var(--warn)}
.v-err{color:var(--err)}
.v-ok{color:var(--ok)}
</style>
</head>
<body>
<h1>&#x26A0; OTA 복구모드</h1>
<p class="sub">CAN 기능이 비활성화된 상태입니다. 새 펌웨어를 업로드하거나 재부팅하세요.</p>

<div class="card">
  <h2>&#x1F4E6; 펌웨어 업로드</h2>
  <div class="warn-box">복구모드에서도 OTA 업로드가 가능합니다. 올바른 .bin 파일을 선택하세요.</div>
  <input type="file" id="otaFile" accept=".bin">
  <progress id="otaProgress" value="0" max="100"></progress>
  <button class="btn" id="otaBtn" onclick="uploadOta()">&#x2B06; 펌웨어 업로드</button>
  <div class="status" id="otaStatus"></div>
</div>

<div class="card">
  <h2>&#x1F504; 재부팅</h2>
  <p style="font-size:.82em;color:var(--tx3);margin-bottom:8px">재부팅 후 OTA 상태가 초기화되면 정상 모드로 진입할 수 있습니다.</p>
  <button class="btn danger" onclick="rebootDevice()">&#x1F501; 재부팅</button>
  <div class="status" id="rebootStatus"></div>
</div>

<div class="card">
  <h2>&#x2139; 현재 상태</h2>
  <div class="ota-status-grid">
    <div class="stat"><div class="stat-lbl">현재 파티션</div><div class="stat-val v-dim" id="recCurrent">--</div></div>
    <div class="stat"><div class="stat-lbl">이전 파티션</div><div class="stat-val v-dim" id="recFallback">--</div></div>
    <div class="stat"><div class="stat-lbl">현재 펌웨어</div><div class="stat-val v-acc" id="recCurrentFw">--</div></div>
    <div class="stat"><div class="stat-lbl">이전 펌웨어</div><div class="stat-val v-warn" id="recFallbackFw">--</div></div>
    <div class="stat"><div class="stat-lbl">현재 빌드 시각</div><div class="stat-val v-dim" id="recCurrentBuilt">--</div></div>
    <div class="stat"><div class="stat-lbl">이전 빌드 시각</div><div class="stat-val v-dim" id="recFallbackBuilt">--</div></div>
    <div class="stat"><div class="stat-lbl">업로드 시각</div><div class="stat-val v-warn" id="recUploadAt">--</div></div>
    <div class="stat"><div class="stat-lbl">확인 상태</div><div class="stat-val v-dim" id="recState">--</div></div>
    <div class="stat"><div class="stat-lbl">복구 모드</div><div class="stat-val v-err" id="recRecovery">--</div></div>
    <div class="stat"><div class="stat-lbl">현재 Build ID</div><div class="stat-val v-dim" id="recBuildId">--</div></div>
  </div>
</div>

<script>
var _recOtaReloadTimer=null;
function recShortDateTime(text){
  var s=String(text||'').trim();
  if(!s)return '--';
  var iso=s.match(/^(\d{4})-(\d{2})-(\d{2})[T ](\d{2}):(\d{2}):(\d{2})/);
  if(iso)return iso[1].slice(2)+'-'+iso[2]+'-'+iso[3]+' '+iso[4]+':'+iso[5]+':'+iso[6];
  var months={Jan:'01',Feb:'02',Mar:'03',Apr:'04',May:'05',Jun:'06',Jul:'07',Aug:'08',Sep:'09',Oct:'10',Nov:'11',Dec:'12'};
  var c=s.match(/^(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+(\d{1,2})\s+(\d{4})\s+(\d{2}):(\d{2}):(\d{2})/);
  if(c)return c[3].slice(2)+'-'+months[c[1]]+'-'+('0'+c[2]).slice(-2)+' '+c[4]+':'+c[5]+':'+c[6];
  return s.length>19?s.slice(0,19):s;
}
function recBuildDateOnly(bid,builtAt){
  var m=String(bid||'').match(/FW\d+-(\d{2})(\d{2})(\d{2})\d{4}/);
  if(m)return m[1]+'-'+m[2]+'-'+m[3];
  var dt=recShortDateTime(builtAt);
  return dt==='--'?'':dt.slice(0,8);
}
function recFwBrief(ver,bid,builtAt){
  var v=ver||'--',d=recBuildDateOnly(bid,builtAt);
  return d?v+' · '+d:v;
}
function setRecStat(id,text,cls){
  var el=document.getElementById(id);
  if(el){el.textContent=text||'--';el.className='stat-val '+(cls||'v-dim');}
}
function renderRecInfo(d){
  var state=d.ota_pending_state||0;
  var stateText=state===2?'확인 대기':state===4?'복구 확인':state===5?'복구 모드':'정상';
  var stateLevel=state===0?'v-ok':state===2?'v-warn':state===4?'v-warn':'v-err';
  setRecStat('recCurrent',d.ota_current_label||'--','v-dim');
  setRecStat('recFallback',d.ota_fallback_label||'--','v-dim');
  setRecStat('recCurrentFw',recFwBrief(d.ota_current_version||d.firmware_version,d.ota_current_build_id||d.firmware_build_id,d.ota_current_build_at||d.firmware_build_at),'v-acc');
  setRecStat('recFallbackFw',recFwBrief(d.ota_fallback_version,d.ota_fallback_build_id,d.ota_fallback_build_at),d.ota_fallback_version?'v-warn':'v-dim');
  setRecStat('recCurrentBuilt',recShortDateTime(d.ota_current_build_at||d.firmware_build_at),'v-dim');
  setRecStat('recFallbackBuilt',recShortDateTime(d.ota_fallback_build_at),'v-dim');
  setRecStat('recUploadAt',d.ota_upload_at||'--',d.ota_upload_at?'v-warn':'v-dim');
  setRecStat('recState',stateText,stateLevel);
  setRecStat('recRecovery',d.ota_recovery_mode?'ON':'OFF',d.ota_recovery_mode?'v-err':'v-ok');
  setRecStat('recBuildId',d.firmware_build_id||d.ota_current_build_id||'--','v-dim');
}
async function loadRecInfo(){
  try{
    var r=await fetch('/api/status?recovery='+Date.now(),{cache:'no-store'});
    var d=await r.json();
    renderRecInfo(d);
  }catch(e){setRecStat('recState','상태 조회 실패','v-err');}
}
loadRecInfo();

function scheduleRecoveryOtaReload(st){
  if(_recOtaReloadTimer)return;
  var started=Date.now();
  function waitReady(){
    fetch('/api/status?ota_reload='+Date.now(),{cache:'no-store'}).then(function(r){
      if(!r.ok)throw new Error('status');
      if(st)st.textContent='재연결됨. 새 페이지 로드 중...';
      location.replace('/?ota_reload='+Date.now());
    }).catch(function(){
      if(st)st.textContent='업로드 완료. 보드 재부팅 중... 재연결 대기 '+Math.floor((Date.now()-started)/1000)+'초';
      _recOtaReloadTimer=setTimeout(waitReady,2000);
    });
  }
  _recOtaReloadTimer=setTimeout(waitReady,9000);
}

function uploadOta(){
  var file=document.getElementById('otaFile').files[0];
  var st=document.getElementById('otaStatus');
  var btn=document.getElementById('otaBtn');
  var prog=document.getElementById('otaProgress');
  var resetBtn=function(){btn.disabled=false;btn.innerHTML='&#x2B06; 펌웨어 업로드';};
  if(!file){st.textContent='파일을 선택해주세요.';return;}
  if(!confirm('펌웨어를 업로드합니다. 업로드 중 전원을 끄지 마세요.'))return;
  btn.disabled=true;btn.textContent='업로드 중...';
  prog.style.display='block';prog.value=0;
  st.textContent='업로드 시작...';
  var xhr=new XMLHttpRequest();
  xhr.timeout=300000;
  xhr.open('POST','/api/ota');
  xhr.setRequestHeader('Content-Type','application/octet-stream');
  xhr.setRequestHeader('X-Upload-Epoch-Ms',String(Date.now()));
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      prog.value=Math.round(e.loaded/e.total*100);
      st.textContent='업로드 중: '+prog.value+'%';
    }
  };
  xhr.onload=function(){
    if(xhr.status<200||xhr.status>=300){st.textContent='실패: '+(xhr.responseText||xhr.status);resetBtn();return;}
    try{var d=JSON.parse(xhr.responseText);
      if(d.ok){st.textContent='업로드 완료 — 재부팅 중...';prog.value=100;scheduleRecoveryOtaReload(st);}
      else{st.textContent='실패: '+(d.error||'알 수 없음');resetBtn();}
    }catch(ex){st.textContent='응답 파싱 오류';resetBtn();}
  };
  xhr.onerror=function(){st.textContent='네트워크 오류';resetBtn();};
  xhr.ontimeout=function(){st.textContent='업로드 실패: 시간 초과';resetBtn();};
  xhr.onabort=function(){st.textContent='업로드 취소됨';resetBtn();};
  xhr.onloadend=function(){if(!(xhr.status>=200&&xhr.status<300))resetBtn();};
  xhr.send(file);
}
function rebootDevice(){
  if(!confirm('재부팅합니다.'))return;
  var st=document.getElementById('rebootStatus');
  st.textContent='재부팅 중...';
  fetch('/api/reboot',{method:'POST'}).catch(function(){});
}
</script>
</body>
</html>)rawliteral";
