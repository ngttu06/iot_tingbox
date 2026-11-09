# SmartPay ESP32 — ThueAPI MBBank + VietQR

MVP một tài khoản/một thiết bị: web POS tạo QR, backend đồng bộ lịch sử MBBank, ESP32 hiển thị và đọc số tiền tiếng Việt. **Bộ 24 MP3 đã có trong project và nhúng vào app Flash: upload sketch có luôn audio, không chép filesystem riêng, không gen audio trên ESP32.**

Spec hiện hành: `docs/SPEC-v1.1.md`; kết quả kiểm thử: `docs/VALIDATION.md`. Spec gốc giữ để truy vết và có chú thích superseded.

> **Bản thử audio hiện tại dùng MP3-TF-16P / DFPlayer qua UART + BUSY, không dùng I2S mặc định.** Cần thẻ microSD với 24 file đánh số, đổi dây và xác nhận module tương thích trước khi thử. Hướng dẫn tại `C:\Users\info\Documents\iot_tingbox\docs\DFPLAYER_TRIAL.md`. Các bảng MAX98357A bên dưới dành cho backend `-Audio i2s`; quay lại bằng build/nạp `-Audio i2s` hoặc revert commit thử. Mốc khôi phục: `c630daf`.

## Chạy backend mock

Node.js 24 trở lên (dùng SQLite tích hợp, không cần native addon). Từ workspace hiện tại:

```sh
cd /Users/ngttu/Workspace/iot_tingbox/backend
npm ci
cp .env.example .env
npm run build
npm test
npm start
```

Mở trình duyệt tới localhost port 3000. Nhập `ADMIN_TOKEN` trong `.env`, device `ESP32-001`, số tiền rồi tạo đơn. Trong MOCK có nút giả lập thanh toán; **không chuyển tiền thật vào tài khoản mẫu**. Token chỉ giữ trong bộ nhớ trang, không localStorage.

Backend mặc định bind `127.0.0.1`. Nếu ESP32 cần truy cập LAN mock, đặt `HOST=0.0.0.0`, dùng IP máy chủ và `ALLOW_LOCAL_HTTP=1` trên mạng thử nghiệm tin cậy. Không đưa cấu hình HTTP này lên Internet.

## Cấu hình production

1. Đổi token ThueAPI đã lộ trong hội thoại; chỉ điền token mới vào `.env` backend (`THUEAPI_TOKEN`), không paste URL chứa secret vào logs.
2. Điền đúng `BANK_ACCOUNT_NUMBER`, `BANK_ACCOUNT_NAME` in hoa không dấu; mặc định BIN MBBank `970422`.
3. Xác nhận bằng quản trị rằng token thuộc chính tài khoản nhận QR, rồi đặt `PROVIDER_ACCOUNT_VERIFIED=true`. Response không có account number để kiểm tra độc lập.
4. Tạo hai token khác nhau bằng `openssl rand -hex 32`, thay ADMIN_TOKEN/DEVICE_TOKEN, `MOCK_MODE=false`.
5. Đặt backend sau reverse proxy HTTPS, giới hạn quyền `.env`/DB, không public port nội bộ. Chỉ tin proxy do bạn quản lý; app không mặc định trust proxy.
6. Đợi baseline thành công qua `/v1/admin/sync-status`, rồi mới tạo đơn. Baseline chỉ nhập lịch sử, không phát thông báo cũ.
7. Xác minh định dạng ngày giờ provider; hiện hỗ trợ dd/MM/yyyy [HH:mm:ss], UTC+7. Dữ liệu chỉ có ngày không được tự PAID/phát loa; phải xem NEEDS_REVIEW. Định dạng khác quarantine.
8. Thử giao dịch thật nhỏ có kiểm soát, kiểm tra đúng QR/tài khoản/số tiền/mã đơn trên ít nhất hai ứng dụng ngân hàng.

Provider polling mặc định 30 giây, backoff khi lỗi. Chưa xác minh phân trang/giới hạn lịch sử nên không bảo đảm phục hồi sau offline dài. Không có SePay hoặc webhook; không nhập SePay keys.

## Nạp ESP32

