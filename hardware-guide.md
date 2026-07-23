# Hướng dẫn build và chạy trên phần cứng thật — Game Ping Pong STM32

Tài liệu này hướng dẫn lắp ráp dây nối, build firmware và chạy game trực tiếp trên board **STM32F429I-DISCO** với biến trở và module motor rung thật.

---

## 1. Danh sách linh kiện cần chuẩn bị

| STT | Linh kiện | Số lượng | Ghi chú |
|:---:|---|:---:|---|
| 1 | Board STM32F429I-DISCO | 1 | Đã có LCD 240×320, touchscreen và USER button PA0 |
| 2 | Biến trở trượt hoặc xoay | 2 | Loại 3 chân: VCC, GND, OUT (Wiper) |
| 3 | Module motor rung | 2 | Loại đã có IC driver (không nối motor trần vào GPIO) |
| 4 | Dây nối jumper Male-Female | ≥ 12 sợi | Để nối từ chân header P1/P2 trên board ra linh kiện |
| 5 | Cáp USB Mini-B | 1 | Flash firmware qua cổng ST-LINK (CN1) |
| 6 | Nguồn 5V (tuỳ chọn) | 1 | Nếu module motor cần dòng lớn hơn USB cấp được |

---

## 2. Sơ đồ kết nối

> Sơ đồ chi tiết từng chân đã có trong **`README.md` → mục Sơ đồ schematic**. Bảng dưới tóm tắt nhanh để tra cứu khi lắp ráp.

| Thiết bị | Chân module | Chân board | Vị trí header |
|---|---|---|---|
| Biến trở Player 1 | VCC | 3V3 | P1 – pin 1 |
| Biến trở Player 1 | GND | GND | P1 – pin 2 |
| Biến trở Player 1 | OUT (Wiper/OTA) | **PA5** (ADC1_IN5) | P1 – pin 21 |
| Biến trở Player 2 | VCC | 3V3 | P1 – pin 1 |
| Biến trở Player 2 | GND | GND | P1 – pin 2 |
| Biến trở Player 2 | OUT (Wiper/OTA) | **PC3** (ADC2_IN13) | P1 – pin 15 |
| Motor Player 1 | VCC | 5V | P1 – pin 3 (hoặc nguồn ngoài) |
| Motor Player 1 | GND | GND | P1 – pin 2 |
| Motor Player 1 | IN | **PC8** (active-high) | P2 – pin 55 |
| Motor Player 2/CPU | VCC | 5V | P1 – pin 3 (hoặc nguồn ngoài) |
| Motor Player 2/CPU | GND | GND | P1 – pin 2 |
| Motor Player 2/CPU | IN | **PC11** (active-high) | P2 – pin 44 |
| Nút bấm | — | **PA0** (có sẵn trên board) | USER button |

> **Lưu ý:** Biến trở chỉ cấp **3V3** (không cấp 5V). Motor phải dùng **module có IC driver** — không nối motor trần vào GPIO.

---

## 3. Kiểm tra phần cứng trước khi flash

Trước khi nạp firmware, kiểm tra nhanh bằng đồng hồ đo điện:

| Kiểm tra | Giá trị kỳ vọng | Nếu sai |
|---|---|---|
| Điện áp giữa VCC và GND biến trở | 3.3V ± 0.1V | Kiểm tra lại dây nguồn |
| Điện áp OUT biến trở khi vặn hết sang trái | ≈ 0V | Biến trở có thể đấu ngược VCC/GND |
| Điện áp OUT biến trở khi vặn hết sang phải | ≈ 3.3V | Bình thường |
| Điện áp VCC motor | 5V ± 0.3V | Kiểm tra nguồn 5V |
| Không có chập giữa PA5 và GND | Điện trở > 10kΩ | Kiểm tra dây |

---

## 4. Build Firmware bằng STM32CubeIDE

### 4.1 Import project

1. Mở **STM32CubeIDE**.
2. Vào **File → Import → General → Existing Projects into Workspace**.
3. Nhấn **Browse**, chọn thư mục con:

   ```
   <thư mục gốc dự án>\STM32CubeIDE\
   ```

4. Tick chọn project `STM32F429I-DISCO` trong danh sách, nhấn **Finish**.

### 4.2 Generate code giao diện (nếu chưa làm)

Nếu vừa chỉnh sửa giao diện trong **TouchGFX Designer**, cần generate code trước khi build:

1. Mở `TouchGFX\Project-Nhung.touchgfx` trong TouchGFX Designer.
2. Nhấn **F4** (Generate Code).
3. Đóng Designer, quay lại STM32CubeIDE.

> Bước này không cần thiết nếu không sửa giao diện.

### 4.3 Build firmware

1. Chọn build configuration **Debug** trên thanh toolbar (dropdown cạnh biểu tượng búa).
2. Nhấn **Ctrl + B** hoặc menu **Project → Build All**.
3. Tab **Console** hiển thị quá trình biên dịch. Kết thúc thành công khi thấy:

   ```
   Finished building: STM32F429I-DISCO.elf
   ```

