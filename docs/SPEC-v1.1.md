# SmartPay v1.1 — hợp đồng triển khai hiện hành

Ngày 09/11/2026. Thay thế các phần mâu thuẫn của spec v1.0. Không còn SePay.

## Phạm vi MVP

- Một tài khoản MBBank, một ESP32 DevKit V1/WROOM-32, TFT ILI9488 SPI 480×320, MAX98357A + loa.
- Backend Node.js 24 + TypeScript, Express 5, `node:sqlite`, zod, pino, VietQR 1.2.0; tests Vitest + Supertest.
- POS HTML/CSS/JavaScript thuần; không MQTT, cảm ứng, TTS tự do hay webhook trong phiên bản này.
- ESP32 chỉ gọi backend. Secret ngân hàng không xuất hiện trong sketch, POS hoặc git.

## ThueAPI MBBank

- GET cố định `https://thueapi.pro/historyapimbbankv2/<token>`; token từ biến môi trường.
- Response phải có `status=success`, `transactions` là mảng. HTTP 200 không đủ để kết luận thành công.
- Field: `transactionID` string/number nguyên an toàn; `amount` số nguyên hoặc chuỗi chữ số; `description` nguyên bản; `transactionDate` theo `dd/MM/yyyy` hoặc `dd/MM/yyyy HH:mm:ss`; `type` IN/OUT.
- Ngày giờ có múi giờ Asia/Ho_Chi_Minh (UTC+7). Định dạng khác đưa quarantine, không tự đoán.
- Chu kỳ tối thiểu 30 giây, không chạy chồng request, timeout 15 giây, backoff + Retry-After.
- Nguồn gắn với tài khoản do admin xác nhận: response không chứa số tài khoản để kiểm tra độc lập từng giao dịch.
- Lần đầu import baseline im lặng, sau đó mới cho tạo đơn production. Persist DB qua restart.
- Dedupe theo source + provider ID, không giả định ID tăng dần. ID cũ đổi nội dung đưa quarantine.
- Chưa xác minh phân trang/giới hạn lịch sử: không bảo đảm phục hồi khi offline dài hơn cửa sổ lịch sử nguồn.

## Đơn và QR

- Đơn `DH` + 16 ký tự hex ngẫu nhiên, tối đa một PENDING/thiết bị, TTL cấu hình 60–86400 giây.
- Sinh QR offline qua `vietqr.generateQR`, BIN 970422 mặc định, tài khoản/số tiền/memo từ cấu hình và đơn.
- POS hiển thị PNG data URL; firmware nhận chuỗi EMV và tự vẽ ma trận, quiet zone 4 module.
- Không gọi API generate cũ, không QR-encode URL ảnh.
- Chỉ PAID khi IN, amount khớp, đúng nguồn đã xác minh, mã đơn có ranh giới ASCII chính xác, PENDING, thời gian hợp lệ, chưa dùng giao dịch.
- Timestamp độ chính xác ngày: NEEDS_REVIEW khi có đơn, không tự PAID/không phát loa. Không cộng nhiều khoản hoặc tự chấp nhận thừa/thiếu tiền.
- Ghi transaction/order/event nguyên tử; UNIQUE(device,transaction). ORDER_PAID hoặc BANK_CREDIT, không phát cả hai.

## Hợp đồng HTTP

HTTPS ngoài máy local. Bearer token admin và device riêng biệt, hash token device trong SQLite. Mọi truy cập account thuộc một source/device cấu hình; không phải hệ thống multi-tenant.

| Method | Path | Quyền / kết quả |
|---|---|---|
| GET | /healthz | Public; status/mode |
| GET | /v1/device/config | Device; settings |
| GET | /v1/device/current-order | Device; null hoặc order_id/status/amount/memo/qr_payload/expires_at |
| GET | /v1/device/events?after_id=0 | Device; tối đa 10 event chưa ACK, thứ tự ID |
| POST | /v1/device/events/:id/ack | Device; `{device_id,status:"PLAYED"}`, idempotent |
| POST | /v1/orders | Admin; `{amount,device_id}`, trả order + QR |
| GET | /v1/orders/:id | Admin; trạng thái |
| GET | /v1/transactions | Admin; limit, before (ISO), status |
| GET | /v1/admin/sync-status | Admin; baseline/last_success/error/quarantine count |
| GET | /v1/admin/quarantine | Admin; metadata, không raw payload |
| POST | /dev/simulate-credit | Admin, chỉ mock; schema ThueAPI |