**Có Wi-Fi setup portal tiếng Việt:** ESP32 phát `TingBox-XXXXXX` (6 ký tự mã chip), mật khẩu ngẫu nhiên hiện trên TFT hoặc USB Serial. Kết nối bằng điện thoại để chọn Wi-Fi 2.4 GHz, nhập mật khẩu và xem trạng thái tự cập nhật; nếu không tự bật, truy cập `http://192.168.4.1/`. Có quét lại mạng, hiện/ẩn mật khẩu và nhập thủ công cho mạng ẩn. Giao diện hoạt động không cần Internet. Lưu NVS sau khi kết nối thành công, không cần flash lại để đổi mạng. AP tự tắt sau 2 phút có Wi-Fi, mở lại khi mất Wi-Fi 30 giây. Chi tiết và giới hạn: `docs/WIFI_SETUP.md`.

Hỗ trợ ESP32 DevKit V1/WROOM-32 flash ≥4 MB và ESP32-S3 N16R8, TFT ILI9488 **SPI**, MAX98357A. Bảng dưới dành riêng cho WROOM; ESP32-S3 dùng bảng riêng bên dưới. Không dùng cho LCD parallel.

| TFT | GPIO | I2S/MAX98357A | GPIO |
|---|---|---|---|
| SCK | 18 | BCLK | 26 |
| MOSI | 23 | WS/LRC | 25 |
| MISO | 19 | DIN | 22 |
| CS | 5 | VIN | 5V theo module |
| DC | 27 | GND | GND chung |
| RST | 14 | Loa | SPK+/SPK-, không nối GND |

Kiểm tra VCC/backlight TFT theo module thực; GPIO logic 3.3V, không đưa 5V vào GPIO. GPIO5 là chân strapping, kiểm tra khả năng boot khi đấu TFT. Touch chưa dùng.

### ESP32-S3 N16R8 trên Windows

Flash 16 MB QIO, PSRAM 8 MB OPI; nạp qua cổng USB-UART (CH343). Không dùng sơ đồ chân WROOM: GPIO22/23/25 không có trên S3, GPIO26/27 dành cho bộ nhớ.

| TFT ILI9488 SPI | GPIO S3 | MAX98357A | GPIO S3 |
|---|---|---|---|
| SCK | 12 | BCLK | 4 |
| MOSI / SDI | 11 | LRC / WS | 5 |
| MISO / SDO | 13 | DIN | 6 |
| CS | 10 | VIN | Theo module, thường 5V |
| DC / RS | 9 | GND | GND chung |
| RST | 14 | Loa | SPK+/SPK-, không nối GND |

Kiểm tra điện áp VCC/LED của đúng module màn hình; không mặc định mọi module chịu được 5V. Đấu dây khi đã ngắt nguồn. Chưa hỗ trợ touch. Chưa có màn hình vẫn có thể đọc tên/mật khẩu AP qua Serial Monitor 115200 baud; mật khẩu AP đổi mỗi lần mở portal. Không chia sẻ log chứa mật khẩu.

Từ PowerShell, với Arduino CLI trong PATH hoặc Arduino IDE cài tại vị trí mặc định trong LocalAppData:

```powershell
& 'C:\Users\info\Documents\iot_tingbox\tools\firmware.ps1' -Action setup
& 'C:\Users\info\Documents\iot_tingbox\tools\firmware.ps1' -Action upload -Board esp32s3 -Audio i2s -Port COM7
```

Script chọn đúng flash/PSRAM/partition và giữ USB Serial qua UART0 (không bật USB CDC). Build từng board ở thư mục riêng trong `firmware/build/`. Có thể dùng `-Board esp32` để giữ cấu hình WROOM cũ. Khi chưa có `config.h`, firmware dùng `config.example.h`: cài Wi-Fi được nhưng **chưa có backend/token/CA để nhận thanh toán**.

### Kiểm tra Serial và loa không cần backend

