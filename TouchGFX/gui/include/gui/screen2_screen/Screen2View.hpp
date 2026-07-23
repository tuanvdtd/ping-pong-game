#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/game/GameEngine.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

/*
 * Screen2View - màn hình CHƠI GAME, lớp điều phối giữa GameEngine
 * (logic thuần) và "thế giới thật" (LCD, motor rung, nút bấm).
 *
 * Kế thừa Screen2ViewBase (TouchGFX Designer tự sinh lúc build, chứa
 * các widget: p1/p2 = 2 paddle, circle1 = bóng, pause/continueButton,
 * home, popup kết quả box1/over/textResult1/flexButton1).
 *
 * Các hàm `virtual` phía dưới được Base gọi theo interaction đã cấu hình
 * trong Designer (file Project-Nhung.touchgfx):
 *   - ball_timertick : interaction "RedBall_Moving" - MỖI TICK (~60Hz)
 *   - togglePause    : interaction "Pause_Clicked"  - chạm nút Pause
 *   - goHomeSafely   : interaction "Home_Clicked"   - chạm nút Home
 *   - playAgain      : interaction "playAgain"      - chạm nút Chơi lại
 */
class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}

    // Framework gọi sau khi dựng màn: chuẩn bị ván đấu mới (reset engine...).
    virtual void setupScreen();

    // Framework gọi trước khi hủy màn: dọn dẹp (tắt rung, reset nút PA0).
    virtual void tearDownScreen();

    // GAME LOOP - chạy mỗi khung hình: đọc nút PA0, update engine,
    // rung motor, vẽ lại, xử lý kết thúc ván.
    virtual void ball_timertick();

    // Tạm dừng/tiếp tục. Có 3 đường vào cùng đổ về đây:
    // nút Pause cảm ứng, nút PA0 nhấn đơn, nút Continue.
    virtual void togglePause();

    // Về màn chọn chế độ AN TOÀN (tắt rung trước rồi mới chuyển màn).
    // 2 đường vào: nút Home cảm ứng, nút PA0 nhấn đôi.
    virtual void goHomeSafely();

    // Nút "Chơi lại" trên popup: ván mới, GIỮ nguyên chế độ chơi.
    virtual void playAgain();

    // Getter tiện ích (hiện chưa nơi nào dùng - để dành mở rộng).
    bool isGameOverPending() const;
    GameWinner getWinner() const;

protected:
    // Đồng bộ "số -> hình": chép tọa độ paddle/bóng và số mạng
    // từ engine sang các widget trên màn (chỉ vẽ lại cái thay đổi).
    void syncWidgets();

    // Hoán đổi cặp ảnh Pause/Continue (chung một ô 30x30).
    void setPauseButtonState(bool paused);

    // Ẩn/hiện 4 widget của popup kết quả (nền, chữ, tên người thắng, nút).
    void setResultPopupVisible(bool visible);

    // Xử lý kết thúc ván: chọn chữ người thắng + hiện popup (chỉ 1 lần).
    void onGameOver();

    // Handler khi chạm nút Continue (đăng ký thủ công qua setAction).
    void continueButtonCallbackHandler(
        const touchgfx::AbstractButtonContainer& source);

    GameEngine gameEngine;      // engine luật chơi - View SỞ HỮU engine
    bool gameOverPending;       // chốt "đã xử lý game over" - chống lặp popup
    touchgfx::Box p1LifeBox[3]; // 3 ô vuông hiển thị mạng của P1 (góc dưới)
    touchgfx::Box p2LifeBox[3]; // 3 ô vuông hiển thị mạng của P2 (góc trên)
    touchgfx::Callback<Screen2View,
                       const touchgfx::AbstractButtonContainer&>
        continueButtonCallback; // đối tượng nối nút Continue với handler
};

#endif // SCREEN2VIEW_HPP
