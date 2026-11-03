# CODEJUDGE – C ONLINE JUDGE SYSTEM

**Ứng dụng chấm bài lập trình C tự động theo mô hình Client–Server / IPC**

---

## 1. Giới thiệu đề tài

### 1.1. Tổng quan

CodeJudge là hệ thống chấm bài lập trình C tự động được xây dựng bằng ngôn ngữ C và thư viện giao diện GTK 3. Hệ thống hoạt động theo mô hình Client–Server, cho phép sinh viên gửi mã nguồn C từ máy Client đến Server để biên dịch, thực thi và kiểm tra kết quả dựa trên các Test Case được thiết lập trước.

Server sử dụng TCP Socket để giao tiếp với Client và pthread để xử lý nhiều kết nối đồng thời. Chức năng chấm bài sử dụng các API quản lý tiến trình như `fork()`, `exec()` và `waitpid()` để biên dịch, chạy chương trình và theo dõi kết quả thực thi.

Thông tin tài khoản, đề bài, Test Case, lịch sử nộp bài và nhật ký hoạt động được lưu trữ bằng SQLite.

### 1.2. Mục tiêu

- Xây dựng hệ thống chấm bài lập trình C tự động.
- Áp dụng mô hình Client–Server trong giao tiếp mạng.
- Sử dụng TCP Socket để trao đổi dữ liệu giữa Client và Server.
- Sử dụng pthread để xử lý nhiều Client đồng thời.
- Áp dụng kỹ thuật tạo và quản lý tiến trình trong Linux.
- Sử dụng SQLite để lưu trữ và quản lý dữ liệu.
- Xây dựng giao diện sinh viên và giao diện quản trị trực quan bằng GTK 3.

### 1.3. Công nghệ sử dụng

| Công nghệ | Vai trò |
|---|---|
| C | Ngôn ngữ lập trình chính |
| GTK 3 | Xây dựng giao diện đồ họa |
| TCP Socket | Giao tiếp giữa Client và Server |
| pthread | Xử lý nhiều Client đồng thời |
| fork() / exec() | Tạo tiến trình và thực thi chương trình |
| waitpid() | Theo dõi và thu hồi tiến trình con |
| GCC | Biên dịch mã nguồn C |
| SQLite | Lưu trữ dữ liệu |
| Makefile | Tự động hóa quá trình biên dịch |
| Ubuntu Linux | Môi trường phát triển và thực thi |

---

## 2. Kiến trúc hệ thống

### 2.1. Mô hình Client–Server

Hệ thống gồm hai thành phần chính:

**Student Client**

- Giao diện dành cho sinh viên.
- Đăng nhập bằng tài khoản được lưu trên Server.
- Kết nối đến Server thông qua địa chỉ IPv4 và TCP Socket.
- Tải danh sách đề bài và nội dung chi tiết.
- Soạn mã nguồn C hoặc mở file `.c` có sẵn.
- Gửi bài làm đến Server.
- Nhận kết quả chấm và xem lịch sử nộp bài.

**Admin Server**

- Giao diện quản trị dành cho người quản lý hệ thống.
- Khởi động và dừng TCP Server.
- Theo dõi trạng thái hoạt động và số Client đang kết nối.
- Quản lý đề bài và Test Case.
- Theo dõi bài nộp và kết quả chấm.
- Xem bảng xếp hạng sinh viên.
- Theo dõi nhật ký hoạt động.

### 2.2. Sơ đồ kiến trúc

```text
              STUDENT CLIENT
             (GTK 3 - C)
                    |
                    |
              TCP SOCKET
               Port 5050
                    |
                    |
             ADMIN SERVER
             (GTK 3 - C)
                    |
          +---------+---------+
          |         |         |
       pthread   Judge     SQLite
       Client    Engine    Database
       Handler     |
                   |
              fork() / exec()
                   |
                GCC
                   |
            C Program Execution
                   |
              Test Cases
                   |
              Judge Result
                   |
             Return to Client
```

### 2.3. Quy trình hoạt động

