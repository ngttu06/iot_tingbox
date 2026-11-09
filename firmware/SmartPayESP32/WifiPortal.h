#pragma once
#include <Arduino.h>

// Entire portal is served from flash; no Internet, CDN or filesystem required.
static const char WIFI_PORTAL_HTML[] PROGMEM = R"PORTAL(<!doctype html>
<html lang="vi">
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="light"><title>Cài đặt Wi-Fi | TingBox</title>
<style>
:root{--bg:#f1f5f9;--card:#fff;--ink:#172b45;--muted:#526178;--primary:#185adb;--border:#8b9ab0;--soft:#eaf1ff;--error:#a51d32;--success:#17653c}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.55 system-ui,-apple-system,"Segoe UI",sans-serif}main{max-width:520px;margin:auto;padding:28px 16px 32px;padding-bottom:max(32px,env(safe-area-inset-bottom))}.brand{display:flex;gap:10px;align-items:center;font-size:23px;font-weight:800;letter-spacing:-.7px}.brand svg{width:36px;height:36px;color:var(--primary)}.tag{font-size:12px;letter-spacing:1.5px;text-transform:uppercase;color:var(--muted);margin:2px 0 24px 46px}h1{font-size:30px;line-height:1.2;letter-spacing:-.8px;margin:0 0 10px}h2{font-size:18px;margin:0}p{margin:8px 0 16px}.intro,.hint,footer{color:var(--muted)}.hint{font-size:14px;margin:8px 0 0}.device{display:inline-block;background:var(--soft);color:#214d93;padding:6px 12px;border-radius:30px;font-size:13px;overflow-wrap:anywhere;margin:0 0 20px}.card{background:var(--card);border:1px solid #dce3ed;border-radius:20px;padding:22px;margin-bottom:16px;box-shadow:0 8px 28px #172b4506}.heading{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:10px}.step{display:inline-grid;place-items:center;width:28px;height:28px;border-radius:50%;background:var(--soft);color:var(--primary);margin-right:8px;font-size:14px}.status{border-left:4px solid var(--primary);background:var(--soft);border-radius:10px;padding:14px 16px;margin-bottom:20px;overflow-wrap:anywhere}.status p{margin:4px 0 0;font-size:14px}.status[data-state=error]{border-color:var(--error);background:#fff0f2}.status[data-state=success]{border-color:var(--success);background:#edf8f1}button,input{font:inherit}button,a,input{touch-action:manipulation}button{min-height:48px;border:1px solid var(--border);border-radius:10px;background:var(--card);color:var(--ink);padding:10px 14px;cursor:pointer;transition:background .18s,border-color .18s}button:hover{background:var(--soft)}button:active{background:#dbe7ff}button:disabled{cursor:wait;opacity:.6}a{color:var(--primary)}:focus-visible{outline:3px solid var(--primary);outline-offset:3px}.text-button{color:var(--primary);border:0;padding:8px;min-width:48px}.networks{display:grid;gap:8px;max-height:268px;overflow-y:auto;padding:4px;margin:8px -4px}.network{display:flex;text-align:left;justify-content:space-between;align-items:center;gap:12px;width:100%;min-height:66px}.network[aria-pressed=true]{border:2px solid var(--primary);background:var(--soft)}.network-name{font-weight:650;overflow-wrap:anywhere;min-width:0}.network small{display:block;color:var(--muted);font-size:13px;font-weight:400}.signal{font-size:12px;color:var(--muted);flex-shrink:0}.manual{display:block;margin-top:8px}label{display:block;font-weight:650;margin:18px 0 8px}input{width:100%;min-height:50px;border:1px solid var(--border);border-radius:10px;padding:12px;background:var(--card);color:var(--ink)}input[aria-invalid=true]{border:2px solid var(--error)}.password{display:flex;gap:8px}.password input{min-width:0}.password button{min-width:68px}.check{display:flex;gap:10px;align-items:center;font-size:14px;font-weight:400;min-height:48px;margin:8px 0}.check input{width:20px;min-height:20px;height:20px;accent-color:var(--primary)}.primary{width:100%;background:var(--primary);color:white;border-color:var(--primary);font-weight:700;margin-top:20px;min-height:52px}.primary:hover{background:#1048b8}.error{color:var(--error);font-size:14px;margin:6px 0 0}.notice{background:#f5f7fb;border-radius:10px;padding:12px;font-size:14px;color:var(--muted);margin-top:18px}fieldset{border:0;padding:0;margin:0;min-width:0}.help{font-size:14px}.help summary{cursor:pointer;font-weight:650;min-height:44px;padding:10px 0}.help p{color:var(--muted)}footer{text-align:center;font-size:12px;padding:6px 8px}footer a{display:inline-block;padding:12px}.hidden,[hidden]{display:none!important}@media(max-width:360px){.card{padding:16px}main{padding-inline:12px}h1{font-size:27px}}@media(prefers-reduced-motion:reduce){*{transition:none!important}}
.heading .text-button{flex-shrink:0;white-space:nowrap}.heading h2{display:flex;align-items:center}.step{flex-shrink:0}
/* Keep editable controls above iOS's 16px focus-zoom threshold, even if a parent font changes.
   Do not disable pinch zoom in the viewport: users can still enlarge the page manually. */
html{-webkit-text-size-adjust:100%;text-size-adjust:100%}
#ssid,#password{font-size:18px;font-size:max(18px,1rem);line-height:1.5}
.bank-code{text-align:center;margin:18px 0}.bank-code svg{display:block;max-width:100%;height:auto;margin:0 auto 12px;background:white}.bank-code code{display:block;overflow-wrap:anywhere;font-size:14px;font-weight:700;user-select:all}.bank-actions{display:grid;gap:10px;margin:16px 0}#bank-heading{color:var(--primary)}
</style><script src="/portal.js" defer></script></head>
<body><main>
<header><div class="brand"><svg viewBox="0 0 32 32" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" aria-hidden="true"><rect x="3" y="3" width="26" height="26" rx="8"/><path d="M8 13a12 12 0 0 1 16 0M11 17a7 7 0 0 1 10 0M14 21a3 3 0 0 1 4 0"/><path d="M16 24h.01"/></svg>TingBox</div><div class="tag">Kết nối thật đơn giản</div>
<h1>Cài đặt Wi-Fi</h1><p class="intro">Chọn Wi-Fi tại nhà hoặc cửa hàng để TingBox kết nối.</p><div class="device">Thiết bị: <strong id="ap-name">{{AP}}</strong></div></header>
<section id="status" class="status" data-state="{{STATE}}" role="status" aria-live="polite" aria-atomic="true"><strong id="status-title">Trạng thái thiết bị</strong><p id="status-message">{{STATUS}}</p></section>
<section class="card" aria-labelledby="bank-heading"><h2 id="bank-heading">Liên kết ngân hàng</h2>
<p class="hint">QR hiện sẵn khi mở trang, không cần bấm nút. Admin quét QR hoặc sao chép mã SPK bên dưới để liên kết ngân hàng.</p>
<div class="bank-code">{{BANK_QR}}<code id="bank-key">{{BANK_KEY}}</code></div>
<p class="hint">Mã này cố định và cũng là khóa API. Chỉ chia sẻ với admin liên kết, không đăng ảnh QR công khai.</p>
<p id="bank-status" role="status" aria-live="polite">Đang kiểm tra gateway…</p>
<div class="bank-actions"><button type="button" id="bank-link">Hiện lại QR trên màn hình loa</button>
<button type="button" id="bank-retry">Đã liên kết / Kiểm tra lại</button></div>
<details><summary>Đổi tài khoản hoặc phiên liên kết</summary><p class="hint">Chỉ thực hiện sau khi admin đã đổi liên kết và hàng đợi cũ được xóa trên server. Không xóa lịch sử ID đã phát.</p>
<label><input type="checkbox" id="bank-confirm"> Tôi xác nhận đã đổi liên kết trên hệ thống bank.</label>
<button type="button" id="bank-scope">Xác nhận phạm vi liên kết mới</button></details>
<details><summary>Khôi phục sau mất điện/lỗi âm thanh</summary><p class="hint">Chỉ dùng khi loa báo giao dịch dang dở sau reset. Kiểm tra và thông báo giao dịch đó thủ công trước; xác nhận này cho phép ACK, không phát lại.</p>
<label><input type="checkbox" id="bank-reviewed"> Tôi đã kiểm tra và thông báo giao dịch dang dở thủ công.</label>
<button type="button" id="bank-review">Xác nhận đã xử lý thủ công</button></details>
<noscript>QR và mã SPK vẫn dùng được khi tắt JavaScript. Bật JavaScript nếu muốn gửi thao tác kiểm tra lại tới loa.</noscript></section>
<form id="wifi-form" method="post" action="/save"><input type="hidden" name="csrf" value="{{CSRF}}">
<fieldset id="fields" {{DISABLED}}><div class="card">
<div class="heading"><h2><span class="step">1</span>Chọn mạng Wi-Fi</h2><button type="button" class="text-button" id="scan" hidden>Quét lại</button></div>
<p class="hint" id="scan-message" role="status">Nhập tên mạng bên dưới. TingBox hỗ trợ Wi-Fi 2.4 GHz.</p><div id="networks" class="networks" aria-label="Mạng Wi-Fi tìm thấy" hidden></div>
<button type="button" id="manual" class="text-button manual" hidden>Nhập tên mạng thủ công</button>
<label for="ssid">Tên Wi-Fi</label><input id="ssid" name="ssid" value="{{SSID}}" maxlength="32" required autocomplete="off" autocapitalize="none" spellcheck="false" aria-describedby="ssid-hint ssid-error"><p id="ssid-hint" class="hint">Có thể nhập trực tiếp nếu mạng bị ẩn hoặc không có trong danh sách.</p><p id="ssid-error" class="error" hidden></p>
</div><div class="card"><h2><span class="step">2</span>Nhập mật khẩu</h2>
<label for="password">Mật khẩu Wi-Fi</label><div class="password"><input id="password" name="password" type="password" maxlength="63" autocomplete="new-password" aria-describedby="password-hint password-error"><button id="show-password" type="button" aria-controls="password" aria-pressed="false" hidden>Hiện</button></div>
<label class="check" id="open-label" hidden><input id="open-network" type="checkbox">Mạng này không có mật khẩu</label><p id="password-hint" class="hint">Nhập mật khẩu của mạng đã chọn, không phải mật khẩu Wi-Fi TingBox. Để trống nếu mạng không có mật khẩu.</p><p id="password-error" class="error" hidden></p>
<button type="submit" id="connect" class="primary">Kết nối và lưu</button><div class="notice">Chỉ lưu sau khi kết nối thành công. Mật khẩu đã lưu không được hiển thị lại.</div>
</div></fieldset></form>
<noscript><p>Trình duyệt đang tắt JavaScript. Bạn vẫn có thể nhập mạng và mật khẩu, sau đó tải lại trang để xem kết quả.</p></noscript>
<details class="card help"><summary>Không kết nối được?</summary><p>Hãy chọn Wi-Fi <strong>2.4 GHz</strong>, kiểm tra mật khẩu và đặt TingBox gần bộ phát Wi-Fi.</p><p>Nếu điện thoại báo “Không có Internet”, hãy chọn giữ kết nối. Nếu bị ngắt, kết nối lại mạng <strong>{{AP}}</strong> rồi mở <a href="http://192.168.4.1/">192.168.4.1</a>.</p><p>Kết nối Wi-Fi thành công chưa có nghĩa Internet hoặc dịch vụ giao dịch đã sẵn sàng.</p></details>
<footer>Cài đặt trực tiếp trên thiết bị · Không cần Internet<br><a href="/">Tải lại trạng thái</a></footer>
</main></body></html>)PORTAL";

