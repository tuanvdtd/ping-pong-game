#ifndef GAME_ENGINE_HPP
#define GAME_ENGINE_HPP

#include <gui/game/GameTypes.hpp>
#include <stdint.h>

class GameEngine
{
public:
    enum Event : uint8_t
    {
        EVENT_NONE = 0U,
        EVENT_HIT_PLAYER_1 = 1U << 0,
        EVENT_HIT_PLAYER_2 = 1U << 1,
        EVENT_GAME_OVER = 1U << 2
    };

    GameEngine()
    {
        reset(GameMode::VS_CPU);
    }

    void reset(GameMode newMode)
    {
        mode = newMode;
        state = GameState::READY;
        winner = GameWinner::NONE;
        player1Lives = 3U;
        player2Lives = 3U;
        bottomPaddleX = PADDLE_CENTER_X;
        topPaddleX = PADDLE_CENTER_X;
        cpuTargetX = PADDLE_CENTER_X;
        ballXQ8 = BALL_START_X * FIXED_ONE;
        ballYQ8 = BALL_START_Y * FIXED_ONE;
        ballVelocityXQ8 = 320;
        ballVelocityYQ8 = -512;
        readyTicks = 0U;
        cpuReactionTicks = 0U;
        rallyCount = 0U;
    }

    uint8_t update(uint16_t player1Input, uint16_t player2Input)
    {
        uint8_t events = EVENT_NONE;

        if ((state == GameState::PAUSED) || (state == GameState::GAME_OVER))
        {
            return events;
        }

        bottomPaddleX = mapInputToPaddle(player1Input);

        if (mode == GameMode::TWO_PLAYERS)
        {
            topPaddleX = mapInputToPaddle(player2Input);
        }
        else
        {
            updateCpu();
        }

        if (state == GameState::READY)
        {
            ++readyTicks;
            if (readyTicks >= READY_TICKS)
            {
                state = GameState::PLAYING;
            }
            return events;
        }

        ballXQ8 += ballVelocityXQ8;
        ballYQ8 += ballVelocityYQ8;

        handleSideWalls();

        if ((ballVelocityYQ8 < 0) &&
            circleTouchesPaddle(topPaddleX, TOP_PADDLE_Y))
        {
            ballYQ8 = (TOP_PADDLE_Y + PADDLE_HEIGHT) * FIXED_ONE;
            ballVelocityYQ8 = positive(ballVelocityYQ8);
            updateHorizontalVelocity(topPaddleX);
            ++rallyCount;
            events = (uint8_t)(events | EVENT_HIT_PLAYER_2);
        }
        else if ((ballVelocityYQ8 > 0) &&
                 circleTouchesPaddle(bottomPaddleX, BOTTOM_PADDLE_Y))
        {
            ballYQ8 = (BOTTOM_PADDLE_Y - BALL_SIZE) * FIXED_ONE;
            ballVelocityYQ8 = negative(ballVelocityYQ8);
            updateHorizontalVelocity(bottomPaddleX);
            ++rallyCount;
            events = (uint8_t)(events | EVENT_HIT_PLAYER_1);
        }

        const int16_t ballY = getBallY();
        if ((ballY + BALL_SIZE) < FIELD_TOP)
        {
            if (player2Lives > 0U) { --player2Lives; }
            if (player2Lives == 0U)
            {
                state = GameState::GAME_OVER;
                winner = GameWinner::PLAYER_1;
                events = (uint8_t)(events | EVENT_GAME_OVER);
            }
            else
            {
                respawnBall(1);
            }
        }
        else if (ballY > FIELD_BOTTOM)
        {
            if (player1Lives > 0U) { --player1Lives; }
            if (player1Lives == 0U)
            {
                state = GameState::GAME_OVER;
                winner = GameWinner::PLAYER_2;
                events = (uint8_t)(events | EVENT_GAME_OVER);
            }
            else
            {
                respawnBall(-1);
            }
        }

        return events;
    }

