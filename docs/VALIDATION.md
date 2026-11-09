# Kết quả kiểm thử — 09/11/2026

## Bản thử MP3-TF-16P / DFPlayer

- Commit mốc MAX98357A/I2S: `c630dafb4b6210d990cd48671d7c1eeda7935b1a`. Đã sao lưu binary trước thử tại `C:\Users\info\Documents\iot_tingbox\.tools\rollback-max98357a`; không commit Flash hoặc secrets. Git chưa có identity, commit tác vụ dùng `Cline <cline@localhost>` qua `git -c`, không đổi cấu hình toàn cục.
- Thêm backend UART1 9600 baud và BUSY active LOW (S3 RX4/TX5/BUSY6). Giữ backend I2S sau compile flag; script chọn `-Audio dfplayer` hoặc `-Audio i2s`, build ở các thư mục tách riêng. Không thêm dependency.
- Chuỗi MP3 chỉ thành công sau từng chu kỳ BUSY LOW/HIGH có debounce; timeout không bắt đầu/kẹt LOW, BUSY LOW trước lệnh, lỗi module/SD removed/UART làm playback thất bại. Giao diện audio cũ giữ nguyên để payment chỉ ACK sau khi sequence hoàn tất và ghi NVS. Chưa thử giao dịch thật; BUSY không xác nhận đầu ra âm học hoặc nội dung thẻ.
- Host DFPlayer test với `-Wall -Wextra -pedantic`: PASS cho frame/checksum, parser nhiễu/checksum sai, mapping, toàn bộ clip tiền mẫu, sequence, debounce, timeout, retry, lỗi module và tràn bộ đếm thời gian. Mapping chân S3/WROOM PASS. Audio 24 clip/hash/ffmpeg và các executable regression startup/sequence/money/serial: PASS.
- Công cụ `C:\Users\info\Documents\iot_tingbox\tools\audio\prepare_dfplayer.py` tạo 24 file có hash hợp lệ tại `C:\Users\info\Documents\iot_tingbox\.tools\dfplayer-sd\mp3`, mapping cố định `0001`..`0024`; chế độ `--check` PASS. Không format hay ghi trực tiếp thẻ của người dùng.
- ESP32-S3/core 3.3.12 compile/link **cả hai backend PASS**: DFPlayer app **1.131.643 byte (35%)**, RAM tĩnh **48.484 byte (14%)**; I2S app **1.319.295 byte (41%)**, RAM tĩnh **48.516 byte (14%)**. Log `trial-dfplayer-build.log` và `trial-i2s-build.log` tại `C:\Users\info\Documents\iot_tingbox\.tools`; stderr rỗng.
- Hướng dẫn chân, thẻ, giới hạn clone và rollback tại `C:\Users\info\Documents\iot_tingbox\docs\DFPLAYER_TRIAL.md`. Người dùng đã xác nhận chuẩn bị thẻ/đổi dây trước nạp thử. Kết quả build/host không thay thế nghiệm thu phần cứng.
- Đã nạp commit thử `3803622` qua COM7, esptool xác minh hash ghi PASS. App SHA-256 `F66BAC1DFA0538885811E76679517E507A362A6CA07581F72A313F98D0010B1E`. Lần đầu cổng bị Serial Monitor giữ; dừng riêng monitor rồi nạp lại thành công.
- Chạy thật lần đầu và thử lại sau khi người dùng xác nhận vừa nối BUSY→GPIO6: câu chào `/mp3/0001.mp3`, Wi-Fi `/mp3/0002.mp3` và `audio-test` đều **FAILED: BUSY did not go LOW**. Không nhận frame phản hồi UART hợp lệ trong khoảng log; không suy ra chắc chắn module hỏng hay file thẻ sai. ESP32 không thấy panic trong khoảng quan sát, Wi-Fi connected/backend offline, heap 236.540 byte. **Chưa nghiệm thu audio DFPlayer**.
- Log local đã che mật khẩu: `C:\Users\info\Documents\iot_tingbox\.tools\trial-dfplayer-live.log` và `C:\Users\info\Documents\iot_tingbox\.tools\trial-dfplayer-busy-retry.log`. Chưa rollback: người dùng yêu cầu giữ DFPlayer để thử lại sau khi nối BUSY; không nạp I2S khi đầu ra BUSY còn nối GPIO6 (I2S biến GPIO6 thành đầu ra). Cổng Serial được đóng sau thử.