## Firmware / âm thanh

- Bổ sung Wi-Fi provisioning qua SoftAP + DNS wildcard + HTTP portal bằng thư viện đi kèm core (WebServer/DNSServer/Preferences). SSID `TingBox-XXXXXX` theo chip, password ngẫu nhiên trên TFT/USB Serial; giao diện tiếng Việt có dấu, quét mạng bất đồng bộ, nhập mạng ẩn, hiện/ẩn mật khẩu và tự cập nhật trạng thái. Lưu credentials NVS sau kết nối thành công, fallback config.h. Cửa sổ boot/auto-recovery và giới hạn theo docs/WIFI_SETUP.md. Không hứa captive pop-up trên mọi điện thoại, không coi Wi-Fi connected là backend online.

- Giữ pin mapping v1.0, không nhầm ESP32-S3. Kiểm tra điện áp TFT và GPIO5 strapping trên module thật.
- Task network core 0 xử lý HTTPS; loop core 1 duy trì decoder/UI. Không gọi HTTP blocking trong vòng decode audio.
- Một event in-flight, NVS lưu sau phát thành công, sau đó ACK; không ACK trước khi phát.
- Không cam kết exactly-once khi cắt nguồn giữa playback và ghi NVS. Xóa NVS có thể phát lại event chưa ACK.
- Bộ 22 MP3 thực (43.780 byte) trong firmware/assets/audio; chữ số, mười/mươi, trăm/nghìn/triệu/tỷ, mốt/lăm/linh, cảm ơn, đã nhận được, Việt Nam đồng.
- Giọng Linh vi_VN tạo trên Mac, không tạo trên ESP32. Có script tái tạo, manifest SHA-256, mã PROGMEM tạo sẵn. Quyền phân phối giọng hệ thống chưa được xác minh.
- Câu đọc: “Cảm ơn, đã nhận được <số tiền> Việt Nam đồng”. Ghép luồng MP3 trực tiếp từ Flash, một decoder mỗi câu, không tải audio mạng, không giữ cả câu trong RAM.
- Audio nằm trong app Flash, không phải ROM bất biến, không cần LittleFS/microSD/upload dữ liệu riêng; build bình thường đã chứa âm thanh.
- Đọc tiền 0–999999999999; thanh toán phải >0. Quy ước nghìn/linh/bốn, phân biệt mười/mươi.
- Nội dung gốc lưu backend; TFT mặc định hiển thị bản chuyển không dấu an toàn (160 ký tự). Font Unicode đầy đủ là nâng cấp, không tuyên bố đã hỗ trợ dấu.
- Tồn đọng được phát tuần tự toàn bộ, không bỏ event; có thể phát nhiều thông báo khi offline lâu. Giới hạn/tổng kết backlog là nâng cấp, chưa triển khai.

## Nghiệm thu

Backend mock build/test; QR payload và CRC, parse dữ liệu lỗi, idempotency, baseline, restart và rollback. Audio decode đủ clip, kiểm tra manifest và thuật toán C++.

Hardware smoke test trước khi nối mạng: QR thử + đọc 150000. Sau đó quét bằng hai app ngân hàng, thử mạng mất/khôi phục, restart, ACK và giao dịch nhỏ. Không đánh dấu đạt phần cứng nếu chỉ có compile.

Production cần tài khoản thật, token mới, xác minh ánh xạ nguồn, HTTPS CA hợp lệ, backup/retention SQLite và kiểm tra thời gian thực tế của provider. Không ghi token đã chia sẻ vào repo.