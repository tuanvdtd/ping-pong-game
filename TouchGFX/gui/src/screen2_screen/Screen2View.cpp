/*
 * Screen2View.cpp - màn hình chơi game.
 *
 * Vai trò: lớp ĐIỀU PHỐI giữa GameEngine (logic thuần, chỉ tính toán và
 * trả về cờ sự kiện) và thế giới bên ngoài (vẽ LCD, rung motor, nút bấm).
 * Engine không biết gì về widget/motor; mọi "hành động thật" do View
 * quyết định dựa trên cờ mà engine trả về.
 */
#include <gui/screen2_screen/Screen2View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

// Constructor: dựng engine (thành viên), gắn sẵn callback cho nút Continue.
// Chạy khi makeTransition dựng màn này (lúc chuyển từ Screen1 sang).
Screen2View::Screen2View()
    : gameEngine(),
      gameOverPending(false),
      continueButtonCallback(
          this,
          &Screen2View::continueButtonCallbackHandler)
{

}

// Chuẩn bị ván đấu mới - framework gọi 1 lần ngay sau khi dựng màn.
void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();

    // Đưa engine về trạng thái đầu ván (READY, bóng giữa sân, 3 mạng mỗi bên)
    // theo đúng chế độ chơi đã chọn ở Screen1 (đọc lại từ Model).
    gameEngine.reset(presenter->getGameMode());
    gameOverPending = false;

    // Tạo 6 ô vuông hiển thị MẠNG SỐNG bằng code (không qua Designer):
    // - P1: 3 ô màu cyan ở góc dưới (cạnh sân của P1), mỗi ô 10x10, cách 14px
    // - P2: 3 ô màu hồng ở góc trên (cạnh sân của P2)
    // add(...) gắn widget vào cây hiển thị của màn hình.
    for (int i = 0; i < 3; i++)
    {
        p1LifeBox[i].setPosition(24 + (i * 14), 300, 10, 10);
        p1LifeBox[i].setColor(touchgfx::Color::getColorFromRGB(0, 240, 255));
        p1LifeBox[i].setVisible(true);
        add(p1LifeBox[i]);

        p2LifeBox[i].setPosition(170 + (i * 14), 46, 10, 10);
        p2LifeBox[i].setColor(touchgfx::Color::getColorFromRGB(255, 60, 180));
        p2LifeBox[i].setVisible(true);
        add(p2LifeBox[i]);
    }

    // Vứt sự kiện nút PA0 tồn đọng (cú nhấn từ màn trước không được lọt vào ván này)
    presenter->resetPa0Gesture();

    // Kích hoạt callback cho nút Continue (đăng ký thủ công, không qua Designer)
    continueButton.setAction(continueButtonCallback);

    setPauseButtonState(false);      // hiện nút Pause (ẩn Continue)
    setResultPopupVisible(false);    // ẩn popup kết quả
    syncWidgets();                   // vẽ vị trí ban đầu (paddle giữa, bóng giữa sân)
}

// Dọn dẹp khi rời màn (về Screen1) - framework tự gọi dù rời bằng đường nào.
void Screen2View::tearDownScreen()
{
    presenter->stopAllHaptics();     // tắt motor - không để kẹt rung
    presenter->resetPa0Gesture();    // nút PA0 không mang sự kiện cũ sang màn khác
    Screen2ViewBase::tearDownScreen();
}