1. Quản trị viên mở chương trình Server và nhấn Start Server.
2. Server bắt đầu lắng nghe kết nối TCP trên port 5050.
3. Sinh viên mở chương trình Client và nhập địa chỉ IP của Server.
4. Client gửi thông tin đăng nhập đến Server.
5. Server kiểm tra tài khoản thông qua SQLite.
6. Sau khi đăng nhập thành công, Client tải danh sách đề bài.
7. Sinh viên chọn đề, viết mã nguồn C và gửi bài.
8. Server tiếp nhận bài nộp và chuyển đến Online Judge Engine.
9. Judge Engine biên dịch mã nguồn bằng GCC và thực thi chương trình.
10. Kết quả được so sánh với các Test Case đã lưu trong SQLite.
11. Server lưu lịch sử chấm bài và gửi verdict về Client.

---

## 3. Giao diện và trải nghiệm người dùng

### 3.1. Student Portal

Giao diện dành cho sinh viên gồm các thành phần:

- Màn hình đăng nhập.
- Khu vực hiển thị danh sách đề bài.
- Khu vực hiển thị nội dung và yêu cầu của đề.
- Trình soạn thảo mã nguồn C.
- Chức năng mở file `.c` từ máy tính.
- Nút Submit Solution để gửi bài.
- Khu vực hiển thị trạng thái kết nối và kết quả chấm.
- Chức năng Refresh Submission History để tải lại lịch sử nộp bài.

### 3.2. Admin Console

Giao diện quản trị gồm các khu vực:

- **Dashboard:** Hiển thị thống kê tổng quan của hệ thống.
- **Problems:** Quản lý danh sách đề bài.
- **Test Cases:** Quản lý dữ liệu kiểm thử.
- **Submissions:** Theo dõi các bài nộp và kết quả chấm.
- **Leaderboard:** Hiển thị bảng điểm tổng hợp theo sinh viên.
- **Activity Log:** Theo dõi nhật ký hoạt động.

### 3.3. Cải tiến giao diện và chức năng

- Làm mới bố cục Student Portal và Admin Console.
- Đồng bộ phong cách giao diện giữa các khu vực.
- Bổ sung bảng dữ liệu có thể cuộn.
- Không điền sẵn mật khẩu demo vào trường mật khẩu.
- Cho phép tải lại danh sách đề bài từ Server mà không cần đăng nhập lại.
- Cải thiện thông báo khi biên dịch thất bại bằng cách cố gắng trả về thông báo lỗi từ GCC.

---

## 4. Cài đặt môi trường trên Ubuntu

### 4.1. Cập nhật hệ thống

Mở Terminal và thực hiện:

```bash
sudo apt update
sudo apt upgrade -y
```

### 4.2. Cài đặt các thư viện cần thiết

```bash
sudo apt install build-essential gcc make pkg-config libgtk-3-dev libsqlite3-dev
```

### 4.3. Kiểm tra môi trường

Kiểm tra GCC:

```bash
gcc --version
```

Kiểm tra GTK 3:

```bash
pkg-config --modversion gtk+-3.0
```

Kiểm tra SQLite:

```bash
pkg-config --modversion sqlite3
```

Nếu các lệnh trên hiển thị phiên bản tương ứng, môi trường đã có các thành phần cần thiết để biên dịch dự án.

---

## 5. Biên dịch chương trình

Mở Terminal và di chuyển đến thư mục gốc của dự án:

```bash
cd ~/demo-online-judge
```

Xóa các file biên dịch cũ:

```bash
make clean
```

Tiến hành biên dịch:

```bash
make
```

Sau khi biên dịch thành công, hệ thống tạo ra hai chương trình:

| Chương trình | Chức năng |
|---|---|
| `oj-server` | Chương trình Server quản trị |
| `oj-client` | Chương trình Client dành cho sinh viên |

Nếu quá trình biên dịch xuất hiện lỗi thiếu thư viện, cần kiểm tra lại các gói GTK 3 và SQLite development đã được cài đặt hay chưa.

---

## 6. Hướng dẫn chạy hệ thống

Hệ thống cần chạy Server trước khi khởi động Client.

