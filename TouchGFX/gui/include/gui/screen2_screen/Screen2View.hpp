#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/game/GameEngine.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void ball_timertick();
    virtual void togglePause();
    virtual void goHomeSafely();
    virtual void playAgain();

    bool isGameOverPending() const;
    GameWinner getWinner() const;

protected:
    void syncWidgets();
    void setPauseButtonState(bool paused);
    void setResultPopupVisible(bool visible);
    void onGameOver();
    void continueButtonCallbackHandler(
        const touchgfx::AbstractButtonContainer& source);

    GameEngine gameEngine;
    bool gameOverPending;
    touchgfx::Box p1LifeBox[3];
    touchgfx::Box p2LifeBox[3];
    touchgfx::Callback<Screen2View,
                       const touchgfx::AbstractButtonContainer&>
        continueButtonCallback;
};

#endif // SCREEN2VIEW_HPP