    void respawnBall(int16_t serveDirectionY)
    {
        ballXQ8 = BALL_START_X * FIXED_ONE;
        ballYQ8 = BALL_START_Y * FIXED_ONE;
        ballVelocityXQ8 = (rallyCount % 2 == 0) ? 320 : -320;
        ballVelocityYQ8 = (serveDirectionY > 0) ? 512 : -512;
        state = GameState::READY;
        readyTicks = 0U;
    }

    void togglePause()
    {
        if (state == GameState::PLAYING)
        {
            state = GameState::PAUSED;
        }
        else if (state == GameState::PAUSED)
        {
            state = GameState::PLAYING;
        }
    }

    uint8_t getPlayer1Lives() const
    {
        return player1Lives;
    }

    uint8_t getPlayer2Lives() const
    {
        return player2Lives;
    }

    int16_t getBottomPaddleX() const
    {
        return bottomPaddleX;
    }

    int16_t getTopPaddleX() const
    {
        return topPaddleX;
    }

    int16_t getBallX() const
    {
        return (int16_t)(ballXQ8 / FIXED_ONE);
    }

    int16_t getBallY() const
    {
        return (int16_t)(ballYQ8 / FIXED_ONE);
    }

    GameState getState() const
    {
        return state;
    }

    GameWinner getWinner() const
    {
        return winner;
    }

    GameMode getMode() const
    {
        return mode;
    }

private:
    static const int16_t FIELD_LEFT = 28;
    static const int16_t FIELD_RIGHT = 212;
    static const int16_t FIELD_TOP = 58;
    static const int16_t FIELD_BOTTOM = 299;

    static const int16_t TOP_PADDLE_Y = 65;
    static const int16_t BOTTOM_PADDLE_Y = 280;
    static const int16_t PADDLE_WIDTH = 48;
    static const int16_t PADDLE_HEIGHT = 11;
    static const int16_t PADDLE_MIN_X = FIELD_LEFT;
    static const int16_t PADDLE_MAX_X = FIELD_RIGHT - PADDLE_WIDTH;
    static const int16_t PADDLE_CENTER_X =
        PADDLE_MIN_X + ((PADDLE_MAX_X - PADDLE_MIN_X) / 2);

    static const int16_t BALL_SIZE = 12;
    static const int16_t BALL_RADIUS = 6;
    static const int16_t BALL_START_X = 114;
    static const int16_t BALL_START_Y = 169;

    static const int16_t FIXED_ONE = 256;
    static const uint16_t READY_TICKS = 30U;
    static const uint8_t CPU_REACTION_TICKS = 4U;
    static const int16_t CPU_SPEED = 1;

    GameMode mode;
    GameState state;
    GameWinner winner;
    uint8_t player1Lives;
    uint8_t player2Lives;

    int16_t bottomPaddleX;
    int16_t topPaddleX;
    int16_t cpuTargetX;
    int32_t ballXQ8;
    int32_t ballYQ8;
    int16_t ballVelocityXQ8;
    int16_t ballVelocityYQ8;
    uint16_t readyTicks;
    uint8_t cpuReactionTicks;
    uint16_t rallyCount;

    static int16_t clamp(int16_t value, int16_t minimum, int16_t maximum)
    {
        if (value < minimum)
        {
            return minimum;
        }
        if (value > maximum)
        {
            return maximum;
        }
        return value;
    }

    static int16_t positive(int16_t value)
    {
        return (value < 0) ? (int16_t)-value : value;
    }

    static int16_t negative(int16_t value)
    {
        return (value > 0) ? (int16_t)-value : value;
    }

    // Ánh xạ tín hiệu điều khiển (ADC 0..1000) sang vị trí hoành độ X của Paddle trên màn hình
    static int16_t mapInputToPaddle(uint16_t input)
    {
        if (input > 1000U)
        {
            input = 1000U;
        }

        return (int16_t)(PADDLE_MIN_X +
                         (((uint32_t)input *
                           (uint32_t)(PADDLE_MAX_X - PADDLE_MIN_X)) /
                          1000U));
    }

