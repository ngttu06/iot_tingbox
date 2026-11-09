# Bộ giọng đọc nhúng sẵn

24 file MP3 tạo trên Mac bằng `say`, giọng nữ Linh (`vi_VN`), tốc độ 185; FFmpeg chuẩn hóa mono 22050 Hz/32 kbps. Kích thước và hash từng clip nằm trong manifest. Cần nghe thử trên loa thật để đánh giá phát âm và nhịp nghỉ.

Hai câu khởi động:
- `chao_mung`: **Chào mừng bạn đến với TingBox.** — phát khi bật nguồn.
- `ket_noi_wifi`: **Đã kết nối Wi-Fi thành công.** — phát khi kết nối Wi-Fi lần đầu, không phụ thuộc backend.

Mỗi câu phát một lần mỗi lần khởi động, không chồng tiếng hoặc ngắt câu thanh toán. Chế độ hardware smoke test chỉ phát số tiền mẫu như trước.

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