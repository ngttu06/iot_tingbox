import { DatabaseSync } from 'node:sqlite';
import { createHash, randomUUID } from 'node:crypto';
import { generateQR } from 'vietqr';
import type { Config } from './config.js';
import { normalize, type Credit } from './provider.js';

export type Row = Record<string, any>;
export const hash = (v: string) => createHash('sha256').update(v).digest('hex');
export class Store {
  db: DatabaseSync;
  constructor(public config: Config) {
    this.db = new DatabaseSync(config.DB_PATH);
    this.db.exec(`PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON; PRAGMA busy_timeout=5000;
      CREATE TABLE IF NOT EXISTS schema_migrations(version INTEGER PRIMARY KEY);
      CREATE TABLE IF NOT EXISTS bank_accounts(id TEXT PRIMARY KEY,bin TEXT,account_number TEXT,account_name TEXT);
      CREATE TABLE IF NOT EXISTS devices(id TEXT PRIMARY KEY,token_hash TEXT,bank_account_id TEXT REFERENCES bank_accounts(id));
      CREATE TABLE IF NOT EXISTS provider_sources(id TEXT PRIMARY KEY,bank_account_id TEXT,identity_hash TEXT);
      CREATE TABLE IF NOT EXISTS provider_sync_state(id TEXT PRIMARY KEY,baseline INTEGER NOT NULL DEFAULT 0,last_success TEXT,last_error TEXT);
      CREATE TABLE IF NOT EXISTS provider_sync_runs(id INTEGER PRIMARY KEY,created_at TEXT,success INTEGER,item_count INTEGER);
      CREATE TABLE IF NOT EXISTS orders(id TEXT PRIMARY KEY,device_id TEXT,amount INTEGER,memo TEXT UNIQUE,qr_payload TEXT,qr_image TEXT,status TEXT,created_at TEXT,expires_at TEXT,paid_transaction_id TEXT UNIQUE);
      CREATE TABLE IF NOT EXISTS transactions(id TEXT PRIMARY KEY,source TEXT,provider_id TEXT,amount INTEGER,content TEXT,direction TEXT,transaction_at TEXT,precision TEXT,raw_payload TEXT,matching_status TEXT,created_at TEXT,UNIQUE(source,provider_id));
      CREATE TABLE IF NOT EXISTS device_events(id INTEGER PRIMARY KEY AUTOINCREMENT,device_id TEXT,transaction_id TEXT,order_id TEXT,type TEXT,payload_json TEXT,created_at TEXT,acked_at TEXT,UNIQUE(device_id,transaction_id));
      CREATE TABLE IF NOT EXISTS transaction_quarantine(id TEXT PRIMARY KEY,reason TEXT,raw_payload TEXT,created_at TEXT);
      INSERT OR IGNORE INTO schema_migrations VALUES(1);`);
    const identity = hash(JSON.stringify([config.MOCK_MODE, config.BANK_BIN, config.BANK_ACCOUNT_NUMBER, config.THUEAPI_TOKEN]));
    const previous = this.get('SELECT * FROM provider_sources WHERE id=?', 'mb');
    if (previous && previous.identity_hash !== identity) throw new Error('Provider/account identity changed: use a separate database and re-baseline');
    this.run('INSERT OR IGNORE INTO bank_accounts VALUES(?,?,?,?)','mb',config.BANK_BIN,config.BANK_ACCOUNT_NUMBER,config.BANK_ACCOUNT_NAME);
    this.run('INSERT OR IGNORE INTO provider_sources VALUES(?,?,?)','mb','mb',identity);
    this.run('INSERT INTO devices VALUES(?,?,?) ON CONFLICT(id) DO UPDATE SET token_hash=excluded.token_hash',config.DEVICE_ID,hash(config.DEVICE_TOKEN),'mb');
    this.run('INSERT OR IGNORE INTO provider_sync_state(id,baseline) VALUES(?,?)','mb',config.MOCK_MODE ? 1 : 0);
  }
  get(sql: string, ...args: any[]): Row | undefined { return this.db.prepare(sql).get(...args) as Row | undefined; }
  all(sql: string, ...args: any[]): Row[] { return this.db.prepare(sql).all(...args) as Row[]; }
  run(sql: string, ...args: any[]) { return this.db.prepare(sql).run(...args); }
  atomic<T>(fn: () => T): T {
    this.db.exec('BEGIN IMMEDIATE');
    try { const out = fn(); this.db.exec('COMMIT'); return out; }
    catch (e) { this.db.exec('ROLLBACK'); throw e; }
  }
  expire() { this.run("UPDATE orders SET status='EXPIRED' WHERE status='PENDING' AND expires_at<?",new Date().toISOString()); }
  async createOrder(amount: number) {
    if (!this.get('SELECT baseline FROM provider_sync_state WHERE id=?','mb')?.baseline) throw new Error('Baseline not ready');
    const memo = `DH${randomUUID().replaceAll('-','').slice(0,16).toUpperCase()}`;
    const qr = await generateQR({accountNo:this.config.BANK_ACCOUNT_NUMBER, accountName:this.config.BANK_ACCOUNT_NAME,
      acqId:this.config.BANK_BIN, amount, addInfo:memo});
    if (qr.code !== '00' || !qr.data.qrCode.startsWith('000201')) throw new Error('QR generation failed');
    const now = new Date().toISOString();
    return this.atomic(() => {
      this.expire();
      if (this.get("SELECT id FROM orders WHERE status='PENDING' AND device_id=?",this.config.DEVICE_ID)) throw new Error('An order is already pending');
      this.run('INSERT INTO orders VALUES(?,?,?,?,?,?,?,?,?,NULL)',memo,this.config.DEVICE_ID,amount,memo,qr.data.qrCode,qr.data.qrDataURL,'PENDING',now,new Date(Date.now()+this.config.ORDER_TTL_SECONDS*1000).toISOString());
      return this.get('SELECT * FROM orders WHERE id=?',memo)!;
    });
  }
  ingest(t: Credit, baseline = false) {
    const prior = this.get('SELECT * FROM transactions WHERE source=? AND provider_id=?','mb',t.id);
    if (prior) {
      if (prior.amount !== t.amount || prior.content !== t.content || prior.direction !== t.direction || prior.transaction_at !== t.at) this.quarantine(JSON.parse(t.raw),'CONFLICTING_PROVIDER_ID');
      return;
    }
    const id = randomUUID(), now = new Date().toISOString();
    let status = baseline ? 'BASELINE' : t.direction === 'OUT' ? 'IGNORED' : 'UNMATCHED';
    let order: Row | undefined;
    if (!baseline && t.direction === 'IN') {
      const codes = [...new Set(t.content.match(/(?<![A-Za-z0-9])DH[A-Z0-9]{6,32}(?![A-Za-z0-9])/g) ?? [])];
      if (codes.length === 1) order = this.get('SELECT * FROM orders WHERE memo=?',codes[0]);
      if (codes.length > 1) status = 'NEEDS_REVIEW';
      if (order) {
        const at = Date.parse(t.at);
        const valid = order.status === 'PENDING' && order.amount === t.amount && t.precision === 'second'
          && at >= Math.floor(Date.parse(order.created_at)/1000)*1000 && at <= Date.parse(order.expires_at) && at <= Date.now()+300000;
        status = valid ? 'MATCHED' : 'NEEDS_REVIEW';
      }
    }
    this.run('INSERT INTO transactions VALUES(?,?,?,?,?,?,?,?,?,?,?)',id,'mb',t.id,t.amount,t.content,t.direction,t.at,t.precision,t.raw,status,now);
    if (status === 'MATCHED' && order) this.run("UPDATE orders SET status='PAID',paid_transaction_id=? WHERE id=?",id,order.id);
    // Date-only/old records remain visible for review, but must not trigger stale announcements.
    const fresh = t.precision === 'second' && Date.parse(t.at) <= Date.now()+300000;
    if (!baseline && t.direction === 'IN' && fresh && (status === 'MATCHED' || this.config.ANNOUNCE_UNMATCHED)) {
      const type = status === 'MATCHED' ? 'ORDER_PAID' : 'BANK_CREDIT';
      const payload = {transaction_id:id,type,bank:'MBBank',amount:t.amount,content:t.content,reference:t.id,order_id:type==='ORDER_PAID'?order!.id:null,created_at:t.at};
      this.run('INSERT INTO device_events(device_id,transaction_id,order_id,type,payload_json,created_at) VALUES(?,?,?,?,?,?)',this.config.DEVICE_ID,id,payload.order_id,type,JSON.stringify(payload),now);
    }
  }
  quarantine(raw: unknown, reason: string) {
    const text = JSON.stringify(raw) ?? 'null';
    this.run('INSERT OR IGNORE INTO transaction_quarantine VALUES(?,?,?,?)',hash(text),reason,text,new Date().toISOString());
  }
  sync(rows: unknown[]) {
    this.atomic(() => {
      const baseline = !this.get('SELECT baseline FROM provider_sync_state WHERE id=?','mb')?.baseline;
      const valid: Credit[] = [];
      for (const row of rows) { try { valid.push(normalize(row)); } catch { this.quarantine(row,'INVALID_SCHEMA_OR_DATE'); } }
      valid.sort((a,b)=>a.at.localeCompare(b.at)||a.id.localeCompare(b.id));
      for (const t of valid) this.ingest(t,baseline);
      this.run('UPDATE provider_sync_state SET baseline=1,last_success=?,last_error=NULL WHERE id=?',new Date().toISOString(),'mb');
      this.run('INSERT INTO provider_sync_runs(created_at,success,item_count) VALUES(?,1,?)',new Date().toISOString(),rows.length);
    });
  }
  syncFailure() {
    this.run('UPDATE provider_sync_state SET last_error=? WHERE id=?','PROVIDER_UNAVAILABLE','mb');
    this.run('INSERT INTO provider_sync_runs(created_at,success,item_count) VALUES(?,0,0)',new Date().toISOString());
  }
  close() { this.db.close(); }
}