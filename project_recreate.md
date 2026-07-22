# HƯỚNG DẪN TÁI TẠO DỰ ÁN GAME PING PONG (STM32F429I-DISCO & TOUCHGFX)

Tài liệu này tổng hợp **quy trình 5 bước chuẩn mực** để xây dựng dự án Game Ping Pong từ con số 0, giúp nhóm phát triển hiểu rõ kiến trúc phần mềm, sự phân tách luồng trong FreeRTOS và mô hình thiết kế MVP (Model-View-Presenter).

---

## 📐 TỔNG QUAN KIẾN TRÚC HỆ THỐNG

Dự án được chia làm 3 tầng hoạt động độc lập và song song:
1. **Tầng Phần cứng (Hardware - Code C):** Đọc ADC biến trở, lọc nhiễu, điều khiển động cơ rung Haptic.
2. **Tầng Giao diện (TouchGFX - Code C++):** Quản lý đồ họa, nút bấm, khung hình 60 FPS.
3. **Tầng Bộ não (Game Engine - Code C++):** Thuật toán tính toán vị trí bóng, va chạm, điểm số, AI CPU.

---

## 🚀 BƯỚC 1: KHỞI TẠO NỀN TẢNG PHẦN CỨNG & FREERTOS (STM32CubeMX)

### 1.1 Cấu hình ngoại vi chip
- **System Clock:** Cấu hình HCLK = 180 MHz.
- **Display & RAM:** Bật LTDC, DMA2D (cho màn hình LCD 240x320) và FMC (điều khiển SDRAM ngoài).
- **ADC1:** Cấu hình 2 kênh đọc Analog: `PA5` (ADC1_IN5 cho Player 1) và `PC3` (ADC1_IN13 cho Player 2).
- **GPIO Output:** Cấu hình chân `PC8` (Motor Player 1) và `PC11` (Motor Player 2/CPU) ở trạng thái mặc định LOW.
- **GPIO Input:** Chân `PA0` (Nút bấm USER button).

### 1.2 Cấu hình FreeRTOS Tasks & Queue
Trong STM32CubeMX, cấu hình các thành phần hệ điều hành:
- **`HardwareTask`:** Chu kỳ 10ms, stack 2048 bytes, độ ưu tiên `osPriorityBelowNormal`.
- **`GUI_Task`:** Chu kỳ 16.6ms (VSYNC 60 FPS), stack 32768 bytes, độ ưu tiên `osPriorityNormal`.
- **`hapticQueue`:** Hàng đợi chứa 4 phần tử `uint8_t` dùng để gửi lệnh rung từ GUI xuống Hardware.

📍 **Code khởi tạo (`Core/Src/main.c`):**
```c
/* Khai báo Task và Queue */
osThreadId_t HardwareTaskHandle;
osMessageQueueId_t hapticQueueHandle;

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TouchGFX_Init();

    /* Tạo Hàng đợi và Task */
    hapticQueueHandle = osMessageQueueNew(4, sizeof(uint8_t), NULL);
    HardwareTaskHandle = osThreadNew(StartHardwareTask, NULL, &HardwareTask_attributes);

    /* Khởi động FreeRTOS Kernel */
    osKernelStart();
}
```

---

## 🎨 BƯỚC 2: THIẾT KẾ UI & SINH CODE TỰ ĐỘNG (TouchGFX Designer)

### 2.1 Thiết kế Màn hình
Mở file `.touchgfx` trên TouchGFX Designer và tạo 2 màn hình:
- **`Screen1` (Menu):** Ảnh nền `bgpingpong1.png`, Nút `oneButton` (1 Player), Nút `twoButton` (2 Players).
- **`Screen2` (Gameplay):** 
  - Ảnh nền `bgpingpong2.png`, Sân `san2.png`.
  - Widget ảnh: Thanh đỡ dưới `p1`, Thanh đỡ trên `p2`.
  - Widget tròn: Quả bóng `circle1` (bán kính 6px, màu đỏ/trắng).
  - Nút bấm: Nút `pause`, Nút `home`, Nút `continueButton`, Popup kết quả `box1`.

