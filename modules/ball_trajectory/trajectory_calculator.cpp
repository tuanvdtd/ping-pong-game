/**
 * @file trajectory_calculator.cpp
 * @brief Triển khai thuật toán tính toán góc bay và dự đoán quỹ đạo bóng
 * @author Đỗ Sơn Tùng (MSSV: 20225425) - Nhóm Anti
 */

#include "trajectory_calculator.hpp"

namespace AntiPhysics
{

TrajectoryCalculator::TrajectoryCalculator()
{
}

ReflectionResult TrajectoryCalculator::CalculateBounceTrajectory(const ImpactParameters& params)
{
    ReflectionResult result;

    // 1. Tính khoảng cách lệch tâm giữa tâm quả bóng và tâm paddle
    int16_t offset = params.ballCenterX - params.paddleCenterX;
    int16_t halfWidth = params.paddleWidth / 2;

    // Giới hạn độ lệch tâm trong khoảng [-halfWidth, halfWidth]
    if (offset < -halfWidth) offset = -halfWidth;
    if (offset > halfWidth)  offset = halfWidth;

    // 2. Tính tỉ lệ va chạm chuẩn hóa (-1.0 đến 1.0 dạng Fixed-point Q8.8)
    int32_t normalizedOffsetQ8 = ((int32_t)offset * FIXED_ONE) / halfWidth;

    // 3. Tính toán vận tốc X mới dựa trên điểm nảy ở tâm hay rìa paddle
    // Đánh ở tâm -> góc bật thẳng, Đánh ở rìa -> góc bật chéo rộng
    int32_t maxVxQ8 = 768; // Vận tốc ngang tối đa (3.0 pixel/tick)
    result.outgoingVxQ8 = (int16_t)((normalizedOffsetQ8 * maxVxQ8) / FIXED_ONE);

    // 4. Đảo ngược vận tốc Y và tăng nhẹ tốc độ sau mỗi lượt đỡ bóng thành công
    int16_t absVy = (params.incomingVyQ8 < 0) ? -params.incomingVyQ8 : params.incomingVyQ8;
    int16_t newVy = absVy + 16; // Tăng nhẹ 0.0625 pixel/tick
    if (newVy > 1024) newVy = 1024; // Limit max Y speed

    result.outgoingVyQ8 = (params.incomingVyQ8 < 0) ? newVy : -newVy;

    // 5. Tính góc nảy tương đương (Degree)
    result.bounceAngleDeg = FastAtan2Deg(result.outgoingVxQ8, result.outgoingVyQ8);

    // 6. Ước tính độ xoáy (Spin) dựa trên vận tốc ngang
    result.spinFactor = (uint16_t)((normalizedOffsetQ8 < 0 ? -normalizedOffsetQ8 : normalizedOffsetQ8) * 100 / FIXED_ONE);

    return result;
}

Vector2D_Q8 TrajectoryCalculator::PredictLandingPosition(Vector2D_Q8 currentPos, Vector2D_Q8 currentVel, int16_t targetY)
{
    Vector2D_Q8 predictedPos = currentPos;

    // Nếu bóng không di chuyển theo trục Y hướng về targetY thì giữ nguyên vị trí
    if (currentVel.y == 0) return predictedPos;

    int32_t targetYQ8 = (int32_t)targetY * FIXED_ONE;
    int32_t deltaY = targetYQ8 - currentPos.y;

    // Số tick cần thiết để bóng bay tới mức targetY
    int32_t ticksToTarget = deltaY / currentVel.y;
    if (ticksToTarget < 0) ticksToTarget = -ticksToTarget;

    // Dự đoán vị trí X tương ứng
    predictedPos.x = currentPos.x + (currentVel.x * ticksToTarget);
    predictedPos.y = targetYQ8;

    return predictedPos;
}

int16_t TrajectoryCalculator::FastAtan2Deg(int16_t vx, int16_t vy)
{
    if (vy == 0) return (vx >= 0) ? 90 : -90;

    // Xấp xỉ xấp xỉ arctan(vx/vy) đơn giản bằng tỉ lệ góc
    int32_t ratioQ8 = ((int32_t)vx * FIXED_ONE) / vy;
    int32_t angle = (ratioQ8 * 45) / FIXED_ONE;

    if (angle > 90) angle = 90;
    if (angle < -90) angle = -90;

    return (int16_t)angle;
}

} // namespace AntiPhysics
