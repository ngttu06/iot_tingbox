import { z } from 'zod';

const bool = z.enum(['true', 'false']).transform(v => v === 'true');
const schema = z.object({
  PORT: z.coerce.number().int().min(1).max(65535).default(3000),
  HOST: z.string().default('127.0.0.1'),
  MOCK_MODE: bool.default(true),
  DB_PATH: z.string().default('smartpay.sqlite'),
  ADMIN_TOKEN: z.string().min(32), DEVICE_TOKEN: z.string().min(32),
  DEVICE_ID: z.string().regex(/^[A-Za-z0-9_-]{1,64}$/).default('ESP32-001'),
  BANK_BIN: z.string().regex(/^\d{6}$/).default('970422'),
  BANK_ACCOUNT_NUMBER: z.string().regex(/^\d{6,19}$/),
  BANK_ACCOUNT_NAME: z.string().regex(/^[A-Z0-9 ]{5,50}$/),
  THUEAPI_TOKEN: z.string().default(''),
  PROVIDER_ACCOUNT_VERIFIED: bool.default(false),
  PROVIDER_POLL_MS: z.coerce.number().int().min(30000).default(30000),
  ORDER_TTL_SECONDS: z.coerce.number().int().min(60).max(86400).default(600),
  ANNOUNCE_UNMATCHED: bool.default(true)
});
export type Config = z.infer<typeof schema>;
export function loadConfig(env: NodeJS.ProcessEnv): Config {
  const c = schema.parse(env);
  if (c.ADMIN_TOKEN === c.DEVICE_TOKEN) throw new Error('Separate admin and device tokens required');
  if (!c.MOCK_MODE && (!c.THUEAPI_TOKEN || !c.PROVIDER_ACCOUNT_VERIFIED || c.BANK_ACCOUNT_NUMBER === '0000000000' || c.ADMIN_TOKEN.startsWith('mock-') || c.DEVICE_TOKEN.startsWith('mock-'))) {
    throw new Error('Production requires rotated secrets and verified provider/account mapping');
  }
  return c;
}