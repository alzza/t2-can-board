// include/web_ui.h
#pragma once

const char WEB_UI_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#1a1a2e">
<link rel="icon" href="data:,">
<title>TeslaCAN</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{min-height:100%}
body{
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif;
  background:#1a1a2e;
  color:#e0e0e0;
  padding:16px;
  margin:0 auto;
  width:100%;
  max-width:520px;
  overflow-x:hidden
}
h1{
  font-size:1.3em;
  color:#00d4aa;
  margin-bottom:16px;
  text-align:center
}
.card{
  background:#16213e;
  border-radius:12px;
  padding:16px;
  margin-bottom:12px;
  box-shadow:0 8px 24px rgba(0,0,0,.18)
}
.card h2{
  font-size:.85em;
  color:#7b84a3;
  margin-bottom:10px;
  text-transform:uppercase;
  letter-spacing:.08em
}
.row{
  display:flex;
  align-items:center;
  justify-content:space-between;
  gap:12px;
  padding:8px 0
}
.row+.row{border-top:1px solid #1a1a2e}
.labelWrap{
  display:flex;
  flex:1 1 auto;
  flex-direction:column;
  gap:2px;
  min-width:0
}
.label{
  color:#cfd5e6;
  font-size:.92em;
  line-height:1.3;
  overflow-wrap:anywhere
}
.meta{
  color:#7b84a3;
  font-size:.72em;
  line-height:1.35
}
.val{
  flex:0 0 auto;
  font-weight:600;
  font-size:1.05em;
  text-align:right
}
.on{color:#00d4aa}
.off{color:#555}
.sw{
  position:relative;
  flex:0 0 auto;
  width:48px;
  height:26px
}
.sw input{opacity:0;width:0;height:0}
.sl{
  position:absolute;
  cursor:pointer;
  inset:0;
  background:#333;
  border-radius:26px;
  transition:.2s ease
}
.sl:before{
  content:"";
  position:absolute;
  height:20px;
  width:20px;
  left:3px;
  bottom:3px;
  background:#888;
  border-radius:50%;
  transition:.2s ease
}
input:checked+.sl{background:#00d4aa}
input:checked+.sl:before{transform:translateX(22px);background:#fff}
input:disabled+.sl{background:#202438;cursor:not-allowed;opacity:.55}
input:disabled+.sl:before{background:#5f6680}
#log{
  background:#0f0f23;
  border-radius:8px;
  padding:10px;
  height:200px;
  overflow-y:auto;
  font-family:"SF Mono",Monaco,Consolas,monospace;
  font-size:.75em;
  color:#a8b2d8
}
.le{
  padding:3px 0;
  border-bottom:1px solid #151528;
  overflow-wrap:anywhere
}
.ts{color:#6a7392;margin-right:6px}
.dot{
  width:8px;
  height:8px;
  border-radius:50%;
  display:inline-block
}
.dot.on{background:#00d4aa;box-shadow:0 0 6px #00d4aa}
.dot.off{background:#555}
.err{
  color:#ff6b6b;
  text-align:center;
  font-size:.8em;
  padding:8px;
  display:none
}
.btn{
  width:100%;
  margin-top:10px;
  padding:12px 14px;
  border:0;
  border-radius:10px;
  background:#00d4aa;
  color:#08111b;
  font-weight:700;
  font-size:.95em
}
.btn:disabled{background:#2e3755;color:#7a849f}
.file{
  width:100%;
  padding:12px;
  border:1px solid #27304d;
  border-radius:10px;
  background:#0f0f23;
  color:#cbd3f0
}
.hint{
  color:#7b84a3;
  font-size:.75em;
  line-height:1.4
}
#otaStatus{
  margin-top:10px;
  font-size:.8em;
  color:#a8b2d8;
  min-height:1.2em
}
@media (max-width:380px){
  body{padding:12px}
  .card{padding:14px}
  .label{font-size:.88em}
  .meta{font-size:.69em}
}
</style>
</head>
<body>
<h1>TeslaCAN</h1>
<div id="connErr" class="err">Connection lost. Retrying...</div>

<div class="card">
  <h2>Status</h2>
  <div class="row"><span class="label">FSD Active</span><span class="val" id="fsd">--</span></div>
  <div class="row"><span class="label">Bypass TLSSC</span><span class="val" id="ffsd">--</span></div>
  <div class="row"><span class="label">ISA Chime Suppress</span><span class="val" id="isa">--</span></div>
  <div class="row"><span class="label">Emergency Detection</span><span class="val" id="evd">--</span></div>
  <div class="row"><span class="label">Enhanced Autopilot</span><span class="val" id="eap">--</span></div>
  <div class="row"><span class="label">Autosteer Nag Killer</span><span class="val" id="nag">--</span></div>
  <div class="row"><span class="label">Speed Profile</span><span class="val" id="prof">--</span></div>
  <div class="row"><span class="label">Speed Offset</span><span class="val" id="soff">--</span></div>
  <div class="row"><span class="label">Uptime</span><span class="val" id="up">--</span></div>
</div>

<div class="card">
  <h2>CAN Bus</h2>
  <div class="row"><span class="label">State</span><span class="val" id="canst">--</span></div>
  <div class="row"><span class="label">Frames Received</span><span class="val" id="canfr">0</span></div>
  <div class="row"><span class="label">Frames Modified</span><span class="val" id="canfs">0</span></div>
  <div class="row"><span class="label">RX Errors</span><span class="val" id="canrx">0</span></div>
  <div class="row"><span class="label">TX Errors</span><span class="val" id="cantx">0</span></div>
  <div class="row"><span class="label">Bus Errors</span><span class="val" id="canbe">0</span></div>
  <div class="row"><span class="label">RX Missed</span><span class="val" id="canrm">0</span></div>
</div>

<div class="card">
  <h2>Controls</h2>
  <div class="row">
    <div class="labelWrap">
      <span class="label">Bypass TLSSC</span>
      <span class="meta" id="metaFsd">--</span>
    </div>
    <label class="sw"><input type="checkbox" id="tFsd" onchange="togSwitch('/api/bypass-tlssc',this,'Bypass TLSSC requirement?')"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">ISA Speed Chime Suppress</span>
      <span class="meta" id="metaIsa">--</span>
    </div>
    <label class="sw"><input type="checkbox" id="tIsa" onchange="togSwitch('/api/isa-speed-chime-suppress',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">Emergency Vehicle Detection</span>
      <span class="meta" id="metaEvd">--</span>
    </div>
    <label class="sw"><input type="checkbox" id="tEvd" onchange="togSwitch('/api/emergency-vehicle-detection',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">Enhanced Autopilot</span>
      <span class="meta" id="metaEap">--</span>
    </div>
    <label class="sw"><input type="checkbox" id="tEap" onchange="togSwitch('/api/enhanced-autopilot',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">Autosteer Nag Killer</span>
      <span class="meta" id="metaNag">--</span>
    </div>
    <label class="sw"><input type="checkbox" id="tNag" onchange="togSwitch('/api/nag-killer',this)"><span class="sl"></span></label>
  </div>
  <div class="row">
    <div class="labelWrap">
      <span class="label">Enable Log</span>
      <span class="meta">Serial and web log polling</span>
    </div>
    <label class="sw"><input type="checkbox" id="tLog" checked onchange="togLog(this)"><span class="sl"></span></label>
  </div>
</div>

<div class="card">
  <h2>OTA Update</h2>
  <p class="hint">Upload a compiled firmware binary to flash the device over WiFi. The board will reboot automatically after a successful update.</p>
  <input class="file" type="file" id="otaFile" accept=".bin,application/octet-stream">
  <button class="btn" id="otaBtn" onclick="uploadOta()">Upload Firmware</button>
  <div id="otaStatus"></div>
</div>

<div class="card">
  <h2>Log</h2>
  <div id="log"></div>
</div>

<script>
var logSince=0,errCount=0;
var profNames=['Chill','Normal','Hurry','Max','Sloth'];

function fmt(s){
  var h=Math.floor(s/3600),m=Math.floor((s%3600)/60),sec=s%60;
  return h+':'+(m<10?'0':'')+m+':'+(sec<10?'0':'')+sec;
}

function setDot(el,on){
  el.textContent='';
  var d=document.createElement('span');
  d.className='dot '+(on?'on':'off');
  el.appendChild(d);
  el.appendChild(document.createTextNode(' '+(on?'Active':'Off')));
  el.className='val '+(on?'on':'off');
}

function setFeatureState(el,feature){
  if(!feature||!feature.supported){
    el.textContent='Unavailable';
    el.className='val off';
    return;
  }
  setDot(el,feature.enabled);
}

function setFeatureToggle(toggleId,metaId,feature){
  var toggle=document.getElementById(toggleId);
  var meta=document.getElementById(metaId);
  if(!feature){
    toggle.checked=false;
    toggle.disabled=true;
    meta.textContent='Unavailable';
    return;
  }
  toggle.checked=!!feature.enabled;
  toggle.disabled=!feature.supported;
  if(!feature.supported){
    meta.textContent='Not available in this build';
  }else if(feature.build_enabled){
    meta.textContent='Included in this build';
  }else{
    meta.textContent='Runtime switch';
  }
}

function setOtaStatus(msg,isErr){
  var el=document.getElementById('otaStatus');
  el.textContent=msg||'';
  el.style.color=isErr?'#ff6b6b':'#a8b2d8';
}

async function poll(){
  try{
    var r=await fetch('/api/status?log_since='+logSince);
    if(!r.ok)throw new Error('status');
    var d=await r.json();
    var f=d.features||{};

    setDot(document.getElementById('fsd'),d.fsd_enabled);
    setFeatureState(document.getElementById('ffsd'),f.bypass_tlssc_requirement);
    setFeatureState(document.getElementById('isa'),f.isa_speed_chime_suppress);
    setFeatureState(document.getElementById('evd'),f.emergency_vehicle_detection);
    setFeatureState(document.getElementById('eap'),f.enhanced_autopilot);
    setFeatureState(document.getElementById('nag'),f.nag_killer);
    document.getElementById('prof').textContent=profNames[d.speed_profile]||('P'+d.speed_profile);
    document.getElementById('soff').textContent=d.speed_offset;
    document.getElementById('up').textContent=fmt(d.uptime_s);

    setFeatureToggle('tFsd','metaFsd',f.bypass_tlssc_requirement);
    setFeatureToggle('tIsa','metaIsa',f.isa_speed_chime_suppress);
    setFeatureToggle('tEvd','metaEvd',f.emergency_vehicle_detection);
    setFeatureToggle('tEap','metaEap',f.enhanced_autopilot);
    setFeatureToggle('tNag','metaNag',f.nag_killer);
    document.getElementById('tLog').checked=d.enable_print;

    if(d.can){
      var cs=document.getElementById('canst');
      cs.textContent='';
      var cd=document.createElement('span');
      cd.className='dot '+(d.can.state==='RUNNING'?'on':'off');
      cs.appendChild(cd);
      cs.appendChild(document.createTextNode(' '+d.can.state));
      cs.className='val '+(d.can.state==='RUNNING'?'on':(d.can.state==='BUS_OFF'?'off':''));
      document.getElementById('canfr').textContent=d.can.frames_received;
      document.getElementById('canfs').textContent=d.can.frames_sent;
      document.getElementById('canrx').textContent=d.can.rx_errors;
      document.getElementById('cantx').textContent=d.can.tx_errors;
      document.getElementById('canbe').textContent=d.can.bus_errors;
      document.getElementById('canrm').textContent=d.can.rx_missed;
    }

    if(d.logs&&d.logs.length){
      var logEl=document.getElementById('log');
      for(var i=0;i<d.logs.length;i++){
        var e=document.createElement('div');
        e.className='le';
        var ts=document.createElement('span');
        ts.className='ts';
        ts.textContent=fmt(Math.floor(d.logs[i].ts/1000));
        e.appendChild(ts);
        e.appendChild(document.createTextNode(d.logs[i].msg));
        logEl.insertBefore(e,logEl.firstChild);
      }
      while(logEl.children.length>100)logEl.removeChild(logEl.lastChild);
    }

    logSince=d.log_head;
    errCount=0;
    document.getElementById('connErr').style.display='none';
  }catch(e){
    errCount++;
    if(errCount>3)document.getElementById('connErr').style.display='block';
  }
}

async function togSwitch(path,el,confirmMsg){
  if(el.disabled)return;
  if(confirmMsg&&el.checked&&!confirm(confirmMsg)){el.checked=false;return;}
  try{
    var r=await fetch(path,{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({enabled:el.checked})
    });
    if(!r.ok)throw new Error('toggle');
  }catch(e){
    el.checked=!el.checked;
  }
}

async function togLog(el){
  try{
    var r=await fetch('/api/enable-print',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({enabled:el.checked})
    });
    if(!r.ok)throw new Error('log');
  }catch(e){
    el.checked=!el.checked;
  }
}

function uploadOta(){
  var file=document.getElementById('otaFile').files[0];
  if(!file){setOtaStatus('Select a firmware file first',true);return;}
  if(!confirm('Flash '+file.name+' and reboot the device?'))return;

  var btn=document.getElementById('otaBtn');
  btn.disabled=true;
  setOtaStatus('Uploading 0%',false);

  var xhr=new XMLHttpRequest();
  xhr.open('POST','/api/ota');
  xhr.setRequestHeader('Content-Type','application/octet-stream');
  xhr.upload.onprogress=function(ev){
    if(ev.lengthComputable){
      setOtaStatus('Uploading '+Math.round((ev.loaded/ev.total)*100)+'%',false);
    }
  };
  xhr.onload=function(){
    btn.disabled=false;
    if(xhr.status>=200&&xhr.status<300){
      setOtaStatus('Upload complete. Device is rebooting...',false);
    }else{
      setOtaStatus(xhr.responseText||'OTA upload failed',true);
    }
  };
  xhr.onerror=function(){
    btn.disabled=false;
    setOtaStatus('OTA upload failed',true);
  };
  xhr.send(file);
}

setInterval(poll,500);
poll();
</script>
</body>
</html>)rawliteral";