// ================== GAME LOOP - chạy MỖI KHUNG HÌNH (~60Hz) ==================
// Nguồn gọi: interaction "RedBall_Moving" (TriggerOnEveryTick) trong Designer
// -> Screen2ViewBase::handleTickEvent() (code tự sinh) -> hàm này.
// Mỗi tick làm 6 việc theo thứ tự: đọc nút -> update engine -> rung -> vẽ -> game over.
void Screen2View::ball_timertick()
{
    // (1) Đọc sự kiện nút PA0 (consume-once: mỗi cú nhấn chỉ nhận 1 lần)
    const Pa0ButtonEvent pa0Event = presenter->consumePa0ButtonEvent();

    // Nhấn ĐÔI -> về màn chọn chế độ, kết thúc tick ngay (return)
    if (pa0Event == Pa0ButtonEvent::DOUBLE_PRESS)
    {
        goHomeSafely();
        return;
    }

    // Nhấn ĐƠN -> tạm dừng / tiếp tục
    if (pa0Event == Pa0ButtonEvent::SINGLE_PRESS)
    {
        togglePause();
    }

    // (2) Chạy MỘT BƯỚC game: engine nhận input 2 biến trở (0..1000),
    // tính paddle/bóng/va chạm/CPU/mất mạng, trả về 1 byte CỜ sự kiện.
    const uint8_t events =
        gameEngine.update(presenter->getPlayer1Input(),
                          presenter->getPlayer2Input());

    // (3)(4) Engine báo "vừa đỡ bóng" -> rung motor người tương ứng 80ms.
    // Kiểm tra từng bit cờ bằng phép AND.
    if ((events & GameEngine::EVENT_HIT_PLAYER_1) != 0U)
    {
        presenter->vibratePlayer1();
    }

    if ((events & GameEngine::EVENT_HIT_PLAYER_2) != 0U)
    {
        presenter->vibratePlayer2();
    }

    // (5) Đồng bộ hình ảnh: chép tọa độ + số mạng từ engine lên widget
    syncWidgets();

    // (6) Engine báo "trận kết thúc" (ai đó hết 3 mạng) -> hiện popup.
    // Cờ gameOverPending đảm bảo popup chỉ hiện MỘT lần, không lặp mỗi tick.
    if (((events & GameEngine::EVENT_GAME_OVER) != 0U) &&
        !gameOverPending)
    {
        onGameOver();
    }
}

// Tạm dừng / tiếp tục trận đấu.
// 3 đường vào cùng đổ về đây: nút Pause cảm ứng (Designer),
// nút PA0 nhấn đơn (từ ball_timertick), nút Continue (callback).
void Screen2View::togglePause()
{
    const GameState previousState = gameEngine.getState();

    // Engine tự lật PLAYING <-> PAUSED (các trạng thái khác từ chối pause)
    gameEngine.togglePause();

    // Chỉ đổi ảnh nút khi trạng thái THẬT SỰ thay đổi
    // (đang READY/GAME_OVER thì engine không pause -> không đổi ảnh vô nghĩa)
    if (gameEngine.getState() != previousState)
    {
        setPauseButtonState(gameEngine.getState() == GameState::PAUSED);
    }

    // Vào PAUSED thì tắt motor - không để rung dở giữa lúc dừng
    if (gameEngine.getState() == GameState::PAUSED)
    {
        presenter->stopAllHaptics();
    }
}

// Về màn chọn chế độ AN TOÀN: phải tắt rung TRƯỚC rồi mới chuyển màn.
// Nếu chuyển màn trước, lệnh tắt không kịp gửi -> motor kẹt rung mãi.
void Screen2View::goHomeSafely()
{
    presenter->stopAllHaptics();
    application().gotoScreen1ScreenNoTransition();
}

// Nút "Chơi lại" trên popup kết quả: bắt đầu ván mới NGAY,
// giữ nguyên chế độ chơi (không bắt người chơi về menu chọn lại).
void Screen2View::playAgain()
{
    presenter->stopAllHaptics();
    gameEngine.reset(presenter->getGameMode());  // ván mới: 3 mạng, bóng giữa sân
    gameOverPending = false;
    setPauseButtonState(false);                  // nút về trạng thái Pause
    setResultPopupVisible(false);                // ẩn popup
    syncWidgets();                               // kéo widget về vị trí đầu ván ngay
}

// Getter tiện ích - hiện chưa nơi nào dùng, để dành mở rộng/debug.
bool Screen2View::isGameOverPending() const
{
    return gameOverPending;
}

GameWinner Screen2View::getWinner() const
{
    return gameEngine.getWinner();
}

