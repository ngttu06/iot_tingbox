# ESP32 SMARTPAY — ĐẶC TẢ BÀN GIAO CHO CODING AGENT

> **CẬP NHẬT ĐÃ DUYỆT v1.1 — 09/11/2026:** Nội dung v1.0 bên dưới được giữ để truy vết, **không còn là hợp đồng triển khai hiện hành** ở các mục SePay, webhook, thư viện SQLite và lưu audio LittleFS. Đặc tả hiện hành nằm tại `docs/SPEC-v1.1.md`: dùng ThueAPI MBBank polling, Node.js built-in SQLite thay dependency SQLite ngoài, VietQR offline và **21 clip MP3 có sẵn nhúng PROGMEM vào firmware** (một lần upload có audio, không cần upload filesystem). Không triển khai SePay hoặc webhook HMAC trong bản này. Đọc `README.md` và `docs/VALIDATION.md` cho cách chạy và phạm vi đã kiểm thử.

Phiên bản: 1.0 — 09/11/2026  
Ngôn ngữ triển khai: Arduino C++ (ESP32) + Node.js TypeScript (backend).  
Mục tiêu: Viết mã nguồn **chạy được** cho ESP32 + backend nhận giao dịch ngân hàng, hiển thị QR động, hiện số tiền/nội dung giao dịch và loa đọc tiền bằng tiếng Việt. Không chỉ đưa pseudocode.

> QUYẾT ĐỊNH KIẾN TRÚC: ESP32 **không** gọi API ngân hàng trực tiếp và **không** chứa SePay API key. SePay gửi webhook HTTPS đến backend; backend xác thực và lưu dữ liệu; ESP32 lấy sự kiện từ backend bằng HTTPS polling. Đây là MVP ưu tiên an toàn, dễ chạy trong Arduino IDE. Không triển khai MQTT ở bản đầu.

## 1. KẾT QUẢ BÀN GIAO BẮT BUỘC

- Thư mục `firmware/` — Arduino IDE sketch `.ino`, mã nguồn C++ tách module nếu cần, cấu hình chân và các hướng dẫn nạp.
- Thư mục `backend/` — Node.js TypeScript + Express, SQLite, cấu hình `.env.example`, migrations, unit/integration tests.
- Thư mục `firmware/data/audio/` — bảng tên clip âm thanh, hướng dẫn tạo và chép vào LittleFS; có thể dùng clip mẫu tự tạo hoặc nêu rõ asset cần bổ sung. Không dùng file MP3 từ nguồn không có quyền sử dụng.
- `README.md` — hướng dẫn cài Node.js, Arduino IDE, thư viện, cấu hình SePay, thiết bị, nạp firmware, kiểm thử bằng giao dịch giả lập.
- Mọi endpoint backend phải hoạt động với `MOCK_MODE=true` mà không cần tài khoản ngân hàng thật.
- Mã không hard-code API key, thông tin tài khoản ngân hàng thật, Wi-Fi hoặc mật khẩu trong repo.

## 2. CHỨC NĂNG

**A. Chế độ loa thông báo chuyển khoản (BANK SPEAKER)**
1. Khi tài khoản đã liên kết nhận tiền, SePay gửi giao dịch đến backend.
2. Backend xác thực webhook, chỉ nhận `transferType=in`, chống trùng, lưu nguyên nội dung chuyển khoản `content`.
3. ESP32 nhận sự kiện và hiển thị số tiền, tên ngân hàng, nội dung chuyển khoản (cắt dòng nếu dài), thời gian.
4. Loa đọc: `Đã nhận <số tiền bằng chữ> đồng` bằng các clip âm thanh offline trong LittleFS.
5. Tùy chọn `announce_unmatched_incoming=true`: tất cả tiền vào hợp lệ của tài khoản đã định danh đều được thông báo, kể cả không thuộc đơn hàng QR.

