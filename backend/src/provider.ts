import { z } from 'zod';

export const amountSchema = z.union([z.number().int(), z.string().regex(/^\d+$/).transform(Number)])
  .pipe(z.number().int().positive().max(999_999_999_999));
const rowSchema = z.object({
  transactionID: z.union([z.string().min(1).max(128), z.number().int().nonnegative().safe().transform(String)]),
  amount: amountSchema, description: z.string().max(16384),
  transactionDate: z.string().min(1).max(64), type: z.enum(['IN', 'OUT'])
});
export type Credit = {
  id: string; amount: number; content: string; direction: 'IN' | 'OUT';
  at: string; precision: 'day' | 'second'; raw: string;
};
export function bankDate(value: string): {at: string; precision: 'day' | 'second'} {
  const m = /^(\d{2})\/(\d{2})\/(\d{4})(?:[ T](\d{2}):(\d{2}):(\d{2}))?$/.exec(value);
  if (!m) throw new Error('Unsupported bank date');
  const [, d, mo, y, h = '00', mi = '00', s = '00'] = m;
  const iso = `${y}-${mo}-${d}T${h}:${mi}:${s}+07:00`;
  const at = new Date(iso);
  if (!Number.isFinite(at.getTime()) || +h > 23 || +mi > 59 || +s > 59 ||
      new Date(at.getTime() + 7 * 3600000).toISOString().slice(0, 10) !== `${y}-${mo}-${d}`) throw new Error('Invalid bank date');
  return { at: at.toISOString(), precision: m[4] ? 'second' : 'day' };
}
export function normalize(raw: unknown): Credit {
  const t = rowSchema.parse(raw);
  return { id: t.transactionID, amount: t.amount, content: t.description,
    direction: t.type, ...bankDate(t.transactionDate), raw: JSON.stringify(raw) };
}
export function parseSnapshot(body: unknown): unknown[] {
  return z.object({status: z.literal('success'), transactions: z.array(z.unknown()).max(50000)}).parse(body).transactions;
}
export class ProviderError extends Error {
  constructor(public delayMs = 0) { super('Provider unavailable'); }
}
export async function fetchSnapshot(token: string): Promise<unknown[]> {
  try {
    const res = await fetch(`https://thueapi.pro/historyapimbbankv2/${encodeURIComponent(token)}`, {
      signal: AbortSignal.timeout(15000), redirect: 'error', headers: {Accept: 'application/json'}
    });
    if (!res.ok) {
      const retry = res.headers.get('retry-after');
      const ms = retry ? (/^\d+$/.test(retry) ? Number(retry) * 1000 : Date.parse(retry) - Date.now()) : 0;
      throw new ProviderError(Math.min(3600000, Math.max(0, ms || 0)));
    }
    const text = await res.text();
    if (text.length > 8_000_000) throw new ProviderError();
    return parseSnapshot(JSON.parse(text));
  } catch (e) {
    // Never propagate fetch errors: they may contain the secret-bearing URL.
    if (e instanceof ProviderError) throw e;
    throw new ProviderError();
  }
}