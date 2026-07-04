# Thiết kế chế độ hai người chơi

## Trạng thái

- Ngày xác nhận: 2026-07-04
- Phương án: Hoàn thiện nhánh `TWO_PLAYERS` trong `GameEngine` hiện có
- Understanding Lock: Đã xác nhận
- Thiết kế: Đã xác nhận

## Understanding Summary

- Xây dựng chế độ Pong hai người chơi cục bộ trên STM32F429I-DISCO.
- Player 1 điều khiển paddle dưới bằng ADC1/PA5.
- Player 2 điều khiển paddle trên bằng ADC1/PC3.
- Cả hai input cùng quy ước: giá trị tăng thì paddle đi sang phải.
- Khi chọn `TWO_PLAYERS`, CPU không được điều khiển paddle trên.
- Bên nào để bóng lọt qua trước sẽ thua ngay; không có điểm số.
- Paddle đỡ bóng sẽ kích hoạt motor của chính người chơi đó trong 80 ms.
- Giữ nguyên Pause, Home, Play Again, popup kết quả và mode VS CPU.

## Giả định và yêu cầu phi chức năng

- Hai biến trở và hai motor đã được đấu đúng theo cấu hình hiện tại.
- Game chỉ phục vụ hai người chơi cục bộ trên một board; không có mạng và không lưu dữ liệu.
- HardwareTask đọc hai ADC theo chu kỳ khoảng 10 ms.
- GUI không được block bởi ADC hoặc haptic.
- Snapshot của hai input được publish đồng nhất trong một word 32-bit.
- Không thêm task, queue, screen hoặc lớp engine mới.
- Pause, Home và Game Over phải tắt motor an toàn.
- Firmware cần chạy ổn định tối thiểu 30 phút, không rò heap hoặc tràn stack.

## Phương án được chọn

Giữ một `GameEngine` dùng chung cho cả `VS_CPU` và `TWO_PLAYERS`.

Mỗi tick:

1. Screen 2 lấy `player1Input` và `player2Input` mới nhất từ Model.
2. Player 1 luôn điều khiển paddle dưới.
3. Trong `TWO_PLAYERS`, Player 2 điều khiển paddle trên bằng cùng hàm ánh xạ với Player 1.
4. Trong `VS_CPU`, paddle trên tiếp tục do `updateCpu()` điều khiển.
5. Hai mode dùng chung chuyển động bóng, collision, haptic, luật thua và popup.

Phương án này có phạm vi thay đổi nhỏ nhất, không nhân đôi physics và giảm nguy cơ làm hỏng mode VS CPU đang hoạt động.

## Luồng dữ liệu

```text
PA5 / PC3
    -> HardwareTask đọc và lọc ADC
    -> chuẩn hóa từng input về 0..1000
    -> đóng gói P1/P2 trong một word 32-bit
    -> Model đọc snapshot mới nhất
    -> Screen2 chuyển cả hai input vào GameEngine
    -> GameEngine cập nhật paddle và physics
    -> Event va chạm/kết thúc được chuyển về haptic và popup
```

## Luật gameplay

- Paddle dưới là Player 1.
- Paddle trên là Player 2 trong `TWO_PLAYERS`.
- Cả hai paddle dùng direct mapping: ADC tăng thì X tăng.
- Bóng vượt hoàn toàn cạnh trên: Player 2 thua, Player 1 thắng.
- Bóng vượt hoàn toàn cạnh dưới: Player 1 thua, Player 2 thắng.
- Không có điểm số hoặc nhiều round; một lần lọt bóng kết thúc trận.
- Paddle Player 1 chạm bóng phát `EVENT_HIT_PLAYER_1`.
- Paddle Player 2 chạm bóng phát `EVENT_HIT_PLAYER_2`.
- Mỗi event chỉ kích hoạt motor tương ứng trong 80 ms.

## Trạng thái và điều hướng

- `READY`: reset vị trí và chờ 30 tick; paddle vẫn nhận input.
- `PLAYING`: cập nhật input, physics, collision và haptic.
- `PAUSED`: giữ nguyên trạng thái động và tắt motor.
- `GAME_OVER`: dừng physics/input, tắt motor và hiện popup.
- `Play Again`: reset toàn bộ trận nhưng giữ `TWO_PLAYERS`.
- `Home`: tắt motor trước khi quay về màn hình chọn mode.
- Khi quay lại `VS_CPU`, Player 2 không còn điều khiển paddle trên.

## Biên lỗi

- Clamp input lớn hơn 1000 trước khi ánh xạ sang tọa độ.
- Nếu đọc ADC lỗi, tiếp tục dùng giá trị đã lọc gần nhất.
- Không cho haptic queue block GUI.
- Khi nhận `STOP_ALL`, xóa lệnh haptic đang chờ và tắt cả hai motor.
- Chỉ xét collision với paddle mà bóng đang di chuyển tới.
- Sau collision, đẩy bóng ra ngoài hitbox để tránh phát event lặp.

## Kiểm thử và nghiệm thu

### Logic

- `TWO_PLAYERS` dùng player2Input và không gọi thuật toán CPU.
- Input 0, 500 và 1000 đưa mỗi paddle đến trái, giữa và phải.
- Hai input cùng tăng làm cả hai paddle đi sang phải.
- Bóng lọt cạnh trên tạo winner Player 1.
- Bóng lọt cạnh dưới tạo winner Player 2.
- Va paddle dưới/trên tạo đúng một haptic event tương ứng.
- Pause giữ nguyên vị trí; resume tiếp tục đúng vận tốc.
- Play Again giữ mode và reset winner/trạng thái.
- Chuyển lại VS CPU khôi phục điều khiển CPU.

### Tích hợp

- Build firmware thành công và tạo lại file `.hex`.
- Nút hai người chơi chuyển đúng sang Screen 2 với mode `TWO_PLAYERS`.
- Popup hiển thị `Player 1 Win` hoặc `Player 2 Win` đúng trường hợp.
- Trên board, xác nhận ADC của mỗi biến trở điều khiển đúng paddle.
- Trên board, xác nhận motor tương ứng rung `80 +/- 10 ms`.
- Chạy liên tục 30 phút không reset, rò heap, tràn stack hoặc giảm frame rate rõ rệt.

## Decision Log

| Quyết định | Phương án khác | Lý do |
|---|---|---|
| Direct mapping cho cả hai ADC | Đảo chiều Player 2 | Người dùng yêu cầu giá trị tăng đều đi sang phải |
| Sudden-death | Thắng khi đạt điểm mục tiêu | Giữ luật hiện tại và yêu cầu đã xác nhận |
| Motor rung theo paddle vừa đỡ | Rung motor đối phương hoặc cả hai | Phản hồi trực tiếp cho người vừa chạm bóng |
| Dùng chung `GameEngine` | Tách hàm, engine hoặc screen riêng | Tránh lặp physics và giảm rủi ro |
| Không thêm task/queue/screen | Tạo GameTask hoặc UI riêng | Hạ tầng hiện tại đã đủ |
| Giữ mode khi Play Again | Quay về màn hình chọn mode | Trận mới bắt đầu nhanh và nhất quán với mode VS CPU |
| Không thay đổi VS CPU | Dùng chung hai input ở mọi mode | Bảo toàn chức năng đang hoạt động |