- Dùng cổng **USB-UART CH343**, Serial Monitor **115200 baud**; số COM có thể thay đổi khi đổi cổng/cáp USB. Firmware hiện không xuất console qua cổng USB native của S3 (`USB CDC On Boot: Disabled`).
- Chỉ một chương trình được mở cổng COM tại một thời điểm. Nếu báo `Access denied`, đóng Serial Monitor/Plotter khác trước khi đọc hoặc nạp. Mở monitor rồi nhấn RESET/EN để xem log boot; firmware không tự lặp log khởi động.
- Chọn kết thúc dòng Newline hoặc Both NL & CR. Gửi `help` để xem lệnh; `status` trả uptime/heap/trạng thái Wi-Fi, backend và audio, không in mật khẩu hay token.
- Gửi `audio-test` để phát lại câu chào, không cần Wi-Fi/backend, không tạo giao dịch hoặc ACK. Lệnh bị từ chối khi đang phát, có thanh toán đang chờ/xử lý hoặc firmware đang ở trạng thái lỗi. Log `Audio started` và `Audio finished: OK` chỉ xác nhận luồng phần mềm, **không xác nhận loa thực sự phát tiếng**.
- ESP32-S3: MAX98357A **BCLK → GPIO4, LRC/WS → GPIO5, DIN → GPIO6**, GND chung. Khi đã ngắt nguồn, kiểm tra đúng dây, nguồn VIN theo module và loa nối hai đầu SPK+/SPK- (không nối một đầu loa xuống GND). Nếu chân SD/EN bị kéo LOW thì ampli bị tắt; kiểm tra theo đúng module, không đưa 5V vào GPIO ESP32.
- Nếu audio hoàn tất nhưng vẫn im: đo nguồn ampli, kiểm tra SD/EN, dây/loa và dạng sóng BCLK/LRC/DIN. Không đổi pin hoặc tăng gain để thay cho kiểm tra phần cứng.

### Arduino CLI (cách nạp tái lập được)

```sh
bash /Users/ngttu/Workspace/iot_tingbox/tools/firmware.sh setup
cp /Users/ngttu/Workspace/iot_tingbox/firmware/SmartPayESP32/config.example.h /Users/ngttu/Workspace/iot_tingbox/firmware/SmartPayESP32/config.h
# Điền Wi-Fi, backend URL, DEVICE_TOKEN, ROOT_CA trong config.h.
bash /Users/ngttu/Workspace/iot_tingbox/tools/firmware.sh build
bash /Users/ngttu/Workspace/iot_tingbox/tools/firmware.sh upload /dev/cu.YOUR_SERIAL_PORT
```

Script cấu hình TFT bằng build flags, chọn Huge APP 3 MB/no OTA trên flash 4 MB. Tất cả clip nằm trong app; không cần LittleFS uploader. `config.h` bị ignore. Nếu clone ở đường dẫn khác, thay prefix tuyệt đối tương ứng.

### Arduino IDE

Cài Espressif core **3.3.7**, ArduinoJson **7.4.2**, TFT_eSPI **2.5.43**, ESP8266Audio **2.4.1**. QRCode của Richard Moore **0.0.1** đã vendor kèm license trong sketch để tránh xung đột header với ESP-IDF. Chọn ESP32 Dev Module, flash đúng dung lượng, partition Huge APP. Copy nội dung `firmware/tft/User_Setup.h` vào User_Setup.h của TFT_eSPI (sao lưu cấu hình cũ). Mở `firmware/SmartPayESP32/SmartPayESP32.ino`, điền config.h rồi Upload. Thư viện cập nhật có thể ghi đè cấu hình TFT; CLI tránh vấn đề này.

### Smoke test bắt buộc

Đặt `HARDWARE_SMOKE_TEST=1`, build/upload: hiển thị QR mẫu tài khoản giả và đọc **Đã nhận một trăm năm mươi nghìn đồng** một lần không cần Wi-Fi. Không chuyển tiền vào QR mẫu. Sau xác nhận loa/màn hình, trả về `0` để dùng online. Không đổi chân ngẫu nhiên nếu chưa kiểm tra dây nguồn.

Production HTTPS cần CA PEM gốc đúng domain; không dùng `setInsecure`. Đồng bộ thời gian NTP trước TLS. Network task riêng giữ HTTP không chặn decoder. Không ACK trước khi phát xong và ghi NVS. Có cửa sổ phát lại khi mất điện sau phát/trước ghi NVS; không tuyên bố exactly once.

TFT mặc định chuyển nội dung tiếng Việt thành không dấu để tránh font lỗi; backend giữ Unicode gốc. Backlog phát tuần tự, chưa có tổng kết/giới hạn theo phiên. Nội dung tự do không được đọc bằng bộ clip số.

