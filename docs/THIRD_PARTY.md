# Third-party notices

Các dependency không được coi là mã tự viết của project. Giữ thông báo bản quyền/license khi phân phối theo điều khoản tương ứng. File lock backend ghi phiên bản thực tế.

- Espressif Arduino core 3.3.7: https://github.com/espressif/arduino-esp32
- ArduinoJson 7.4.2: https://github.com/bblanchon/ArduinoJson
- TFT_eSPI 2.5.43: https://github.com/Bodmer/TFT_eSPI
- QRCode 0.0.1: Richard Moore, MIT, https://github.com/ricmoo/QRCode. Hai file nguồn được vendor nguyên bản trong firmware/SmartPayESP32/src/qr cùng LICENSE để tránh trùng header qrcode.h của ESP-IDF; không phải mã tự viết.
- ESP8266Audio 2.4.1: Earle F. Philhower và contributors, GPL-3.0; codec MP3 và thư viện con có thông báo riêng. https://github.com/earlephilhower/ESP8266Audio
- eSpeak NG 1.52.0: engine GPL-3.0-or-later, https://github.com/espeak-ng/espeak-ng. Engine không được nhúng trong firmware; chỉ các clip được tổng hợp mới từ danh sách từ có trong manifest.
- FFmpeg/libmp3lame dùng ở bước tạo asset, không nhúng executable vào firmware. https://ffmpeg.org/legal.html
- vietqr 1.2.0 và dependency backend: tham khảo LICENSE trong thư mục package cài bởi npm ci.

Không sử dụng macOS system voice, audio ngân hàng tải lại hoặc dịch vụ TTS thương mại không có quyền phân phối. Không tuyên bố toàn bộ firmware là proprietary/permissive: cần tuân thủ điều khoản decoder và dependency khi phát hành sản phẩm, bao gồm nghĩa vụ cung cấp source nếu áp dụng.