### 6.1. Khởi động Server

Mở Terminal thứ nhất:

```bash
cd ~/demo-online-judge
./oj-server
```

Cửa sổ Admin Console xuất hiện.

Nhấn nút **Start Server** để khởi động TCP Server.

Server bắt đầu lắng nghe kết nối trên:

```text
TCP Port: 5050
```

### 6.2. Khởi động Client

Mở Terminal thứ hai:

```bash
cd ~/demo-online-judge
./oj-client
```

Tại màn hình đăng nhập, nhập địa chỉ IPv4 của Server.

Nếu Client và Server chạy trên cùng một máy:

```text
Server IP: 127.0.0.1
```

Nếu chạy trên hai máy khác nhau trong cùng mạng LAN, nhập địa chỉ IPv4 của máy Server.

### 6.3. Tài khoản demo

Hệ thống khởi tạo sẵn hai tài khoản mẫu:

| Vai trò | Tên đăng nhập | Mật khẩu |
|---|---|---|
| Quản trị viên | admin | admin123 |
| Sinh viên demo | student | student123 |

### 6.4. Quy trình nộp bài

Sau khi đăng nhập thành công:

1. Client tải danh sách đề bài từ Server.
2. Sinh viên chọn một đề cần giải.
3. Đọc mô tả, yêu cầu đầu vào, đầu ra và ví dụ.
4. Viết mã nguồn C trực tiếp trong trình soạn thảo hoặc mở file `.c`.
5. Nhấn **Submit Solution** để gửi bài.
6. Server tiếp nhận và tiến hành chấm bài.
7. Client nhận kết quả và điểm số.
8. Nhấn **Refresh Submission History** để tải lại lịch sử nộp bài.

---

## 7. Các chức năng chính

### 7.1. Chức năng quản trị

| Chức năng | Mô tả |
|---|---|
| Start Server | Khởi động TCP Server |
| Stop Server | Dừng TCP Server |
| Dashboard | Hiển thị số liệu thống kê tổng quan |
| Problems | Thêm, sửa và xóa đề bài |
| Test Cases | Thêm, sửa và xóa Test Case |
| Submissions | Theo dõi bài nộp và trạng thái chấm |
| Leaderboard | Hiển thị bảng điểm theo sinh viên |
| Activity Log | Theo dõi nhật ký hoạt động |

### 7.2. Quản lý đề bài

Quản trị viên có thể:

- Thêm đề bài mới.
- Chỉnh sửa nội dung đề bài.
- Xóa đề bài.
- Thiết lập mô tả và yêu cầu đầu vào, đầu ra.
- Cung cấp ví dụ minh họa.
- Thiết lập giới hạn thời gian thực thi.

### 7.3. Quản lý Test Case

Mỗi đề bài có thể được thiết lập nhiều Test Case.

Thông tin mỗi Test Case gồm:

- Input: Dữ liệu đầu vào.
- Expected Output: Kết quả đầu ra mong đợi.

Quản trị viên có thể thêm, chỉnh sửa và xóa Test Case tương ứng với từng đề bài.

### 7.4. Chức năng chấm bài tự động

Khi nhận được mã nguồn từ Client, Server thực hiện:

1. Lưu và chuẩn bị mã nguồn để biên dịch.
2. Gọi GCC để biên dịch chương trình C.
3. Nếu biên dịch thất bại, trả về kết quả CE.
4. Nếu biên dịch thành công, thực thi chương trình.
5. Chạy chương trình với từng Test Case.
6. So sánh kết quả thực tế với Expected Output.
7. Tổng hợp kết quả và gửi verdict về Client.

### 7.5. Các trạng thái chấm bài

| Verdict | Ý nghĩa |
|---|---|
| AC (Accepted) | Bài làm vượt qua tất cả Test Case |
| WA (Wrong Answer) | Kết quả không đúng với đáp án mong đợi |
| CE (Compilation Error) | Lỗi biên dịch |
| RE (Runtime Error) | Lỗi trong quá trình thực thi |
| TLE (Time Limit Exceeded) | Vượt quá giới hạn thời gian |

