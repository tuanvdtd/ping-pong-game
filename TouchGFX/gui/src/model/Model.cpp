#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifndef SIMULATOR
#include "app_backend.h"
#endif

Model::Model()
    : modelListener(0),
      currentGameMode(GameMode::VS_CPU),
      player1Input(500U),
      player2Input(500U)
{

}

void Model::tick()
{
#ifndef SIMULATOR
    uint16_t newestPlayer1;
    uint16_t newestPlayer2;

    if (AppBackend_GetLatestInput(&newestPlayer1, &newestPlayer2) != 0U)
    {
        player1Input = newestPlayer1;
        player2Input = newestPlayer2;
    }
#endif
}

void Model::setGameMode(GameMode mode)
{
    currentGameMode = mode;
}

GameMode Model::getGameMode() const
{
    return currentGameMode;
}

uint16_t Model::getPlayer1Input() const
{
    return player1Input;
}

uint16_t Model::getPlayer2Input() const
{
    return player2Input;
}

void Model::vibratePlayer1()
{
#ifndef SIMULATOR
    AppBackend_SendHaptic(APP_HAPTIC_PLAYER_1);
#endif
}

void Model::vibratePlayer2()
{
#ifndef SIMULATOR
    AppBackend_SendHaptic(APP_HAPTIC_PLAYER_2);
#endif
}

void Model::stopAllHaptics()
{
#ifndef SIMULATOR
    AppBackend_SendHaptic(APP_HAPTIC_STOP_ALL);
#endif
}