**B. Chế độ QR động theo đơn hàng (DYNAMIC QR)**
1. Người bán nhập số tiền bằng web mini-POS của backend. Bản MVP không bắt buộc cảm ứng, triển khai nút web trước.
2. Backend tạo đơn `PENDING`, sinh mã duy nhất `DH` + ít nhất 6 chữ số / token ngắn không trùng; ví dụ `DH000128`.
3. Backend tạo payload VietQR chuẩn chứa đúng BIN, STK, số tiền, nội dung `DH000128`, bằng npm `vietqr` >= 1.2.0 (tạo offline trên server, KHÔNG dùng API `/v2/generate` cũ).
4. ESP32 tải đơn hiện tại từ backend, dùng thư viện QR trên ESP32 để vẽ ma trận từ **chuỗi VietQR/EMV**, không tạo QR từ URL trang web. Phải thử quét với ít nhất 2 ứng dụng ngân hàng. Hiện cả tiền và mã đơn bên dưới.
5. Khi webhook có mã đơn chính xác + đúng số tiền + đúng tài khoản nhận, backend đổi trạng thái đơn sang `PAID`, tạo sự kiện cho ESP32.
6. ESP32 hiện dấu tích xanh và phát loa một lần. Giao dịch không khớp tuyệt đối KHÔNG tự đánh dấu `PAID`.

**C. Nội dung chuyển khoản**
- Hiển thị `content` gốc của ngân hàng (giới hạn hiển thị khoảng 100–160 ký tự, lưu full ở backend).
- Phân biệt `content` (nội dung chuyển khoản thực tế), `description` (mô tả mở rộng đôi khi trống), `code` (mã SePay bóc tách, có thể null).
- **MVP chỉ đọc số tiền bằng loa.** Đọc **nội dung bất kỳ** là tính năng nâng cao: cần backend TTS tiếng Việt tạo MP3 động và ESP32 tải/stream; bộ clip số đếm offline không thể tự đọc chuẩn mọi nội dung tự do. Có tùy chọn `speak_content=false` mặc định; nếu bật TTS phải giới hạn độ dài, loại nội dung nhạy cảm và hỏi người quản trị về quyền riêng tư.

## 3. PHẦN CỨNG MỤC TIÊU

- ESP32 DevKit V1 / ESP32-WROOM-32, Flash thực tế ≥4 MB, Wi-Fi 2.4 GHz.
- TFT LCD 3.5 inch giao tiếp SPI, driver ILI9488 (xác nhận model thật trước khi chọn User_Setup). Độ phân giải tham khảo 480x320.
- MAX98357A I2S DAC + amplifier.
- Loa 4 ohm 3 W, cấp nguồn USB 5V đủ dòng.
- Optional: cảm ứng XPT2046 dùng chung SPI, giai đoạn 2.

### Pin mapping thống nhất (ESP32 DevKit V1)

| Thành phần | Tên chân | ESP32 |
|---|---|---|
| TFT ILI9488 | SCK | GPIO18 |
| TFT ILI9488 | MOSI / SDI | GPIO23 |
| TFT ILI9488 | MISO / SDO (nếu có) | GPIO19 |
| TFT ILI9488 | CS | GPIO5 |
| TFT ILI9488 | DC / RS | GPIO27 |
| TFT ILI9488 | RST | GPIO14 |
| TFT ILI9488 | VCC | 3V3 **nếu module yêu cầu 3.3V**; theo tài liệu module thực tế |
| TFT ILI9488 | GND | GND |
| TFT ILI9488 | LED / BL | 3V3 hoặc chân điều khiển phù hợp với mạch đèn nền thực tế; không nối trực tiếp nếu quá dòng |
| MAX98357A | BCLK / BCK | GPIO26 |
| MAX98357A | LRC / WS | GPIO25 |
| MAX98357A | DIN | GPIO22 |
| MAX98357A | VIN | 5V từ nguồn USB/5V được cấp đúng chuẩn module |
| MAX98357A | GND | GND chung ESP32 |
| Loa 4Ω | Cực loa | SPK+ và SPK- trên MAX98357A; **không** nối cực loa xuống GND |
| XPT2046 (optional) | T_CS | GPIO32 |
| XPT2046 (optional) | T_IRQ | GPIO33 |
| XPT2046 (optional) | T_CLK / T_DIN / T_DO | Chung GPIO18 / GPIO23 / GPIO19 của TFT |

