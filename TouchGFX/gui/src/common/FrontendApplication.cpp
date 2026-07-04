#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      screen2TransitionCallback()
{

}

void FrontendApplication::gotoScreen2ScreenNoTransition()
{
    screen2TransitionCallback =
        touchgfx::Callback<FrontendApplication>(
            this,
            &FrontendApplication::gotoScreen2ScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &screen2TransitionCallback;
}

void FrontendApplication::gotoScreen2ScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen2View,
                             Screen2Presenter,
                             touchgfx::NoTransition,
                             Model>(&currentScreen,
                                    &currentPresenter,
                                    frontendHeap,
                                    &currentTransition,
                                    &model);
}
