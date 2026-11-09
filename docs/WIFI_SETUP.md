# Cài đặt Wi-Fi bằng điện thoại

## Cách sử dụng

1. Bật ESP32. Thiết bị mở mạng **TingBox-XXXXXX** trong giai đoạn cài đặt; sáu ký tự cuối lấy từ mã chip (ví dụ `TingBox-04291c`).
2. Xem SSID và mật khẩu ngẫu nhiên **đúng 8 chữ số (0–9)** trên màn TFT, kết nối điện thoại vào mạng đó. Có thể có số 0 ở đầu; nhập đủ cả 8 số. Mật khẩu đổi mỗi lần portal được mở lại/khởi động lại. Chỉ mật khẩu AP của loa đổi định dạng; không đổi mật khẩu router, mã SPK hoặc CSRF token.
3. Điện thoại có thể hiện thông báo đăng nhập mạng; chạm để mở trang cấu hình. Nếu không tự bật, mở trình duyệt tại **http://192.168.4.1/**. Chọn giữ kết nối khi điện thoại báo mạng không có Internet; có thể cần tạm tắt chuyển mạng thông minh/VPN/private DNS.
4. Chọn Wi-Fi **2.4 GHz** trong danh sách hoặc bấm **Nhập tên mạng thủ công** (dùng được với mạng ẩn). **Quét lại** để tìm mạng; mức sóng và trạng thái có/không có mật khẩu hiện ngay trên từng mạng. Nhập mật khẩu, dùng **Hiện / Ẩn** nếu cần, rồi bấm **Kết nối và lưu**. Mạng mở được phép để mật khẩu trống. SSID tối đa 32 byte; mật khẩu WPA 8–63 ký tự ASCII in được; WPA enterprise/PSK hex 64 ký tự không hỗ trợ.
5. Chờ tối đa 30 giây; trạng thái tự cập nhật. Khi kết nối thành công, cấu hình được lưu NVS để dùng sau reboot. Nếu kết nối hoặc lưu NVS thất bại, giữ cấu hình cũ. Nếu điện thoại bị ngắt, kết nối lại `TingBox-XXXXXX` và mở lại địa chỉ trên. Trình duyệt tắt JavaScript vẫn nhập/gửi được form; dùng **Tải lại trạng thái** để xem kết quả.
6. Sau khi có kết nối Wi-Fi ổn định 2 phút, AP cài đặt tự tắt. Mất Wi-Fi 30 giây sẽ mở lại. Khởi động lại cũng mở cửa sổ cài đặt để có thể đổi mạng dù mạng cũ vẫn còn.

Không cần nạp lại firmware để thay Wi-Fi. Wi-Fi NVS ưu tiên hơn fallback trong config.h. Không xuất mật khẩu mạng đã lưu vào Serial hoặc trang HTML, không đưa NVS/secrets lên Git. Mật khẩu AP cài đặt ngẫu nhiên hiện trên TFT và USB Serial để hỗ trợ thiết bị chưa đấu màn hình.

## Kết nối Internet / lịch sử giao dịch

Portal chỉ cấu hình **Wi-Fi**, không cung cấp Internet cho điện thoại, không NAT, không tự cấu hình backend URL hoặc CA. ESP32 vẫn lấy giao dịch từ backend qua HTTPS; backend mới gọi ThueAPI. Kết nối Wi-Fi thành công chỉ chứng minh đã vào router, không chứng minh Internet hoặc backend sẵn sàng.

Cần điền domain HTTPS thật, root CA và device token trong config.h trước khi dùng online. Hiện local config còn backend URL mẫu nên chưa thể nhận giao dịch chỉ bằng việc kết nối Wi-Fi.

## Bảo mật và giới hạn

- AP có mật khẩu ngẫu nhiên hiển thị trên TFT; tối đa 2 clients.
- DNS wildcard + HTTP captive redirect; không chặn/giả chứng chỉ HTTPS. Việc tự mở trang tùy hệ điều hành, cần thử trên Android/iOS thật.
- Chỉ phục vụ request tới interface AP; form có session CSRF token, escape SSID/thông báo và không hiển thị lại mật khẩu đã lưu.
- Giao diện tiếng Việt có dấu, HTML/CSS/JavaScript nhúng trong Flash, không dùng CDN/font ngoài. CSP chỉ cho phép script/kết nối cùng nguồn, không cho phép inline script; dữ liệu quét được hiển thị bằng text, không diễn giải thành HTML.
- Cấu hình chỉ ghi NVS sau khi kết nối thành công bằng một JSON record. NVS không mặc định mã hóa; muốn chống đọc Flash vật lý cần triển khai flash/NVS encryption riêng.
- Network task quản lý portal; audio loop riêng. Request HTTPS có thể làm portal phản hồi chậm vài giây, cần đo trên phần cứng.
- Quét bất đồng bộ, tối đa một lần mới mỗi 10 giây; kết quả tối đa 20 mạng mạnh nhất trong 256 kết quả đầu, gộp mạng trùng tên và kiểu mở/khóa. Không quét khi đang thử kết nối; không bắt đầu kết nối khi đang quét. Quét thất bại có thể xảy ra lúc radio đang kết nối mạng cũ: quét lại sau hoặc nhập thủ công. Không ngắt Wi-Fi đang dùng chỉ để quét.
- Chưa có nút reset vật lý hoặc provisioning nhiều thiết bị tự động.

## Test thiết bị cần thực hiện

- Lần đầu không có mạng → AP xuất hiện, TFT hiển thị đúng mật khẩu.
- Android/iOS: captive pop-up và truy cập IP thủ công.
- Đúng/sai mật khẩu; mạng mở; SSID có ký tự HTML và Unicode; không in/trả mật khẩu cũ.
- Quét khi đang nối mạng cũ, không tìm thấy mạng, hai điện thoại cùng mở portal, quét lại nhiều lần; theo dõi heap và khả năng đáp ứng trong lúc audio/HTTPS hoạt động.
- Đúng Wi-Fi nhưng mất Internet/backend: không báo thanh toán giả.
- Reboot dùng NVS; sai cấu hình mới vẫn giữ cấu hình cũ; mất Wi-Fi tự mở lại AP.
- Portal tự tắt sau 2 phút có Wi-Fi; đồng thời nhận/phát audio không giật, heap còn đủ.

Kiểm thử trên máy gồm validation/HTML escape, trình duyệt Chromium với API giả lập và compile. Chạy kiểm thử trình duyệt (Node.js 22+ và Chrome, chỉ dùng thư viện tích hợp):

```powershell
node 'C:\Users\info\Documents\iot_tingbox\firmware\tests\wifi_portal_test.mjs' 'C:\Program Files\Google\Chrome\Application\chrome.exe'
```

Ảnh chụp kiểm thử nằm trong `C:\Users\info\Documents\iot_tingbox\.tools\wifi-portal-test`. Kiểm thử mô phỏng không thay thế nghiệm thu radio, captive popup, DHCP hoặc NVS trên ESP32; bản giao diện mới chưa được nạp/thử trên thiết bị thật.