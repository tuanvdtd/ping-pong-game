# TÀI LIỆU PHÂN TÍCH VÀ PHẢN BIỆN CHUYÊN SÂU DỰ ÁN PING PONG STM32 (FreeRTOS + TouchGFX)

> **Mục đích:** Tài liệu tổng hợp dùng cho việc ôn tập và trả lời phản biện hội đồng bảo vệ đồ án. Phân tích chi tiết từng Task, từng file code, thuật toán và cơ chế hoạt động phần cứng/phần mềm.

---

## 📌 MỤC LỤC
- [Phần 0: Bối cảnh tổng quan (TouchGFX, FreeRTOS & Kiến trúc MVP)](#phần-0-bối-cảnh-tổng-quan)
- [Phần 1: Phân loại File (Tự viết vs Sinh tự động)](#phần-1-phân-loại-file)
- [Phần 2: Kiến trúc MVP trong TouchGFX (Task 2 - Nền tảng)](#phần-2-kiến-trúc-mvp-trong-touchgfx)
- [Phần 3: Game Engine - Math Fixed-Point Q8.8](#phần-3-game-engine---math-fixed-point-q88)
- [Phần 4: Thuật toán va chạm Hình tròn ↔ Hình chữ nhật](#phần-4-thuật-toán-va-chạm)
- [Phần 5: Tính góc bật linh hoạt `updateHorizontalVelocity()`](#phần-5-tính-góc-bật-linh-hoạt)
- [Phần 6: Thuật toán AI CPU `updateCpu()`](#phần-6-thuật-toán-ai-cpu)
- [Phần 7: State Machine trận đấu (READY, PLAYING, PAUSED, GAME_OVER)](#phần-7-state-machine-trận-đấu)
- [Phần 8: Vòng lặp chính `ball_timertick()`](#phần-8-vòng-lặp-chính-ball_timertick)
- [Phần 9: Cơ chế VSYNC Rendering & Invalidate Widget](#phần-9-cơ-chế-vsync-rendering--invalidate-widget)
- [Phần 10: IPC Cầu nối C ↔ C++ (`AppBackend` & `Model`)](#phần-10-ipc-cầu-nối-c--c)
- [Phụ lục A: Bảng Thông Số Kỹ Thuật Cheat Sheet](#phụ-lục-a-bảng-thông-số-kỹ-thuật-cheat-sheet)
- [Phụ lục B: Kịch Bản Thuyết Minh 5 Phút Bảo Vệ Đồ Án](#phụ-lục-b-kịch-bản-thuyết-minh-5-phút-bảo-vệ-đồ-án)

---

<a name="phần-0-bối-cảnh-tổng-quan"></a>
## 🟢 PHẦN 0: BỐI CẢNH TỔNG QUAN (TouchGFX, FreeRTOS & KIẾN TRÚC MVP)

### 1. TouchGFX là gì và tại sao chọn cho STM32F429?
* **TouchGFX** là framework đồ họa C++ của STMicroelectronics tối ưu riêng cho dòng vi điều khiển STM32.
* **Cơ chế Dirty Region (Redraw từng phần):** Thay vì vẽ lại toàn bộ 240x320 pixel mỗi khung hình, TouchGFX chỉ render lại đúng vùng có thay đổi (ví dụ: vị trí quả bóng hoặc vị trí thanh trượt). Điều này giảm tải CPU xuống mức cực thấp (< 15%).
* **Tích hợp phần cứng STM32F429:** Màn hình LCD được điều khiển bởi bộ **LTDC** (LCD-TFT Display Controller) phối hợp cùng bộ tăng tốc đồ họa **DMA2D (Chrom-ART Accelerator)** để tự động copy dữ liệu ảnh từ RAM ra LCD mà không tốn cycle CPU.

### 2. GUI_Task chạy thế nào trong hệ điều hành thời gian thực FreeRTOS?
Hệ thống sử dụng **2 Task FreeRTOS chính**:
1. **`HardwareTask` (Task 1 - C Code, 100Hz / 10ms):** Quản lý phần cứng mức thấp (ADC biến trở qua Dual ADC + DMA, debounce nút bấm PA0, phát lệnh rung haptic motor).
2. **`GUI_Task` (Task 2 - C++ Code, 60Hz / VSYNC):** Quản lý giao diện và logic vật lý game TouchGFX.

**Cơ chế đồng bộ 60 FPS theo VSYNC:**
* Màn hình LCD phát tín hiệu ngắt phần cứng **VSYNC** (Vertical Sync) khi quét xong 1 khung hình.
* Hàm ngắt VSYNC đánh thức một **FreeRTOS Semaphore**.
* `GUI_Task` nằm ở trạng thái Chờ (`OSWrappers::waitForVSync()`). Khi nhận được Semaphore, `GUI_Task` thức dậy, thực thi 1 tick logic game (`Screen2View::ball_timertick()`), cập nhật giao diện rồi quay về trạng thái `Blocked` chờ khung hình tiếp theo.

### 3. Mô hình kiến trúc MVP (Model - View - Presenter)
TouchGFX áp dụng mô hình thiết kế **MVP** để tách bạch dữ liệu và giao diện:

```text
[ Hardware Task 1 ] 
         │ (AppBackend IPC)
         ▼
    +---------+       Cập nhật dữ liệu      +-----------+
    │  Model  │ ──────────────────────────> │ Presenter │
    +---------+                             +-----------+
         ▲                                        │ Truyền dữ liệu
         │ Lấy Input / Phát sự kiện               ▼
         +----------------------------------+  View     │
                                            +-----------+
```

* **Model (`Model.cpp/.hpp`):** Nơi lưu trữ dữ liệu tập trung. Đọc input tay cầm từ `AppBackend` (Task 1 C-code) và lưu trạng thái `GameMode` (VS_CPU hoặc TWO_PLAYERS).
* **View (`Screen2View.cpp/.hpp`):** Quản lý đối tượng đồ họa màn hình LCD (nút bấm, bóng, paddle). **View trực tiếp chứa đối tượng `GameEngine` để chạy logic vật lý game.**
* **Presenter (`Screen2Presenter.cpp/.hpp`):** Cầu nối trung gian giữa Model và View. View không bao giờ truy cập trực tiếp Model mà thông qua Presenter.

---

<a name="phần-1-phân-loại-file"></a>
## 🟢 PHẦN 1: PHÂN LOẠI FILE (TỰ VIẾT vs SINH TỰ ĐỘNG)

### 1. Nhóm 1: File TỰ VIẾT (Code Logic Cốt Lõi Dự Án)

| Đường dẫn File | Ngôn ngữ | Vai trò cốt lõi |
| :--- | :---: | :--- |
| `TouchGFX/gui/include/gui/game/GameEngine.hpp` | C++ | **Vật lý & Logic game**: Tọa độ Fixed-Point Q8.8, va chạm biên/paddle, góc bật, AI CPU, đếm 3 mạng. |
| `TouchGFX/gui/include/gui/game/GameTypes.hpp` | C++ | Định nghĩa Enum: `GameMode`, `GameState`, `GameWinner`, `Pa0ButtonEvent`. |
| `TouchGFX/gui/src/screen2_screen/Screen2View.cpp` | C++ | **Game Loop chính**: Chứa `ball_timertick()`, đồng bộ vị trí widget, popup Game Over, nút PA0. |
| `TouchGFX/gui/src/screen2_screen/Screen2Presenter.cpp` | C++ | Cầu nối lấy dữ liệu từ `Model` cung cấp cho `Screen2View`. |
| `TouchGFX/gui/src/model/Model.cpp` | C++ | Đọc dữ liệu IPC từ `AppBackend` (Task 1) đưa vào hệ thống TouchGFX. |
| `Core/Src/app_backend.c` & `app_backend.h` | C | **Cầu nối IPC**: Biến 32-bit `latestInputMessage` và FreeRTOS Queue `hapticQueueHandle`. |
| `Core/Src/pa0_gesture.c` & `pa0_gesture.h` | C | Lọc nhiễu (Debounce) và nhận diện nhấn đơn / nhấn kép cho nút USER PA0. |

### 2. Nhóm 2: File do TouchGFX Designer Sinh Ra (Generated UI)
Nằm trong `TouchGFX/gui/include/gui_generated/` và `TouchGFX/generated/`:
* **`Screen2ViewBase.cpp / .hpp`**: Tự động sinh ra khi kéo thả widget (tạo `circle1`, `p1`, `p2`, `pause`, popup).
* **`FrontendHeapBase.hpp`**: Cấp phát bộ nhớ tĩnh cho Screen & Presenter.
* **`BitmapDatabase.cpp`, `Texts.cpp`**: Chuyển đổi tài nguyên hình ảnh và font chữ sang mảng C++.
> ⚠️ **Không chỉnh sửa trực tiếp các file `Base` này** vì sẽ bị TouchGFX Designer ghi đè khi Generate Code.

### 3. Nhóm 3: File do STM32CubeMX & Code phần cứng sinh ra
Nằm trong `Core/Src/` và `TouchGFX/target/`:
* **`main.c`**: Khởi tạo RCC, GPIO, ADC, DMA, LTDC, FMC/SDRAM và `StartHardwareTask`.
* **`freertos.c`**: Khởi tạo FreeRTOS Task và Queue.
* **`TouchGFXHAL.cpp`**: Kết nối TouchGFX framework với driver phần cứng LTDC/DMA2D của STM32F429.

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 0 & 1

1. **Tại sao không gộp gộp logic game vào `main.c` mà phải tách ra `GUI_Task` và `HardwareTask`?**
   * *Trả lời:* Tách Task giúp `HardwareTask` đọc ADC và xử lý nút bấm phản hồi nhanh với tần số ổn định 100Hz (10ms), độc lập hoàn toàn với tốc độ quét màn hình 60Hz.

2. **Mô hình MVP giúp ích gì cho dự án này?**
   * *Trả lời:* Tách biệt giao diện (View) khỏi dữ liệu (Model). Khi thay đổi thiết kế trên TouchGFX Designer, logic vật lý (`GameEngine.hpp`) và logic đọc ngoại vi không bị ảnh hưởng.

3. **Làm sao `GUI_Task` biết khi nào màn hình sẵn sàng để vẽ khung hình mới?**
   * *Trả lời:* Nhờ ngắt VSYNC của LTDC. Mỗi khi quét xong 1 khung hình, ngắt VSYNC giải phóng Semaphore để `GUI_Task` thức dậy chạy 1 tick.

---

<a name="phần-2-kiến-trúc-mvp-trong-touchgfx"></a>
## 🟢 PHẦN 2: KIẾN TRÚC MVP TRONG TOUCHGFX (Task 2 - Nền Tảng)

### 1. Chi tiết 3 tầng Model - Presenter - View trong dự án Ping Pong

#### A. Tầng `Model` (`TouchGFX/gui/src/model/Model.cpp`)
* **Vai trò:** Là thành phần tồn tại suốt toàn bộ vòng đời ứng dụng (Singleton lưu trong `FrontendHeap`). Nó chủ động giao tiếp với backend C phần cứng.
* **Các phương thức chính:**
  * `Model::tick()`: Được TouchGFX framework tự động gọi mỗi khung hình (60Hz). Tại đây, nó gọi `AppBackend_GetLatestInput()` để cập nhật 2 biến `player1Input` và `player2Input`.
  * `vibratePlayer1()`, `vibratePlayer2()`, `stopAllHaptics()`: Chuyển các lệnh điều khiển motor xuống `AppBackend_SendHaptic()`.
  * `consumePa0ButtonEvent()`: Lấy sự kiện nút bấm PA0 từ C backend (Nhấn đơn / Nhấn kép).

#### B. Tầng `Presenter` (`TouchGFX/gui/src/screen2_screen/Screen2Presenter.cpp`)
* **Vai trò:** Là "kẻ môi giới" (Middleman). Mới mỗi màn hình (Screen), TouchGFX sẽ tạo ra một Presenter tương ứng (ví dụ: `Screen2Presenter`).
* **Tại sao cần Presenter?**
  * `View` chỉ có quyền truy cập vào `Presenter` của nó (`presenter->...`).
  * `Presenter` có con trỏ trỏ tới `Model` (`model->...`).
  * Khi `Screen2View` cần lấy dữ liệu input, nó gọi `presenter->getPlayer1Input()`. Presenter sẽ chuyển tiếp cuộc gọi tới `model->getPlayer1Input()`.
  * Điều này đảm bảo `View` độc lập hoàn toàn với `Model`, giúp dễ unit test View và Presenter riêng biệt.

#### C. Tầng `View` (`TouchGFX/gui/src/screen2_screen/Screen2View.cpp`)
* **Vai trò:** Quản lý giao diện người dùng và trực tiếp sở hữu đối tượng `GameEngine gameEngine`.
* **Luồng hoạt động chính:**
  1. Trong `setupScreen()`: Khởi tạo các thanh máu (3 ô mạng đại diện cho mỗi player), đăng ký callback cho nút bấm, reset `GameEngine` theo `GameMode` đọc từ Presenter.
  2. Trong `ball_timertick()`: Đọc input từ `presenter`, truyền vào `gameEngine.update()`, nhận event va chạm để phát lệnh rung qua `presenter->vibratePlayer1()`, và đồng bộ vị trí widget (`p1`, `p2`, `circle1`).

---

### 2. Sơ đồ luồng dữ liệu tương tác giữa các tầng (Data Flow Diagram)

```text
[ Biến trở phần cứng PA5/PC3 ]
               │
               ▼ (Đọc ADC 10ms - Task 1)
     [ AppBackend (C-code) ]
               │
               ▼ (AppBackend_GetLatestInput)
       [ Model::tick() ]  (Được gọi 60 FPS ngầm)
               │
               │ (Lưu trữ player1Input, player2Input)
               ▼
   [ Screen2View::ball_timertick() ]
               │
               ├──> 1. Gọi presenter->getPlayer1Input() ──> Model
               │
               ├──> 2. Đưa vào gameEngine.update(p1, p2)
               │
               ├──> 3. Nếu chạm paddle P1:
               │       Gọi presenter->vibratePlayer1() ──> Model ──> AppBackend_SendHaptic ──> FreeRTOS Queue ──> Motor PC8
               │
               └──> 4. Đồng bộ vị trí UI: p1.moveTo(), circle1.moveTo()
```

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 2

1. **Tại sao View không giữ con trỏ trỏ trực tiếp đến Model luôn mà phải qua Presenter?**
   * *Trả lời:* Thiết kế này tuân thủ nguyên lý **Separation of Concerns (Phân tách trách nhiệm)** và **Loosely Coupled (Liên kết lỏng lẻo)**. Nếu View truy cập trực tiếp Model, View sẽ bị phụ thuộc chặt vào cấu trúc của Model. Khi có nhiều Screen cùng dùng 1 Model, việc dùng Presenter giúp từng Screen chỉ lọc lấy đúng dữ liệu mà Screen đó cần.

2. **Ai tạo ra đối tượng Model, Presenter, View và bộ nhớ của chúng nằm ở đâu?**
   * *Trả lời:* Chúng được cấp phát tĩnh trong class `FrontendHeap` (`TouchGFX/gui/include/gui/common/FrontendHeap.hpp`) sử dụng cấu trúc `touchgfx::Partition`. Bộ nhớ này nằm ở vùng RAM/SDRAM của STM32F429 mà không dùng heap động (`malloc`/`new`), tránh nguy cơ phân mảnh bộ nhớ (Memory Fragmentation) trong hệ thống nhúng.

3. **`Model::tick()` chạy ở luồng nào (Thread)?**
   * *Trả lời:* `Model::tick()` chạy hoàn toàn trong ngữ cảnh của `GUI_Task` (Task 2), được TouchGFX framework gọi mỗi khi có tín hiệu ngắt VSYNC (60Hz).

---

<a name="phần-3-game-engine---math-fixed-point-q88"></a>
## 🟢 PHẦN 3: GAME ENGINE — KỸ THUẬT FIXED-POINT Q8.8 (Task 2 - Vật Lý Game)

### 1. Tại sao lại dùng Fixed-Point Q8.8 mà không dùng `float`?
* **Tại sao không dùng `float`?**
  * STM32F429 (ARM Cortex-M4F) có bộ tính toán số thực FPU (Single-Precision Floating-Point Unit). Tuy nhiên, phép chia và tính toán số thực `float` vẫn tốn nhiều chu kỳ CPU hơn phép tính số nguyên `int32_t`.
  * Khi quả bóng di chuyển với tốc độ lẻ (ví dụ 1.25 pixel / khung hình), nếu dùng số nguyên `int16_t` thông thường, phần thập phân `.25` sẽ bị làm tròn về `1` hoặc `0`. Việc cộng dồn sai số này khiến bóng bị "giật" (jitter) hoặc di chuyển sai hướng.
* **Giải pháp Fixed-Point Q8.8 là gì?**
  * Dùng 1 số nguyên `int32_t` để biểu diễn số thực bằng cách **dịch trái 8 bit** (nhân với $2^8 = 256$).
  * **8 bit thấp** lưu phần thập phân (Fractional part).
  * **Các bit cao** lưu phần nguyên (Integer part).
  * `FIXED_ONE = 256` tương đương với giá trị `1.0` thực tế.

---

### 2. Phân tích đoạn code thực tế trong `GameEngine.hpp`

```cpp
static const int16_t FIXED_ONE = 256; // 2^8 = 256 (Tương đương 1.0)

// Tọa độ Fixed-Point Q8.8 (Lưu trữ dưới dạng int32_t để tránh tràn số)
int32_t ballXQ8 = BALL_START_X * FIXED_ONE; // 114 * 256 = 29184
int32_t ballYQ8 = BALL_START_Y * FIXED_ONE; // 169 * 256 = 43264

// Vận tốc Fixed-Point Q8.8
int16_t ballVelocityXQ8 = 320;  // 320 / 256 = 1.25 pixel/frame
int16_t ballVelocityYQ8 = -512; // -512 / 256 = -2.0 pixel/frame

// Cập nhật vị trí bóng mỗi frame (Cộng số nguyên cực nhanh):
ballXQ8 += ballVelocityXQ8;
ballYQ8 += ballVelocityYQ8;

// Quy đổi từ Q8.8 ra Tọa độ Pixel màn hình (Chia 256 bằng phép chia/dịch):
int16_t getBallX() const { return (int16_t)(ballXQ8 / FIXED_ONE); }
int16_t getBallY() const { return (int16_t)(ballYQ8 / FIXED_ONE); }
```

#### Tính toán ví dụ thực tế:
* **Ban đầu (Frame 0):** `ballXQ8 = 29184` $\rightarrow$ `getBallX() = 29184 / 256 = 114` pixel.
* **Frame 1:** `ballXQ8 = 29184 + 320 = 29504` $\rightarrow$ `getBallX() = 29504 / 256 = 115` pixel (Phần dư 64 / 256 = 0.25).
* **Frame 2:** `ballXQ8 = 29504 + 320 = 29824` $\rightarrow$ `getBallX() = 29824 / 256 = 116` pixel (Phần dư 128 / 256 = 0.50).
* **Tốc độ ở 60 FPS:**
  * Vận tốc $Y = -512 / 256 = -2.0$ pixel/frame $\rightarrow$ $2.0 \times 60 = 120$ pixel/giây.
  * Vận tốc $X = 320 / 256 = 1.25$ pixel/frame $\rightarrow$ $1.25 \times 60 = 75$ pixel/giây.

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 3

1. **Tại sao em dùng `int32_t` cho biến tọa độ `ballXQ8` nhưng lại dùng `int16_t` cho vận tốc `ballVelocityXQ8`?**
   * *Trả lời:* Tọa độ `ballXQ8` bị tích lũy cộng dồn liên tục trong thời gian dài (ví dụ $240 \times 256 = 61440$, lớn hơn giới hạn `int16_t` max là 32767). Do đó phải dùng `int32_t` để tránh tràn số (Overflow). Trong khi đó, vận tốc tối đa chỉ khoảng $\pm 768$ (tương đương 3 pixel/frame) nên `int16_t` là hoàn toàn đủ bộ nhớ.

2. **Tại sao không dùng phép dịch bit `>> 8` thay cho phép chia `/ FIXED_ONE`?**
   * *Trả lời:* Trong C/C++, với số nguyên âm (như khi bóng đi ngược lên với tọa độ âm), phép chia `/ 256` sẽ làm tròn về `0` (Truncate towards zero), còn phép dịch bit `>> 8` sẽ làm tròn về âm vô cùng (Floor). Để đảm bảo tính chính xác nhất quán của tọa độ hình học, viết `/ FIXED_ONE` giúp trình biên dịch C++ (GCC ARM) tự động tối ưu hóa phép toán phù hợp.

---
<a name="phần-4-thuật-toán-va-chạm"></a>
## 🟢 PHẦN 4: THUẬT TOÁN VA CHẠM HÌNH TRÒN ↔ HÌNH CHỮ NHẬT (Task 2 - Hình Học)

### 1. Thuật toán `circleTouchesPaddle()` là gì?
Khi xét va chạm giữa Quả bóng (Hình tròn bán kính $R=6$) và Paddle (Hình chữ nhật $48 \times 11$), nếu chỉ kiểm tra điểm tâm quả bóng rơi vào chữ nhật (Box Collision đơn giản) thì sẽ bị **sai lệch ở 4 góc bo của quả bóng**.

Thuật toán chuẩn xác là **Find Closest Point on Rectangle (Tìm điểm trên hình chữ nhật gần tâm đường tròn nhất)**:

```text
       (Tâm bóng C)
           O  ── r = 6
          /
         / d (Khoảng cách)
        v
       +-----------------------+
       |   Paddle (Rectangle)   |
       +-----------------------+
```

1. **Bước 1:** Xác định tâm đường tròn $C(centerX, centerY)$.
2. **Bước 2:** Tìm điểm $P(closestX, closestY)$ nằm trên hoặc bên trong paddle gần tâm $C$ nhất bằng hàm `clamp()`:
   * $closestX = \text{clamp}(centerX, paddleX, paddleX + PADDLE\_WIDTH)$
   * $closestY = \text{clamp}(centerY, paddleY, paddleY + PADDLE\_HEIGHT)$
3. **Bước 3:** Tính khoảng cách giữa tâm $C$ và điểm $P$: $\Delta X = centerX - closestX$, $\Delta Y = centerY - closestY$.
4. **Bước 4:** Kiểm tra $\Delta X^2 + \Delta Y^2 \le R^2$.

---

### 2. Phân tích đoạn code thực tế trong `GameEngine.hpp`

```cpp
bool circleTouchesPaddle(int16_t paddleX, int16_t paddleY) const
{
    const int16_t centerX = (int16_t)(getBallX() + BALL_RADIUS); // R = 6
    const int16_t centerY = (int16_t)(getBallY() + BALL_RADIUS);

    // Kẹp tọa độ tâm bóng vào khoảng giới hạn của Paddle để tìm điểm gần nhất
    const int16_t closestX =
        clamp(centerX, paddleX, (int16_t)(paddleX + PADDLE_WIDTH));
    const int16_t closestY =
        clamp(centerY, paddleY, (int16_t)(paddleY + PADDLE_HEIGHT));

    const int32_t deltaX = (int32_t)centerX - closestX;
    const int32_t deltaY = (int32_t)centerY - closestY;

    // So sánh bình phương khoảng cách với bình phương bán kính (tránh dùng căn bậc 2 sqrt)
    return ((deltaX * deltaX) + (deltaY * deltaY)) <=
           (BALL_RADIUS * BALL_RADIUS);
}
```

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 4

1. **Tại sao em so sánh `(deltaX*deltaX + deltaY*deltaY) <= R*R` chứ không dùng `sqrt()` để tính khoảng cách thực?**
   * *Trả lời:* Hàm tính căn bậc hai `sqrt()` tốn rất nhiều chu kỳ xử lý của CPU. Bằng cách bình phương 2 vế của bất đẳng thức $d \le R \iff d^2 \le R^2$, ta chỉ cần dùng các phép nhân và cộng số nguyên cơ bản, giúp thuật toán chạy cực nhanh (chỉ 2 chu kỳ CPU).

2. **Tại sao trước khi gọi `circleTouchesPaddle()`, code phải kiểm tra `ballVelocityYQ8 > 0` hoặc `ballVelocityYQ8 < 0`?**
   * *Trả lời:* Để tránh lỗi va chạm kép (Double-Hit Glitch). Nếu bóng vừa nảy đổi chiều đi xuống (`ballVelocityYQ8 > 0`), mà ở frame tiếp theo bóng vẫn còn "chạm nhẹ" biên paddle trên, nếu không kiểm tra hướng bay thì bóng sẽ bị kẹt va chạm và đổi hướng liên tục tại paddle.

---

<a name="phần-5-tính-góc-bật-linh-hoạt"></a>
## 🟢 PHẦN 5: TÍNH GÓC BẬT LINH HOẠT `updateHorizontalVelocity()` (Task 2 - Gameplay)

### 1. Tại sao cần góc bật linh hoạt?
Trong game Ping Pong thực tế, nếu bóng va vào paddle mà góc bật không đổi (chỉ đơn thuần đổi dấu $Y$), trò chơi sẽ rất nhàm chán.
Để tăng tính hấp dẫn:
* Nếu bóng đập vào **chính giữa paddle** $\rightarrow$ Bóng bật thẳng.
* Nếu bóng đập vào **mép ngoài paddle** $\rightarrow$ Bóng bật chéo góc lớn hơn (tăng vận tốc ngang $X$).

---

### 2. Phân tích đoạn code thực tế trong `GameEngine.hpp`

```cpp
void updateHorizontalVelocity(int16_t paddleX)
{
    const int16_t paddleCenter = (int16_t)(paddleX + (PADDLE_WIDTH / 2)); // Paddle rộng 48 -> tâm ở +24
    const int16_t ballCenter = (int16_t)(getBallX() + BALL_RADIUS);       // Tâm bóng

    // Độ lệch giữa tâm bóng và tâm paddle
    int16_t velocity = (int16_t)((ballCenter - paddleCenter) * 32);

    // Giới hạn vận tốc X tối đa trong khoảng [-768, 768] Q8.8 (tương đương [-3.0, 3.0] pixel/frame)
    velocity = clamp(velocity, -768, 768);

    // Dead-zone: Nếu bóng chạm gần đúng tâm (lệch ít hơn 128 / 256 = 0.5 pixel),
    // duy trì vận tốc tối thiểu 0.5 pixel/frame theo hướng cũ để bóng không bị bay thẳng tắp đứng yên X.
    if ((velocity > -128) && (velocity < 128))
    {
        velocity = (ballVelocityXQ8 < 0) ? -128 : 128;
    }

    ballVelocityXQ8 = velocity; // Gán vận tốc X mới Q8.8
}
```

#### Bảng giá trị thử nghiệm góc bật:

| Vị trí va chạm bóng trên Paddle | `ballCenter - paddleCenter` | `velocity` tính toán (Q8.8) | Vận tốc $X$ thực tế (pixel/frame) |
| :--- | :---: | :---: | :---: |
| Chính tâm Paddle | `0` pixel | `0 * 32 = 0` $\rightarrow$ gán `128` | $\pm 0.5$ pixel/frame (Dead-zone) |
| Lệch phải 10 pixel | `+10` pixel | `10 * 32 = +320` | $+1.25$ pixel/frame (Bật sang phải) |
| Mép mép phải (+24 pixel) | `+24` pixel | `24 * 32 = +768` | $+3.0$ pixel/frame (Bật chéo gấp) |
| Mép mép trái (-24 pixel) | `-24` pixel | `-24 * 32 = -768` | $-3.0$ pixel/frame (Bật chéo gấp) |

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 5

1. **Hệ số `32` trong công thức `(ballCenter - paddleCenter) * 32` từ đâu ra?**
   * *Trả lời:* Bán kính lệch tối đa từ tâm ra mép paddle là $48 / 2 = 24$ pixel. Ta muốn vận tốc ngang $X$ tối đa ở mép là $3.0$ pixel/frame, tương đương $3.0 \times 256 = 768$ dạng Q8.8. Ta có phương trình: $24 \times K = 768 \Rightarrow K = 768 / 24 = 32$. Do đó hệ số $32$ được tính toán chính xác để ánh xạ tối đa độ lệch ra vận tốc cực đại.

2. **Khái niệm Dead-zone `[-128, 128]` có tác dụng gì đối với trải nghiệm chơi?**
   * *Trả lời:* Nếu không có dead-zone, khi bóng đập vào chính giữa paddle, vận tốc $X$ sẽ bằng $0$. Bóng sẽ nảy lên nảy xuống theo phương thẳng đứng vĩnh viễn (nhà mchán). Dead-zone đảm bảo bóng luôn có tốc độ di chuyển ngang tối thiểu là $\pm 0.5$ pixel/frame.

---
<a name="phần-6-thuật-toán-ai-cpu"></a>
## 🟢 PHẦN 6: THUẬT TOÁN AI CPU `updateCpu()` (Task 2 - Trí Tuệ Nhân Tạo Bot)

### 1. Ý tưởng thiết kế AI CPU vừa sức người chơi
Một bot CPU "hoàn hảo" có thể bám đuổi tọa độ bóng chính xác $100\%$ không bao giờ thua. Để tạo ra đối thủ thực tế có độ khó vừa phải:
1. **Trì hoãn phản xạ (Reaction Delay):** CPU chỉ tính toán lại vị trí mục tiêu (`cpuTargetX`) **mỗi 4 tick** (`CPU_REACTION_TICKS = 4U`, khoảng 66.6ms ở 60 FPS).
2. **Sai số ngẫu nhiên có kiểm soát (Error Offset):** Sử dụng mảng sai số $error \in \{-7, 5, -3, 8\}$ dựa trên bộ đếm số lượt đỡ bóng thành công (`rallyCount & 0x03U`).
3. **Giới hạn tốc độ di chuyển:** CPU di chuyển tối đa `CPU_SPEED = 1` pixel mỗi khung hình.
4. **Chiến thuật thu quân:** Khi bóng đang bay xuống phía Player 1 (`ballVelocityYQ8 > 0`), CPU dừng theo đuổi bóng và tự động di chuyển về vị trí giữa sân `PADDLE_CENTER_X` để thủ thế.

---

### 2. Phân tích đoạn code thực tế trong `GameEngine.hpp`

```cpp
static const uint8_t CPU_REACTION_TICKS = 4U;
static const int16_t CPU_SPEED = 1; // Tốc độ di chuyển 1 pixel/frame (60 pixel/giây)

void updateCpu()
{
    ++cpuReactionTicks;
    if (cpuReactionTicks >= CPU_REACTION_TICKS)
    {
        cpuReactionTicks = 0U;

        if (ballVelocityYQ8 < 0) // Chỉ bám theo bóng khi bóng đang bay LÊN phía CPU
        {
            int16_t error;
            switch (rallyCount & 0x03U) // Tạo sai số thay đổi theo từng lượt nảy
            {
            case 0U: error = -7; break;
            case 1U: error =  5; break;
            case 2U: error = -3; break;
            default: error =  8; break;
            }

            // Tính tọa độ mục tiêu mà mép paddle CPU cần di chuyển tới
            cpuTargetX = clamp(
                (int16_t)(getBallX() + BALL_RADIUS - (PADDLE_WIDTH / 2) + error),
                PADDLE_MIN_X,
                PADDLE_MAX_X);
        }
        else // Bóng đang bay XUỐNG phía Player 1 -> CPU lui về giữa sân
        {
            cpuTargetX = PADDLE_CENTER_X;
        }
    }

    // Từng frame, di chuyển nhích từng 1 pixel về hướng cpuTargetX
    if (topPaddleX < cpuTargetX)
    {
        topPaddleX = clamp((int16_t)(topPaddleX + CPU_SPEED), PADDLE_MIN_X, PADDLE_MAX_X);
    }
    else if (topPaddleX > cpuTargetX)
    {
        topPaddleX = clamp((int16_t)(topPaddleX - CPU_SPEED), PADDLE_MIN_X, PADDLE_MAX_X);
    }
}
```

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 6

1. **Tốc độ tối đa của AI CPU là bao nhiêu và tại sao chọn `CPU_SPEED = 1`?**
   * *Trả lời:* Tốc độ $1$ pixel/frame $\rightarrow 1 \times 60 = 60$ pixel/giây. Sân chơi rộng $184$ pixel, tức là CPU tốn khoảng $3$ giây để đi từ mép trái sang mép phải. Tốc độ này vừa đủ để CPU đón được các cú bóng nảy bình thường nhưng sẽ bị "đánh bại" nếu Player 1 tạo ra các cú trả bóng bật chéo gấp ở mép ngoài paddle ($X = 3.0$ pixel/frame).

2. **Tại sao lại dùng `rallyCount & 0x03U` để tính sai số `error`?**
   * *Trả lời:* Toán tử `& 0x03U` là phép chia lấy dư `% 4` dạng bitwise nhanh trên vi điều khiển. Nó giúp xoay vòng 4 giá trị sai số $\{-7, 5, -3, 8\}$ theo từng cú đánh, làm cho nét đánh của CPU không bị rập khuôn và tạo cơ hội cho Player 1 ghi điểm khi rally lên cao.

---

<a name="phần-7-state-machine-trận-đấu"></a>
## 🟢 PHẦN 7: STATE MACHINE TRẬN ĐẤU (Task 2 - Luồng Trận Đấu)

### 1. Sơ đồ chuyển trạng thái (State Transition Diagram)

Hệ thống điều khiển trận đấu thông qua 4 trạng thái (`GameState`):

```text
               +----------------------------------+
               |              READY               | (Khởi tạo positions, 3 mạng)
               +----------------------------------+
                                │
                                │ (Sau 30 ticks ~ 0.5s)
                                ▼
  Nút PA0 Nhấn Đơn  +----------------------------------+
 ┌─────────────────>│             PLAYING              │<─────────────────┐
 │                  +----------------------------------+                  │
 │                                │                                       │
 │ Nút PA0                        │ (Bóng vượt biên & mạng == 0)          │ Nút Continue /
 │ Nhấn Đơn                       ▼                                       │ Nút PA0 Nhấn Đơn
 │                  +----------------------------------+                  │
 └──────────────────│              PAUSED              │──────────────────┘
                    +----------------------------------+

                                  PLAYING
                                     │
                                     ▼ (Hết 3 mạng)
                    +----------------------------------+
                    |            GAME_OVER             |
                    +----------------------------------+
                                     │
                                     ▼ (Nhấn "Chơi lại")
                                   READY
```

---

### 2. Chi tiết luật Đấu 3 Mạng (First-to-3 / Best of 5)

* **Khởi tạo:** Mỗi bên bắt đầu trận đấu với **`3 mạng`** (`player1Lives = 3`, `player2Lives = 3`).
* **Trạng thái `READY` (Trì hoãn phát bóng):** Khi vừa bắt đầu hoặc sau mỗi bàn thua, bóng đứng yên tại trung tâm sân (`BALL_START_X = 114`, `BALL_START_Y = 169`) trong `30 tick` (0.5 giây) để người chơi kịp chuẩn bị tay cầm.
* **Mất mạng & Respawn bóng:**
  * Nếu bóng vượt biên dưới (`ballY > FIELD_BOTTOM`): `Player 1` bị trừ 1 mạng (`--player1Lives`). Nếu vẫn còn mạng, gọi `respawnBall(-1)` (bóng phát ngược lên phía trên).
  * Nếu bóng vượt biên trên (`ballY + BALL_SIZE < FIELD_TOP`): `Player 2/CPU` bị trừ 1 mạng (`--player2Lives`). Nếu vẫn còn mạng, gọi `respawnBall(1)` (bóng phát xuống phía dưới).
* **Kết thúc game (`GAME_OVER`):** Khi 1 trong 2 bên bị trừ về `0` mạng, `state = GameState::GAME_OVER`, xác định `winner` và phát sự kiện `EVENT_GAME_OVER` hiển thị popup kết quả.

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 7

1. **Tại sao cần có trạng thái `READY` với khoảng chờ 30 tick mà không bắt đầu `PLAYING` ngay?**
   * *Trả lời:* Trạng thái `READY` 0.5 giây giúp người chơi có thời gian phản xạ định vị lại thanh trượt sau khi ghi/mất điểm, tránh việc bóng vừa respawn đã lao đi với tốc độ cao khiến người chơi bị ngợp.

2. **Khi ở trạng thái `PAUSED` hoặc `GAME_OVER`, hàm `gameEngine.update()` xử lý thế nào?**
   * *Trả lời:* Hàm ngay lập tức kiểm tra cờ ở đầu hàm `if (state == GameState::PAUSED || state == GameState::GAME_OVER) return EVENT_NONE;` để bỏ qua toàn bộ phép tính di chuyển vật lý và kiểm tra va chạm, giúp đóng đóng băng hoàn toàn màn hình game.

---
<a name="phần-8-vòng-lặp-chính-ball_timertick"></a>
## 🟢 PHẦN 8: VÒNG LẶP CHÍNH `Screen2View::ball_timertick()` (Task 2 - Trái Tim GUI Task)

### 1. Vai trò của `ball_timertick()`
`ball_timertick()` là hàm chạy mỗi tick TouchGFX (60 lần/giây), đóng vai trò **Game Loop chính**. Thứ tự xử lý trong hàm này cực kỳ nghiêm ngặt để đảm bảo game không bị trễ input và không nảy sinh lỗi hiển thị.

---

### 2. Phân tích 5 bước thực thi trong `ball_timertick()`

```cpp
void Screen2View::ball_timertick()
{
    // BƯỚC 1: Đọc và ưu tiên xử lý nút bấm vật lý PA0 trước tiên
    const Pa0ButtonEvent pa0Event = presenter->consumePa0ButtonEvent();

    if (pa0Event == Pa0ButtonEvent::DOUBLE_PRESS)
    {
        goHomeSafely(); // Nhấn đúp: Tắt motor rung và thoát về Menu Screen1 ngay lập tức
        return;
    }

    if (pa0Event == Pa0ButtonEvent::SINGLE_PRESS)
    {
        togglePause();  // Nhấn đơn: Chuyển đổi trạng thái Tạm dừng / Tiếp tục
    }

    // BƯỚC 2: Cập nhật physics game engine từ Input mới nhất đọc từ Presenter
    const uint8_t events =
        gameEngine.update(presenter->getPlayer1Input(),
                          presenter->getPlayer2Input());

    // BƯỚC 3: Xử lý sự kiện va chạm để phát phản hồi rung haptic motor
    if ((events & GameEngine::EVENT_HIT_PLAYER_1) != 0U)
    {
        presenter->vibratePlayer1(); // Paddle dưới đỡ bóng -> Rung Motor PC8
    }

    if ((events & GameEngine::EVENT_HIT_PLAYER_2) != 0U)
    {
        presenter->vibratePlayer2(); // Paddle trên đỡ bóng -> Rung Motor PC11
    }

    // BƯỚC 4: Đồng bộ tọa độ vật lý lên widget giao diện TouchGFX (p1, p2, circle1, máu)
    syncWidgets();

    // BƯỚC 5: Xử lý khi trận đấu kết thúc
    if (((events & GameEngine::EVENT_GAME_OVER) != 0U) && !gameOverPending)
    {
        onGameOver(); // Tắt haptic, hiển thị popup chiến thắng
    }
}
```

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 8

1. **Tại sao việc check `DOUBLE_PRESS` lại đặt lên đầu tiên và có lệnh `return` ngay?**
   * *Trả lời:* Để đảo bảo tính phản hồi tức thì (Immediate Response). Nếu người chơi muốn thoát khẩn cấp về Home Menu, hệ thống hủy ngay các bước cập nhật vật lý phía dưới, giúp ứng dụng chuyển màn hình mượt mà không bị khựng giật.

2. **Cờ `gameOverPending` dùng để làm gì?**
   * *Trả lời:* Dùng để chống lặp sự kiện (Event Debounce trong GUI). Khi trận đấu vừa kết thúc ở frame hiện tại, `onGameOver()` được kích hoạt. Nếu không có cờ `gameOverPending = true`, ở các frame 60Hz tiếp theo, sự kiện `EVENT_GAME_OVER` tiếp tục duy trì sẽ làm popup bị reset và vẽ lại liên tục (gây nhấp nháy màn hình).

---

<a name="phần-9-cơ-chế-vsync-rendering--invalidate-widget"></a>
## 🟢 PHẦN 9: CƠ CHẾ VSYNC RENDERING & INVALIDATE WIDGET (Task 2 - Phần Cứng Màn Hình)

### 1. VSYNC và Double Buffering trong TouchGFX
* **VSYNC Interrupt:** Ngắt tần số quét dọc của màn hình LCD (60Hz). Khi ngắt xảy ra, driver phần cứng báo hiệu màn hình vừa vẽ xong 1 khung hình.
* **Double Buffering (Bộ đệm kép):** Ứng dụng dùng 2 vùng nhớ Framebuffer trong SDRAM:
  * **Front Buffer:** Vùng RAM đang được bộ LTDC đọc và hiển thị ra màn hình LCD.
  * **Back Buffer:** Vùng RAM để TouchGFX vẽ khung hình tiếp theo ngầm bên dưới.
  * Khi VSYNC xảy ra, LTDC tráo đổi con trỏ giữa Front và Back Buffer (Frame Swapping) để không bao giờ xảy ra hiện tượng **Tearing** (xé hình).

---

### 2. Kỹ thuật `invalidate()` và quy tắc Invalidate kép (Double Invalidate Pattern)

Trong TouchGFX, khi muốn redraw một widget, ta gọi `widget.invalidate()`. Dưới đây là đoạn code xử lý Popup kết quả trong `Screen2View.cpp`:

```cpp
void Screen2View::setResultPopupVisible(bool visible)
{
    if (!visible)
    {
        // QUY TẮC 1: Invalidate TRƯỚC KHI HIDE
        // TouchGFX bỏ qua yêu cầu invalidate của widget đã bị setVisible(false).
        // Do đó phải báo cho TouchGFX vẽ lại vùng ảnh nền TRƯỚC KHI ẩn popup.
        box1.invalidate();
        over.invalidate();
        textResult1.invalidate();
        flexButton1.invalidate();
    }

    box1.setVisible(visible);
    over.setVisible(visible);
    textResult1.setVisible(visible);
    flexButton1.setVisible(visible);

    if (visible)
    {
        // QUY TẮC 2: Invalidate SAU KHI SHOW
        // Báo cho TouchGFX vẽ popup mới hiện lên trên nền.
        box1.invalidate();
        over.invalidate();
        textResult1.invalidate();
        flexButton1.invalidate();
    }
}
```

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 9

1. **Nếu em quên gọi `invalidate()` trước khi gọi `setVisible(false)` thì điều gì xảy ra?**
   * *Trả lời:* Màn hình sẽ xuất hiện **Bóng ma (Ghosting/Artifacts)**. Popup tuy đã bị đánh dấu ẩn (`visible = false`), nhưng TouchGFX không hề biết vùng màn hình đó cần vẽ lại. Do đó hình ảnh của Popup vẫn sẽ "dính chặt" trên màn hình LCD cho tới khi bóng hoặc paddle di chuyển đè qua.

2. **Tối ưu hóa trong `syncWidgets()` giúp tiết kiệm băng thông thế nào?**
   * *Trả lời:* Trong `syncWidgets()`, code luôn kiểm tra `if (p1.getX() != bottomX) p1.moveTo(bottomX, p1.getY());`. Nếu vị trí thanh trượt không hề thay đổi so với frame trước, `moveTo()` sẽ không được gọi, nhờ đó TouchGFX không cần tốn thời gian redraw lại khu vực đó.

---

<a name="phần-10-ipc-cầu-nối-c--c"></a>
## 🟢 PHẦN 10: MODULE CẦU NỐI IPC C ↔ C++ (`AppBackend` & `Model`)

### 1. Tại sao cần Module IPC `AppBackend`?
Dự án có sự bất đồng bộ giữa 2 môi trường:
* **Task 1 (Code C - `HardwareTask`):** Đọc ngoại vi với chu kỳ **10ms (100Hz)**.
* **Task 2 (Code C++ - `GUI_Task`):** Render đồ họa với chu kỳ **16.6ms (60Hz)**.

`app_backend.c/.h` đóng vai trò làm lớp đệm IPC (Inter-Thread Communication) an toàn thread-safe giữa 2 Task.

---

### 2. Chi tiết 3 kênh truyền dữ liệu IPC

```text
  HARDWARE TASK 1 (C Code, 100Hz)                   GUI TASK 2 (C++ Code, 60Hz)
+---------------------------------+               +----------------------------+
| Đọc ADC 10ms & Lọc EMA          |               | Model::tick()              |
| 打包 32-bit:                    |  Atomic Read  | Đọc AppBackend_            |
| latestInputMessage              |──────────────>| GetLatestInput()           |
+---------------------------------+               +----------------------------+
| Pa0GestureDetector_Update()     |  Critical     | Model::consumePa0...()     |
| latestPa0EventMessage           |──────────────>| AppBackend_ConsumePa0...() |
+---------------------------------+  Section      +----------------------------+
| osMessageQueueGet()             |  FreeRTOS     | Model::vibratePlayer1()    |
| (Nhận lệnh bật PC8/PC11 80ms)   |<──────────────| AppBackend_SendHaptic()    |
+---------------------------------+   Queue       +----------------------------+
```

#### A. Kênh truyền Input Analog (Bóng / Paddle)
* **Kỹ thuật:** Sử dụng biến 32-bit atomic `latestInputMessage = (player2 << 16) | player1`.
* **Ưu điểm:** Đọc/Ghi 32-bit trên vi xử lý ARM Cortex-M4 là 1 chu kỳ lệnh đơn (`LDR`/`STR` atomic). Không bao giờ xảy ra tình trạng Player 1 nhận giá trị ở frame hiện tại còn Player 2 nhận giá trị ở frame cũ.

#### B. Kênh truyền Sự kiện Nút bấm PA0
* **Kỹ thuật:** Đọc/Xóa cờ sự kiện với bảo vệ cờ ngắt critical section `AppBackend_EnterCritical()` và `AppBackend_ExitCritical()` (tương đương `taskENTER_CRITICAL()`).

#### C. Kênh truyền Lệnh Rung Motor Haptic (Non-blocking Queue)
* **Kỹ thuật:** Gọi `osMessageQueuePut(hapticQueueHandle, &command, 0U, 0U)`.
* **Cơ chế chống treo (Non-blocking):** Nếu hàng đợi Queue bị đầy (Queue Full), hàm tự động gọi `osMessageQueueGet` để bỏ lệnh cũ và nạp lệnh mới vào. Điều này đảm bảo `GUI_Task` **không bao giờ bị đơ/chờ đợi phần cứng (Zero Block Time)**.

#### D. Hỗ trợ Biên Dịch Giả Lập (TouchGFX Simulator PC)
* Mọi lời gọi C backend trong `Model.cpp` đều được bọc bởi `#ifndef SIMULATOR`. Điều này cho phép nạp và test toàn bộ game trên máy tính Windows (dùng Visual Studio hoặc GCC) mà không cần nạp board thật.

---

## 🎯 CÂU HỎI PHẢN BIỆN PHẦN 10

1. **Tại sao kênh Input tay cầm không dùng FreeRTOS Queue mà lại dùng biến 32-bit atomic?**
   * *Trả lời:* Vì dữ liệu tay cầm trượt là dữ liệu liên tục (Continuous Data), GUI chỉ quan tâm đến **giá trị mới nhất (Latest Value)** tại thời điểm render. Nếu dùng Queue, dữ liệu cũ trễ chuỗi sẽ bị tích tụ làm tay cầm trượt bị độ trễ (Input Lag). Dùng biến atomic 32-bit giúp tốc độ truy xuất cực nhanh ($O(1)$) và trễ bằng $0$.

2. **Tác dụng của `#ifndef SIMULATOR` trong `Model.cpp` là gì?**
   * *Trả lời:* Giúp phân tách mã phụ thuộc phần cứng (Hardware Dependent Code). Khi build ứng dụng trên máy tính PC để test giao diện bằng TouchGFX Simulator, trình biên dịch sẽ bỏ qua các hàm C của STM32 HAL/FreeRTOS, tránh gây lỗi thiếu thư viện phần cứng.

---

<a name="phụ-lục-a-bảng-thông-số-kỹ-thuật-cheat-sheet"></a>
## 📌 PHỤ LỤC A: BẢNG THÔNG SỐ KỸ THUẬT TRA CỨU NHANH (DEFENSE CHEAT SHEET)

| Hạng mục kỹ thuật | Thông số chi tiết | Giải thích ngắn gọn cho Hội đồng |
| :--- | :--- | :--- |
| **Vi điều khiển chính** | STM32F429ZI (ARM Cortex-M4F @ 180MHz) | Có FPU, 2MB Flash, 256KB SRAM, tích hợp LTDC & Chrom-ART (DMA2D). |
| **Bộ nhớ ngoài** | IS42S16400J SDRAM (8MB) | Chứa 2 khung hình Framebuffer của TouchGFX (Bộ đệm kép Double Buffering). |
| **Màn hình hiển thị** | LCD-TFT 2.4 inch (240x320 pixel, RGB565) | Kết nối qua LTDC 16-bit, tần số quét ngắt VSYNC 60Hz. |
| **Hệ điều hành thời gian thực** | FreeRTOS (CMSIS-RTOS v2 API) | Gồm 2 Task chính: `HardwareTask` (100Hz) & `GUI_Task` (60Hz / VSYNC). |
| **Định dạng số toán học** | Fixed-Point Q8.8 ($2^8 = 256$) | Tọa độ & Vận tốc bóng dùng số nguyên `int32_t`/`int16_t` thay `float` để tăng tốc CPU. |
| **Phạm vi vận tốc bóng X** | $[-768, 768]$ Q8.8 | Tương đương $[-3.0, 3.0]$ pixel/frame ($\pm 180$ pixel/giây). |
| **Cơ chế đọc Input ADC** | PA5 (Paddle 1) & PC3 (Paddle 2) | Đọc qua ADC + DMA (100Hz) & lọc trung bình động lũy thừa (EMA). |
| **Cơ chế Rung Haptic** | PC8 (Motor P1) & PC11 (Motor P2) | Nhận lệnh qua FreeRTOS Queue non-blocking, thời gian rung xung 80ms. |
| **Nhận diện cử chỉ PA0** | Nút bấm USER PA0 (PA0_Gesture) | Debounce phần mềm: Nhấn đơn = Tạm dừng/Tiếp tục, Nhấn đúp = Thao tác thoát khẩn Home. |

---

<a name="phụ-lục-b-kịch-bản-thuyết-minh-5-phút-bảo-vệ-đồ-án"></a>
## 🎙️ PHỤ LỤC B: KỊCH BẢN THUYẾT MINH 5 PHÚT BẢO VỆ ĐỒ ÁN

### 1. Mở đầu (1 phút)
> *"Kính thưa Thầy/Cô trong Hội đồng phản biện. Em xin đại diện nhóm trình bày đồ án: **Thiết kế Game Ping Pong thời gian thực trên Vi điều khiển STM32F429I-DISCO tích hợp TouchGFX và FreeRTOS**.*
> *Mục tiêu đồ án của chúng em là tối ưu hóa hiệu năng hệ thống nhúng để đạt được trải nghiệm chơi game đồ họa 60 FPS mượt mà, phản hồi tay cầm độ trễ cực thấp (Zero Input Lag) và tích hợp phản hồi xúc giác (Haptic Feedback) sống động trên phần cứng có tài nguyên hạn chế."*

### 2. Kiến trúc Hệ thống & Tách luồng FreeRTOS (2 phút)
> *"Để giải quyết bài toán thời gian thực, hệ thống của chúng em được chia thành 2 luồng xử lý FreeRTOS chính:*
> 1. * **Hardware Task (C-code, 100Hz):** Quản lý phần cứng mức thấp, đọc tín hiệu biến trở qua ADC+DMA, lọc nhiễu tín hiệu và nhận diện cử chỉ nút bấm PA0.*
> 2. * **GUI Task (C++ code, 60Hz):** Quản lý giao diện đồ họa TouchGFX và chạy logic vật lý game. Task này được đồng bộ chuẩn xác 60 FPS theo tín hiệu ngắt phần cứng VSYNC của màn hình LCD.*
>
> *Giữa 2 Task, chúng em thiết kế module cầu nối IPC **AppBackend** sử dụng biến **32-bit Atomic** để truyền dữ liệu tay cầm trượt với độ trễ bằng 0, và **Non-blocking FreeRTOS Queue** để truyền lệnh rung haptic motor mà không làm treo luồng đồ họa.*
> *Đồng thời, mã nguồn TouchGFX tuân thủ nghiêm ngặt **Kiến trúc MVP (Model-View-Presenter)** giúp phân tách hoàn toàn dữ liệu Model và giao diện View."*

### 3. Thuật toán Cốt lõi & Vật lý Game (1.5 phút)
> *"Về phần thuật toán xử lý vật lý game, chúng em áp dụng 3 kỹ thuật nổi bật:*
> 1. * **Toán học Fixed-Point Q8.8:** Chuyển đổi toàn bộ tọa độ số thực về số nguyên dịch bit ($2^8 = 256$) giúp tính toán vận tốc lẻ cực nhanh mà không tốn cycle FPU.*
> 2. * **Thuật toán Va chạm Closest Point:** Tìm điểm gần nhất trên hình chữ nhật Paddle tới tâm quả bóng để xét va chạm chuẩn xác ở 4 góc bo, kết hợp so sánh bình phương khoảng cách để triệt tiêu hàm `sqrt()`.*
> 3. * **AI CPU vừa sức:** Thiết kế bot có độ trễ phản xạ 66.6ms kết hợp mảng sai số ngẫu nhiên có kiểm soát, tạo trải nghiệm thi đấu chân thực cho người chơi đơn."*

### 4. Kết luận (0.5 phút)
> *"Hệ thống đã chạy thực nghiệm ổn định trên board STM32F429, tải CPU duy trì dưới 15%, giao diện đáp ứng 60 FPS không trễ giật. Em xin chân thành cảm ơn Thầy/Cô đã lắng nghe và em sẵn sàng nhận câu hỏi phản biện từ Hội đồng!"*

---

## 🏁 TỔNG KẾT BỘ TÀI LIỆU BẢO VỆ ĐỒ ÁN PING PONG STM32
> 🎉 **Chúc mừng bạn đã hoàn thành bộ tài liệu phân tích 10 phần cùng Phụ lục Bảo vệ!** Hãy đọc kỹ các mục code thực tế và bộ câu hỏi phản biện ở từng phần để đạt điểm tối đa trong buổi bảo vệ đồ án!