Điểm số được hiển thị theo thang điểm từ 0 đến 10.

---

## 8. Cơ sở dữ liệu và cấu trúc thư mục

### 8.1. SQLite Database

Hệ thống sử dụng SQLite để lưu trữ dữ liệu.

File cơ sở dữ liệu:

```text
online_judge.db
```

Database được tự động tạo tại thư mục chạy `oj-server` khi khởi động lần đầu.

Các nhóm dữ liệu chính:

- Tài khoản người dùng.
- Danh sách đề bài.
- Test Case của từng đề bài.
- Lịch sử nộp bài.
- Kết quả chấm bài.
- Nhật ký hoạt động.

### 8.2. Dữ liệu mẫu

Khi khởi tạo lần đầu, hệ thống tạo sẵn:

- Tài khoản quản trị viên `admin`.
- Tài khoản sinh viên `student`.
- Đề bài `Sum Two Numbers`.
- Đề bài `Find Maximum`.
- Đề bài `Even or Odd`.

Mỗi đề bài mẫu có ba Test Case để phục vụ kiểm thử.

### 8.3. Cấu trúc thư mục

```text
demo-online-judge/
│
├── Makefile
├── README.md
├── online_judge.db
│
├── include/
│   ├── protocol.h
│   └── database.h
│
├── src/
│   ├── client.c
│   ├── server.c
│   ├── database.c
│   └── protocol.c
│
├── oj-server
└── oj-client
```

Trong đó:

| Thư mục / File | Chức năng |
|---|---|
| `include/` | Chứa các file header |
| `protocol.h` | Khai báo cấu trúc và thông tin giao tiếp |
| `database.h` | Khai báo các cấu trúc và hàm xử lý database |
| `client.c` | Xử lý giao diện và chức năng Client |
| `server.c` | Xử lý Server, giao tiếp mạng và giao diện quản trị |
| `database.c` | Thao tác với cơ sở dữ liệu SQLite |
| `protocol.c` | Xử lý giao thức trao đổi dữ liệu |
| `Makefile` | Quy định các bước biên dịch |
| `online_judge.db` | Cơ sở dữ liệu được tạo khi chạy Server |
| `oj-server` | File thực thi Server |
| `oj-client` | File thực thi Client |

**Lưu ý:** Không xóa `online_judge.db` nếu muốn giữ lại tài khoản, đề bài, Test Case và lịch sử chấm. Nếu cần khởi tạo lại dữ liệu mẫu, hãy dừng Server và sao lưu database trước khi xóa.

---

## 9. Nội dung lập trình hệ thống và IPC

Đây là phần kiến thức hệ điều hành và lập trình hệ thống được áp dụng trực tiếp trong đồ án.

### 9.1. TCP Socket

TCP Socket là cơ chế giao tiếp chính giữa Client và Server.

Các chức năng trao đổi dữ liệu gồm:

- Gửi thông tin đăng nhập.
- Yêu cầu tải danh sách đề bài.
- Yêu cầu tải nội dung đề bài.
- Gửi mã nguồn C khi nộp bài.
- Nhận trạng thái và kết quả chấm bài.

Server lắng nghe kết nối trên TCP port 5050.

### 9.2. pthread

Server sử dụng pthread để xử lý nhiều Client đồng thời.

Khi có Client kết nối, Server tạo luồng xử lý tương ứng để tiếp nhận và xử lý yêu cầu.

Cách tổ chức này giúp Server không phải xử lý toàn bộ Client theo kiểu tuần tự.

### 9.3. fork() và exec()

Trong quá trình chấm bài, Server sử dụng:

- `fork()`: Tạo tiến trình con.
- `exec()`: Thay thế chương trình đang chạy trong tiến trình con bằng GCC hoặc chương trình cần thực thi.

Nhờ đó, việc biên dịch và chạy bài nộp được thực hiện trong tiến trình con, tách biệt với luồng xử lý chính của Server.

### 9.4. waitpid() và signal

- `waitpid()`: Theo dõi trạng thái tiến trình con và thu hồi tài nguyên tiến trình.
- `signal`: Hỗ trợ xử lý tín hiệu và dừng tiến trình khi cần thiết.

