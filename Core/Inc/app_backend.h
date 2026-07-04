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

uint8_t AppBackend_GetLatestInput(uint16_t* player1, uint16_t* player2);
void AppBackend_SendHaptic(uint8_t command);

#ifdef __cplusplus
}
#endif

#endif /* APP_BACKEND_H */