## Chẩn đoán Serial và loa trên board

- Ban đầu Windows thấy COM8 (USB Serial Device), sau đó thiết bị hiện tại là **COM7, USB-Enhanced-SERIAL CH343**, VID 1A86/PID 55D3. Mở COM7 bị `Access denied`; sau khi dừng riêng tiến trình Arduino `serial-monitor`, mở cổng và đọc boot 115200 baud thành công. Không đóng Arduino IDE. Firmware dùng UART0, USB CDC on boot tắt; console không nằm trên USB native S3.
- Bổ sung log cấu hình/bắt đầu/kết thúc audio, dọn I2S nếu decoder khởi tạo thất bại, và console giới hạn bộ đệm: `help`, `status`, `audio-test`. Test audio chỉ phát câu chào, không tạo/ACK giao dịch; từ chối khi audio/thanh toán đang bận, hàng đợi có thanh toán hoặc trạng thái lỗi.
- Build ESP32-S3/core 3.3.12 PASS: app **1.319.295 byte (41%)**, RAM tĩnh **48.516 byte (14%)**. Upload COM7 PASS, hash dữ liệu ghi được esptool xác minh; không erase toàn bộ Flash/NVS. Binary SHA-256 `FAA138E089114C87DA190E5359D41527F83C9A84D3EAF52FAF77787725BD2BD8`.
- Chạy thật: boot nhận Flash 16 MB/PSRAM 8 MB; log `BCLK=4, LRC=5, DIN=6, gain=0.45`. Câu chào đọc **7523/7523 byte**, câu kết nối Wi-Fi **8359/8359 byte**, đều `Audio finished: OK`. Gửi `audio-test` qua Serial và nhận hoàn tất 7523/7523 byte. `status` xác nhận Wi-Fi connected, backend offline, audio idle, last_audio OK; heap tại các lần đọc 236.280 byte. Không thấy panic trong log quan sát.
- Thử `help` CRLF, dòng 100 ký tự rồi `status`, hai `audio-test` liên tiếp: console hồi phục sau dòng quá dài, lần thử thứ hai bị từ chối khi đang phát; lần đầu hoàn tất OK. Cổng được đóng/giải phóng sau kiểm tra để người dùng mở monitor.
- Host test bộ đọc lệnh (bật `-Wall -Wextra -pedantic`), startup, clip sequence, mapping S3/WROOM và đọc tiền 100.000 số: PASS. 24 MP3/hash/ffmpeg decode và luồng ghép: PASS với `python -X utf8` trên Windows. Build/upload stderr rỗng; git diff whitespace PASS (chỉ cảnh báo LF/CRLF của audio).
- Log local đã che mật khẩu AP: `C:\Users\info\Documents\iot_tingbox\.tools\serial-audio-live.log`, `C:\Users\info\Documents\iot_tingbox\.tools\serial-audio-commands.log`; build/upload tương ứng `serial-audio-build.log`/`serial-audio-upload.log` cùng thư mục.
- **Chưa xác nhận nghe được loa hoặc đo dạng sóng I2S/nguồn ampli.** Log hoàn tất chỉ xác nhận luồng phần mềm. Nếu loa vẫn im: ngắt nguồn rồi kiểm tra MAX98357A BCLK→GPIO4/LRC→GPIO5/DIN→GPIO6, GND chung, VIN theo module, SD/EN không bị kéo LOW và loa ở SPK+/SPK-; sau đó đo nguồn/tín hiệu. Backend offline nên chưa kiểm thử phát thanh toán thật.

