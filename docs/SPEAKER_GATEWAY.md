# TingBox speaker gateway

Firmware gọi trực tiếp `https://tingbox.ngttu.com/api/speaker-gateway`.
Backend Node/POS cũ vẫn được giữ để tham khảo nhưng không còn là nguồn sự kiện
của firmware này. Không tự bỏ batch đầu: baseline và hạn 1 giờ do server xử lý.

## Liên kết lần đầu

1. Nạp firmware bình thường, **không erase Flash/NVS**. Kết nối AP `TingBox-XXXXXX`
   bằng mật khẩu hiện trên màn hình hoặc USB Serial; mở `http://192.168.4.1/`.
2. QR `SPK-<UUID v4>` **tự hiện trên TFT trong 2 phút sau khởi động**, cùng tên/
   mật khẩu AP và địa chỉ portal. QR + mã SPK cũng hiện sẵn trong mục **Liên kết
   ngân hàng** trên trang Wi-Fi, kể cả khi tắt JavaScript. Không cần bấm nút để
   thấy QR. Cấu hình Wi-Fi 2.4 GHz nếu chưa có.
3. Admin quét/nhập nguyên mã đó vào hệ thống bank để đăng ký. **Nội dung QR là mã
   SPK thuần, không phải URL, không phải VietQR chuyển tiền.**
4. Chọn **Đã liên kết / Kiểm tra lại** để ẩn QR và thử gateway. Sau 403 firmware
   dừng và lưu trạng thái này qua reboot; không tự retry cho đến thao tác này.

Mã được tạo bởi RNG phần cứng sau khi Wi-Fi bật và lưu trong namespace `spk-device`.
Không đổi khi restart, đổi Wi-Fi hoặc nạp app giữ NVS. Xóa NVS/thay board sẽ mất mã.
Không có key chung hardcode trong binary. Mã là **API credential**, không chia sẻ
QR/ảnh màn hình công khai. Theo yêu cầu hiển thị trên trang Wi-Fi, SPK chỉ được
nhúng trong HTML cho client đang nối AP cài đặt, với `Cache-Control: no-store`;
không mở qua interface STA/LAN, không ghi SPK vào log thông thường. Portal dùng
HTTP cục bộ, vì vậy chỉ cho người tin cậy biết mật khẩu AP. Portal giữ mở khi
có điện thoại kết nối; tự đóng sau 2 phút có Wi-Fi mà không có client AP.
NVS hiện chưa mã hóa; bảo vệ truy cập vật lý, cân nhắc Flash Encryption/Secure Boot
trước production. Không tự coi QR là phương thức xác thực quyền sở hữu tài khoản.

Serial 115200/Newline: `bank-link`, `bank-retry`, `setup-wifi`, `status`, `audio-test`.
`setup-wifi` mở lại AP khi portal đã tự đóng; mật khẩu AP không phải mã SPK.
QR tự hiện khi boot không làm chậm poll/ACK; giao dịch mới vẫn ưu tiên âm thanh.
Nút **Hiện lại QR trên màn hình loa** tạm dừng poll tối đa 2 phút như trước.

## Phát và chống đọc trùng

- Poll khoảng 3 giây, một thông báo mới được phát tại một thời điểm. HTTP chạy ở
  network task, không chặn vòng audio. Lỗi mạng/5xx tăng chờ đến 60 giây.
- ID đã hoàn tất: chỉ ACK. ID mới nhưng cùng `reference` giao dịch thật trong
  cùng phạm vi liên kết: lưu thêm ID mới là đã xử lý rồi ACK, không phát lại.
- `reference` rỗng: chỉ chống lặp bằng `event.id`; không suy diễn từ số tiền.
- Trước phát, ghi NVS trạng thái **đang xử lý**. Sau audio hoàn tất, ghi
  **đã hoàn tất** rồi mới POST `{"ids":["event.id"]}`. Chỉ ACK theo event ID,
  không ACK theo reference. Response poll/ACK phải là HTTP 200 và `ok: true`.
- ACK lỗi: giữ bản ghi hoàn tất; retry ACK mà không phát lại. Sau reboot, server
  trả lại ID đó thì cũng chỉ ACK. API không bảo đảm exactly-once tuyệt đối.
- Reset giữa lúc lưu intent và hoàn tất: **không tự phát lại, không tự ACK**.
  Portal hiển thị reference và event ID cần kiểm tra. Người vận hành kiểm tra/
  thông báo thủ công, mở mục khôi phục và xác nhận đã xử lý. Đây là xác nhận
  thủ công, không phải firmware giả định audio thành công. Lỗi audio phải sửa
  phần cứng và reset trước; không dùng xác nhận để bỏ qua tiền chưa kiểm tra.
- Clip/module báo hoàn tất chỉ là xác nhận phần mềm/BUSY, không đo âm thanh thực.

### Giao dịch dang dở mà người vận hành xác nhận chưa nghe

Sau reset, `bank-retry` không gỡ chặn bản ghi dang dở. Không xóa NVS hoặc xác
nhận “đã xử lý” nếu chưa nghe/chưa kiểm tra. Khi xác nhận chắc chắn chưa nghe,
có thể dùng Serial 115200/Newline:

```text
bank-replay <event-id-dang-do>
```

