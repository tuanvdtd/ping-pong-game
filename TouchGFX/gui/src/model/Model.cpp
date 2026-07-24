/*
 * Model.cpp - tầng dữ liệu của MVP, đồng thời là CẦU NỐI C++ <-> C:
 * đây là file C++ DUY NHẤT gọi các hàm AppBackend_* (viết bằng C, main.c).
 *
 * Mọi đoạn chạm phần cứng đều bọc trong #ifndef SIMULATOR: khi build
 * bản chạy thử trên PC (simulator không có ADC/nút/motor) các hàm này
 * rỗng, dùng giá trị mặc định - nhờ đó giao diện vẫn chạy được trên PC.
 */
#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifndef SIMULATOR
#include "app_backend.h"   /* ban hop dong 4 ham cau noi voi main.c (C) */
#endif

// Constructor: đặt giá trị mặc định an toàn trước khi có dữ liệu thật.
// Input = 500 (giữa thang 0..1000) để paddle đứng giữa sân
// nếu GUI chạy trước khi HardwareTask kịp đọc ADC lần đầu.
Model::Model()
    : modelListener(0),
      currentGameMode(GameMode::VS_CPU),
      player1Input(500U),
      player2Input(500U)
{

}

// Chạy MỖI KHUNG HÌNH, trước tick của màn hình.
// Đọc "ảnh chụp" input mới nhất (latest-wins) mà HardwareTask
// công bố mỗi 10ms qua biến chia sẻ 32-bit (ghi/đọc atomic).
void Model::tick()
{
#ifndef SIMULATOR
    uint16_t newestPlayer1;
    uint16_t newestPlayer2;

    // Đọc word 32-bit chung rồi tách thành 2 giá trị 16-bit (P1 | P2)
    if (AppBackend_GetLatestInput(&newestPlayer1, &newestPlayer2) != 0U)
    {
        player1Input = newestPlayer1;
        player2Input = newestPlayer2;
    }
#endif
}

// Screen1 gọi (qua Screen1Presenter) khi người chơi chạm nút chọn chế độ.
// Lưu ở Model vì Screen1 sắp bị hủy khi chuyển màn - chỉ Model sống sót.
void Model::setGameMode(GameMode mode)
{
    currentGameMode = mode;
}

// Screen2 gọi khi vào màn chơi: gameEngine.reset(getGameMode()).
GameMode Model::getGameMode() const
{
    return currentGameMode;
}

// Trả input đã được tick() làm tươi - game loop gọi mỗi khung hình.
uint16_t Model::getPlayer1Input() const
{
    return player1Input;
}

uint16_t Model::getPlayer2Input() const
{
    return player2Input;
}

// Lấy sự kiện nút PA0. Vai trò "phiên dịch": đổi mã C (APP_PA0_EVENT_*)
// sang enum C++ (Pa0ButtonEvent) để tầng GUI không phải biết mã C.
// Cơ chế consume-once nằm dưới AppBackend (so số thứ tự với bản đã đọc)
// nên mỗi cú nhấn chỉ được xử lý đúng một lần.
Pa0ButtonEvent Model::consumePa0ButtonEvent()
{
#ifndef SIMULATOR
    const uint8_t event = AppBackend_ConsumePa0Event();

    if (event == APP_PA0_EVENT_SINGLE_PRESS)
    {
        return Pa0ButtonEvent::SINGLE_PRESS;
    }

    if (event == APP_PA0_EVENT_DOUBLE_PRESS)
    {
        return Pa0ButtonEvent::DOUBLE_PRESS;
    }
#else
    /* The simulator has no on-board PA0 USER button. */
#endif

    return Pa0ButtonEvent::NONE;
}

// Xóa sự kiện nút tồn đọng + reset máy trạng thái debounce/gesture bên C.
// Screen2 gọi lúc setupScreen/tearDownScreen.
void Model::resetPa0Gesture()
{
#ifndef SIMULATOR
    AppBackend_ResetPa0Gesture();
#endif
}

// Ba hàm rung: đẩy 1 byte lệnh vào hàng đợi hapticQueue (không chặn).
// HardwareTask rút lệnh ra, bật motor tương ứng và tự tắt sau 80ms.
void Model::vibratePlayer1()
{
#ifndef SIMULATOR
    AppBackend_SendHaptic(APP_HAPTIC_PLAYER_1);   /* motor nguoi choi 1 (PC8) */
#endif
}

void Model::vibratePlayer2()
{
#ifndef SIMULATOR
    AppBackend_SendHaptic(APP_HAPTIC_PLAYER_2);   /* motor nguoi choi 2 / CPU (PC11) */
#endif
}

// Tắt ngay cả hai motor (lệnh STOP_ALL còn xóa sạch hàng đợi trước khi gửi).
// Được gọi ở MỌI "lối ra": pause, game over, chơi lại, về home, rời màn -
// phòng thủ để motor không bao giờ kẹt ở trạng thái rung.
void Model::stopAllHaptics()
{
#ifndef SIMULATOR
    AppBackend_SendHaptic(APP_HAPTIC_STOP_ALL);
#endif
}
