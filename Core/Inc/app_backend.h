#ifndef APP_BACKEND_H
#define APP_BACKEND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    APP_HAPTIC_PLAYER_1 = 1U,
    APP_HAPTIC_PLAYER_2 = 2U,
    APP_HAPTIC_STOP_ALL = 3U
};

enum
{
    APP_PA0_EVENT_NONE = 0U,
    APP_PA0_EVENT_SINGLE_PRESS = 1U,
    APP_PA0_EVENT_DOUBLE_PRESS = 2U
};

uint8_t AppBackend_GetLatestInput(uint16_t* player1, uint16_t* player2);
uint8_t AppBackend_ConsumePa0Event(void);
void AppBackend_ResetPa0Gesture(void);
void AppBackend_SendHaptic(uint8_t command);

#ifdef __cplusplus
}
#endif

#endif /* APP_BACKEND_H */
