# CodeJudge – C Online Judge (Client–Server / IPC)

Ứng dụng chấm bài lập trình C tự động, viết bằng **C + GTK 3**, giao tiếp giữa Client và Server bằng **TCP Socket**. Server hỗ trợ nhiều Client đồng thời bằng `pthread`, biên dịch và chạy bài nộp bằng `fork()` / `exec()` / `waitpid()`. Dữ liệu tài khoản, đề bài, test case và lịch sử chấm được lưu trong SQLite.

## 1. Cài đặt thư viện trên Ubuntu

```bash
sudo apt update
sudo apt install build-essential gcc make pkg-config libgtk-3-dev libsqlite3-dev
```

## 2. Biên dịch

Mở Terminal tại thư mục gốc `online-judge-c`:

```bash
make clean
make
```

Sau khi biên dịch thành công sẽ có hai chương trình `oj-server` và `oj-client`.

## 3. Chạy hệ thống

**Terminal 1 – Server quản trị:**

```bash
cd ~/online-judge-c
./oj-server
```

Trong cửa sổ Server, nhấn **Start Server**. Server lắng nghe TCP port `5050`.

**Terminal 2 – Client sinh viên:**

```bash
cd ~/online-judge-c
./oj-client
```

Nhập Server IP `127.0.0.1` nếu Client và Server chạy trên cùng máy. Tài khoản mặc định:

| Vai trò | Tên đăng nhập | Mật khẩu |
|---|---|---|
| Quản trị viên | `admin` | `admin123` |
| Sinh viên demo | `student` | `student123` |

Đăng nhập xong, Client tải danh sách bài tập. Chọn đề, đọc mô tả/yêu cầu/ví dụ, viết code hoặc mở file `.c`, rồi nhấn **Submit Solution**. Nút **Refresh Submission History** tải lịch sử từ Server.

## 4. Chức năng Server quản trị

- Khởi động và dừng TCP Server.
- Theo dõi trạng thái Server, số Client đang kết nối, tổng bài nộp và số bài tập.
- Quản lý bài tập: thêm, sửa, xóa; mỗi đề có mô tả, input, output, ví dụ và time limit.
- Quản lý Test Case: chọn bài, thêm, sửa, xóa input/expected output.
- Theo dõi bài nộp, trạng thái `JUDGING`, verdict và điểm.
- Xem bảng điểm tổng hợp theo sinh viên.
- Xem nhật ký hoạt động; nhật ký được lưu trong SQLite.

## 5. Chức năng Client sinh viên

- Đăng nhập bằng tài khoản được lưu trên Server và nhập địa chỉ IPv4 của Server.
- Xem danh sách bài tập và nội dung chi tiết gồm mô tả, input, output, ví dụ.
- Viết code C trong trình soạn thảo hoặc chọn file `.c` trên máy.
- Nộp bài qua TCP Socket.
- Nhận kết quả `AC`, `WA`, `CE`, `RE`, `TLE` và điểm 0–10.
- Xem lịch sử nộp bài đã lưu trên Server.

## 6. Dữ liệu và cấu trúc thư mục

Cơ sở dữ liệu `online_judge.db` tự tạo tại thư mục chạy `oj-server`. Khi khởi tạo lần đầu, hệ thống tạo sẵn tài khoản demo và ba bài mẫu: Sum Two Numbers, Find Maximum, Even or Odd, mỗi bài có ba test case.

```text
online-judge-c/
├── Makefile
├── README.md
├── include/
│   ├── protocol.h
│   └── database.h
├── src/
│   ├── client.c
│   ├── server.c
│   ├── database.c
│   └── protocol.c
└── online_judge.db   # tự sinh khi chạy Server lần đầu
```

Không xóa `online_judge.db` nếu muốn giữ tài khoản, bài tập, test case và lịch sử. Muốn khởi tạo dữ liệu mẫu lại từ đầu, hãy dừng Server và chỉ xóa database khi đã sao lưu dữ liệu cần giữ.

## 7. Nội dung IPC / hệ điều hành thể hiện trong đồ án

- **TCP Socket:** cơ chế IPC chính để Client gửi yêu cầu đăng nhập, tải đề, nộp source và nhận verdict từ Server.
- **pthread:** Server tạo luồng xử lý từng Client, cho phép nhiều Client kết nối đồng thời.
- **SQLite:** lưu trữ dữ liệu dùng chung của hệ thống.
- **fork / exec:** chạy GCC và chương trình bài nộp trong tiến trình con.
- **waitpid / signal:** theo dõi tiến trình và dừng chương trình vượt thời gian giới hạn.
- **File descriptor / redirection:** chuyển input/output test case vào chương trình được chấm.

`fork()` và `exec()` là API quản lý tiến trình, không phải cơ chế IPC. Trong đồ án này, TCP Socket là cơ chế giao tiếp liên tiến trình giữa hai ứng dụng Client và Server.

## 8. Lưu ý bảo mật quan trọng

Đây là phiên bản phục vụ học tập và demo trong máy ảo/lab, **không phải hệ thống chấm bài an toàn để mở Internet**. Chương trình bài nộp chạy bằng quyền của tài khoản Server; time limit đơn giản không thay thế sandbox. Không chạy mã không tin cậy trên máy có dữ liệu quan trọng. Khi triển khai thực tế cần chạy mỗi bài trong container/VM riêng, tài khoản không đặc quyền, giới hạn CPU/RAM/process, giới hạn filesystem/network và syscall.

Mật khẩu trong phiên bản học tập được băm SHA-256 trước khi lưu SQLite, nhưng chưa có salt riêng, cơ chế đổi mật khẩu, giới hạn đăng nhập hay phân quyền quản trị từ xa. Admin quản lý dữ liệu trực tiếp trên giao diện Server.

## 9. Xử lý lỗi thường gặp

- `Package gtk+-3.0 was not found`: cài `libgtk-3-dev`.
- `sqlite3.h not found`: cài `libsqlite3-dev`.
- `Cannot bind port 5050`: port đang được chương trình khác sử dụng; dừng chương trình đó hoặc đổi `OJ_PORT` trong `include/protocol.h` rồi biên dịch lại.
- Client không kết nối: kiểm tra Server đã mở và đã nhấn **Start Server**, địa chỉ IP đúng, firewall cho phép TCP 5050.
- Không thấy dữ liệu mẫu: chạy `./oj-server` từ thư mục project để database được tạo đúng vị trí.
