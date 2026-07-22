# PHÂN TÍCH CHI TIẾT CÁC TASK TRONG DỰ ÁN PING PONG STM32

Dự án sử dụng hệ điều hành thời gian thực **FreeRTOS** để phân tách luồng xử lý phần cứng và luồng hiển thị đồ họa. Hệ thống gồm **2 Task FreeRTOS chính** và **1 Module Cầu nối (IPC Inter-Thread Communication)** giữa C và C++.

---

## 1. TASK 1: Hardware Task (`StartHardwareTask`)
- **Tên Task trong code:** `StartHardwareTask` (`Core/Src/main.c`)
- **Tần số/Chu kỳ chạy:** Vòng lặp `osDelay(10U)` — Chạy mỗi 10ms (100Hz).
- **Vai trò:** Quản lý toàn bộ các giao tiếp ngoại vi phần cứng mức thấp (ADC, GPIO, Motor rung, Debounce nút bấm).

### Chi tiết các nhiệm vụ & Lệnh/Hàm thực thi:

#### A. Đọc & Xử lý tín hiệu biến trở trượt (Input Người chơi)
* **Lệnh HAL thực thi:** 
  - `ReadAdcChannel(ADC_CHANNEL_5, &player1Raw)`: Đọc tín hiệu Analog từ biến trở 1 (PA5) điều khiển Paddle dưới.
  - `ReadAdcChannel(ADC_CHANNEL_13, &player2Raw)`: Đọc tín hiệu Analog từ biến trở 2 (PC3) điều khiển Paddle trên.
* **Lược đồ xử lý:**
  - Lấy trung bình 8 mẫu ADC liên tiếp để giảm nhiễu gai.
  - Lọc bằng thuật toán **EMA (Exponential Moving Average)** dạng số nguyên để tín hiệu không bị giật.
  - Chuẩn hóa (Scale) khoảng ADC raw 12-bit (`0..4095`) về dải chuẩn `0..1000`.
* **Đóng gói dữ liệu:**
  - Đóng gói 2 giá trị `uint16_t` vào 1 biến 32-bit `latestInputMessage = (player2 << 16) | player1`.

#### B. Nhận diện thao tác Nút bấm PA0 (User Button)
* **Lệnh HAL thực thi:** `HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin)` (Đọc chân PA0).
* **Xử lý:**
  - Đưa mẫu đọc vào `Pa0GestureDetector_Update(...)` để lọc nhiễu dội phím (debounce) và phân loại thao tác: Nhấn đơn (`PA0_GESTURE_EVENT_SINGLE_PRESS`) hoặc Nhấn kép (`PA0_GESTURE_EVENT_DOUBLE_PRESS`).
  - Đẩy sự kiện ra biến toàn cục `latestPa0EventMessage` dưới sự bảo vệ của cờ ngắt critical section (`AppBackend_EnterCritical()`).

#### C. Xử lý phản hồi rung Haptic Motor
* **Đọc Queue:** Gọi `osMessageQueueGet(hapticQueueHandle, &hapticCommand, NULL, 0U)` để nhặt lệnh haptic từ GUI gửi xuống.
* **Lệnh HAL thực thi:**
  - Nếu `APP_HAPTIC_PLAYER_1`: Gọi `HAL_GPIO_WritePin(MOTOR_PLAYER_GPIO_Port, MOTOR_PLAYER_Pin, GPIO_PIN_SET)`, hẹn giờ tắt sau 80ms (`now + 80U`).
  - Nếu `APP_HAPTIC_PLAYER_2`: Gọi `HAL_GPIO_WritePin(MOTOR_CPU_GPIO_Port, MOTOR_CPU_Pin, GPIO_PIN_SET)`, hẹn giờ tắt sau 80ms (`now + 80U`).
  - Nếu `APP_HAPTIC_STOP_ALL`: Gọi `HAL_GPIO_WritePin(..., GPIO_PIN_RESET)` cho cả 2 motor ngay lập tức.
* **Tự động tắt:** Khi đến thời gian ngắt (deadline), tự động gọi `HAL_GPIO_WritePin(..., GPIO_PIN_RESET)` để ngắt điện motor.

---

## 2. TASK 2: GUI Task (`GUI_Task` & Game Loop)
- **Tên Task trong code:** `GUI_Task` (Do TouchGFX sinh tự động).
- **Tần số/Chu kỳ chạy:** Đồng bộ theo chu kỳ quét khung hình màn hình LCD (Tần số quét VSYNC / 60 FPS).
- **Vai trò:** Tính toán logic trò chơi (vật lý, va chạm, AI CPU) và render giao diện đồ họa TouchGFX lên màn hình LCD.

### Chi tiết các nhiệm vụ & Lệnh/Hàm thực thi:

#### A. Vòng lặp Game (Game Loop - `Screen2View::ball_timertick`)
* Mỗi khung hình, TouchGFX phát sự kiện `handleTickEvent()`, kích hoạt hàm `ball_timertick()`.
* **Cập nhật Input:** Gọi `model.getPlayer1Input()` và `model.getPlayer2Input()` để lấy vị trí tay cầm hiện tại.

