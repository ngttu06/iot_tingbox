import express, { type RequestHandler } from 'express';
import { rateLimit } from 'express-rate-limit';
import { timingSafeEqual } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { z } from 'zod';
import { hash, Store } from './store.js';
import { amountSchema, normalize } from './provider.js';

export function createApp(store: Store) {
  const app = express(), c = store.config;
  app.disable('x-powered-by');
  app.use((_req,res,next)=>{
    res.set({'Cache-Control':'no-store','X-Content-Type-Options':'nosniff','Referrer-Policy':'no-referrer',
      'Content-Security-Policy':"default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; frame-ancestors 'none'"}); next();
  });
  app.use(rateLimit({windowMs:60000,limit:300,standardHeaders:'draft-8',legacyHeaders:false}));
  app.use(express.json({limit:'32kb'}));
  const auth = (expected: string): RequestHandler => (req,res,next)=>{
    const token = req.headers.authorization?.match(/^Bearer (.+)$/)?.[1] ?? '';
    if (!timingSafeEqual(Buffer.from(hash(token)),Buffer.from(expected))) { res.status(401).json({error:'UNAUTHORIZED'}); return; }
    next();
  };
  const admin = auth(hash(c.ADMIN_TOKEN));
  app.use('/v1/device',auth(store.get('SELECT token_hash FROM devices WHERE id=?',c.DEVICE_ID)!.token_hash));
  app.get('/healthz',(_req,res)=>res.json({status:'ok',mode:c.MOCK_MODE?'mock':'production'}));
  app.get('/v1/device/config',(_req,res)=>res.json({device_id:c.DEVICE_ID,bank_display:'MBBank',
    account_masked:`****${c.BANK_ACCOUNT_NUMBER.slice(-4)}`,poll_interval_ms:3000,
    announce_unmatched_incoming:c.ANNOUNCE_UNMATCHED,speak_content:false,volume:70}));
  app.get('/v1/device/current-order',(_req,res)=>{
    store.expire();
    const o = store.get("SELECT * FROM orders WHERE device_id=? AND status='PENDING' ORDER BY created_at DESC LIMIT 1",c.DEVICE_ID);
    res.json(o ? {order_id:o.id,status:o.status,amount:o.amount,memo:o.memo,qr_payload:o.qr_payload,expires_at:o.expires_at} : null);
  });
  app.get('/v1/device/events',(req,res)=>{
    const after = z.coerce.number().int().nonnegative().safe().parse(req.query.after_id ?? 0);
    const rows = store.all('SELECT * FROM device_events WHERE device_id=? AND id>? AND acked_at IS NULL ORDER BY id LIMIT 10',c.DEVICE_ID,after);
    res.json({events:rows.map(r=>{
      const payload=JSON.parse(r.payload_json);
      return {event_id:r.id,...payload,content:Array.from(payload.content as string).slice(0,160).join('')};
    })});
  });
  app.post('/v1/device/events/:id/ack',(req,res)=>{
    const id = z.coerce.number().int().positive().safe().parse(req.params.id);
    z.object({device_id:z.literal(c.DEVICE_ID),status:z.literal('PLAYED')}).parse(req.body);
    const found = store.get('SELECT id FROM device_events WHERE id=? AND device_id=?',id,c.DEVICE_ID);
    if (!found) {res.status(404).json({error:'NOT_FOUND'});return;}
    store.run('UPDATE device_events SET acked_at=COALESCE(acked_at,?) WHERE id=?',new Date().toISOString(),id);
    res.json({success:true});
  });
  app.post('/v1/orders',admin,async(req,res)=>{
    const body=z.object({amount:amountSchema,device_id:z.literal(c.DEVICE_ID)}).parse(req.body);
    try {res.status(201).json(await store.createOrder(body.amount));}
    catch {res.status(409).json({error:'ORDER_UNAVAILABLE',message:'Check pending order and provider baseline'});}
  });
  app.get('/v1/orders/:id',admin,(req,res)=>{
    store.expire(); const o=store.get('SELECT * FROM orders WHERE id=?',req.params.id);
    res.status(o?200:404).json(o??{error:'NOT_FOUND'});
  });
  app.get('/v1/transactions',admin,(req,res)=>{
    const q=z.object({limit:z.coerce.number().int().min(1).max(100).default(50),before:z.string().datetime().optional(),status:z.enum(['MATCHED','UNMATCHED','NEEDS_REVIEW','BASELINE','IGNORED']).optional()}).parse(req.query);
    res.json(store.all(`SELECT id,provider_id,amount,content,direction,transaction_at,matching_status,created_at FROM transactions WHERE created_at<? ${q.status?'AND matching_status=?':''} ORDER BY created_at DESC LIMIT ?`,q.before??'9999',...(q.status?[q.status]:[]),q.limit));
  });
  app.get('/v1/admin/sync-status',admin,(_req,res)=>res.json({...store.get('SELECT * FROM provider_sync_state WHERE id=?','mb'),quarantine_count:store.get('SELECT COUNT(*) AS n FROM transaction_quarantine')!.n}));
  app.get('/v1/admin/quarantine',admin,(_req,res)=>res.json(store.all('SELECT id,reason,created_at FROM transaction_quarantine ORDER BY created_at DESC LIMIT 100')));
  if(c.MOCK_MODE) app.post('/dev/simulate-credit',admin,(req,res)=>{
    const t=normalize(req.body); store.atomic(()=>store.ingest(t)); res.json({success:true});
  });
  app.use(express.static(fileURLToPath(new URL('../public/',import.meta.url))));
  app.use((_req,res)=>res.status(404).json({error:'NOT_FOUND'}));
  app.use(((err, _req,res,_next)=>{
    res.status(err instanceof z.ZodError || err instanceof SyntaxError ? 400:500).json({error:err instanceof z.ZodError?'INVALID_INPUT':'REQUEST_FAILED'});
  }) as express.ErrorRequestHandler);
  return app;
}