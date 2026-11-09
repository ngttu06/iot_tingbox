# Bộ giọng đọc nhúng sẵn

22 file MP3 tạo trên Mac bằng `say`, giọng nữ Linh (`vi_VN`), tốc độ 185; FFmpeg chuẩn hóa mono 22050 Hz/32 kbps. Tổng 43.780 byte. Cần nghe thử trên loa thật để đánh giá phát âm và nhịp nghỉ.

Câu đọc: **Cảm ơn, đã nhận được <số tiền> Việt Nam đồng**.
Ví dụ 10000: `cam_on` + `da_nhan_duoc` + `muoi` + `nghin` + `viet_nam_dong`.
ESP32 chỉ ghép phát byte MP3 từ Flash, không gen audio, không chạy TTS và không tải audio từ Internet.
Nguồn `AudioFileSourceClipSequence` đọc qua ranh giới clip cho một decoder duy nhất mỗi câu; không chứa cả câu trong RAM. MP3 không có ID3/Xing, tắt bit reservoir. Bộ lọc chỉ cắt khoảng lặng đầu/cuối, giữ khoảng nghỉ bên trong cụm từ.

`manifest.json` ghi từ đọc, hash và kích thước. `AudioAssets.cpp` chứa sẵn toàn bộ byte MP3 trong PROGMEM. Người build/nạp không cần eSpeak, FFmpeg, Internet, tài khoản TTS hoặc upload LittleFS riêng.

Giọng macOS Linh được chọn theo yêu cầu của chủ dự án. Quyền tái phân phối audio giọng hệ thống chưa được xác minh; cần làm rõ trước khi phát hành/thương mại hóa hoặc thay bằng bản thu có quyền sử dụng phù hợp. Bộ decoder ESP8266Audio có thông báo license riêng trong tài liệu third-party.

Tái tạo trên Mac có giọng Linh và FFmpeg (chỉ người phát triển, chạy từ thư mục bất kỳ):

```sh
python3 /Users/ngttu/Workspace/iot_tingbox/tools/audio/generate.py
```

Nếu thay MP3 bằng bản thu được cấp phép, cập nhật manifest hash/size và chạy:

```sh
python3 /Users/ngttu/Workspace/iot_tingbox/tools/audio/embed.py
```

Phân biệt `mot`=một / `mot_sau_muoi`=mốt, `nam`=năm / `lam`=lăm, `muoi`=mười / `muoi_hang_chuc`=mươi.