### 2.2 Đặt Interaction (Tương tác)
- `oneButton` Click ➜ Change Screen sang `Screen2`.
- `pause` Click ➜ Call virtual function: `togglePause()`.
- `home` Click ➜ Call virtual function: `goHomeSafely()`.

### 2.3 Generate Code
Bấm **Generate Code** (F4). TouchGFX Designer sẽ tự động sinh lớp Base `Screen2ViewBase.cpp`.

📍 **Code tự động sinh (`TouchGFX/generated/gui_generated/src/screen2_screen/Screen2ViewBase.cpp`):**
```cpp
void Screen2ViewBase::flexButtonCallbackHandler(const touchgfx::AbstractButtonContainer& src) {
    if (&src == &pause) {
        togglePause(); // Gọi hàm ảo togglePause
    }
    if (&src == &home) {
        goHomeSafely(); // Gọi hàm ảo goHomeSafely
    }
}
```
*(Lưu ý: Không sửa file trong thư mục `generated/` bằng tay).*

---

## 🧠 BƯỚC 3: XÂY DỰNG BỘ NÃO GAME ENGINE (Code C++)

Bộ não game được viết dạng **Header-Only** thuần C++, không phụ thuộc ngoại vi phần cứng để dễ dàng test trên PC.

📍 **File:** `TouchGFX/gui/include/gui/game/GameTypes.hpp` & `GameEngine.hpp`

### 3.1 Định nghĩa Enum loại dữ liệu (`GameTypes.hpp`)
```cpp
enum class GameMode : uint8_t { VS_CPU, TWO_PLAYERS };
enum class GameState : uint8_t { READY, PLAYING, PAUSED, GAME_OVER };
enum class GameWinner : uint8_t { NONE, PLAYER_1, PLAYER_2, CPU };
```

### 3.2 Thuật toán di chuyển & va chạm (`GameEngine.hpp`)
- **Số thực cố định Q8.8:** Nhân vận tốc/tọa độ với 256 để tính toán bằng số nguyên siêu nhanh.
- **Hàm `update()`:**
```cpp
uint8_t GameEngine::update(uint16_t player1Input, uint16_t player2Input) {
    if (state != GameState::PLAYING) return EVENT_NONE;

    // 1. Di chuyển bóng
    ballXQ8 += ballVelocityXQ8;
    ballYQ8 += ballVelocityYQ8;

    // 2. Va chạm tường trái/phải -> Đổi hướng X
    handleSideWalls();

    // 3. Va chạm với Paddle Player 1 -> Nảy bóng lên & phát sự kiện rung
    if (circleTouchesPaddle(bottomPaddleX, PADDLE_BOTTOM_Y)) {
        ballVelocityYQ8 = -abs(ballVelocityYQ8);
        events |= EVENT_HIT_PLAYER_1;
    }

    // 4. Sudden-Death: Bóng lọt qua biên ngang -> Game Over
    if (getBallY() > FIELD_MAX_Y) {
        winner = (mode == GameMode::VS_CPU) ? GameWinner::CPU : GameWinner::PLAYER_2;
        state = GameState::GAME_OVER;
        events |= EVENT_GAME_OVER;
    }

    return events;
}
```

---

## 🔗 BƯỚC 4: KẾT NỐI UI VỚI GAME ENGINE (`Screen2View.cpp`)

Kết nối Lớp giao diện (Bước 2) với Lớp bộ não (Bước 3).

📍 **File:** `TouchGFX/gui/src/screen2_screen/Screen2View.cpp`

### 4.1 Khởi tạo & Game Loop (60 FPS)
```cpp
void Screen2View::setupScreen() {
    Screen2ViewBase::setupScreen();
    gameEngine.reset(presenter->getGameMode()); // Reset bóng về giữa sân
}

/* Được TouchGFX gọi mỗi khung hình 16.6ms */
void Screen2View::ball_timertick() {
    // 1. Cập nhật vị trí biến trở vào Engine
    uint8_t events = gameEngine.update(presenter->getPlayer1Input(), 
                                       presenter->getPlayer2Input());

    // 2. Nếu chạm Paddle P1 -> Yêu cầu Presenter gửi lệnh rung
    if (events & GameEngine::EVENT_HIT_PLAYER_1) {
        presenter->vibratePlayer1();
    }

    // 3. Đồng bộ tọa độ bóng/paddle mới nhất lên hình ảnh LCD
    syncWidgets();
}

void Screen2View::syncWidgets() {
    p1.moveTo(gameEngine.getBottomPaddleX(), p1.getY());
    p2.moveTo(gameEngine.getTopPaddleX(), p2.getY());
    circle1.moveTo(gameEngine.getBallX(), gameEngine.getBallY());
}
```

