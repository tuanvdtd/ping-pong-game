/**
 * @file trajectory_calculator.hpp
 * @brief Mô phỏng và Tính toán Góc bay nâng cao cho Quả bóng (Ball Trajectory & Physics Engine)
 * @author Đỗ Sơn Tùng (MSSV: 20225425) - Nhóm Anti
 * @note Thư viện thử nghiệm tính toán vector góc nảy và dự đoán quỹ đạo theo số thực cố định (Fixed-Point Q8.8).
 */

#ifndef TRAJECTORY_CALCULATOR_HPP
#define TRAJECTORY_CALCULATOR_HPP

#include <stdint.h>
#include <stdbool.h>

namespace AntiPhysics
{

/**
 * @brief Cấu trúc biểu diễn vị trí và vận tốc trong hệ tọa độ 2D (Fixed-Point Q8.8)
 */
struct Vector2D_Q8
{
    int32_t x; ///< Hoành độ X (mã hóa Q8.8, nhân 256)
    int32_t y; ///< Tung độ Y (mã hóa Q8.8, nhân 256)
};

/**
 * @brief Cấu trúc tham số va chạm giữa Quả bóng và Thanh đỡ (Paddle)
 */
struct ImpactParameters
{
    int16_t ballCenterX;    ///< Tâm bóng X (Pixel)
    int16_t ballCenterY;    ///< Tâm bóng Y (Pixel)
    int16_t paddleCenterX;  ///< Tâm Paddle X (Pixel)
    int16_t paddleWidth;    ///< Chiều rộng Paddle (Pixel)
    int16_t incomingVxQ8;   ///< Vận tốc X trước va chạm (Q8.8)
    int16_t incomingVyQ8;   ///< Vận tốc Y trước va chạm (Q8.8)
};

/**
 * @brief Kết quả tính toán góc nảy và vectơ vận tốc phản hồi
 */
struct ReflectionResult
{
    int16_t outgoingVxQ8;   ///< Vận tốc X mới sau khi bật (Q8.8)
    int16_t outgoingVyQ8;   ///< Vận tốc Y mới sau khi bật (Q8.8)
    int16_t bounceAngleDeg; ///< Góc nảy tính theo độ (0 .. 180 deg)
    uint16_t spinFactor;    ///< Hệ số xoáy dự kiến
};

class TrajectoryCalculator
{
public:
    TrajectoryCalculator();
    ~TrajectoryCalculator() = default;

    /**
     * @brief Tính toán góc bay và vận tốc mới của bóng dựa trên điểm tiếp xúc với Paddle
     * @param params Các thông số va chạm
     * @return ReflectionResult Vectơ vận tốc và góc nảy mới
     */
    static ReflectionResult CalculateBounceTrajectory(const ImpactParameters& params);

    /**
     * @brief Dự đoán tọa độ giao điểm của bóng với đường Paddle (cho CPU / Bot AI)
     * @param currentPos Vị trí bóng hiện tại
     * @currentVel Vận tốc bóng hiện tại
     * @targetY Mức Y của Paddle cần dự đoán
     * @return Vector2D_Q8 Vị trí dự đoán bóng sẽ rơi xuống
     */
    static Vector2D_Q8 PredictLandingPosition(Vector2D_Q8 currentPos, Vector2D_Q8 currentVel, int16_t targetY);

    /**
     * @brief Tính độ nghiêng quỹ đạo (Arc Angle) theo số nguyên
     * @param vx Vận tốc ngang (Q8.8)
     * @param vy Vận tốc dọc (Q8.8)
     * @return int16_t Góc tính theo độ (-90 đến 90)
     */
    static int16_t FastAtan2Deg(int16_t vx, int16_t vy);

private:
    static const int32_t FIXED_ONE = 256;
    static const int16_t MAX_BOUNCE_ANGLE_DEG = 60;
};

} // namespace AntiPhysics

#endif // TRAJECTORY_CALCULATOR_HPP