## Nạp lại để kiểm tra loa, chưa đấu màn hình

- Nạp firmware mới nhất ESP32-S3 qua COM7 PASS; đã đóng Arduino Serial Monitor đang giữ cổng. esptool xác minh hash dữ liệu ghi và reset qua RTS; không erase toàn bộ Flash/NVS.
- Log upload: `C:\Users\info\Documents\iot_tingbox\.tools\speaker-reflash.log`. Theo dõi Serial sau reset 16 giây: ESP32-S3, flash=16.777.216, PSRAM=8.388.608, AP `TingBox-04291c`; không thấy panic hoặc `Startup audio failed` trong thời gian quan sát. Log tại `C:\Users\info\Documents\iot_tingbox\.tools\speaker-reflash-boot.log` đã che mật khẩu AP.
- Firmware dùng GPIO4 BCLK / GPIO5 LRC / GPIO6 DIN, gain 0.45; chế độ bình thường có câu chào khi bật nguồn. Chạy lại các executable host test startup/audio/pin S3 sẵn có: PASS. Chưa xác nhận nghe được loa, nguồn MAX98357A hoặc dạng sóng I2S; không suy ra đầu ra âm thanh thực từ log khởi động.

## Hạn chế tự zoom khi nhập Wi-Fi

- Ô tên Wi-Fi/mật khẩu đặt font riêng tối thiểu 18px, không phụ thuộc cỡ chữ phần tử cha; chuẩn hóa text-size-adjust 100%. Không thêm `user-scalable=no` hoặc giới hạn `maximum-scale`, giữ pinch zoom để hỗ trợ khả năng tiếp cận.
- Chrome headless: toàn bộ kiểm thử portal PASS; bổ sung kiểm tra font khi cha có font 12/20px, focus hai ô không đổi visualViewport.scale và không khóa zoom, tại 5 kích thước màn hình. Chromium không mô phỏng cơ chế tự zoom của iOS: cần xác nhận Safari/captive browser trên iPhone thật.
- Build ESP32-S3 PASS: chương trình 1.318.007 byte (41%), RAM tĩnh 48.476 byte (14%). Log: `C:\Users\info\Documents\iot_tingbox\.tools\portal-focus-build.log`; stderr rỗng. Chưa nạp bản sửa xuống thiết bị.

## Trang Wi-Fi tiếng Việt và SSID TingBox-XXXXXX

- Đổi tiền tố AP thành `TingBox-`, giữ sáu ký tự mã chip. Không đổi mật khẩu AP ngẫu nhiên hoặc địa chỉ `http://192.168.4.1/`.
- Giao diện HTML/CSS/JS trong Flash: danh sách Wi-Fi, sóng mạnh/vừa/yếu, mạng mở, nhập mạng ẩn, hiện/ẩn mật khẩu, trạng thái tự cập nhật và form dự phòng khi tắt JavaScript. API quét bất đồng bộ có CSRF, giới hạn tần suất/kích thước kết quả và loại trừ lúc thử kết nối.
- Host C++ Wi-Fi: **16/16 PASS**, gồm SSID Unicode, giới hạn byte, mật khẩu, HTML escaping và chuỗi giống template token. Không chạy sanitizer trên Windows.
- Chrome headless với API giả lập, Node 22.14.0, không thêm npm dependency: **PASS** cho chọn mạng, SSID chèn HTML, validation, hiện/ẩn mật khẩu, mạng mở/nhập thủ công, gửi cấu hình, lỗi/thử lại/thành công, quét đang chạy/rỗng/lỗi, CSP, reduced motion và tắt JavaScript. Không phát hiện lỗi JS/CSP.
- Không tràn ngang ở 320/375/768/812/1440 px; đã xem ảnh mobile 320/375 px. Ảnh kiểm thử tại `C:\Users\info\Documents\iot_tingbox\.tools\wifi-portal-test`. API giả lập không xác minh radio, DHCP, NVS hay captive browser Android/iOS.
- Build cuối ESP32-S3/core 3.3.12 bằng `C:\Users\info\Documents\iot_tingbox\tools\firmware.ps1 -Action build -Board esp32s3`: **PASS**, chương trình **1.317.687 byte (41%)**, RAM tĩnh **48.476 byte (14%)**. Đã xác minh bản sao header trong build chứa nội dung giao diện cuối. Log tại `C:\Users\info\Documents\iot_tingbox\.tools\portal-build-final.log`, không có lỗi trong stderr. Binary tại `C:\Users\info\Documents\iot_tingbox\firmware\build\esp32s3\SmartPayESP32.ino.bin`.
- `node --check` cho script kiểm thử và `git diff --check`: PASS (Git có cảnh báo LF/CRLF của các file audio đã sửa từ trước).
- **Chưa nạp bản mới**, chưa build lại WROOM trong lượt này. Cần nghiệm thu quét/kết nối/lưu trên board, hai client, Android/iOS và khả năng đáp ứng khi HTTPS/audio đang hoạt động theo checklist Wi-Fi. Các kết quả nạp và SSID SmartPay bên dưới là lịch sử trước thay đổi này.