Các cơ chế này được sử dụng trong quá trình theo dõi và kiểm soát việc thực thi chương trình.

### 9.5. File Descriptor và Redirection

File Descriptor được sử dụng để quản lý các luồng dữ liệu vào và ra của tiến trình.

Redirection cho phép chuyển dữ liệu Test Case vào chương trình và thu thập kết quả đầu ra để so sánh với đáp án mong đợi.

### 9.6. Vai trò của SQLite

SQLite đảm nhiệm việc lưu trữ dữ liệu dùng chung của hệ thống, bao gồm tài khoản, đề bài, Test Case, lịch sử nộp bài và nhật ký hoạt động.

### 9.7. Phân biệt IPC và quản lý tiến trình

Cần phân biệt rõ:

- TCP Socket là cơ chế IPC được sử dụng để giao tiếp giữa hai ứng dụng Client và Server.
- pthread là cơ chế đa luồng, hỗ trợ xử lý nhiều kết nối đồng thời.
- `fork()` và `exec()` là các API tạo và thực thi tiến trình, không phải cơ chế IPC.
- `waitpid()` và `signal` hỗ trợ quản lý, theo dõi và điều khiển tiến trình.

Như vậy, đồ án thể hiện cả giao tiếp liên tiến trình, lập trình mạng, đa luồng và quản lý tiến trình trên Linux.

---

## 10. Kiểm thử hệ thống

### 10.1. Kiểm thử kết nối

- Khởi động Server thành công.
- Client kết nối đến Server qua địa chỉ IPv4.
- Kiểm tra trường hợp Client nhập sai địa chỉ IP.
- Kiểm tra trường hợp Server chưa được khởi động.

### 10.2. Kiểm thử đăng nhập

- Đăng nhập bằng tài khoản hợp lệ.
- Kiểm tra tài khoản không tồn tại.
- Kiểm tra mật khẩu không chính xác.
- Kiểm tra đăng nhập với hai vai trò quản trị viên và sinh viên.

### 10.3. Kiểm thử chức năng đề bài

- Tải danh sách đề bài.
- Xem nội dung chi tiết.
- Thêm, sửa và xóa đề bài.
- Kiểm tra chức năng tải lại danh sách.

### 10.4. Kiểm thử Test Case

- Thêm Test Case mới.
- Chỉnh sửa Input và Expected Output.
- Xóa Test Case.
- Kiểm tra kết quả khi chạy chương trình với nhiều Test Case.

### 10.5. Kiểm thử Online Judge Engine

| Trường hợp | Kết quả mong đợi |
|---|---|
| Chương trình đúng tất cả Test Case | AC |
| Chương trình có kết quả sai | WA |
| Mã nguồn có lỗi cú pháp | CE |
| Chương trình phát sinh lỗi khi chạy | RE |
| Chương trình chạy quá thời gian | TLE |

### 10.6. Kiểm thử nhiều Client

Khởi động Server và mở nhiều chương trình Client để kiểm tra khả năng tiếp nhận và xử lý nhiều kết nối đồng thời thông qua pthread.

---

## 11. Lưu ý bảo mật và hạn chế

### 11.1. Lưu ý bảo mật

CodeJudge được xây dựng nhằm mục đích học tập, thực hành và demo trong môi trường phòng lab hoặc máy ảo Ubuntu.

Hệ thống hiện chưa phải một nền tảng chấm bài trực tuyến an toàn để triển khai công khai trên Internet.

Mã nguồn C do sinh viên gửi được thực thi bằng quyền của tài khoản chạy Server. Vì vậy, không nên sử dụng hệ thống để chạy mã nguồn không đáng tin cậy trên máy tính chứa dữ liệu quan trọng.

### 11.2. Hạn chế hiện tại