#### B. Xử lý Logic GameEngine (`TouchGFX/gui/include/gui/game/GameEngine.hpp`)
* Gọi hàm `gameEngine.update(player1Input, player2Input)`:
  - **Di chuyển bóng:** Cập nhật vị trí bóng dựa trên vận tốc X, Y bằng toán số nguyên cố định (**Fixed-point Q8.8**) giúp bóng di chuyển mượt mà không bị làm tròn số.
  - **Xử lý va chạm biên:** Khi bóng đập vào mép trái/phải sân chơi, đổi chiều vận tốc X (`velocityX = -velocityX`).
  - **Xử lý va chạm Paddle:** Gọi `circleTouchesPaddle(...)` để kiểm tra va chạm giữa bóng tròn và paddle hình chữ nhật.
  - **Tính góc bật linh hoạt:** Gọi `updateHorizontalVelocity(paddleX)` — nếu bóng đập vào mép ngoài của paddle, vận tốc ngang X sẽ tăng lên để tạo góc bật xoáy.
  - **Xử lý AI CPU (`updateCpu()`):** Trong chế độ `VS_CPU`, paddle trên tự động bám theo tọa độ bóng X mỗi 4 tick, có thêm sai số ngẫu nhiên để tạo độ khó vừa phải.
  - **Trạng thái trận đấu (State Machine):** Chuyển đổi giữa `READY` (chờ 30 tick), `PLAYING`, `PAUSED`, và `GAME_OVER` (khi bóng vượt hoàn toàn biên trên/dưới).

#### C. Cập nhật Đồ họa Widgets TouchGFX
* Lấy tọa độ tính toán từ `GameEngine` để cập nhật trực tiếp lên các đối tượng đồ họa:
  - `p1.setXY(paddle1X, 280)` (Cập nhật Paddle 1)
  - `p2.setXY(paddle2X, 65)` (Cập nhật Paddle 2 / CPU)
  - `circle1.setXY(ballX, ballY)` (Cập nhật vị trí bóng)
  - Gọi `.invalidate()` để ép TouchGFX vẽ lại vùng ảnh mới trên LCD.
* **Xử lý Popup:** Khi trạng thái là `GAME_OVER`, kích hoạt Popup kết quả, hiển thị ai thắng (Player 1 / Player 2 / CPU) và hiện nút `Chơi lại`.

---

## 3. MODULE CẦU NỐI: IPC Inter-Thread (`AppBackend` & `Model`)
- **Tên Module trong code:** `app_backend.c/.h` và `Model.cpp/.hpp`
- **Vai trò:** Cung cấp kênh giao tiếp an toàn, bất đồng bộ (Non-blocking), không gây xung đột vùng nhớ (Thread-safe) giữa `HardwareTask` (Code C) và `GUI_Task` (Code C++).

### Chi tiết các nhiệm vụ & Lệnh/Hàm thực thi:

#### A. Kênh truyền Input tay cầm (`HardwareTask` $\rightarrow$ `TouchGFX`)
* **Hàm thực thi:** `AppBackend_GetLatestInput(uint16_t* player1, uint16_t* player2)`
* **Cách hoạt động:**
  - Trong `Model::tick()`, TouchGFX gọi hàm này để đọc biến 32-bit `latestInputMessage`.
  - Hàm tách 16 bit thấp lấy `player1` và 16 bit cao lấy `player2`.
  - Phao đọc atomic này giúp GUI luôn nhận được dữ liệu 2 người chơi đồng thời tại cùng 1 khung hình.

#### B. Kênh truyền Sự kiện Nút bấm (`HardwareTask` $\rightarrow$ `TouchGFX`)
* **Hàm thực thi:** `AppBackend_ConsumePa0Event()` & `AppBackend_ResetPa0Gesture()`
* **Cách hoạt động:**
  - Sử dụng cờ ngắt critical section `AppBackend_EnterCritical()` / `AppBackend_ExitCritical()` để tránh xung đột dữ liệu khi 2 task cùng đọc/ghi cờ nút nhấn.
  - `Screen2View` gọi hàm này để phát hiện người dùng vừa nhấn PA0 (Pause trận đấu hoặc thoát ra Home) và reset lại trạng thái cờ.

#### C. Kênh truyền Lệnh Rung Motor (`TouchGFX` $\rightarrow$ `HardwareTask`)
* **Hàm thực thi:** `AppBackend_SendHaptic(uint8_t command)`
* **Cách hoạt động:**
  - Khi bóng chạm paddle, `Screen2View` gọi `model.vibratePlayer1()` $\rightarrow$ gọi `AppBackend_SendHaptic(APP_HAPTIC_PLAYER_1)`.
  - Hàm sử dụng `osMessageQueuePut(hapticQueueHandle, &command, 0U, 0U)` để nạp lệnh vào Queue của FreeRTOS.
  - **Cơ chế chống treo (Non-blocking):** Nếu hàng đợi Queue bị đầy (full), hàm tự động gọi `osMessageQueueGet` để hủy lệnh cũ và nạp lệnh mới vào. Nhờ đó, `GUI_Task` không bao giờ bị đơ/chờ đợi phần cứng.

---

## TỔNG KẾT LUỒNG DỮ LIỆU TOÀN HỆ THỐNG

```text
[ Biến trở PA5/PC3 ]  --->  HardwareTask (Đọc ADC 10ms & Lọc EMA)
                                    |
                                    v (Biến 32-bit atomic)
                          AppBackend_GetLatestInput()
                                    |
                                    v
                              Model::tick()
                                    |
                                    v
                        Screen2View::ball_timertick() (60 FPS)
                                    |
                                    +---> GameEngine::update() (Tính vật lý/va chạm)
                                    |           |
                                    |           +---> (Có va chạm paddle)
                                    v           |
                          Cập nhật Widget LCD   v
                          (p1, p2, circle1)   AppBackend_SendHaptic()
                                                |
                                                v (FreeRTOS hapticQueue)
                                            HardwareTask (Bật PC8 / PC11 trong 80ms)
                                                |
                                                v
                                        [ Motor rung thật ]
```
