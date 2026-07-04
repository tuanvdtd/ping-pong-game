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
    void setResultPopupVisible(bool visible);
    void onGameOver();

    GameEngine gameEngine;
    bool gameOverPending;
};

#endif // SCREEN2VIEW_HPP
