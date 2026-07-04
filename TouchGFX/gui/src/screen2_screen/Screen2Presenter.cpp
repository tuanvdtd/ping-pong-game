#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{

}

void Screen2Presenter::activate()
{

}

void Screen2Presenter::deactivate()
{

}

GameMode Screen2Presenter::getGameMode() const
{
    return model->getGameMode();
}

uint16_t Screen2Presenter::getPlayer1Input() const
{
    return model->getPlayer1Input();
}

uint16_t Screen2Presenter::getPlayer2Input() const
{
    return model->getPlayer2Input();
}

bool Screen2Presenter::consumePa0ButtonPress()
{
    return model->consumePa0ButtonPress();
}

void Screen2Presenter::vibratePlayer1()
{
    model->vibratePlayer1();
}

void Screen2Presenter::vibratePlayer2()
{
    model->vibratePlayer2();
}

void Screen2Presenter::stopAllHaptics()
{
    model->stopAllHaptics();
}