- Chưa có sandbox cô lập hoàn chỉnh cho chương trình bài nộp.
- Giới hạn thời gian thực thi đơn giản chưa thay thế được cơ chế kiểm soát tài nguyên.
- Mật khẩu được băm SHA-256 nhưng chưa có salt riêng.
- Chưa có cơ chế đổi mật khẩu và giới hạn số lần đăng nhập.
- Chưa có cơ chế xác thực phiên đăng nhập và phân quyền quản trị từ xa.
- Kết nối TCP chưa được mã hóa.

### 11.3. Hướng phát triển

Trong tương lai, hệ thống có thể được cải tiến bằng cách:

- Sử dụng Docker hoặc Virtual Machine để cô lập môi trường thực thi.
- Giới hạn CPU, RAM, số tiến trình và dung lượng file.
- Kiểm soát quyền truy cập filesystem và network.
- Bổ sung xác thực phiên đăng nhập.
- Bổ sung cơ chế quản lý mật khẩu an toàn hơn.
- Mã hóa kết nối giữa Client và Server.
- Bổ sung thống kê và báo cáo kết quả học tập.
- Hỗ trợ thêm các ngôn ngữ lập trình khác.

---

## 12. Xử lý lỗi thường gặp

| Lỗi | Nguyên nhân và cách khắc phục |
|---|---|
| `Package gtk+-3.0 was not found` | Chưa cài GTK 3 development package. Chạy `sudo apt install libgtk-3-dev`. |
| `sqlite3.h: No such file or directory` | Chưa cài SQLite development package. Chạy `sudo apt install libsqlite3-dev`. |
| Không thể biên dịch chương trình | Kiểm tra GCC, Makefile và các thư viện cần thiết. |
| `Cannot bind port 5050` | Port 5050 đang được chương trình khác sử dụng. Kiểm tra bằng `ss -ltnp` và dừng tiến trình chiếm port nếu phù hợp. |
| Client không kết nối được | Kiểm tra Server đã chạy và đã nhấn Start Server, địa chỉ IPv4 đúng và firewall cho phép TCP port 5050. |
| Không thấy dữ liệu mẫu | Kiểm tra thư mục chạy `oj-server` và vị trí file `online_judge.db`. |
| Không đăng nhập được | Kiểm tra tên đăng nhập, mật khẩu và dữ liệu tài khoản trong SQLite. |

---

## 13. Phân công công việc nhóm

### 13.1. Nguyên tắc phân công

Nhóm chia công việc theo từng module chức năng để các thành viên có thể phát triển độc lập và phối hợp tích hợp hệ thống.

Mỗi thành viên chịu trách nhiệm chính một module, đồng thời phối hợp với các thành viên khác trong quá trình kiểm thử và hoàn thiện sản phẩm.

### 13.2. Phân công chi tiết

**Thành viên 1 – Student Client**

- Xây dựng giao diện sinh viên bằng GTK 3.
- Thiết kế màn hình đăng nhập.
- Xử lý kết nối đến Server qua TCP Socket.
- Tải danh sách đề bài và nội dung chi tiết.
- Xây dựng trình soạn thảo mã nguồn C.
- Hỗ trợ mở file `.c` từ máy tính.
- Gửi bài làm và hiển thị kết quả chấm.
- Xây dựng chức năng xem lịch sử nộp bài.

**Thành viên 2 – TCP Server và Socket**

- Xây dựng TCP Server.
- Xử lý các thao tác socket như tạo socket, bind, listen, accept, send và recv.
- Tiếp nhận yêu cầu từ Client.
- Xử lý giao tiếp giữa Client và Server.
- Xây dựng cơ chế tạo pthread để xử lý nhiều Client đồng thời.
- Phối hợp với Database và Judge Engine để xử lý yêu cầu.

**Thành viên 3 – Online Judge Engine**

- Xây dựng chức năng biên dịch mã nguồn C bằng GCC.
- Sử dụng `fork()` để tạo tiến trình con.
- Sử dụng `exec()` để thực thi GCC và chương trình bài nộp.
- Sử dụng `waitpid()` để theo dõi tiến trình.
- Chuyển dữ liệu Test Case vào chương trình thông qua File Descriptor và Redirection.
- So sánh kết quả thực tế với Expected Output.
- Xác định các verdict AC, WA, CE, RE và TLE.
- Tổng hợp kết quả và điểm số trả về Server.