**Quan trọng:** Cấu hình chân trong firmware phải đồng nhất bảng này. Điện áp VCC TFT phụ thuộc từng bo mạch TFT 3.5 inch; kiểm tra sơ đồ module trước khi cấp nguồn. Logic SPI của ESP32 là 3.3V; không cấp 5V vào GPIO. Chú ý GPIO5 là chân strapping, bảo đảm mạch TFT không gây lỗi boot; có thể đổi CS nếu cần.

## 4. THƯ VIỆN GỢI Ý (AGENT PHẢI KIỂM TRA TÍNH TƯƠNG THÍCH)

**Arduino IDE:**
- ESP32 core của Espressif (ghi rõ phiên bản đã kiểm thử).
- `WiFi.h`, `WiFiClientSecure.h`, `HTTPClient.h`, `LittleFS.h`, `Preferences.h`.
- ArduinoJson 7 hoặc phiên bản đã khóa và test.
- TFT_eSPI (cấu hình ILI9488, SPI pins); cần font hỗ trợ tiếng Việt nếu hiển thị có dấu. Có thể dùng LovyanGFX thay thế nếu tương thích tốt hơn.
- Thư viện QR Code trên ESP32 (ví dụ `QRCode`/ricmoo) nhận chuỗi payload và vẽ pixel đen trắng. Kiểm tra dung lượng QR/version; không giả lập mã QR bằng đồ họa.
- `ESP8266Audio` trên ESP32 với `AudioGeneratorMP3` + nguồn đọc LittleFS + `AudioOutputI2S` (xác nhận pin API với core phiên bản chọn); hoặc một thư viện I2S phát MP3 tương đương đã test.

**Backend:** Node.js LTS, TypeScript, Express, better-sqlite3 hoặc SQLite tương thích, `vietqr` >=1.2.0, zod (validate), pino (logs), Vitest/Jest.

## 5. DỮ LIỆU NGÂN HÀNG: SEPAY

### 5.1 Webhook realtime (ưu tiên)

Tạo webhook trên Dashboard SePay: chỉ `Có tiền vào`, định dạng JSON, bảo mật **HMAC-SHA256**, địa chỉ HTTPS public:

`POST https://YOUR_DOMAIN/webhooks/sepay`

**Payload SePay tham khảo (ví dụ giả lập):**

```json
{
  "id": 92704,
  "gateway": "Vietcombank",
  "transactionDate": "2026-11-09 10:30:00",
  "accountNumber": "0123456789",
  "subAccount": "",
  "code": "DH000128",
  "content": "DH000128 thanh toan don hang",
  "transferType": "in",
  "description": "KHACH HANG chuyen tien",
  "transferAmount": 150000,
  "accumulated": 5000000,
  "referenceCode": "FT261109XYZ"
}
```

Đây là **JSON ví dụ**, không phải giao dịch thật.

Quy tắc HMAC theo tài liệu SePay:
- `X-SePay-Timestamp`: timestamp Unix giây.
- `X-SePay-Signature`: `sha256=<hex>`.
- `expected = HMAC_SHA256(secret, timestamp + '.' + RAW_BODY)`; so sánh constant-time.
- Kiểm tra timestamp lệch tối đa khoảng 300 giây, HTTPS, IP allowlist nếu phù hợp.
- Không dùng `JSON.stringify(req.body)` thay thế raw body để xác minh chữ ký.
- Sau khi **ghi nhận payload an toàn vào DB/queue**, trả HTTP 200 + `{"success":true}` nhanh; khi DB không nhận được thì trả lỗi để SePay retry.
- `id` numeric trong webhook là khóa idempotency cho nguồn webhook. UNIQUE trên `(source, source_transaction_id)`.