## Audio và test C++

Nghe thử các MP3 trong `/Users/ngttu/Workspace/iot_tingbox/firmware/assets/audio/`. Giọng Linh vi_VN tạo trên Mac; 24 clip có sẵn, gồm bộ ghép câu **“Cảm ơn, đã nhận được <số tiền> Việt Nam đồng”** và hai câu khởi động. Ví dụ 10000 đọc “mười nghìn”. ESP32 đọc trực tiếp từ Flash bằng một decoder mỗi câu. Quyền tái phân phối giọng Linh chưa được xác minh; cần làm rõ trước khi phát hành hoặc thay bằng bản thu có quyền sử dụng phù hợp.

- Bật nguồn: **“Chào mừng bạn đến với TingBox.”**
- Kết nối Wi-Fi thành công lần đầu: **“Đã kết nối Wi-Fi thành công.”**, không chờ backend/Internet hoạt động.
- Mỗi câu phát một lần mỗi lần khởi động; Wi-Fi kết nối lại không phát lặp. Câu Wi-Fi đợi câu chào hoặc thông báo thanh toán đang phát kết thúc. Giao dịch đến trong lúc phát âm khởi động được giữ trong hàng đợi, chỉ ACK sau khi đọc số tiền xong như trước.
- Chế độ `HARDWARE_SMOKE_TEST=1` giữ nguyên: chỉ đọc số tiền mẫu, không phát hai câu khởi động.

```sh
python3 /Users/ngttu/Workspace/iot_tingbox/tools/audio/embed.py
c++ -std=c++17 /Users/ngttu/Workspace/iot_tingbox/firmware/tests/money_test.cpp /Users/ngttu/Workspace/iot_tingbox/firmware/SmartPayESP32/VietnameseMoney.cpp -o /tmp/smartpay-money-test
/tmp/smartpay-money-test
c++ -std=c++17 -I/Users/ngttu/Workspace/iot_tingbox/firmware/tests/stubs /Users/ngttu/Workspace/iot_tingbox/firmware/tests/startup_sounds_test.cpp /Users/ngttu/Workspace/iot_tingbox/firmware/SmartPayESP32/AudioAssets.cpp -o /tmp/tingbox-startup-test
/tmp/tingbox-startup-test
```

Chỉ để tái tạo giọng trên máy phát triển: Mac có giọng `Linh` và `ffmpeg`, chạy `/Users/ngttu/Workspace/iot_tingbox/tools/audio/generate.py`. Không cần các công cụ này khi upload firmware với audio có sẵn. Xem manifest và tài liệu audio.

## Vận hành / giới hạn

- Dữ liệu SQLite và raw payload nhạy cảm; quyền file 0600 khi chạy server. Backup bằng SQLite backup API hoặc dừng backend trước khi sao chép cùng WAL; không copy mỗi file DB khi đang ghi.
- Hiện không tự xóa lịch sử/raw payload: quản trị cần thiết lập retention trước production. Giữ records/idempotency keys theo thời gian lịch sử có thể được provider trả lại; xóa bừa sẽ gây phát trùng.
- Đổi nguồn/token/tài khoản bị chặn khi dùng DB cũ để tránh trộn dữ liệu. Token rotation hiện cần migration quản trị hoặc DB mới + baseline; DB mới mất liên kết các đơn cũ. Không đổi khi đang có đơn chờ.
- Chưa có panel xử lý duyệt tay, multi-account, OTA, touch, font tiếng Việt đầy đủ hoặc TTS tự do. Trang POS xem được lịch sử/NEEDS_REVIEW và tình trạng sync.
- Dependency licenses nằm trong packages; ESP8266Audio GPL-3.0 và các codec có điều khoản riêng. Đọc `docs/THIRD_PARTY.md` trước khi phân phối binary.

## Nguồn tích hợp

- https://thueapi.pro — schema lịch sử MBBank.
- https://www.vietqr.io/generate — sinh QR offline bằng vietqr 1.2.0.
- https://github.com/earlephilhower/ESP8266Audio — nguồn đọc PROGMEM và MP3/I2S.
- https://github.com/espeak-ng/espeak-ng — engine tạo giọng.