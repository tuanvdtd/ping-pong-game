#ifndef SCREEN2PRESENTER_HPP
#define SCREEN2PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen2View;

class Screen2Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen2Presenter(Screen2View& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    GameMode getGameMode() const;
    uint16_t getPlayer1Input() const;
    uint16_t getPlayer2Input() const;
    Pa0ButtonEvent consumePa0ButtonEvent();
    void resetPa0Gesture();

    void vibratePlayer1();
    void vibratePlayer2();
    void stopAllHaptics();

    virtual ~Screen2Presenter() {}

private:
    Screen2Presenter();

    Screen2View& view;
};

#endif // SCREEN2PRESENTER_HPP