### 5.2 API lấy giao dịch để đối soát

SePay API v2:

```bash
curl 'https://userapi.sepay.vn/v2/transactions?transfer_type=in&transaction_date_sort=asc&per_page=100' \
  -H 'Authorization: Bearer SEPAY_API_TOKEN'
```

- Response v2 là `{ "status":"success", "data":[...] }`.
- Các field cần: `id` (UUID), `transaction_date`, `account_number`, `amount_in`, `transfer_type`, `transaction_content`, `reference_number`, `bank_brand_name`.
- Với polling dùng `since_id=<UUID>&transaction_date_sort=asc`, đọc hết các trang rồi mới lưu cursor ID cuối cùng. Không cập nhật cursor sớm làm mất giao dịch.
- Rate limit API v2 hiện ghi 3 request/giây/IP; xử lý 429 theo `Retry-After`.
- Chạy đối soát 15–30 phút/lần và lúc backend khôi phục kết nối, không poll quá nhanh.
- **Cảnh báo:** `id` webhook kiểu integer còn `id` API v2 là UUID; KHÔNG mặc định hai ID giống nhau. Khi gộp webhook và polling, chuẩn hóa giao dịch nội bộ và kiểm tra trùng thông qua `(tài khoản, referenceCode/reference_number, amount, hướng)` khi có mã tham chiếu; nếu reference thiếu hoặc mơ hồ, đưa vào hàng kiểm tra thủ công thay vì tự xác nhận 2 lần.

### 5.3 Điều kiện nhận và khớp giao dịch

- Chỉ xử lý `transferType = "in"`, `transferAmount > 0`, tài khoản nhận thuộc allowlist backend.
- Lưu và hiển thị `content` như **dữ liệu**, không bao giờ diễn giải nội dung chuyển khoản thành lệnh thực thi.
- Mã đơn là token ASCII duy nhất, ví dụ `DH000128`. Có thể lấy từ `code` nếu cấu hình SePay đúng; luôn có parser fallback cho `content` với ranh giới ký tự chính xác, không khớp `DH00012` với `DH000128`.
- Chỉ `PAID` nếu: mã đơn chính xác, đơn đang `PENDING`, đúng tài khoản nhận, đúng số tiền, không trùng giao dịch, không bị gán cho đơn khác. Thời gian tạo đơn và thời gian giao dịch phải hợp lệ. Kiểm tra số tiền chuyển thiếu/thừa, chuyển nhiều lần, sai mã, mã rỗng, giao dịch đến muộn hoặc ngoài giờ: `NEEDS_REVIEW` / `UNMATCHED`, không tự đánh dấu PAID.
- Một giao dịch chỉ tạo **một sự kiện loa** cho mỗi thiết bị được gán tài khoản. Không phát trùng cho cả `BANK_CREDIT` và `ORDER_PAID` của cùng giao dịch; `ORDER_PAID` là loại sự kiện ưu tiên nếu khớp đơn.

## 6. HỢP ĐỒNG API NỘI BỘ BACKEND ↔ ESP32

Mọi endpoint thiết bị dùng `Authorization: Bearer <DEVICE_TOKEN>` và HTTPS. Backend kiểm tra token theo device; device không có API key SePay. Ví dụ JSON dưới đây là **schema chúng ta tự thiết kế**, không phải JSON trả về của SePay.

### 6.1 Lấy cấu hình

`GET /v1/device/config`

```json
{
  "device_id": "ESP32-001",
  "bank_display": "Vietcombank",
  "account_masked": "****6789",
  "poll_interval_ms": 3000,
  "announce_unmatched_incoming": true,
  "speak_content": false,
  "volume": 70
}
```

### 6.2 Lấy đơn đang hiển thị

`GET /v1/device/current-order`

```json
{
  "order_id": "DH000128",
  "status": "PENDING",
  "amount": 150000,
  "memo": "DH000128",
  "qr_payload": "000201010212...6304ABCD",
  "expires_at": "2026-11-09T04:00:00Z"
}
```

