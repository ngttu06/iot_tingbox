import { afterEach, describe, expect, it, vi } from 'vitest';
import { createRequire } from 'node:module';
import type { QRCode as DecodedQR } from 'jsqr';
import { PNG } from 'pngjs';
import request from 'supertest';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { loadConfig } from '../src/config.js';
import { Store } from '../src/store.js';
import { createApp } from '../src/app.js';
import { bankDate, fetchSnapshot, normalize, parseSnapshot, ProviderError } from '../src/provider.js';
const jsQR = createRequire(import.meta.url)('jsqr') as (data:Uint8ClampedArray,width:number,height:number)=>DecodedQR|null;

const env={MOCK_MODE:'true',DB_PATH:':memory:',ADMIN_TOKEN:'test-admin-00000000000000000000000000',DEVICE_TOKEN:'test-device-0000000000000000000000000',BANK_ACCOUNT_NUMBER:'0000000000',BANK_ACCOUNT_NAME:'TAI KHOAN THU NGHIEM'};
const stores:Store[]=[];
function setup(extra:Record<string,string>={}){const config=loadConfig({...env,...extra});const store=new Store(config);stores.push(store);return {store,app:createApp(store),admin:`Bearer ${config.ADMIN_TOKEN}`,device:`Bearer ${config.DEVICE_TOKEN}`};}
afterEach(()=>{for(const s of stores.splice(0))s.close();vi.unstubAllGlobals();});
function raw(id='1',description='khach chuyen tien',amount:unknown='150000',date?:string){
  const now=new Intl.DateTimeFormat('en-GB',{timeZone:'Asia/Ho_Chi_Minh',day:'2-digit',month:'2-digit',year:'numeric',hour:'2-digit',minute:'2-digit',second:'2-digit',hourCycle:'h23'}).format(new Date()).replace(',','');
  return {transactionID:id,amount,description,transactionDate:date??now,type:'IN'};
}
describe('provider validation',()=>{
  it('accepts observed strings and documented numeric fields',()=>{expect(normalize(raw()).amount).toBe(150000);expect(normalize({...raw(),transactionID:123,amount:2000}).id).toBe('123');});
  it.each(['-1','0','1.5','1,000','1e5','NaN',-5,0,1.1,1000000000000])('rejects unsafe amount %s',amount=>expect(()=>normalize(raw('1','memo',amount))).toThrow());
  it('parses Vietnam time and rejects invalid/ambiguous dates',()=>{expect(bankDate('09/11/2026 10:30:00').at).toBe('2026-11-09T03:30:00.000Z');expect(bankDate('09/11/2026').precision).toBe('day');for(const d of ['31/02/2026','2026-11-09','09/11/2026 24:00:00'])expect(()=>bankDate(d)).toThrow();});
  it('does not treat HTTP 200 error bodies as empty success',()=>{expect(()=>parseSnapshot({status:'error',transactions:[]})).toThrow();expect(()=>parseSnapshot({status:'success'})).toThrow();});
  it('redacts provider errors and honors Retry-After without leaking tokens',async()=>{
    vi.stubGlobal('fetch',vi.fn().mockResolvedValue(new Response('',{status:429,headers:{'Retry-After':'60'}})));
    try{await fetchSnapshot('SECRET');expect.fail('must reject');}catch(e){expect(e).toBeInstanceOf(ProviderError);expect((e as ProviderError).delayMs).toBe(60000);expect(String(e)).not.toContain('SECRET');}
    vi.stubGlobal('fetch',vi.fn().mockRejectedValue(new Error('secret url SECRET')));
    await expect(fetchSnapshot('SECRET')).rejects.toThrow('Provider unavailable');
  });
  it('rejects malformed success responses and accepts valid snapshots',async()=>{
    vi.stubGlobal('fetch',vi.fn().mockResolvedValue(new Response('{"status":"error"}',{status:200})));
    await expect(fetchSnapshot('SECRET')).rejects.toThrow('Provider unavailable');
    vi.stubGlobal('fetch',vi.fn().mockResolvedValue(new Response(JSON.stringify({status:'success',transactions:[raw()]}))));
    expect(await fetchSnapshot('SECRET')).toHaveLength(1);
  });
});
describe('orders, events and auth',()=>{
  it('serves the POS and its script',async()=>{
    const {app}=setup();const page=await request(app).get('/').expect(200);expect(page.text).toContain('SmartPay');await request(app).get('/app.js').expect(200);
  });
  it('creates real EMV with correct account, amount and memo; exact payment produces one event',async()=>{
    const {store,app,admin,device}=setup();
    const result=await request(app).post('/v1/orders').set('Authorization',admin).send({amount:150000,device_id:'ESP32-001'}).expect(201);
    const o=result.body;expect(o.qr_payload).toContain('970422');expect(o.qr_payload).toContain('150000');expect(o.qr_payload).toContain(o.memo);expect(o.qr_image).toMatch(/^data:image\/png;base64,/);
    const png=PNG.sync.read(Buffer.from(o.qr_image.split(',')[1],'base64'));
    expect(jsQR(new Uint8ClampedArray(png.data),png.width,png.height)?.data).toBe(o.qr_payload);
    let crc=0xffff;for(const byte of Buffer.from(o.qr_payload.slice(0,-4))){crc^=byte<<8;for(let i=0;i<8;i++)crc=crc&0x8000?((crc<<1)^0x1021)&0xffff:(crc<<1)&0xffff;}
    expect(o.qr_payload.slice(-4)).toBe(crc.toString(16).toUpperCase().padStart(4,'0'));
    store.run('UPDATE orders SET created_at=? WHERE id=?',new Date(Date.now()-3000).toISOString(),o.id);
    for(let i=0;i<3;i++)await request(app).post('/dev/simulate-credit').set('Authorization',admin).send(raw('paid',o.memo)).expect(200);
    expect(store.get('SELECT status FROM orders WHERE id=?',o.id)?.status).toBe('PAID');
    expect(store.get('SELECT COUNT(*) n FROM transactions')?.n).toBe(1);
    const events=await request(app).get('/v1/device/events?after_id=0').set('Authorization',device).expect(200);
    expect(events.body.events).toHaveLength(1);expect(events.body.events[0].type).toBe('ORDER_PAID');
    const id=events.body.events[0].event_id;
    await request(app).post(`/v1/device/events/${id}/ack`).set('Authorization',device).send({device_id:'WRONG',status:'PLAYED'}).expect(400);
    for(let i=0;i<2;i++)await request(app).post(`/v1/device/events/${id}/ack`).set('Authorization',device).send({device_id:'ESP32-001',status:'PLAYED'}).expect(200);
    expect((await request(app).get('/v1/device/events').set('Authorization',device)).body.events).toHaveLength(0);
  });
  it('separates admin and device scopes, rejects malformed cursors',async()=>{
    const {app,admin,device}=setup();
    await request(app).get('/v1/device/config').expect(401);
    await request(app).get('/v1/device/config').set('Authorization',admin).expect(401);
    await request(app).post('/v1/orders').set('Authorization',device).send({amount:1000}).expect(401);
    await request(app).get('/v1/device/events?after_id=-1').set('Authorization',device).expect(400);
  });
  it.each([149000,151000])('does not mark wrong amount %s paid',async amount=>{
    const {store}=setup();const o=await store.createOrder(150000);store.atomic(()=>store.ingest(normalize(raw('x',o.memo,amount))));
    expect(store.get('SELECT status FROM orders')?.status).toBe('PENDING');expect(store.get('SELECT matching_status FROM transactions')?.matching_status).toBe('NEEDS_REVIEW');
  });
  it('does not match extended token, and preserves Unicode content',async()=>{
    const {store}=setup();const o=await store.createOrder(150000);const text=`Khách Nguyễn chuyển ${o.memo}8`;
    store.atomic(()=>store.ingest(normalize(raw('x',text))));expect(store.get('SELECT status FROM orders')?.status).toBe('PENDING');expect(store.get('SELECT content FROM transactions')?.content).toBe(text);
  });
  it('does not pay ambiguous codes or expired orders',async()=>{
    const {store}=setup();const o=await store.createOrder(150000);
    store.sync([raw('ambiguous',`${o.memo} DHABCDEF12`)]);
    expect(store.get('SELECT matching_status FROM transactions')?.matching_status).toBe('NEEDS_REVIEW');
    store.run('UPDATE orders SET expires_at=?',new Date(Date.now()-10000).toISOString());store.expire();
    store.sync([raw('late',o.memo)]);expect(store.get('SELECT status FROM orders')?.status).toBe('EXPIRED');
    expect(store.get('SELECT matching_status FROM transactions WHERE provider_id=?','late')?.matching_status).toBe('NEEDS_REVIEW');
  });
  it('allows only one pending order and expires it before creating another',async()=>{
    const {store}=setup();await store.createOrder(1000);await expect(store.createOrder(2000)).rejects.toThrow('already pending');
    store.run('UPDATE orders SET expires_at=?',new Date(Date.now()-1000).toISOString());expect((await store.createOrder(2000)).amount).toBe(2000);
  });
  it('date-only, old and future timestamps cannot pay an order',async()=>{
    const {store}=setup();const o=await store.createOrder(150000);
    for(const [i,d] of ['09/11/2026','01/01/2020 10:00:00','01/01/2099 10:00:00'].entries())store.atomic(()=>store.ingest(normalize(raw(String(i),o.memo,150000,d))));
    expect(store.get('SELECT status FROM orders')?.status).toBe('PENDING');
    expect(store.get('SELECT COUNT(*) n FROM transactions WHERE matching_status=?','NEEDS_REVIEW')?.n).toBe(3);
  });
  it('announces unmatched once but not outgoing and respects setting',()=>{
    const {store}=setup();store.sync([raw(),{...raw('out'),type:'OUT'}]);store.sync([raw()]);expect(store.get('SELECT COUNT(*) n FROM device_events')?.n).toBe(1);
    const other=setup({ANNOUNCE_UNMATCHED:'false'}).store;other.sync([raw()]);expect(other.get('SELECT COUNT(*) n FROM device_events')?.n).toBe(0);
  });
  it('limits device content without damaging stored Unicode',async()=>{
    const {store,app,device}=setup();const content='ầ'.repeat(1000);store.sync([raw('long',content)]);
    const r=await request(app).get('/v1/device/events').set('Authorization',device).expect(200);
    expect(Array.from(r.body.events[0].content)).toHaveLength(160);expect(store.get('SELECT content FROM transactions')?.content).toBe(content);
  });
  it('disables simulator and blocks orders until baseline in production',async()=>{
    const {app,admin}=setup({MOCK_MODE:'false',THUEAPI_TOKEN:'rotated-test-token',PROVIDER_ACCOUNT_VERIFIED:'true',BANK_ACCOUNT_NUMBER:'1234567890'});
    await request(app).post('/dev/simulate-credit').set('Authorization',admin).send(raw()).expect(404);
    await request(app).post('/v1/orders').set('Authorization',admin).send({amount:150000,device_id:'ESP32-001'}).expect(409);
  });
});
describe('durable synchronization',()=>{
  it('baselines history silently, quarantines malformed entries, handles shuffled 100 rows and duplicates',()=>{
    const {store}=setup();store.run('UPDATE provider_sync_state SET baseline=0');store.sync([raw('history'),{bad:true}]);
    expect(store.get('SELECT COUNT(*) n FROM device_events')?.n).toBe(0);
    const rows=Array.from({length:100},(_,i)=>raw(`new${i}`));store.sync(rows);store.sync([...rows].reverse());
    expect(store.get('SELECT COUNT(*) n FROM transactions')?.n).toBe(101);expect(store.get('SELECT COUNT(*) n FROM device_events')?.n).toBe(100);expect(store.get('SELECT COUNT(*) n FROM transaction_quarantine')?.n).toBe(1);
  });
  it('quarantines conflicting IDs rather than changing payment evidence',()=>{
    const {store}=setup();store.sync([raw()]);store.sync([raw('1','changed','5000')]);
    expect(store.get('SELECT COUNT(*) n FROM device_events')?.n).toBe(1);expect(store.get('SELECT reason FROM transaction_quarantine')?.reason).toBe('CONFLICTING_PROVIDER_ID');
  });
  it('persists baseline and duplicate state across restart',()=>{
    const dir=mkdtempSync(join(tmpdir(),'smartpay-'));
    try{const {store}=setup({DB_PATH:join(dir,'test.sqlite')});store.sync([raw()]);store.close();stores.splice(stores.indexOf(store),1);
      const s=setup({DB_PATH:join(dir,'test.sqlite')}).store;s.sync([raw()]);expect(s.get('SELECT COUNT(*) n FROM device_events')?.n).toBe(1);
      s.close();stores.splice(stores.indexOf(s),1);
    }finally{rmSync(dir,{recursive:true,force:true});}
  });
  it('rolls back transaction+order+event together',async()=>{
    const {store}=setup();const o=await store.createOrder(150000);store.run('UPDATE orders SET created_at=?',new Date(Date.now()-3000).toISOString());
    store.db.exec("CREATE TRIGGER fail_event BEFORE INSERT ON device_events BEGIN SELECT RAISE(ABORT,'test'); END;");
    expect(()=>store.sync([raw('fail',o.memo)])).toThrow();expect(store.get('SELECT status FROM orders')?.status).toBe('PENDING');expect(store.get('SELECT COUNT(*) n FROM transactions')?.n).toBe(0);
  });
});