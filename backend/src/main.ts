import 'dotenv/config';
import pino from 'pino';
import { chmodSync } from 'node:fs';
import { loadConfig } from './config.js';
import { Store } from './store.js';
import { createApp } from './app.js';
import { fetchSnapshot, ProviderError } from './provider.js';

process.umask(0o077);
const log = pino(), config=loadConfig(process.env), store=new Store(config);
if(config.DB_PATH!==':memory:') chmodSync(config.DB_PATH,0o600);
let stopped=false, timer:NodeJS.Timeout|undefined, failures=0;
async function poll() {
  let delay=config.PROVIDER_POLL_MS;
  try {store.sync(await fetchSnapshot(config.THUEAPI_TOKEN));failures=0;}
  catch(e) {
    failures++; store.syncFailure();
    delay=Math.max(Math.min(900000,config.PROVIDER_POLL_MS*2**Math.min(failures,5)),e instanceof ProviderError?e.delayMs:0);
    log.warn({code:'PROVIDER_UNAVAILABLE'},'Provider sync failed; details redacted');
  }
  if(!stopped) timer=setTimeout(poll,delay);
}
if(!config.MOCK_MODE) void poll();
const server=createApp(store).listen(config.PORT,config.HOST,()=>log.info({port:config.PORT,mock:config.MOCK_MODE},'SmartPay ready'));
function stop() {stopped=true;clearTimeout(timer);server.close(()=>{store.close();process.exit(0);});}
process.on('SIGINT',stop);process.on('SIGTERM',stop);