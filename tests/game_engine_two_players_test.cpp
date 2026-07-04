#include <gui/game/GameEngine.hpp>

#include <cstdlib>
#include <iostream>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

uint16_t inputForPaddleAtBall(const GameEngine& engine)
{
    const int16_t paddleMinX = 28;
    const int16_t paddleMaxX = 164;
    const int16_t paddleWidth = 48;
    const int16_t ballRadius = 6;
    int16_t targetX =
        static_cast<int16_t>(engine.getBallX() + ballRadius -
                             (paddleWidth / 2));

    if (targetX < paddleMinX)
    {
        targetX = paddleMinX;
    }
    else if (targetX > paddleMaxX)
    {
        targetX = paddleMaxX;
    }

    return static_cast<uint16_t>(
        (static_cast<uint32_t>(targetX - paddleMinX) * 1000U) /
        static_cast<uint32_t>(paddleMaxX - paddleMinX));
}

void testDirectMappingForBothPlayers()
{
    GameEngine engine;
    engine.reset(GameMode::TWO_PLAYERS);

    engine.update(0U, 0U);
    require(engine.getBottomPaddleX() == 28,
            "Player 1 input 0 must map to the left edge");
    require(engine.getTopPaddleX() == 28,
            "Player 2 input 0 must map to the left edge");

    engine.update(1000U, 1000U);
    require(engine.getBottomPaddleX() == 164,
            "Player 1 increasing input must move right");
    require(engine.getTopPaddleX() == 164,
            "Player 2 increasing input must move right");

    engine.update(250U, 750U);
    require(engine.getBottomPaddleX() < engine.getTopPaddleX(),
            "Each player must use its own ADC input");
}

void testTwoPlayerModeDoesNotUseCpuController()
{
    GameEngine left;
    GameEngine right;
    left.reset(GameMode::TWO_PLAYERS);
    right.reset(GameMode::TWO_PLAYERS);

    for (uint16_t tick = 0U; tick < 20U; ++tick)
    {
        left.update(500U, 0U);
        right.update(500U, 1000U);
    }

    require(left.getTopPaddleX() == 28,
            "Player 2 must retain direct control at the left edge");
    require(right.getTopPaddleX() == 164,
            "Player 2 must retain direct control at the right edge");
}

void testVsCpuStillIgnoresPlayer2Input()
{
    GameEngine first;
    GameEngine second;
    first.reset(GameMode::VS_CPU);
    second.reset(GameMode::VS_CPU);

    for (uint16_t tick = 0U; tick < 100U; ++tick)
    {
        first.update(500U, 0U);
        second.update(500U, 1000U);
        require(first.getTopPaddleX() == second.getTopPaddleX(),
                "VS_CPU top paddle must not depend on Player 2 input");
    }
}

void testPauseFreezesBothPlayers()
{
    GameEngine engine;
    engine.reset(GameMode::TWO_PLAYERS);

    for (uint16_t tick = 0U; tick < 30U; ++tick)
    {
        engine.update(500U, 500U);
    }

    require(engine.getState() == GameState::PLAYING,
            "Engine must enter PLAYING after the ready delay");

    const int16_t ballX = engine.getBallX();
    const int16_t ballY = engine.getBallY();
    const int16_t bottomX = engine.getBottomPaddleX();
    const int16_t topX = engine.getTopPaddleX();

    engine.togglePause();
    require(engine.getState() == GameState::PAUSED,
            "togglePause must pause a playing match");
    require(engine.update(0U, 1000U) == GameEngine::EVENT_NONE,
            "Paused engine must not emit gameplay events");
    require(engine.getBallX() == ballX && engine.getBallY() == ballY,
            "Pause must freeze the ball");
    require(engine.getBottomPaddleX() == bottomX &&
                engine.getTopPaddleX() == topX,
            "Pause must freeze both paddles");
}

void testTopMissAwardsPlayer1()
{
    GameEngine engine;
    engine.reset(GameMode::TWO_PLAYERS);

    uint8_t events = GameEngine::EVENT_NONE;
    for (uint16_t tick = 0U;
         (tick < 300U) &&
         (engine.getState() != GameState::GAME_OVER);
         ++tick)
    {
        events = engine.update(500U, 0U);
    }

    require(engine.getState() == GameState::GAME_OVER,
            "A missed top ball must end the match");
    require(engine.getWinner() == GameWinner::PLAYER_1,
            "A top miss must award Player 1");
    require((events & GameEngine::EVENT_GAME_OVER) != 0U,
            "A miss must emit EVENT_GAME_OVER");
}

void testBottomMissAwardsPlayer2()
{
    GameEngine engine;
    engine.reset(GameMode::TWO_PLAYERS);

    uint8_t events = GameEngine::EVENT_NONE;
    for (uint16_t tick = 0U;
         (tick < 500U) &&
         (engine.getState() != GameState::GAME_OVER);
         ++tick)
    {
        events = engine.update(1000U, 1000U);
    }

    require(engine.getState() == GameState::GAME_OVER,
            "A missed bottom ball must end the match");
    require(engine.getWinner() == GameWinner::PLAYER_2,
            "A bottom miss must award Player 2");
    require((events & GameEngine::EVENT_GAME_OVER) != 0U,
            "A miss must emit EVENT_GAME_OVER");
}

void testEachPaddleEmitsItsOwnHapticEvent()
{
    GameEngine engine;
    engine.reset(GameMode::TWO_PLAYERS);

    bool player1Hit = false;
    bool player2Hit = false;

    for (uint16_t tick = 0U;
         (tick < 1000U) &&
         (engine.getState() != GameState::GAME_OVER) &&
         !(player1Hit && player2Hit);
         ++tick)
    {
        const uint16_t trackingInput = inputForPaddleAtBall(engine);
        const uint8_t events =
            engine.update(trackingInput, trackingInput);

        player1Hit =
            player1Hit ||
            ((events & GameEngine::EVENT_HIT_PLAYER_1) != 0U);
        player2Hit =
            player2Hit ||
            ((events & GameEngine::EVENT_HIT_PLAYER_2) != 0U);
    }

    require(player1Hit,
            "Bottom paddle must emit Player 1 haptic event");
    require(player2Hit,
            "Top paddle must emit Player 2 haptic event");
}
}

int main()
{
    testDirectMappingForBothPlayers();
    testTwoPlayerModeDoesNotUseCpuController();
    testVsCpuStillIgnoresPlayer2Input();
    testPauseFreezesBothPlayers();
    testTopMissAwardsPlayer1();
    testBottomMissAwardsPlayer2();
    testEachPaddleEmitsItsOwnHapticEvent();

    std::cout << "All two-player GameEngine tests passed.\n";
    return EXIT_SUCCESS;
}