**Thành viên 4 – SQLite Database**

- Thiết kế cấu trúc cơ sở dữ liệu SQLite.
- Quản lý dữ liệu tài khoản người dùng.
- Xây dựng chức năng thêm, sửa, xóa và truy vấn đề bài.
- Xây dựng chức năng quản lý Test Case.
- Lưu trữ lịch sử nộp bài và kết quả chấm.
- Lưu nhật ký hoạt động.
- Phối hợp với Server và Admin Console trong việc truy xuất dữ liệu.

**Thành viên 5 – Admin Console và tích hợp hệ thống**

- Xây dựng giao diện quản trị bằng GTK 3.
- Thiết kế Dashboard thống kê.
- Xây dựng giao diện quản lý Problems.
- Xây dựng giao diện quản lý Test Cases.
- Xây dựng giao diện theo dõi Submissions.
- Xây dựng giao diện Leaderboard.
- Xây dựng khu vực nhật ký hoạt động.
- Hoàn thiện bố cục và giao diện tổng thể của Admin Console.
- Phối hợp tích hợp các module và kiểm thử toàn hệ thống.

### 13.3. Bảng tổng hợp phân công

| STT | Thành viên | Module phụ trách | Kết quả bàn giao |
|---|---|---|---|
| 1 | Thành viên 1 | Student Client | Giao diện sinh viên, đăng nhập, tải đề, soạn mã nguồn, gửi bài và xem kết quả |
| 2 | Thành viên 2 | TCP Server và Socket | Server, giao tiếp TCP, xử lý yêu cầu và nhiều Client bằng pthread |
| 3 | Thành viên 3 | Online Judge Engine | Biên dịch, thực thi, kiểm tra Test Case và xác định verdict |
| 4 | Thành viên 4 | SQLite Database | Cơ sở dữ liệu, quản lý tài khoản, đề bài, Test Case và lịch sử bài nộp |
| 5 | Thành viên 5 | Admin Console và tích hợp | Giao diện quản trị, Dashboard, quản lý Problems, Test Cases, Submissions, Leaderboard và kiểm thử toàn hệ thống |

### 13.4. Trách nhiệm chung

Ngoài module được phân công, tất cả thành viên cùng tham gia:

- Phân tích yêu cầu và thống nhất kiến trúc hệ thống.
- Tích hợp các module thành sản phẩm hoàn chỉnh.
- Kiểm thử chức năng và sửa lỗi.
- Kiểm tra khả năng giao tiếp Client–Server.
- Chuẩn bị báo cáo, slide thuyết trình và demo.
- Quản lý mã nguồn và phối hợp làm việc nhóm.

**Lưu ý:** Các thành viên 2, 3 và 5 có thể cùng làm việc trên `src/server.c` nhưng phụ trách các phần khác nhau: giao tiếp mạng, xử lý chấm bài và giao diện quản trị. Việc phân công được thực hiện theo module chức năng, không nhất thiết chia cứng theo từng file.

---

## 14. Kết luận

CodeJudge là đồ án ứng dụng các kiến thức lập trình C, lập trình mạng và hệ điều hành để xây dựng một hệ thống chấm bài lập trình tự động theo mô hình Client–Server.

Thông qua đồ án, nhóm đã vận dụng TCP Socket để giao tiếp giữa các ứng dụng, pthread để xử lý nhiều Client đồng thời, các API quản lý tiến trình trong Linux để biên dịch và thực thi mã nguồn, đồng thời sử dụng SQLite để quản lý dữ liệu.

Hệ thống cung cấp các chức năng cơ bản của một nền tảng Online Judge, bao gồm quản lý đề bài, quản lý Test Case, nộp bài, chấm bài tự động và theo dõi kết quả.

Đồ án giúp nhóm củng cố kiến thức về ngôn ngữ C, lập trình hệ thống, giao tiếp liên tiến trình và phát triển ứng dụng trên môi trường Ubuntu Linux.

---

**Đề tài:** CodeJudge – C Online Judge System

**Phiên bản:** V2