static const char WIFI_PORTAL_JS[] PROGMEM = R"PORTAL('use strict';
(() => {
  const $ = id => document.getElementById(id);
  const form=$('wifi-form'), fields=$('fields'), ssid=$('ssid'), password=$('password');
  let busy=fields.disabled, scanning=false, submitting=false, scanTimer, lastSuccess=false;
  const csrf=form.elements.csrf.value;
  const titles={idle:'Sẵn sàng cài đặt',connecting:'Đang kết nối…',success:'Đã kết nối và lưu Wi-Fi',error:'Cần kiểm tra lại'};
  function message(state,text){$('status').dataset.state=state;$('status-title').textContent=titles[state]||'Trạng thái thiết bị';$('status-message').textContent=text;}
  function controls(){fields.disabled=busy||submitting;$('scan').disabled=scanning;$('connect').disabled=scanning;$('connect').textContent=busy||submitting?'Đang kết nối…':'Kết nối và lưu';}
  async function request(url,options={}){
    const controller=new AbortController(), timer=setTimeout(()=>controller.abort(),15000);
    try{const r=await fetch(url,{...options,signal:controller.signal,cache:'no-store'});if(!r.ok)throw new Error(await r.text());return await r.json();}finally{clearTimeout(timer);}
  }
  function applyStatus(data){busy=data.state==='connecting';lastSuccess=data.state==='success';message(data.state,data.message);if(data.gateway)$('bank-status').textContent=data.gateway;controls();}
  async function poll(){
    try{const data=await request('/status');if(!submitting)applyStatus(data);}catch(e){
      message(lastSuccess?'success':'error',lastSuccess?'Wi-Fi đã được lưu. Nếu trang cài đặt đã đóng, hãy chuyển điện thoại về Wi-Fi thường dùng.':'Không liên lạc được với TingBox. Hãy giữ hoặc kết nối lại mạng '+$('ap-name').textContent+' rồi tải lại trạng thái.');
    }finally{setTimeout(poll,busy?2000:6000);}
  }
  function openNetwork(value){$('open-network').checked=value;password.disabled=value;if(value)password.value='';}
  function clearError(id){$(id+'-error').hidden=true;$(id).removeAttribute('aria-invalid');}
  function error(id,text){$(id+'-error').textContent=text;$(id+'-error').hidden=false;$(id).setAttribute('aria-invalid','true');$(id).focus();}
  function selectNetwork(network,button){ssid.value=network.ssid;clearError('ssid');openNetwork(network.open);password.value='';document.querySelectorAll('.network').forEach(b=>b.setAttribute('aria-pressed',String(b===button)));if(!network.open)password.focus();else $('connect').focus();}
  function renderNetworks(networks){
    $('networks').replaceChildren();$('networks').hidden=!networks.length;
    networks.forEach(network=>{
      const button=document.createElement('button'),name=document.createElement('span'),security=document.createElement('small'),signal=document.createElement('span');
      button.type='button';button.className='network';button.setAttribute('aria-pressed','false');name.className='network-name';name.textContent=network.ssid;
      security.textContent=network.open?'Không có mật khẩu':'Có mật khẩu';signal.className='signal';signal.textContent=network.rssi>=-60?'Sóng mạnh':network.rssi>=-75?'Sóng vừa':'Sóng yếu';
      name.appendChild(security);button.append(name,signal);button.addEventListener('click',()=>selectNetwork(network,button));$('networks').appendChild(button);
    });
  }
  function scanResult(data){
    if(data.state==='scanning'){scanTimer=setTimeout(readScan,1500);return;}
    scanning=false;controls();renderNetworks(data.networks||[]);
    $('scan-message').textContent=data.state==='error'?'Chưa quét được mạng. Bạn có thể quét lại hoặc nhập tên Wi-Fi thủ công.':data.networks.length?'Chạm vào một mạng Wi-Fi 2.4 GHz để chọn.':'Không tìm thấy mạng. Hãy quét lại hoặc nhập tên mạng thủ công.';
  }
  function scanFailed(){scanning=false;controls();$('scan-message').textContent='Không quét được mạng. Hãy thử lại hoặc nhập tên Wi-Fi thủ công.';}
  async function readScan(){try{scanResult(await request('/scan'));}catch(e){scanFailed();}}
  async function scan(){
    if(busy||scanning||submitting)return;scanning=true;controls();clearTimeout(scanTimer);$('scan-message').textContent='Đang tìm Wi-Fi gần đây…';
    try{scanResult(await request('/scan',{method:'POST',body:new URLSearchParams({csrf})}));}catch(e){scanFailed();}
  }
  async function bankAction(action){
    if(action==='scope' && !$('bank-confirm').checked){$('bank-status').textContent='Hãy xác nhận đã đổi liên kết trước.';return;}
    if(action==='review' && !$('bank-reviewed').checked){$('bank-status').textContent='Hãy kiểm tra và thông báo thủ công trước khi xác nhận.';return;}
    const buttons=['bank-link','bank-retry','bank-scope','bank-review'];buttons.forEach(id=>$(id).disabled=true);
    try{
      await request('/bank',{method:'POST',body:new URLSearchParams({csrf,action,confirm:action==='scope'||action==='review'?'yes':''})});
      $('bank-status').textContent=action==='link'?'Xem QR trên màn hình loa (tối đa 2 phút).':'Đã gửi yêu cầu. Chờ trạng thái gateway cập nhật.';
      $('bank-confirm').checked=false;
      $('bank-reviewed').checked=false;
    }catch(e){$('bank-status').textContent=e.message||'Không gửi được yêu cầu.';}
    finally{buttons.forEach(id=>$(id).disabled=false);}
  }
  $('bank-link').addEventListener('click',()=>bankAction('link'));
  $('bank-retry').addEventListener('click',()=>bankAction('retry'));
  $('bank-scope').addEventListener('click',()=>bankAction('scope'));
  $('bank-review').addEventListener('click',()=>bankAction('review'));
  $('scan').addEventListener('click',scan);
  $('manual').addEventListener('click',()=>{document.querySelectorAll('.network').forEach(b=>b.setAttribute('aria-pressed','false'));openNetwork(false);ssid.focus();ssid.select();});
  $('show-password').addEventListener('click',()=>{const show=password.type==='password';password.type=show?'text':'password';$('show-password').textContent=show?'Ẩn':'Hiện';$('show-password').setAttribute('aria-pressed',String(show));});
  $('open-network').addEventListener('change',()=>openNetwork($('open-network').checked));
  ssid.addEventListener('input',()=>{clearError('ssid');openNetwork(false);document.querySelectorAll('.network').forEach(b=>b.setAttribute('aria-pressed','false'));});
  password.addEventListener('input',()=>clearError('password'));
  form.addEventListener('submit',async event=>{
    event.preventDefault();if(busy||scanning||submitting)return;clearError('ssid');clearError('password');
    if(!ssid.value || new TextEncoder().encode(ssid.value).length>32){error('ssid','Tên Wi-Fi phải dài từ 1 đến 32 byte; chữ có dấu có thể chiếm nhiều byte.');return;}
    if(password.value && !/^[\x20-\x7e]{8,63}$/.test(password.value)){error('password','Mật khẩu cần 8–63 ký tự không dấu (chữ, số, khoảng trắng hoặc ký hiệu).');return;}
    const body=new URLSearchParams({csrf,ssid:ssid.value,password:password.value,ajax:'1'});
    submitting=true;controls();message('connecting','Đang gửi cấu hình. Vui lòng giữ kết nối với TingBox.');
    try{applyStatus(await request('/save',{method:'POST',body}));password.value='';password.type='password';$('show-password').textContent='Hiện';$('show-password').setAttribute('aria-pressed','false');}
    catch(e){message('error',e.name==='AbortError'?'Chưa nhận được phản hồi. Thiết bị có thể đang kết nối; hãy chờ trạng thái cập nhật trước khi thử lại.':e.message||'Không gửi được cấu hình. Hãy kiểm tra kết nối với TingBox.');}
    finally{submitting=false;controls();}
  });
  ['scan','manual','show-password','open-label'].forEach(id=>$(id).hidden=false);
  message($('status').dataset.state,$('status-message').textContent);controls();poll();if(!busy)scan();
})();)PORTAL";