`qr_payload` phải là **chuỗi VietQR chuẩn thực do thư viện tạo**, không phải placeholder tại runtime. Nếu không có đơn trả `null`/204 theo hợp đồng nhất quán; web POS tạo đơn qua `POST /v1/orders` (endpoint dành cho admin, không dùng DEVICE_TOKEN).

### 6.3 ESP32 lấy sự kiện loa

`GET /v1/device/events?after_id=124`

```json
{
  "events": [
    {
      "event_id": 125,
      "type": "ORDER_PAID",
      "transaction_id": "internal-txn-uuid",
      "bank": "Vietcombank",
      "amount": 150000,
      "content": "DH000128 thanh toan don hang",
      "reference": "FT261109XYZ",
      "order_id": "DH000128",
      "created_at": "2026-11-09T03:30:00Z"
    }
  ]
}
```

Kiểu `type`: `BANK_CREDIT` hoặc `ORDER_PAID`. Sự kiện được lưu DB, ID tăng dần; không chỉ dùng biến RAM. Backend chỉ trả sự kiện thuộc tài khoản gắn với thiết bị đó.

### 6.4 Xác nhận đã phát

`POST /v1/device/events/125/ack`

```json
{ "device_id": "ESP32-001", "status": "PLAYED" }
```

- ESP32 ghi `last_played_event_id` vào NVS/Preferences **sau khi phát xong**, rồi ACK. Khi reboot không phát lại sự kiện đã lưu local.
- Backend vẫn giữ hàng đợi/ACK và có retry. Không coi HTTP 200 khi poll là đã phát xong.
- Nếu quá nhiều sự kiện trong lúc offline, hiển thị danh sách và phát giới hạn hoặc tổng kết, tránh spam hàng chục thông báo; bản MVP có thể giới hạn 10 thông báo/lần reconnect và nêu rõ cơ chế.

### 6.5 Backend endpoints cho POS/admin

- `POST /v1/orders` — tạo đơn amount (int VND), device_id; token admin.
- `GET /v1/orders/:id` — xem trạng thái.
- `GET /v1/transactions` — lịch sử giao dịch, lọc theo ngày/account/device.
- `GET /healthz` — health check.
- `/dev/simulate-credit` — chỉ kích hoạt trong `MOCK_MODE=true`, không expose khi production.

## 7. DATABASE (SQLITE MVP)

Bảng tối thiểu:

- `devices(id PK, token_hash, bank_account_id, last_seen_at, settings_json)`.
- `bank_accounts(id PK, bank_bin, bank_name, account_number, account_name, enabled)`.
- `orders(id PK, device_id, bank_account_id, amount_vnd INTEGER, memo UNIQUE, qr_payload TEXT, status, created_at, expires_at, paid_transaction_id)`.
- `transactions(id PK UUID nội bộ, provider, provider_webhook_id UNIQUE NULL, provider_api_id UNIQUE NULL, bank_account_id, direction, amount_vnd INTEGER, content TEXT, description TEXT, reference_code, transaction_at, raw_payload_json, matching_status, created_at)`.
- `device_events(id INTEGER PK AUTOINCREMENT, device_id, transaction_id, order_id NULL, type, payload_json, created_at, acked_at, UNIQUE(device_id, transaction_id))`.
- `sepay_api_cursor(id PK, last_uuid, updated_at)`.
- `webhook_inbox(id PK, source, provider_id, payload_raw, verified, processed_at, UNIQUE(source, provider_id))`.

Dùng SQLite transaction để ghi `transactions`, cập nhật `orders` và thêm `device_events` nguyên tử, tránh xác nhận trùng lúc nhận nhiều webhook đồng thời.

## 8. ARDUINO — CẤU TRÚC MODULE VÀ STATE MACHINE