Lệnh chỉ cho phép bản ghi pending trùng chính xác ID/reference/phạm vi, không
xóa lịch sử đã phát. Firmware đợi đúng event từ API, kiểm tra tiền VND rồi phát;
chỉ lưu hoàn tất và ACK sau audio thành công, sau đó tiếp tục poll bình thường.
Quyền phát lại chỉ giữ trong RAM, mất khi reboot; không tự lặp mãi khi mất điện.
Nếu event đã hết hạn 1 giờ hoặc không còn trong batch API, loa giữ chờ/không
đọc event khác, không đoán số tiền hay giả đánh dấu hoàn tất. Cần đối soát và
thông báo thủ công trước khi xác nhận xử lý trong portal. Lệnh phát lại có thể
gây đọc trùng nếu người vận hành xác nhận sai; không dùng khi chưa chắc chắn.

### Khôi phục cục bộ khi event không còn trên API

Chỉ khi người vận hành xác nhận **chưa nghe**, đã đối soát đúng mã giao dịch và
số tiền VND, có thể gửi Serial `bank-replay-amount <event-id-dang-do> <so-tien>`.
Lệnh không tạo event mới, không xóa NVS/lịch sử: chỉ nhận ID pending thật thuộc
phạm vi hiện tại, phát số tiền được xác nhận rồi lưu hoàn tất và ACK ID cũ.
Lỗi âm thanh/lưu trữ vẫn dừng, không ACK. Không dùng lệnh cho event đã hoàn tất
hoặc như cơ chế tự động bỏ qua event hết hạn; xác nhận sai có thể gây đọc trùng.
### Đổi tài khoản/phiên liên kết

API hiện không trả account ID/link generation. Firmware không thể tự phát hiện
đổi tài khoản để phân biệt các reference trùng nhau giữa hai tài khoản.
Sau khi admin đổi liên kết (và server đã xóa queue cũ), chọn **Xác nhận phạm vi
liên kết mới** trong portal. Phạm vi reference đổi, lịch sử event ID và mã SPK
vẫn giữ nguyên. Không làm thao tác này chỉ để retry mạng/403 của cùng liên kết.
Không cho đổi khi có thông báo đang phát hoặc dang dở. Muốn tự động hoàn toàn,
server cần bổ sung trường định danh phạm vi/phiên liên kết.

## Giới hạn cần biết trước khi vận hành

- Bộ nhớ ledger tối đa **128 event ID**, gồm ID bổ sung của giao dịch trùng,
  và có thể đầy sớm hơn do dung lượng NVS. Không tự xóa/evict vì sẽ mất chứng cứ
  chống đọc lặp. Đầy/lỗi thì dừng và không ACK; cần nâng dung lượng/lưu trữ hoặc
  quy trình lưu trữ quản trị trước khi dùng dài hạn. Không erase NVS để chữa lỗi.
- Hiện đọc VND nguyên dương đến `999999999999`; chuỗi decimal có phần lẻ toàn
  số 0 được chấp nhận chính xác. Tiền tệ khác/số lẻ không được đổi thành VND.
- **Thông báo `test: true` phát câu chào “Chào mừng bạn đến với TingBox”** để
  kiểm tra đường âm thanh bằng clip đã có trên SD/Flash, TFT báo THỬ LOA/KHÔNG
  PHẢI TIỀN THẬT. Không đọc số tiền thử như tiền thật. Lưu ID và ACK sau BUSY/
  decoder hoàn tất giống event thật; test không dedup reference với giao dịch thật.
  Muốn đọc cả câu “Thông báo thử” + số tiền cần bổ sung clip/cập nhật SD sau.
  Riêng test dang dở sau reset được phát lại câu chào để hoàn tất kiểm tra; quy
  tắc này không áp dụng cho giao dịch tiền thật. Test có thể lặp sau crash,
  nhưng không được dùng xác nhận tiền và không ảnh hưởng dedup giao dịch thật.
- Event không hỗ trợ/sai định dạng không ACK. Theo hàng đợi hiện tại, các event
  đó có thể chặn tiến độ cho đến khi được admin xử lý hoặc hết hạn. Không coi
  đây là firmware đã hỗ trợ mọi định dạng tiền tệ/TTS.
- HTTPS dùng GTS Root R4, đối chiếu chain WE1 của domain; không `setInsecure`,
  không redirect key sang host khác. Cần NTP trước TLS. Khi đổi CA/domain, cập
  nhật `SPEAKER_ROOT_CA`/`SPEAKER_BASE_URL` bằng cấu hình đã xác minh.
- Không có API đăng ký/đổi liên kết trong repository này; admin đăng ký SPK
  phía hệ thống bank theo quy trình đã xác nhận. Một key chỉ có một consumer.

## Build

```powershell
& 'C:\Users\info\Documents\iot_tingbox\tools\firmware.ps1' -Action build -Board esp32s3 -Audio dfplayer
& 'C:\Users\info\Documents\iot_tingbox\tools\firmware.ps1' -Action build -Board esp32s3 -Audio i2s
```

Không cần đưa bank token vào frontend hoặc chạy backend cũ. Bản DFPlayer đã nạp
qua COM7 ngày 09/11/2026, giữ NVS; boot/Wi-Fi và hai câu khởi động có BUSY hoàn tất.
Gateway vẫn offline trong log quan sát; chưa nghiệm thu liên kết bank thật.
Chọn “Liên kết ngân hàng” hoặc gửi `bank-link` để xem SPK trên màn hình loa.