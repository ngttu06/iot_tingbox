# Thử MP3-TF-16P / DFPlayer, có đường quay lại MAX98357A

## Trạng thái và giới hạn

- Module ảnh có chữ `MP3-TF-16P V3.0`; firmware giả định tương thích giao thức DFPlayer Mini 9600 baud. Chưa xác minh mã chip/pinout thực tế; **đấu theo chữ tên chân trên đúng module**, không suy ra vị trí chân chỉ từ ảnh khe thẻ.
- `AudioBackend.h` mặc định chọn DFPlayer cho thử nghiệm. MAX98357A/I2S vẫn giữ nguyên phía `AUDIO_USE_DFPLAYER=0`. Không có thư viện mới.
- File trên thẻ được phát bằng lệnh `/mp3/NNNN.mp3` (0x12), không dùng thứ tự copy/FAT toàn cục. Firmware chờ BUSY LOW rồi HIGH ổn định cho từng clip. UART ACK hoặc thông báo finish không tự xác nhận hoàn tất.
- Thiếu module/thẻ/file, BUSY ngắt/kẹt hoặc lỗi UART/module dẫn tới lỗi playback, **không ACK thanh toán**. BUSY chỉ báo trạng thái module, không chứng minh có âm ra loa hoặc đúng nội dung thẻ. Không dùng cho thanh toán thật trước khi nghiệm thu đầy đủ.
- MP3 được phát từng file nên có thể có khoảng nghỉ rõ hơn bản I2S. Clone không tương thích giao thức/BUSY có thể không dùng được với bản này.

## 1. Chuẩn bị microSD

1. Dùng microSD tối đa 32 GB, FAT16/FAT32. Nếu cần format hãy tự sao lưu dữ liệu trước; công cụ dự án **không format hay ghi trực tiếp thẻ**.
2. Chạy:

```powershell
python 'C:\Users\info\Documents\iot_tingbox\tools\audio\prepare_dfplayer.py'
```

3. Chép nguyên thư mục `C:\Users\info\Documents\iot_tingbox\.tools\dfplayer-sd\mp3` vào gốc thẻ. Kết quả phải là `X:\mp3\0001.mp3` ... `X:\mp3\0024.mp3`, không lồng thêm `dfplayer-sd`.
4. `0001.mp3` là câu chào; `0002.mp3` là kết nối Wi-Fi. Bảng đủ 24 file: `C:\Users\info\Documents\iot_tingbox\.tools\dfplayer-sd\track-map.txt`. Không tự đổi số file; không thêm file ẩn của macOS vào thư mục mp3.
5. Rút nguồn module trước khi lắp/tháo thẻ.

## 2. Đổi dây (ngắt toàn bộ nguồn trước)

**Tháo MAX98357A khỏi các GPIO và loa trước khi nối module mới. Không nối hai đầu ra ampli chung một loa.**

| Chân module DFPlayer | ESP32-S3 N16R8 | ESP32 WROOM |
|---|---|---|
| VCC | 5V ổn định theo thông số module | Tương tự |
| GND | GND chung với ESP32 | Tương tự |
| TX | GPIO4 (ESP RX) | GPIO26 |
| RX | GPIO5 (ESP TX), qua điện trở nối tiếp 1 kΩ | GPIO25 qua 1 kΩ |
| BUSY | GPIO6 | GPIO22 |
| SPK1 / SPK2 | Hai đầu loa tương thích, ví dụ 4–8 Ω công suất phù hợp module | Tương tự |

- Không nối SPK1/SPK2 xuống GND. Không nhầm DAC_L/DAC_R với đầu ra loa đã khuếch đại.
- Không cấp 5V vào GPIO. TX/BUSY của module nối vào ESP32 phải là logic không vượt 3.3V; kiểm tra đúng module/đo trước, dùng chuyển mức nếu khác. Điện trở 1k trên đường ESP TX không thay thế mạch chuyển mức 5V -> 3.3V.
- BUSY là bắt buộc trong bản thử: active LOW khi phát. Ngõ vào ESP32 dùng pull-up nội 3.3V; không nối BUSY thẳng GND/5V.
- Không đồng thời nối nguồn USB và nguồn 5V ngoài vào cùng đường VBUS nếu chưa biết mạch chống cấp ngược. Nguồn yếu có thể làm treo module dù ESP32 chưa reset.

## 3. Build/nạp và thử