```text
firmware/
  SmartPayESP32/SmartPayESP32.ino
  SmartPayESP32/config.example.h
  SmartPayESP32/DisplayManager.h/.cpp
  SmartPayESP32/AudioManager.h/.cpp
  SmartPayESP32/ApiClient.h/.cpp
  SmartPayESP32/PaymentState.h/.cpp
  SmartPayESP32/VietnameseMoney.h/.cpp
  data/audio/ (MP3 mono 16–32 kbps, 22.05/24 kHz hoặc profile đã test)
```

**Các trạng thái:** `BOOT`, `WIFI_CONNECTING`, `READY`, `SHOW_QR`, `PAYMENT_RECEIVED`, `AUDIO_PLAYING`, `OFFLINE`, `ERROR`.

**Loop phải không blocking:**
- Khởi tạo `Serial`, LittleFS, NVS, I2S audio, TFT, Wi-Fi, thời gian NTP và TLS.
- Mỗi 3 giây (dùng `millis()`, không `delay(3000)`): gọi backend lấy đơn và sự kiện; retry theo exponential backoff khi mất mạng.
- Nếu có `ORDER_PAID`, chuyển UI thành công, lên danh sách đọc tiền, phát clip không chặn toàn bộ loop; hoàn tất rồi lưu last_event_id và ACK.
- Khi có `BANK_CREDIT`, hiện số tiền và `content` nhưng **không** biến đơn đang chờ thành paid.
- Không render lại nguyên màn hình liên tục (flicker), chỉ cập nhật vùng cần thiết.
- QR sử dụng foreground đen/background trắng, quiet zone >= 4 module, vừa màn TFT và không che bởi logo.
- Thêm watchdog hoặc cơ chế tự kết nối lại Wi-Fi/TLS, log qua Serial ở chế độ debug.
- HTTPS đúng CA/time sync; **không dùng `client.setInsecure()` cho production**.

**Hiển thị:**
- Màn READY: `Wi-Fi OK`, tên shop, trạng thái sẵn sàng.
- Màn SHOW_QR: mã QR quét được, số tiền `150.000đ`, mã `DH000128`, trạng thái `Đang chờ`.
- Màn PAYMENT_RECEIVED: dấu tích xanh, `+150.000đ`, `Noi dung: DH000128 thanh toan don hang` (hoặc unicode nếu font hỗ trợ), tên bank, timestamp.
- Màn OFFLINE: không hiện thành công giả; thông báo `Mất kết nối`, duy trì dữ liệu cũ có nhãn rõ ràng.
- Muốn hiển thị tiếng Việt có dấu, nạp font Unicode hỗ trợ glyph; TFT_eSPI font mặc định không bảo đảm đủ dấu tiếng Việt.

## 9. AUDIO OFFLINE — LITTLEFS + MAX98357A

MVP: clip phát từ Flash LittleFS, không cần microSD/Internet để **phát** (Internet vẫn cần nhận thông tin giao dịch mới).

- Tạo tập âm thanh nhất quán giọng nữ hoặc giọng nam Việt Nam, MP3 mono đã chuẩn hóa âm lượng; ví dụ `da_nhan.mp3`, `khong.mp3`, `mot.mp3`... `chin.mp3`, `muoi.mp3`, `tram.mp3`, `nghin.mp3`, `trieu.mp3`, `ty.mp3`, `linh.mp3`, `lam.mp3`, `mot_sau_muoi.mp3`, `dong.mp3`.
- Agent phải triển khai thuật toán `numberToVietnameseClips(uint64_t amount)` cho 0–999.999.999.999 hoặc range phù hợp và test số `1000`, `1001`, `1010`, `1025`, `105000`, `125000`, `150000`, `1000000`, `1205000`, `1000000000`.
- Không dùng nối âm ngây thơ khiến sai các quy tắc `mốt`, `lăm`, `linh/lẻ`, `tư` theo cách đọc đã thống nhất.
- Tiền là số nguyên VND, không float.
- Chống giật âm bằng vòng cập nhật decoder MP3 thường xuyên hoặc task phát âm thanh riêng, chú ý thread-safety và vùng RAM.
- Lưu file vào phân vùng LittleFS (không nhầm RAM/PSRAM). Kiểm tra đủ Flash và phân vùng app/filesystem của thiết bị trước khi build.
- Không đọc `content` tự do bằng bộ clip số. Nếu nâng cấp TTS: backend sinh MP3, lọc nội dung, giới hạn độ dài và chỉ tải qua URL nội bộ có xác thực.

