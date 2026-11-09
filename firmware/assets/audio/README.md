# Bộ giọng đọc nhúng sẵn

21 file MP3 thật, tạo bằng eSpeak NG 1.52.0, voice `vi`, tốc độ 145, pitch 45; FFmpeg chuẩn hóa mono 22050 Hz/32 kbps. Tổng 38.076 byte. Giọng máy tiếng Việt miền Bắc, cần nghe thử trên loa thật; không phải giọng nữ tự nhiên hoặc bản thu thương mại.

`manifest.json` ghi từ đọc, hash và kích thước. `AudioAssets.cpp` chứa sẵn toàn bộ byte MP3 trong PROGMEM. Người build/nạp không cần eSpeak, FFmpeg, Internet, tài khoản TTS hoặc upload LittleFS riêng.

Không dùng giọng macOS Linh hoặc MP3 tải từ dịch vụ không rõ quyền phân phối. Các clip được tổng hợp mới từ từ/cụm từ trong manifest bằng engine mã nguồn mở; không đóng gói executable eSpeak vào firmware. Thông tin engine/license: https://github.com/espeak-ng/espeak-ng (GPL-3.0-or-later). Bộ decoder ESP8266Audio có điều khoản GPL; xem thông báo third-party và license của dependency khi phân phối firmware.

Tái tạo (chỉ người phát triển, chạy từ thư mục bất kỳ):

```sh
python3 /Users/ngttu/Workspace/iot_tingbox/tools/audio/generate.py
```

Nếu thay MP3 bằng bản thu được cấp phép, cập nhật manifest hash/size và chạy:

```sh
python3 /Users/ngttu/Workspace/iot_tingbox/tools/audio/embed.py
```

Phân biệt `mot`=một / `mot_sau_muoi`=mốt, `nam`=năm / `lam`=lăm, `muoi`=mười / `muoi_hang_chuc`=mươi.