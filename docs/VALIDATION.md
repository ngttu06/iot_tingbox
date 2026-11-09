# Kết quả kiểm thử — 09/11/2026

## Âm chào mừng TingBox và kết nối Wi-Fi

- Thêm `chao_mung` (“Chào mừng bạn đến với TingBox.”) và `ket_noi_wifi` (“Đã kết nối Wi-Fi thành công.”), giọng Linh cùng định dạng audio hiện tại. Tổng 24 clip, 59.662 byte; 22 clip thanh toán cũ không thay đổi.
- Host test thứ tự chào trước/Wi-Fi sau, bật nguồn chưa có Wi-Fi, Wi-Fi đến sớm, chờ audio/thanh toán đang phát, không lặp sau 100 thông báo kết nối và reset khi khởi động lại: PASS với AddressSanitizer/UndefinedBehaviorSanitizer.
- Host test nguồn clip với sanitizer, money/announcement test gồm 100.000 số liên tiếp và 12 Wi-Fi validation/escaping cases: PASS.
- Kiểm tra SHA-256, định dạng MPEG/bit reservoir, decode 24 MP3 và các luồng ghép (gồm hai câu khởi động): PASS. Sinh lại mã nhúng cho cùng hash; Python compile và git diff whitespace check: PASS.
- Tín hiệu kết nối được gửi qua queue riêng ngay khi network task thấy Wi-Fi kết nối thành công, không chờ kết quả backend và không bị ghi đè bởi queue cập nhật màn hình. Chỉ audio loop sử dụng decoder.
- ESP32 compile/link và tạo binary: PASS; app 1.336.482 byte (42%), global RAM 48.924 byte (14%). Binary local nằm tại `/Users/ngttu/Workspace/iot_tingbox/firmware/build/SmartPayESP32.ino.bin`; chứa cấu hình local, không chia sẻ công khai.
- Chưa nạp board/nghe thử: chỉ thấy cổng Bluetooth, chưa thấy USB ESP32. Cần thử trên thiết bị: bật offline rồi cấu hình Wi-Fi, Wi-Fi kết nối nhanh, backend không hoạt động, giao dịch đến khi đang chào, mất/kết nối lại Wi-Fi và tắt/bật nguồn.

## Thiết kế lại audio ghép số tiền

- Giọng Linh vi_VN tạo trên Mac: 22 MP3, tổng 43.780 byte; nhúng PROGMEM trong app Flash. ESP32 không tạo giọng đọc, không gọi TTS mạng.
- Câu 10000: `cam_on da_nhan_duoc muoi nghin viet_nam_dong`.
- Money/announcement tests và 100.000 số liên tiếp PASS.
- Host test nguồn clip: đọc xuyên ranh giới với block 1/7/1024/4096 byte, seek, EOF, nguồn lỗi, đối chiếu dữ liệu nhúng của 5 số tiền PASS. Stub chỉ dùng trên host; build firmware kiểm tra giao diện thư viện thật.
- SHA-256, 22 MP3 decode, MPEG frame headers, bit reservoir, ba luồng nối tiếp (10000, 150000, toàn bộ clip) PASS; embed tái lập cùng hash.
- ESP32 build PASS: app 1.319.866 byte (41%), global RAM 48.916 byte (14%); core/libraries theo cấu hình hiện tại. Host test nguồn clip với AddressSanitizer/UndefinedBehaviorSanitizer PASS.
- Chưa nạp board hoặc nghe loa: không thấy cổng USB ESP32. Không suy ra độ tự nhiên hoặc bảo đảm không click từ kiểm thử phần mềm. Preview 10000 tại `/tmp/tingbox_10000.mp3` (file tạm).
- Quyền phân phối giọng hệ thống Linh chưa được xác minh. Số liệu eSpeak bên dưới là baseline lịch sử, đã được thay thế.

## Bổ sung Wi-Fi setup portal

- Firmware sau thêm SoftAP/DNSServer/WebServer: compile PASS, script build exit 0; app 1.313.034 byte (41%), global RAM 48.932 byte (14%).
- 12 tests C++ validation SSID/password và HTML escaping PASS; backend 31/31 vẫn PASS; money tests vẫn PASS.
- Đã sửa xung đột alias FS khi TFT_eSPI và WebServer dùng chung bằng khai báo rõ `fs::FS`.
- Cấu hình local .env/config.h có credentials được chủ thiết bị cung cấp, bị Git ignore; **binary local hiện chứa Wi-Fi/device token thật**, không chia sẻ bin/merged.bin công khai. BACKEND_URL và ROOT_CA vẫn cần điền trước khi online.
- Chưa thử AP/DHCP/DNS/captive pop-up Android/iOS, lưu NVS, timeout/recovery hoặc tải HTTPS+audio đồng thời trên board thật. Xem docs/WIFI_SETUP.md cho checklist.
- Số liệu/build config bên dưới là baseline trước khi thêm portal, giữ để truy vết; không nhầm với kết quả mới ở mục này.