### 4.2 Cài đặt chức năng cho các hàm nút bấm ảo
```cpp
void Screen2View::togglePause() {
    gameEngine.togglePause(); // Đóng băng/cho chạy tiếp physics
}

void Screen2View::goHomeSafely() {
    presenter->stopAllHaptics();
    application().gotoScreen1ScreenNoTransition(); // Về Screen 1
}
```

---

## 🔌 BƯỚC 5: NỐI PHẦN CỨNG VỚI HỆ THỐNG (`Model` & `main.c`)

Thiết lập luồng giao tiếp 2 chiều giữa Phần cứng và Giao diện.

📍 **Files:** `Core/Src/main.c`, `Core/Inc/app_backend.h`, `TouchGFX/gui/src/model/Model.cpp`

### 5.1 Chiều UP (Biến trở ADC ➜ Màn hình UI)
1. **`HardwareTask` trong `main.c`:** Đọc 2 kênh ADC, lọc mượt EMA, đóng gói 32-bit:
```c
ReadSliderInputs(&player1Raw, &player2Raw);
filteredPlayer1 += (((int32_t)player1Raw << 4) - filteredPlayer1) / 4;
latestInputMessage = ((uint32_t)player2 << 16) | (uint32_t)player1;
```
2. **`Model::tick()` trong `Model.cpp`:** Đọc biến 32-bit ở mỗi khung hình:
```cpp
void Model::tick() {
    AppBackend_GetLatestInput(&player1Input, &player2Input);
}
```

### 5.2 Chiều DOWN (Lệnh rung từ UI ➜ Motor Rung)
1. `Screen2View` gọi `presenter->vibratePlayer1()`.
2. `Model` gọi `AppBackend_SendHaptic(APP_HAPTIC_PLAYER_1)`.
3. `AppBackend_SendHaptic()` nạp con số 1 vào `hapticQueue`:
```c
osMessageQueuePut(hapticQueueHandle, &command, 0U, 0U);
```
4. **`HardwareTask` trong `main.c`** nhặt lệnh từ Queue và kích GPIO:
```c
if (osMessageQueueGet(hapticQueueHandle, &hapticCommand, NULL, 0U) == osOK) {
    if (hapticCommand == APP_HAPTIC_PLAYER_1) {
        HAL_GPIO_WritePin(MOTOR_PLAYER_GPIO_Port, MOTOR_PLAYER_Pin, GPIO_PIN_SET);
        playerDeadline = now + 80U; // Hẹn giờ tắt sau 80ms
    }
}

/* Tự động ngắt điện motor sau 80ms */
if (now - playerDeadline >= 0) {
    HAL_GPIO_WritePin(MOTOR_PLAYER_GPIO_Port, MOTOR_PLAYER_Pin, GPIO_PIN_RESET);
}
```

---

## 💡 ĐỀ XUẤT CẢI TIẾN NÂNG CẤP (CHO ĐỒ ÁN ĐẠT ĐIỂM TỐI ĐA)

1. **Nút nhấn PA0:** Chuyển từ Polling sang **Ngắt ngoài (EXTI0 Interrupt)**.
   - Dùng `HAL_GPIO_EXTI_Callback()` để nhận diện sự kiện bấm nút tức thì.
   - Gửi sự kiện vào FreeRTOS bằng hàm `osMessageQueuePutFromISR()`.
2. **Biến trở ADC:** Chuyển từ Polling sang **ADC + DMA + Timer Trigger**.
   - Cấu hình `TIM2` tự kích hoạt ADC1 mỗi 10ms.
   - Dùng DMA chuyển dữ liệu tự động vào mảng RAM `adc_buffer[2]` (0% CPU usage).
3. **Motor Rung:** Dùng Timer ở chế độ **One-Pulse Mode 80ms** để tự động dập chân GPIO về LOW mà không cần vòng lặp đếm deadline.
