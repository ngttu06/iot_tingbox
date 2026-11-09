// Real headless Chromium checks using only Node built-ins. No npm dependencies.
// Usage: node <absolute path to this file> <absolute path to chrome.exe or msedge.exe>
import {readFileSync,mkdirSync,writeFileSync} from 'node:fs';
import {createServer} from 'node:http';
import {spawn} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
import assert from 'node:assert/strict';

const root=fileURLToPath(new URL('../../',import.meta.url));
const source=readFileSync(path.join(root,'firmware/SmartPayESP32/WifiPortal.h'),'utf8');
const blocks=[...source.matchAll(/R"PORTAL\(([\s\S]*?)\)PORTAL"/g)].map(m=>m[1]);
assert.equal(blocks.length,2);
const bankSvg=readFileSync(path.join(root,'.tools/speaker-link.svg'),'utf8');
const sampleKey='SPK-b8fb059c-d258-4926-b51a-bd6e7d90120f';
const html=blocks[0].replaceAll('{{BANK_QR}}',bankSvg).replaceAll('{{BANK_KEY}}',sampleKey).replaceAll('{{AP}}','TingBox-04291c').replaceAll('{{STATE}}','idle')
  .replaceAll('{{STATUS}}','Chọn mạng Wi-Fi 2.4 GHz và nhập mật khẩu để bắt đầu.')
  .replaceAll('{{CSRF}}','test-token').replaceAll('{{DISABLED}}','').replaceAll('{{SSID}}','');