4. File output nằm tại:

   ```
   STM32CubeIDE\Debug\STM32F429I-DISCO.elf
   ```

> Lần đầu build mất khoảng 2–5 phút. Các lần sau chỉ build lại file thay đổi (incremental build).

---

## 5. Flash Firmware lên Board

### 5.1 Kết nối board với PC

1. Cắm cáp **USB Mini-B** vào cổng **CN1** (góc dưới board, nhãn **ST-LINK**).
2. Cắm đầu kia vào PC. Windows tự nhận driver nếu đã cài **STM32CubeIDE** hoặc **STSW-LINK009**.
3. Đèn LED **LD8** (màu đỏ) trên board sáng — chứng tỏ ST-LINK đã nhận nguồn.

### 5.2 Flash và chạy

**Cách 1 — Chạy thẳng (không debug):**

1. Vào menu **Run → Run Configurations**.
2. Chọn cấu hình `STM32F429I_DISCO_REV_D01 Debug`.
3. Nhấn **Run** (hoặc `Ctrl + F11`). STM32CubeIDE tự flash và reset board.

**Cách 2 — Flash và debug (quan sát biến, breakpoint):**

1. Nhấn **F11** hoặc nút **Debug** (biểu tượng con bọ).
2. CubeIDE flash firmware, sau đó dừng tại `main()`.
3. Nhấn **F8** (Resume) để board bắt đầu chạy.

### 5.3 Xác nhận board đang chạy đúng

Sau khi flash thành công, board hiển thị:

- **Screen 1**: Hai nút `1 Player` và `2 Players` trên nền màu.
- Vặn biến trở Player 1 → paddle dưới di chuyển trái/phải trên màn hình.
- Vặn biến trở Player 2 → paddle trên di chuyển (ở chế độ `2 Players`).
- Nhấn nút **USER** (nút xanh trên board, PA0) 1 lần → game tạm dừng.
- Nhấn nút **USER** 2 lần liên tiếp → quay về Screen 1.

---

## 6. Hiệu chỉnh biến trở (Calibration)

Nếu paddle không đi đến tận cùng trái/phải dù đã vặn hết biến trở, firmware dùng chuẩn hóa cứng `raw 0–4095 → input 0–1000` (công thức `input = raw * 1000 / 4095`). Có thể kiểm tra giá trị ADC thực tế qua UART:

1. Board **STM32F429I-DISCO không có Virtual COM Port** qua ST-LINK, nên debug UART chỉ phát ra chân **PA9 (USART1_TX)**. Nối một USB-UART adapter: **RX adapter ↔ PA9**, **GND adapter ↔ GND board** (chỉ cần chiều TX của board).
2. Mở terminal (PuTTY, TeraTerm) với **baudrate 115200**, 8N1.
3. Board log ADC mỗi 500 ms theo đúng định dạng do hàm `DebugUart_LogAdc()` in ra:

   ```
   [ADC] status=OK raw1=2048 raw2=3100 p1=500 p2=756 adc_error=0x00000000
   ```

   Trong đó `raw1`/`raw2` là giá trị ADC thô (0–4095) của Player 1 (PA5) và Player 2 (PC3), còn `p1`/`p2` là giá trị đã chuẩn hóa (0–1000).

4. Vặn hết biến trở sang hai đầu và đọc giá trị `raw1`/`raw2` min/max thực tế.
5. Firmware hiện **không có hằng số calibration** — phép chuẩn hóa nằm trực tiếp trong hàm `StartHardwareTask()` (`Core/Src/main.c`). Nếu biến trở không đạt dải 0/4095, sửa công thức tính `player1` / `player2` trong task này để bù offset/scale theo min/max đo được.

---

## 7. Troubleshooting phần cứng

| Triệu chứng | Nguyên nhân thường gặp | Cách xử lý |
|---|---|---|
| Paddle không di chuyển khi vặn biến trở | Dây OUT (Wiper) chưa nối hoặc nối sai chân | Kiểm tra PA5 (P1-21) và PC3 (P1-15) |
| Paddle đứng im hoặc nhảy lung tung | VCC biến trở bị cấp 5V thay vì 3V3 | Chuyển sang chân 3V3 trên P1 |
| Motor không rung khi đỡ bóng | Dây IN motor chưa nối hoặc GND không chung | Kiểm tra PC8 (P2-55) và PC11 (P2-44) |
| Motor rung liên tục không tắt | GND motor không chung với GND STM32 | Nối GND motor vào GND board |
| Board không được nhận bởi CubeIDE | Driver ST-LINK chưa cài hoặc cáp USB lỗi | Cài **STSW-LINK009**, thử cáp khác |
| Màn hình hiển thị nhưng game không chạy | Firmware bị treo tại HardwareTask | Kiểm tra ADC bị short circuit |
| Board reset liên tục | Nguồn không đủ dòng (motor hút nhiều dòng) | Dùng nguồn 5V ngoài thay vì USB |
| Lỗi "No target connected" khi flash | ST-LINK chưa được nhận hoặc board chưa reset | Bấm nút RESET (B2) trên board trước khi flash |
