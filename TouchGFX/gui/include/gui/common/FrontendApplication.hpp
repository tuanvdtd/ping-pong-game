#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    void gotoScreen2ScreenNoTransition();

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }
private:
    void gotoScreen2ScreenNoTransitionImpl();

    touchgfx::Callback<FrontendApplication> screen2TransitionCallback;
};

#endif // FRONTENDAPPLICATION_HPP