const malicious='<img src=x onerror=alert(1)>';
let state='idle',status='Chọn Wi-Fi để bắt đầu.',saved,scanEmpty=false,scanFails=false;
let scanPending=true;
const bankRequests=[];
const server=createServer(async(req,res)=>{
  res.setHeader('Cache-Control','no-store');
  res.setHeader('Content-Security-Policy',"default-src 'none'; script-src 'self'; connect-src 'self'; style-src 'unsafe-inline'; form-action 'self'; base-uri 'none'; frame-ancestors 'none'");
  if(req.url==='/'){res.setHeader('Content-Type','text/html; charset=utf-8');res.end(html);return;}
  if(req.url==='/portal.js'){res.setHeader('Content-Type','application/javascript');res.end(blocks[1]);return;}
  res.setHeader('Content-Type','application/json; charset=utf-8');
  if(req.url==='/status'){res.end(JSON.stringify({state,message:status,gateway:'Gateway test status'}));return;}
  if(req.url==='/bank'){
    let body='';for await(const chunk of req)body+=chunk;
    bankRequests.push(Object.fromEntries(new URLSearchParams(body)));
    res.end('{"ok":true}');return;
  }
  if(req.url==='/scan'){
    if(scanFails){res.end(JSON.stringify({state:'error',networks:[]}));return;}
    if(scanPending){scanPending=false;res.end(JSON.stringify({state:'scanning',networks:[]}));return;}
    res.end(JSON.stringify({state:'ready',networks:scanEmpty?[]:[{ssid:'Cửa hàng TingBox',open:false,rssi:-45},{ssid:malicious,open:true,rssi:-78}]}));return;
  }
  if(req.url==='/save'){
    let body='';for await(const chunk of req)body+=chunk;
    saved=Object.fromEntries(new URLSearchParams(body));state='connecting';status='Đang thử kết nối, tối đa 30 giây.';
    res.end(JSON.stringify({state,message:status}));return;
  }
  res.statusCode=404;res.end('{}');
});
await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
const url=`http://127.0.0.1:${server.address().port}/`;
const output=path.join(root,'.tools','wifi-portal-test');mkdirSync(output,{recursive:true});
const browser=spawn(process.argv[2]||'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe',[
  '--headless=new','--disable-gpu','--no-first-run','--no-default-browser-check',
  '--remote-debugging-port=0',`--user-data-dir=${path.join(output,'profile')}`,'about:blank'
],{stdio:['ignore','ignore','pipe']});
let ws;
try{
  const endpoint=await new Promise((resolve,reject)=>{
    const timer=setTimeout(()=>reject(new Error('Browser startup timeout')),20000);let log='';
    browser.on('error',reject);browser.stderr.on('data',chunk=>{log+=chunk;const match=log.match(/DevTools listening on (ws:\/\/\S+)/);if(match){clearTimeout(timer);resolve(match[1]);}});
  });
  ws=new WebSocket(endpoint);await new Promise((resolve,reject)=>{ws.onopen=resolve;ws.onerror=reject;});
  let sequence=0;const pending=new Map(),errors=[];
  ws.onmessage=event=>{
    const msg=JSON.parse(event.data);
    if(msg.id){const waiter=pending.get(msg.id);if(waiter){pending.delete(msg.id);msg.error?waiter.reject(new Error(msg.error.message)):waiter.resolve(msg.result);}}
    if(msg.method==='Runtime.exceptionThrown')errors.push(msg.params.exceptionDetails.text);
    if(msg.method==='Log.entryAdded' && msg.params.entry.level==='error')errors.push(msg.params.entry.text);
  };
  function send(method,params={},sessionId){return new Promise((resolve,reject)=>{const id=++sequence;pending.set(id,{resolve,reject});ws.send(JSON.stringify({id,method,params,sessionId}));});}
  const {targetId}=await send('Target.createTarget',{url:'about:blank'});
  const {sessionId}=await send('Target.attachToTarget',{targetId,flatten:true});
  const cdp=(method,params={})=>send(method,params,sessionId);
  await cdp('Runtime.enable');await cdp('Log.enable');await cdp('Page.enable');
  async function evaluate(expression){const r=await cdp('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw new Error(JSON.stringify(r.exceptionDetails));return r.result.value;}
  async function until(expression){for(let i=0;i<100;i++){if(await evaluate(expression))return;await new Promise(r=>setTimeout(r,100));}throw new Error('Timeout: '+expression);}
  const check=async(expression)=>assert.ok(await evaluate(expression),expression);
  await cdp('Emulation.setDeviceMetricsOverride',{width:375,height:812,deviceScaleFactor:1,mobile:true});
  await cdp('Page.navigate',{url});
  await until("document.getElementById('scan-message')?.textContent.includes('Đang tìm')");
  await until("document.getElementById('bank-status').textContent==='Gateway test status'");
  await evaluate("document.getElementById('bank-link').click()");
  await until("document.getElementById('bank-status').textContent.includes('Xem QR')");
  assert.equal(bankRequests.at(-1).action,'link');assert.equal(bankRequests.at(-1).csrf,'test-token');
  await evaluate("document.getElementById('bank-retry').click()");
  await until("!document.getElementById('bank-retry').disabled");
  assert.equal(bankRequests.at(-1).action,'retry');
  const count=bankRequests.length;
  await evaluate("document.getElementById('bank-scope').click();document.getElementById('bank-review').click()");
  assert.equal(bankRequests.length,count,'Dangerous actions require explicit attestation');
  await evaluate("document.getElementById('bank-confirm').checked=true;document.getElementById('bank-scope').click()");
  await until("!document.getElementById('bank-scope').disabled");
  assert.equal(bankRequests.at(-1).action,'scope');assert.equal(bankRequests.at(-1).confirm,'yes');
  await evaluate("document.getElementById('bank-reviewed').checked=true;document.getElementById('bank-review').click()");
  await until("!document.getElementById('bank-review').disabled");
  assert.equal(bankRequests.at(-1).action,'review');assert.equal(bankRequests.at(-1).confirm,'yes');
  await check(`document.getElementById('bank-key').textContent===${JSON.stringify(sampleKey)} && document.getElementById('bank-qr').getBoundingClientRect().width>100`);
  await check("document.getElementById('connect').disabled");
  await until("document.querySelectorAll('.network').length===2");
  await check("document.documentElement.lang==='vi' && document.title.includes('TingBox')");
  await check("document.querySelectorAll('#networks img').length===0");
  await evaluate("document.querySelector('.network').click()");
  await check("document.getElementById('ssid').value==='Cửa hàng TingBox' && document.activeElement.id==='password'");
  await evaluate("document.getElementById('password').value='123';document.getElementById('wifi-form').requestSubmit()");
  await check("!document.getElementById('password-error').hidden && document.activeElement.id==='password'");
  await evaluate("document.getElementById('show-password').click()");
  await check("document.getElementById('password').type==='text' && document.getElementById('show-password').getAttribute('aria-pressed')==='true'");
  await evaluate("document.querySelectorAll('.network')[1].click()");
  await check("document.getElementById('open-network').checked && document.getElementById('password').disabled");
  await evaluate("document.getElementById('manual').click();document.getElementById('ssid').value='ệ'.repeat(20);document.getElementById('wifi-form').requestSubmit()");
  await check("!document.getElementById('ssid-error').hidden");
  await evaluate("document.querySelector('.network').click();document.getElementById('password').value='12345678';document.getElementById('wifi-form').requestSubmit()");
  await until("document.getElementById('status').dataset.state==='connecting' && document.getElementById('password').value===''");
  assert.equal(saved.ssid,'Cửa hàng TingBox');assert.equal(saved.password,'12345678');assert.equal(saved.csrf,'test-token');
  await check("document.getElementById('fields').disabled");
  state='error';status='Không thể kết nối. Hãy kiểm tra mật khẩu.';
  await until("document.getElementById('status').dataset.state==='error' && !document.getElementById('fields').disabled");
  await check("document.getElementById('ssid').value==='Cửa hàng TingBox'");
  await evaluate("document.getElementById('password').value='12345678';document.getElementById('wifi-form').requestSubmit()");
  await until("document.getElementById('status').dataset.state==='connecting'");
  state='success';status='Đã lưu Wi-Fi. Bạn có thể chuyển điện thoại về mạng thường dùng.';
  await until("document.getElementById('status').dataset.state==='success'");
  // Injection was verified above. Use a readable sample name in review screenshots.
  await evaluate("document.querySelectorAll('.network-name')[1].firstChild.textContent='Wi-Fi khách'");
  for(const [width,height] of [[320,740],[375,812],[812,375],[768,1024],[1440,900]]){
    await cdp('Emulation.setDeviceMetricsOverride',{width,height,deviceScaleFactor:1,mobile:true});
    await check('document.documentElement.scrollWidth<=window.innerWidth');
    // Chromium does not emulate iOS focus auto-zoom; guard its font-size trigger instead.
    await check("!/(user-scalable\\s*=\\s*(no|0)|maximum-scale\\s*=)/i.test(document.querySelector('meta[name=viewport]').content)");
    for(const id of ['ssid','password']){
      for(const inheritedSize of ['12px','20px']){
        await evaluate(`document.getElementById('fields').style.fontSize='${inheritedSize}'`);
        await check(`parseFloat(getComputedStyle(document.getElementById('${id}')).fontSize)>=18`);
      }
      await evaluate("document.getElementById('fields').style.removeProperty('font-size')");
      const scale=await evaluate('window.visualViewport.scale');
      await evaluate(`document.getElementById('${id}').focus()`);
      await check(`document.activeElement.id==='${id}' && window.visualViewport.scale===${scale}`);
    }
    await evaluate("document.activeElement.blur()");
    const shot=await cdp('Page.captureScreenshot',{format:'png',captureBeyondViewport:true});
    writeFileSync(path.join(output,`portal-${width}.png`),Buffer.from(shot.data,'base64'));
  }
  scanEmpty=true;await evaluate("document.getElementById('scan').click()");
  await until("document.getElementById('scan-message').textContent.includes('Không tìm thấy mạng')");
  scanFails=true;await evaluate("document.getElementById('scan').click()");
  await until("document.getElementById('scan-message').textContent.includes('Chưa quét được mạng')");
  await check("!document.getElementById('connect').disabled");
  await cdp('Emulation.setEmulatedMedia',{features:[{name:'prefers-reduced-motion',value:'reduce'}]});
  await check("getComputedStyle(document.getElementById('connect')).transitionDuration==='0s'");
  await cdp('Emulation.setScriptExecutionDisabled',{value:true});await cdp('Page.navigate',{url});
  await until("document.readyState==='complete'");
  await check("!document.getElementById('ssid').disabled && document.getElementById('scan').hidden && document.querySelector('noscript').textContent.includes('JavaScript')");
  await check(`document.getElementById('bank-key').textContent===${JSON.stringify(sampleKey)} && document.getElementById('bank-qr').getBoundingClientRect().width>100`);
  const qrShot=await cdp('Page.captureScreenshot',{format:'png',captureBeyondViewport:true});
  writeFileSync(path.join(output,'bank-qr-nojs.png'),Buffer.from(qrShot.data,'base64'));
  assert.deepEqual(errors,[],'No JS or CSP errors');
  console.log('PASS: bank link/retry/scope/review actions, CSRF and confirmations; Vietnamese portal, CSP, SSID injection, selection, validation, password toggle, open/hidden networks, submit/status/error/retry/success, pending/empty/failed scan, 5 responsive sizes, input focus/font-size and pinch-zoom guards, reduced motion, no-JS fallback.');
  console.log(`Screenshots: ${output}`);
  await send('Browser.close');
}finally{if(ws)ws.close();browser.kill();server.closeAllConnections();server.close();}