## 10. MỘT SỐ QUY TẮC BẢO MẬT KHÔNG ĐƯỢC BỎ QUA

- Mọi khóa API SePay và HMAC secret nằm trong `.env` backend; không đưa lên ESP32/firmware hoặc git.
- Backend xác minh SePay HMAC từ **RAW** request body, kiểm tra timestamp, tài khoản ngân hàng, loại giao dịch, amount và idempotency.
- Admin endpoint có đăng nhập/token riêng, device token scope giới hạn vào 1 device và tài khoản tương ứng.
- Không dùng user input hoặc nội dung chuyển khoản để build câu lệnh shell, SQL string trực tiếp, URL ngoài ý muốn, hoặc logic điều khiển thiết bị.
- Không logging secrets hoặc số tài khoản đầy đủ ở log production; raw payload lưu giới hạn quyền truy cập và thời gian retention.
- Không thông báo **đã thanh toán đơn** trước khi có bằng chứng giao dịch thật; screenshot QR, nút giả lập và khách bấm chuyển khoản không phải bằng chứng.
- Có trang admin để xem các giao dịch chưa khớp, giao dịch sai tiền hoặc nhiều giao dịch cùng mã.

## 11. BỘ TEST BẮT BUỘC

1. Khởi động: TFT/loa hiển thị/phát test; QR quét ra đúng STK, tên bank, amount, memo.
2. Gửi webhook giả lập 150000 `DH000128`, đúng tài khoản → đơn PAID; ESP32 đọc **một lần** và hiện đủ nội dung.
3. Gửi lại cùng webhook 3 lần → không tăng số giao dịch, không phát trùng.
4. Nhận tiền 50000 không có mã đơn → vẫn phát `Đã nhận năm mươi nghìn đồng` nếu announce_unmatched_incoming bật, nhưng đơn đang chờ không tự PAID.
5. Sai account, `out`, số tiền âm/0, thiếu field, sai chữ ký, timestamp cũ → bị từ chối/quarantine, không phát.
6. Nội dung `DH0001288` không được khớp `DH000128`.
7. Đơn 150000 nhận 149000 hoặc 151000 → chưa PAID, đưa NEEDS_REVIEW.
8. Wi-Fi rớt lúc có chuyển tiền → khi kết nối lại đồng bộ, không mất và không phát trùng sau reboot.
9. Âm thanh kiểm thử các số 1.000, 1.001, 1.005, 10.015, 21.000, 25.000, 105.000, 125.000, 1.000.001.
10. Chạy mock 100 webhook liên tiếp + duplicate/retry; không crash, backend không rơi sự kiện.
11. Mọi event nhận được lưu đúng `content` nguyên bản; UI cắt dòng an toàn, xử lý Unicode không lỗi crash.
12. Reconciliation API v2 với dữ liệu UUID không được nhân đôi giao dịch numeric ID đã nhận qua webhook.

## 12. THỨ TỰ CODING AGENT PHẢI THỰC HIỆN

**Milestone 1 — Device smoke test:** ESP32 boot, TFT ILI9488 hiển thị text/QR mẫu, MAX98357A phát 1 MP3 LittleFS; xác nhận pin + âm thanh. Không sang bước tiếp nếu phần cứng chưa ổn.

**Milestone 2 — Backend mock:** Node.js/SQLite tạo đơn, tạo VietQR thực, giả lập giao dịch, endpoint `/v1/device/*`, kiểm tra thiết bị qua Bearer token.