## Nạp ESP32-S3 N16R8 trên Windows

- Board thật tại COM7, USB-UART CH343: ESP32-S3 revision 0.2, Flash 16 MB QIO và PSRAM 8 MB OPI. Đã sao lưu đủ 16.777.216 byte Flash trước khi ghi tại `C:\Users\info\Documents\iot_tingbox\.tools\esp32s3-before-flash.bin` (không commit/chia sẻ vì có thể chứa dữ liệu cũ).
- Build cuối dùng Arduino ESP32 core **3.3.12**, ArduinoJson 7.4.2, TFT_eSPI 2.5.43 và ESP8266Audio 2.4.1. Arduino IDE đã đổi core hệ thống trong phiên làm việc; không nhầm bản build cuối với core 3.3.7 của các kiểm thử lịch sử bên dưới. Script PowerShell setup được đồng bộ về 3.3.12.
- Compile/link PASS: app 1.294.951 byte (41% partition 3 MB), RAM tĩnh 48.452 byte (14%). Binary: `C:\Users\info\Documents\iot_tingbox\firmware\build\esp32s3\SmartPayESP32.ino.bin`; SHA-256 `4B6A387B7BDFCDE513F922270667B57DDC77D0D1DF119B4C6799A9CBCD0E9287`.
- Upload bootloader/partition/app qua COM7 PASS, esptool xác minh hash dữ liệu đã ghi. Không erase toàn bộ Flash; có thể vẫn còn NVS/dữ liệu từ firmware trước.
- Kiểm tra chạy thật sau reset, quan sát Serial 18 giây: nhận đúng `flash=16777216, PSRAM=8388608`, mở AP `SmartPay-04291c` và URL `http://192.168.4.1/`; không có panic trong khoảng quan sát. Mật khẩu AP ngẫu nhiên chỉ nằm trong log local/TFT, không ghi vào tài liệu.
- Đã phát hiện StoreProhibited khi khởi tạo TFT ở lần nạp trước; backtrace trỏ đến `TFT_eSPI::begin_tft_write`. Chuyển S3 sang HSPI/SPI3 giải quyết lỗi khởi động trong lần kiểm tra trên. Các chân SPI vẫn dùng GPIO12/13/11 qua GPIO matrix.
- Host tests mapping S3 và WROOM, đọc tiền 100.000 số, Wi-Fi validation, clip sequence và startup sounds: PASS. Audio 24 clip/hash/decode PASS; sinh mã nhúng tái lập cùng hash; PowerShell syntax và git diff whitespace PASS. Các host test Windows không chạy sanitizer.
- Chưa đấu màn hình/loa: **chưa xác minh hiển thị hoặc âm thanh thực tế**. Chưa thử trình duyệt kết nối portal, lưu Wi-Fi, backend hoặc giao dịch thật. Firmware dùng cấu hình mẫu, không có backend/token/CA production.

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