// ============ Đồng bộ "SỐ -> HÌNH" - cầu nối engine với màn hình ============
// Engine chỉ có các con số trong RAM; hàm này chép chúng sang widget để
// TouchGFX vẽ. Nguyên tắc chung: SO SÁNH trước, chỉ cập nhật cái THAY ĐỔI
// -> TouchGFX chỉ vẽ lại đúng vùng đó (partial rendering, tiết kiệm CPU).
void Screen2View::syncWidgets()
{
    // Lấy 4 tọa độ từ engine (getBallX/Y đã đổi fixed-point Q8.8 về pixel)
    const int16_t bottomX = gameEngine.getBottomPaddleX();
    const int16_t topX = gameEngine.getTopPaddleX();
    const int16_t ballX = gameEngine.getBallX();
    const int16_t ballY = gameEngine.getBallY();

    // Paddle chỉ trượt NGANG -> giữ nguyên Y, chỉ moveTo khi X đổi
    if (p1.getX() != bottomX)
    {
        p1.moveTo(bottomX, p1.getY());
    }

    if (p2.getX() != topX)
    {
        p2.moveTo(topX, p2.getY());
    }

    // Bóng bay tự do -> kiểm tra cả 2 trục
    if ((circle1.getX() != ballX) || (circle1.getY() != ballY))
    {
        circle1.moveTo(ballX, ballY);
    }

    // ----- Hiển thị MẠNG SỐNG: ô thứ i hiện khi i < số mạng còn lại -----
    // lives=3 -> hiện cả 3 ô; lives=1 -> chỉ ô 0; lives=0 -> tắt hết.
    // Mất mạng = ô tắt dần từ phải sang, như "thanh máu".
    const uint8_t p1Lives = gameEngine.getPlayer1Lives();
    const uint8_t p2Lives = gameEngine.getPlayer2Lives();

    for (int i = 0; i < 3; i++)
    {
        bool p1Vis = (i < p1Lives);
        // Chỉ đổi khi trạng thái hiện/ẩn thật sự thay đổi (tránh vẽ thừa).
        // invalidate TRƯỚC + SAU setVisible: TouchGFX bỏ qua yêu cầu vẽ lại
        // từ widget đã ẩn, nên phải đánh dấu vùng lúc còn hiện.
        if (p1LifeBox[i].isVisible() != p1Vis)
        {
            p1LifeBox[i].invalidate();
            p1LifeBox[i].setVisible(p1Vis);
            p1LifeBox[i].invalidate();
        }

        bool p2Vis = (i < p2Lives);
        if (p2LifeBox[i].isVisible() != p2Vis)
        {
            p2LifeBox[i].invalidate();
            p2LifeBox[i].setVisible(p2Vis);
            p2LifeBox[i].invalidate();
        }
    }
}

// Hoán đổi cặp ảnh Pause <-> Continue (hai ảnh chung một ô 30x30:
// đang chơi hiện Pause, đang dừng hiện Continue).
void Screen2View::setPauseButtonState(bool paused)
{
    /*
     * Invalidate the currently visible image before hiding it, then
     * invalidate the newly visible image so the shared 30x30 area repaints.
     */
    pause.invalidate();
    continueButton.invalidate();

    pause.setVisible(!paused);
    continueButton.setVisible(paused);

    pause.invalidate();
    continueButton.invalidate();
}

// Handler nút Continue (đăng ký thủ công qua setAction trong setupScreen).
// Kiểm tra đúng nguồn phát rồi dùng chung logic togglePause.
void Screen2View::continueButtonCallbackHandler(
    const touchgfx::AbstractButtonContainer& source)
{
    if (&source == &continueButton)
    {
        togglePause();
    }
}

// Ẩn/hiện 4 widget của popup kết quả:
// box1 (nền), over (chữ "Game Over"), textResult1 (tên người thắng),
// flexButton1 (nút "Chơi lại").
void Screen2View::setResultPopupVisible(bool visible)
{
    if (!visible)
    {
        /*
         * Invalidate while the widgets are still visible. TouchGFX ignores
         * invalidation requests from an already-hidden drawable.
         */
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
        box1.invalidate();
        over.invalidate();
        textResult1.invalidate();
        flexButton1.invalidate();
    }
}

// Xử lý kết thúc trận (ai đó hết 3 mạng) - gọi từ ball_timertick khi có cờ
// EVENT_GAME_OVER. gameOverPending chốt lại để chỉ chạy đúng MỘT lần.
void Screen2View::onGameOver()
{
    const touchgfx::Unicode::UnicodeChar* winnerText;

    gameOverPending = true;
    presenter->stopAllHaptics();

    // Chọn chữ hiển thị người thắng theo winner + chế độ chơi:
    // - paddle DƯỚI thắng  -> "Player 1"
    // - paddle TRÊN thắng  -> "CPU" nếu đấu máy, "Player 2" nếu 2 người
    if (gameEngine.getWinner() == GameWinner::PLAYER_1)
    {
        winnerText = touchgfx::TypedText(T_RESULT_PLAYER_1).getText();
    }
    else if (gameEngine.getMode() == GameMode::VS_CPU)
    {
        winnerText = touchgfx::TypedText(T_RESULT_CPU).getText();
    }
    else
    {
        winnerText = touchgfx::TypedText(T_RESULT_PLAYER_2).getText();
    }

    // Đổ chữ vào ô text (wildcard) rồi hiện popup
    textResult1.setWildcard(winnerText);
    textResult1.resizeToCurrentText();
    setResultPopupVisible(true);
}
