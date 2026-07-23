/*
 * Screen1View.cpp - màn hình chọn chế độ chơi.
 * Nhiệm vụ duy nhất: nhận cú chạm chọn chế độ, nhờ Presenter lưu mode
 * vào Model, rồi yêu cầu chuyển sang màn chơi (Screen2).
 */
#include <gui/screen1_screen/Screen1View.hpp>

Screen1View::Screen1View()
{

}

// Gọi khi vào màn. Không có việc riêng - widget (nền, nút) do Base tự lo.
void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
}

// Gọi khi rời màn. Không giữ tài nguyên gì nên cũng không phải dọn dẹp.
void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

// Người chơi chạm nút "1 người" (đấu với máy):
// 1) lưu mode VS_CPU vào Model (qua Presenter - đúng bậc thang MVP);
// 2) xin chuyển sang màn chơi. Mode phải lưu ở Model vì View này
//    sắp bị hủy khi chuyển màn.
void Screen1View::selectVsCpu()
{
    presenter->selectVsCpu();
    application().gotoScreen2ScreenNoTransition();
}

// Người chơi chạm nút "2 người": như trên nhưng mode TWO_PLAYERS.
void Screen1View::selectTwoPlayers()
{
    presenter->selectTwoPlayers();
    application().gotoScreen2ScreenNoTransition();
}
