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

Pa0ButtonEvent Model::consumePa0ButtonEvent()
{
#ifndef SIMULATOR
    const uint8_t event = AppBackend_ConsumePa0Event();

    if (event == APP_PA0_EVENT_SINGLE_PRESS)
    {
        return Pa0ButtonEvent::SINGLE_PRESS;
    }

    if (event == APP_PA0_EVENT_DOUBLE_PRESS)
    {
        return Pa0ButtonEvent::DOUBLE_PRESS;
    }
#else
    /* The simulator has no on-board PA0 USER button. */
#endif

    return Pa0ButtonEvent::NONE;
}

void Model::resetPa0Gesture()
{
#ifndef SIMULATOR
    AppBackend_ResetPa0Gesture();
#endif
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
