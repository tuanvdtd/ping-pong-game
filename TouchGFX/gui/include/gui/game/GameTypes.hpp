#ifndef GAME_TYPES_HPP
#define GAME_TYPES_HPP

#include <stdint.h>

enum class GameMode : uint8_t
{
    VS_CPU,
    TWO_PLAYERS
};

enum class GameState : uint8_t
{
    READY,
    PLAYING,
    PAUSED,
    GAME_OVER
};

enum class GameWinner : uint8_t
{
    NONE,
    PLAYER_1,
    PLAYER_2
};

#endif // GAME_TYPES_HPP
