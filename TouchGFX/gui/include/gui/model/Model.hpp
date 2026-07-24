#ifndef MODEL_HPP
#define MODEL_HPP

#include <gui/game/GameTypes.hpp>
#include <stdint.h>

class ModelListener;

/*
 * Model - tầng DỮ LIỆU trong kiến trúc MVP (Model - View - Presenter).
 *
 * - Cả ứng dụng chỉ có MỘT đối tượng Model duy nhất (là thành viên của
 *   FrontendHeap), sống suốt vòng đời chương trình - KHÔNG bị hủy khi
 *   chuyển màn hình. Vì vậy dữ liệu cần "sống sót" qua chuyển màn
 *   (như chế độ chơi) phải lưu ở đây.
 * - Là "cửa khẩu" DUY NHẤT phía C++ gọi xuống phần cứng (qua app_backend.h):
 *   đọc input biến trở, đọc sự kiện nút PA0, gửi lệnh rung motor.
 * - View/Presenter KHÔNG bao giờ gọi thẳng AppBackend_* mà phải qua Model.
 */
class Model
{
public:
    Model();

    // Framework gọi khi chuyển màn hình: nối Presenter mới vào Model,
    // để Model có kênh báo ngược lên Presenter khi cần (hiện chưa dùng).
    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    // Được TouchGFX gọi tự động MỖI KHUNG HÌNH (~60Hz), luôn chạy TRƯỚC
    // tick của màn hình: kéo giá trị biến trở mới nhất từ HardwareTask
    // vào player1Input/player2Input để màn hình dùng ngay trong khung đó.
    void tick();

    // Lưu / đọc chế độ chơi (VS_CPU = 1 người đấu máy, TWO_PLAYERS = 2 người).
    // Screen1 ghi vào lúc chọn chế độ; Screen2 đọc ra khi bắt đầu ván.
    void setGameMode(GameMode mode);
    GameMode getGameMode() const;

    // Input biến trở đã chuẩn hóa về thang 0..1000
    // (HardwareTask đọc ADC, lọc nhiễu, chuẩn hóa rồi công bố).
    uint16_t getPlayer1Input() const;
    uint16_t getPlayer2Input() const;

    // Lấy sự kiện nút PA0 (nhấn đơn / nhấn đôi) theo kiểu "consume-once":
    // mỗi cú nhấn chỉ được trả về đúng MỘT lần, các lần gọi sau trả NONE.
    Pa0ButtonEvent consumePa0ButtonEvent();

    // Xóa sự kiện nút còn tồn đọng (gọi khi vào/rời màn chơi
    // để cú nhấn cũ không "lọt" sang màn mới).
    void resetPa0Gesture();

    // Gửi lệnh rung motor xuống HardwareTask (qua hàng đợi hapticQueue).
    // Motor tự tắt sau 80ms; stopAllHaptics tắt ngay cả hai motor.
    void vibratePlayer1();
    void vibratePlayer2();
    void stopAllHaptics();

protected:
    // Con trỏ tới Presenter đang hoạt động (kênh Model -> Presenter).
    ModelListener* modelListener;

private:
    GameMode currentGameMode;   // chế độ chơi hiện tại (VS_CPU / TWO_PLAYERS)
    uint16_t player1Input;      // input biến trở người chơi 1, thang 0..1000
    uint16_t player2Input;      // input biến trở người chơi 2, thang 0..1000
};

#endif // MODEL_HPP
