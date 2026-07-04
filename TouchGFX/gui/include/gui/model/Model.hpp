#ifndef MODEL_HPP
#define MODEL_HPP

#include <gui/game/GameTypes.hpp>
#include <stdint.h>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    void setGameMode(GameMode mode);
    GameMode getGameMode() const;

    uint16_t getPlayer1Input() const;
    uint16_t getPlayer2Input() const;
    bool consumePa0ButtonPress();

    void vibratePlayer1();
    void vibratePlayer2();
    void stopAllHaptics();

protected:
    ModelListener* modelListener;

private:
    GameMode currentGameMode;
    uint16_t player1Input;
    uint16_t player2Input;
};

#endif // MODEL_HPP
