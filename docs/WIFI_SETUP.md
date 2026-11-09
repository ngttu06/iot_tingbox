# Cài đặt Wi-Fi bằng điện thoại

## Cách sử dụng

1. Bật ESP32. Thiết bị mở mạng **SmartPay-XXXXXX** trong giai đoạn cài đặt.
2. Xem SSID và mật khẩu ngẫu nhiên 12 ký tự trên màn TFT, kết nối điện thoại vào mạng đó. Mật khẩu đổi mỗi lần portal được mở lại/khởi động lại.
3. Điện thoại có thể hiện thông báo đăng nhập mạng; chạm để mở trang cấu hình. Nếu không tự bật, mở trình duyệt tại **http://192.168.4.1/**. Chọn giữ kết nối khi điện thoại báo mạng không có Internet; có thể cần tạm tắt chuyển mạng thông minh/VPN/private DNS.
4. Nhập SSID Wi-Fi **2.4 GHz** và mật khẩu, bấm **Kết nối và lưu**. Mạng mở được phép để mật khẩu trống. SSID tối đa 32 byte; mật khẩu WPA 8–63 ký tự ASCII in được; WPA enterprise/PSK hex 64 ký tự không hỗ trợ.
5. Chờ tối đa 30 giây, bấm **Cập nhật trạng thái**. Khi kết nối thành công, cấu hình được lưu NVS để dùng sau reboot. Nếu sai mật khẩu hoặc lưu NVS thất bại, giữ cấu hình cũ.
6. Sau khi có kết nối Wi-Fi ổn định 2 phút, AP cài đặt tự tắt. Mất Wi-Fi 30 giây sẽ mở lại. Khởi động lại cũng mở cửa sổ cài đặt để có thể đổi mạng dù mạng cũ vẫn còn.

Không cần nạp lại firmware để thay Wi-Fi. Wi-Fi NVS ưu tiên hơn fallback trong config.h; fallback local đã được cấu hình theo thông tin chủ thiết bị cung cấp. Không xuất mật khẩu vào Serial hoặc trang HTML, không đưa NVS/secrets lên Git.

## Kết nối Internet / lịch sử giao dịch

Portal chỉ cấu hình **Wi-Fi**, không cung cấp Internet cho điện thoại, không NAT, không tự cấu hình backend URL hoặc CA. ESP32 vẫn lấy giao dịch từ backend qua HTTPS; backend mới gọi ThueAPI. Kết nối Wi-Fi thành công chỉ chứng minh đã vào router, không chứng minh Internet hoặc backend sẵn sàng.

Cần điền domain HTTPS thật, root CA và device token trong config.h trước khi dùng online. Hiện local config còn backend URL mẫu nên chưa thể nhận giao dịch chỉ bằng việc kết nối Wi-Fi.

## Bảo mật và giới hạn

- AP có mật khẩu ngẫu nhiên hiển thị trên TFT; tối đa 2 clients.
- DNS wildcard + HTTP captive redirect; không chặn/giả chứng chỉ HTTPS. Việc tự mở trang tùy hệ điều hành, cần thử trên Android/iOS thật.
- Chỉ phục vụ request tới interface AP; form có session CSRF token, escape SSID/thông báo và không hiển thị lại mật khẩu đã lưu.
- Cấu hình chỉ ghi NVS sau khi kết nối thành công bằng một JSON record. NVS không mặc định mã hóa; muốn chống đọc Flash vật lý cần triển khai flash/NVS encryption riêng.
- Network task quản lý portal; audio loop riêng. Request HTTPS có thể làm portal phản hồi chậm vài giây, cần đo trên phần cứng.
- Chưa có quét danh sách Wi-Fi, nút reset vật lý hoặc provisioning nhiều thiết bị tự động. Nhập SSID thủ công, hỗ trợ SSID ẩn.

## Test thiết bị cần thực hiện

- Lần đầu không có mạng → AP xuất hiện, TFT hiển thị đúng mật khẩu.
- Android/iOS: captive pop-up và truy cập IP thủ công.
- Đúng/sai mật khẩu; mạng mở; SSID có ký tự HTML và Unicode; không in/trả mật khẩu cũ.
- Đúng Wi-Fi nhưng mất Internet/backend: không báo thanh toán giả.
- Reboot dùng NVS; sai cấu hình mới vẫn giữ cấu hình cũ; mất Wi-Fi tự mở lại AP.
- Portal tự tắt sau 2 phút có Wi-Fi; đồng thời nhận/phát audio không giật, heap còn đủ.

Tests trên máy hiện chỉ kiểm tra validation/HTML escape và compile; chưa có board để nghiệm thu radio, captive popup, DHCP hoặc NVS thực.