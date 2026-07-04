#include <gui/screen2_screen/Screen2View.hpp>
#include <texts/TextKeysAndLanguages.hpp>

Screen2View::Screen2View()
    : gameEngine(),
      gameOverPending(false)
{

}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
    gameEngine.reset(presenter->getGameMode());
    gameOverPending = false;
    setResultPopupVisible(false);
    syncWidgets();
}

void Screen2View::tearDownScreen()
{
    presenter->stopAllHaptics();
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::ball_timertick()
{
    const uint8_t events =
        gameEngine.update(presenter->getPlayer1Input(),
                          presenter->getPlayer2Input());

    if ((events & GameEngine::EVENT_HIT_PLAYER_1) != 0U)
    {
        presenter->vibratePlayer1();
    }

    if ((events & GameEngine::EVENT_HIT_PLAYER_2) != 0U)
    {
        presenter->vibratePlayer2();
    }

    syncWidgets();

    if (((events & GameEngine::EVENT_GAME_OVER) != 0U) &&
        !gameOverPending)
    {
        onGameOver();
    }
}

void Screen2View::togglePause()
{
    gameEngine.togglePause();

    if (gameEngine.getState() == GameState::PAUSED)
    {
        presenter->stopAllHaptics();
    }
}

void Screen2View::goHomeSafely()
{
    presenter->stopAllHaptics();
    application().gotoScreen1ScreenNoTransition();
}

void Screen2View::playAgain()
{
    presenter->stopAllHaptics();
    gameEngine.reset(presenter->getGameMode());
    gameOverPending = false;
    setResultPopupVisible(false);
    syncWidgets();
}

bool Screen2View::isGameOverPending() const
{
    return gameOverPending;
}

GameWinner Screen2View::getWinner() const
{
    return gameEngine.getWinner();
}

void Screen2View::syncWidgets()
{
    const int16_t bottomX = gameEngine.getBottomPaddleX();
    const int16_t topX = gameEngine.getTopPaddleX();
    const int16_t ballX = gameEngine.getBallX();
    const int16_t ballY = gameEngine.getBallY();

    if (p1.getX() != bottomX)
    {
        p1.moveTo(bottomX, p1.getY());
    }

    if (p2.getX() != topX)
    {
        p2.moveTo(topX, p2.getY());
    }

    if ((circle1.getX() != ballX) || (circle1.getY() != ballY))
    {
        circle1.moveTo(ballX, ballY);
    }
}

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

void Screen2View::onGameOver()
{
    const touchgfx::Unicode::UnicodeChar* winnerText;

    gameOverPending = true;
    presenter->stopAllHaptics();

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

    textResult1.setWildcard(winnerText);
    textResult1.resizeToCurrentText();
    setResultPopupVisible(true);
}
