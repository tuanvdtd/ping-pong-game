/*
 * Screen2Presenter.cpp - lớp TRUNG GIAN giữa Screen2View và Model.
 *
 * Toàn bộ là hàm "chuyển tiếp" (pass-through): View cần gì thì hỏi
 * Presenter, Presenter hỏi tiếp Model. Tách lớp như vậy để View
 * (đồ họa TouchGFX) và Model (dữ liệu + phần cứng) không dính nhau -
 * mỗi bên thay đổi độc lập, đúng kiến trúc MVP.
 * Con trỏ `model` có được nhờ kế thừa ModelListener (framework bind sẵn).
 */
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{

}

// Framework gọi khi màn này được kích hoạt. Chưa cần khởi tạo gì.
void Screen2Presenter::activate()
{

}

// Framework gọi khi màn này bị thay thế. Không có gì phải dọn.
void Screen2Presenter::deactivate()
{

}

// Screen2 hỏi chế độ chơi (đã chọn ở Screen1, lưu trong Model)
// để reset engine đúng mode khi vào ván.
GameMode Screen2Presenter::getGameMode() const
{
    return model->getGameMode();
}

// Input biến trở 0..1000 - game loop hỏi mỗi khung hình.
uint16_t Screen2Presenter::getPlayer1Input() const
{
    return model->getPlayer1Input();
}

uint16_t Screen2Presenter::getPlayer2Input() const
{
    return model->getPlayer2Input();
}

// Sự kiện nút PA0 (consume-once) - game loop hỏi mỗi khung hình.
Pa0ButtonEvent Screen2Presenter::consumePa0ButtonEvent()
{
    return model->consumePa0ButtonEvent();
}

// Xóa sự kiện nút tồn đọng (khi vào/rời màn chơi).
void Screen2Presenter::resetPa0Gesture()
{
    model->resetPa0Gesture();
}

// Lệnh rung motor khi đỡ bóng (đi tiếp xuống hàng đợi hapticQueue).
void Screen2Presenter::vibratePlayer1()
{
    model->vibratePlayer1();
}

void Screen2Presenter::vibratePlayer2()
{
    model->vibratePlayer2();
}

// Tắt ngay cả hai motor (pause / game over / rời màn).
void Screen2Presenter::stopAllHaptics()
{
    model->stopAllHaptics();
}