## Đã thực hiện

| Hạng mục | Kết quả |
|---|---|
| Node.js | 24.21.0, npm 11.19.0 |
| Backend TypeScript build | PASS |
| Typecheck cả test | PASS |
| Vitest/Supertest | 31 tests PASS |
| HTTP server thực (subprocess) | PASS: health, trang POS, tạo QR, simulate credit, PAID |
| QR | PNG decode bằng jsQR khớp payload, CRC16 kiểm tra độc lập |
| C++ đọc tiền | 22 trường hợp cụ thể + 100.000 số liên tiếp PASS |
| MP3 | 21 clip, 38.076 byte, mono 22050 Hz; SHA256, ffprobe và ffmpeg decode PASS |
| Sinh mã audio | Tái chạy embed cho cùng hash C++ |
| Python/shell/browser JS | py_compile, bash -n, node --check PASS |
| npm audit | 0 vulnerabilities (cả dev và production tại thời điểm kiểm tra) |
| Secret scan deliverables | Không có token ThueAPI đã chia sẻ trong source/assets/docs |
| API thật, chỉ đọc schema | HTTP 200 success; IN/OUT; transactionDate có dạng dd/MM/yyyy HH:mm:ss; không lưu/xuất giá trị giao dịch |
| ESP32 compile | PASS; core 3.3.7, TFT_eSPI 2.5.43, ArduinoJson 7.4.2, ESP8266Audio 2.4.1, vendor QRCode 0.0.1 |

Firmware dùng 1.259.290 byte (40% app partition 3.145.728 byte), RAM global 48.908 byte (14%), còn 278.772 byte cho local/runtime theo báo cáo compiler. Đây không phải phép đo heap cao điểm khi chạy TLS/MP3 trên thiết bị.

Artifact local: `firmware/build/SmartPayESP32.ino.bin` và `.merged.bin` (4 MB). **Build dùng config.example.h không có Wi-Fi/device token/CA thật; không coi đây là firmware production đã provision.**

Core/tool archives chính thức được tải và xác minh SHA256; do Board Manager tải cả toolchain cho chip khác rất lớn, validation dùng cùng core chính thức dưới local hardware và compiler/libraries ESP32 được giải nén trong `.tools/`. Script firmware.sh tự nhận diện môi trường này. Clone mới dùng setup qua Board Manager bình thường; không cần commit `.tools/`.

Đã sửa các lỗi được test phát hiện: SQL placeholder count, đường dẫn static POS, timestamp giây so với mili giây, phân biệt ACK đang phát/đã phát, build flags ESP32 và header QR trùng ESP-IDF.

## Chưa thực hiện / không tuyên bố đạt

- Không phát hiện ESP32 USB; chưa flash board, kiểm tra dây/pin/nguồn, TFT, I2S hoặc nghe loa.
- Chưa thử quét bằng hai ứng dụng ngân hàng thật, giao dịch thật nhỏ hoặc liên kết tài khoản production.
- Chưa kiểm chứng cắt nguồn/Wi-Fi/TLS/NVS trên thiết bị, RAM/heap cao điểm và độ mượt âm thanh.
- Chưa browser automation cho tương tác DOM; chỉ HTTP static và API integration.
- Chưa xác minh phân trang/rate limits/cửa sổ lịch sử theo hợp đồng nhà cung cấp. Có lần request đọc schema trả lỗi HTTP, lần retry bằng Accept/User-Agent trả success; không suy ra SLA.
- Giọng máy eSpeak NG tạo thật, file decode được; chưa có đánh giá nghe hiểu/độ tự nhiên trên loa. Không quảng cáo là giọng thu phòng thu.
- Không có retention job, UI duyệt tay, multi-device provisioning, OTA, touch hoặc Unicode font đầy đủ.

## Chạy lại

```sh
cd /Users/ngttu/Workspace/iot_tingbox/backend
npm ci
npm run build
npm test
npm audit
python3 /Users/ngttu/Workspace/iot_tingbox/tools/audio/verify.py
bash /Users/ngttu/Workspace/iot_tingbox/tools/firmware.sh build
```

Audio verification cần FFmpeg/ffprobe trên máy dev. Build firmware chỉ cần clip và mã nhúng có sẵn, script embed dùng Python chuẩn.