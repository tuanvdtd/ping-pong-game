#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

/*
 * Screen1View - màn hình CHỌN CHẾ ĐỘ CHƠI (menu đầu tiên khi khởi động).
 *
 * Kế thừa Screen1ViewBase (code TouchGFX Designer tự sinh lúc build,
 * chứa các widget: nền, 2 nút chọn chế độ oneButton/twoButton).
 * Các hàm `virtual` bên dưới override hàm virtual của Base - được
 * các "interaction" cấu hình trong Designer gọi khi có sự kiện chạm.
 */
class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}

    // Framework gọi ngay SAU khi màn được dựng (lúc chuyển màn tới đây).
    virtual void setupScreen();

    // Framework gọi ngay TRƯỚC khi màn bị hủy (rời sang Screen2).
    virtual void tearDownScreen();

    // Interaction "OnePlayer_Selected": chạm nút oneButton (chơi với máy).
    virtual void selectVsCpu();

    // Interaction "TwoPlayer_Selected": chạm nút twoButton (2 người chơi).
    virtual void selectTwoPlayers();
protected:
};

#endif // SCREEN1VIEW_HPP