**Milestone 3 — Firmware online:** ESP32 kết nối Wi-Fi HTTPS, poll JSON, hiện QR thật, nhận sự kiện và đọc số tiền, ACK chống trùng.

**Milestone 4 — SePay thật:** Webhook HMAC, đối soát API v2, mapping content và mã đơn, kiểm thử sandbox trước khi thử chuyển khoản thật nhỏ.

**Milestone 5 — Hoàn thiện:** Wi-Fi auto reconnect, UI tiếng Việt, chống trùng sau reboot, hàng đợi, logs, README, ảnh/video kết quả test.

## 13. TIÊU CHÍ CHẤP NHẬN / DEFINITION OF DONE

- Clone repo và theo README có thể chạy mock backend + firmware trên phần cứng đúng model, không phải tự viết tiếp các module cốt lõi.
- ESP32 có thể hiển thị QR VietQR thật chứa đúng thông tin và khách quét được.
- Nhận webhook SePay đã xác thực: đọc số tiền **đúng bằng tiếng Việt**, hiện **nội dung chuyển khoản thực**.
- Chỉ xác nhận PAID cho đơn hợp lệ; giao dịch không có đơn vẫn có thể thông báo loa theo setting.
- Webhook retry, polling, restart, Wi-Fi loss không tạo thông báo phát trùng dễ dàng và không làm rơi giao dịch trong khoảng lưu trữ.
- Đã chạy test và liệt kê chính xác những gì đã test/thứ chưa test; không tuyên bố chạy thật nếu không có thiết bị hay credentials.

## 14. CẤU HÌNH CẦN NGƯỜI DÙNG TỰ ĐIỀN

```env
# backend/.env (KHÔNG COMMIT)
PORT=3000
PUBLIC_BASE_URL=https://example.com
MOCK_MODE=true
SEPAY_API_TOKEN=
SEPAY_WEBHOOK_SECRET=
BANK_BIN=970436
BANK_ACCOUNT_NUMBER=YOUR_ACCOUNT_NUMBER
BANK_ACCOUNT_NAME=YOUR_ACCOUNT_NAME
DEVICE_ID=ESP32-001
# Lưu token dưới dạng hash ở backend; provisioning token thật chỉ trên thiết bị
```

Wi-Fi SSID/password, device token và TLS trust được provisioning qua local config cho thiết bị; không commit giá trị thật.

## 15. CÁC NGUỒN API VÀ RÀNG BUỘC ĐÃ XÁC MINH (09/11/2026)

1. SePay Webhook fields, retry, body success: https://developer.sepay.vn/vi/sepay-webhooks/tich-hop-webhook
2. SePay HMAC-SHA256: https://developer.sepay.vn/vi/sepay-webhooks/xac-thuc
3. SePay API v2 GET transactions, fields, since_id: https://developer.sepay.vn/vi/sepay-api/v2/giao-dich/danh-sach
4. SePay API v2 authentication/rate limit: https://developer.sepay.vn/vi/sepay-api/v2/xac-thuc
5. VietQR library (offline gen, không dùng /v2/generate mới): https://www.vietqr.io/generate
6. VietQR Quick Link: https://www.vietqr.io/qr-dong
7. VietQR `/v2/banks` là **danh sách ngân hàng**, KHÔNG phải API lịch sử chuyển khoản: https://www.vietqr.io/api-bank

## 16. LỆNH CHO CODING AGENT

**Hãy coi toàn bộ tài liệu này là yêu cầu triển khai.** Đầu tiên viết cây thư mục, pin mapping, API contract rồi code **đầy đủ** backend mock và firmware theo các milestone. Không chỉ giải thích. Chọn phiên bản thư viện thực tế tương thích, ghi vào README. Ưu tiên ESP32 DevKit V1 + TFT ILI9488 SPI + MAX98357A; đừng lẫn chân ESP32-S3. Tự viết các unit test cần thiết, nêu rõ phần nào phải kiểm tra trên thiết bị và chỉ bật SePay production sau khi test sandbox thành công.
