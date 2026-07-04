#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

void Screen1Presenter::activate()
{

}

void Screen1Presenter::deactivate()
{

}

void Screen1Presenter::selectVsCpu()
{
    model->setGameMode(GameMode::VS_CPU);
}

void Screen1Presenter::selectTwoPlayers()
{
    model->setGameMode(GameMode::TWO_PLAYERS);
}