```powershell
& 'C:\Users\info\Documents\iot_tingbox\tools\firmware.ps1' -Action build -Board esp32s3 -Audio dfplayer
& 'C:\Users\info\Documents\iot_tingbox\tools\firmware.ps1' -Action upload -Board esp32s3 -Audio dfplayer -Port COM7
```

Build ở `C:\Users\info\Documents\iot_tingbox\firmware\build\esp32s3-dfplayer`. Với Arduino IDE, `AudioBackend.h` là nguồn chọn mặc định (đặt 0 nếu muốn I2S); UART0 Serial Monitor vẫn qua CH343, 115200 baud, CDC disabled như cũ.

Mở Serial Monitor 115200, Newline, nhấn RESET/EN. Module có khoảng khởi tạo hơn 3 giây trước câu chào, volume khởi đầu 20/30. Gửi `status` hoặc `audio-test` để thử lại câu chào mà không tạo/ACK giao dịch. Sau khi sửa dây/thẻ, reset cả module và ESP32 nếu còn lỗi.

- `DFPlayer play: /mp3/0001.mp3`: đã gửi lệnh, **chưa chứng minh phát được**.
- `Audio finished: OK (DFPlayer BUSY cycle, not an acoustic measurement)`: đã thấy chu kỳ BUSY cho tất cả đoạn. Cần nghe để xác nhận đúng nội dung.
- `BUSY did not go LOW`: kiểm tra thẻ/tên file, TX->RX qua 1k, GND, nguồn và BUSY; không nối tắt BUSY để vượt kiểm tra.
- `BUSY stayed LOW` hoặc `BUSY LOW before play`: kiểm tra chập/nhầm chân, module treo, file hoặc chế độ phát lặp.
- UART `command=0x40` là lỗi module; mã cụ thể phụ thuộc chip/clone. Không tự coi phản hồi ACK/finish là thanh toán đã đọc.

Checklist trước online: nghe đúng câu chào, câu Wi-Fi, đọc đúng 10.000/150.000/số nhiều nhóm; kiểm tra thiếu file/mất thẻ/BUSY ngắt không ACK; reset giữa câu; payment đến lúc chào; không phát lặp khi Wi-Fi reconnect. Hiện chưa xác nhận phần cứng DFPlayer, chưa chứng nhận production.

## 4. Quay về MAX98357A

### Không cần thay Git

Ngắt nguồn, tháo DFPlayer và nối lại MAX98357A theo bảng I2S S3: BCLK=4, LRC=5, DIN=6, GND chung, VIN đúng module. Sau đó:

```powershell
& 'C:\Users\info\Documents\iot_tingbox\tools\firmware.ps1' -Action upload -Board esp32s3 -Audio i2s -Port COM7
```

Build I2S riêng ở `C:\Users\info\Documents\iot_tingbox\firmware\build\esp32s3-i2s`. Thẻ microSD không còn cần cho I2S. Sau nạp kiểm tra log `Audio config: BCLK=4, LRC=5, DIN=6`.

### Dùng đúng binary đã chạy trước thử nghiệm (không build)

Bản sao local tại `C:\Users\info\Documents\iot_tingbox\.tools\rollback-max98357a`, app SHA-256 `FAA138E089114C87DA190E5359D41527F83C9A84D3EAF52FAF77787725BD2BD8`. Không commit binary/merged Flash vì có thể chứa dữ liệu nhạy cảm.

```powershell
$cli = 'C:\Users\info\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
& $cli upload --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,FlashMode=qio,CDCOnBoot=default,UploadSpeed=921600' --port COM7 --upload-property upload.speed=460800 --input-dir 'C:\Users\info\Documents\iot_tingbox\.tools\rollback-max98357a' 'C:\Users\info\Documents\iot_tingbox\firmware\SmartPayESP32'
```

### Git

Mốc nguồn trước thử nghiệm: **`c630dafb4b6210d990cd48671d7c1eeda7935b1a`**. Commit thử DFPlayer được tách riêng ngay sau mốc này. Xem `git log --oneline` rồi dùng `git revert <hash-commit-thu-DFPlayer>` để đảo đúng commit thử, giữ lịch sử và các sửa Wi-Fi/Serial trước đó. Không dùng `reset --hard` làm mất thay đổi mới. Revert mã nguồn không tự nạp lại board; vẫn cần nạp binary I2S và trả dây như trên.

## Tham khảo giao thức

- https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299
- https://github.com/DFRobot/DFRobotDFPlayerMini (chỉ đối chiếu giao thức, không thêm dependency hoặc chép thư viện).