    void updateCpu()
    {
        ++cpuReactionTicks;
        if (cpuReactionTicks >= CPU_REACTION_TICKS)
        {
            cpuReactionTicks = 0U;

            if (ballVelocityYQ8 < 0)
            {
                int16_t error;
                switch (rallyCount & 0x03U)
                {
                case 0U:
                    error = -7;
                    break;
                case 1U:
                    error = 5;
                    break;
                case 2U:
                    error = -3;
                    break;
                default:
                    error = 8;
                    break;
                }

                cpuTargetX = clamp(
                    (int16_t)(getBallX() + BALL_RADIUS -
                              (PADDLE_WIDTH / 2) + error),
                    PADDLE_MIN_X,
                    PADDLE_MAX_X);
            }
            else
            {
                cpuTargetX = PADDLE_CENTER_X;
            }
        }

        if (topPaddleX < cpuTargetX)
        {
            topPaddleX = clamp((int16_t)(topPaddleX + CPU_SPEED),
                               PADDLE_MIN_X,
                               PADDLE_MAX_X);
        }
        else if (topPaddleX > cpuTargetX)
        {
            topPaddleX = clamp((int16_t)(topPaddleX - CPU_SPEED),
                               PADDLE_MIN_X,
                               PADDLE_MAX_X);
        }
    }

    // Xử lý va chạm bóng với hai tường dọc biên trái và biên phải (đổi chiều vận tốc Vx)
    void handleSideWalls()
    {
        int16_t ballX = getBallX();

        if (ballX <= FIELD_LEFT)
        {
            ballXQ8 = FIELD_LEFT * FIXED_ONE;
            ballVelocityXQ8 = positive(ballVelocityXQ8);
        }
        else if ((ballX + BALL_SIZE) >= FIELD_RIGHT)
        {
            ballXQ8 = (FIELD_RIGHT - BALL_SIZE) * FIXED_ONE;
            ballVelocityXQ8 = negative(ballVelocityXQ8);
        }
    }

    // Kiểm tra va chạm giữa quả bóng (hình tròn) và Paddle (hình chữ nhật)
    // Thuật toán: Tìm điểm thuộc paddle gần tâm bóng nhất, sau đó tính khoảng cách Euclid (Squared distance)
    bool circleTouchesPaddle(int16_t paddleX, int16_t paddleY) const
    {
        const int16_t centerX = (int16_t)(getBallX() + BALL_RADIUS);
        const int16_t centerY = (int16_t)(getBallY() + BALL_RADIUS);
        const int16_t closestX =
            clamp(centerX, paddleX, (int16_t)(paddleX + PADDLE_WIDTH));
        const int16_t closestY =
            clamp(centerY, paddleY, (int16_t)(paddleY + PADDLE_HEIGHT));
        const int32_t deltaX = (int32_t)centerX - closestX;
        const int32_t deltaY = (int32_t)centerY - closestY;

        return ((deltaX * deltaX) + (deltaY * deltaY)) <=
               (BALL_RADIUS * BALL_RADIUS);
    }

    // Xử lý tính toán góc bay và vận tốc ngang của bóng khi nảy từ paddle
    // Độ lệch giữa tâm bóng và tâm paddle quyết định góc bật (nghiêng nhiều hơn nếu đánh ở mép paddle)
    void updateHorizontalVelocity(int16_t paddleX)
    {
        const int16_t paddleCenter = (int16_t)(paddleX + (PADDLE_WIDTH / 2));
        const int16_t ballCenter = (int16_t)(getBallX() + BALL_RADIUS);
        int16_t velocity = (int16_t)((ballCenter - paddleCenter) * 32);

        velocity = clamp(velocity, -768, 768);

        if ((velocity > -128) && (velocity < 128))
        {
            velocity = (ballVelocityXQ8 < 0) ? -128 : 128;
        }

        ballVelocityXQ8 = velocity;
    }
};

#endif // GAME_ENGINE_HPP
