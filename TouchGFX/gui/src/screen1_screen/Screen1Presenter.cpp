/*
 * Screen1Presenter.cpp - lớp TRUNG GIAN giữa Screen1View và Model.
 * View không được gọi thẳng Model; mọi yêu cầu đi qua Presenter.
 * Con trỏ `model` có được nhờ kế thừa ModelListener (framework bind sẵn).
 */
#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

// Framework gọi khi màn này được kích hoạt. Chưa cần khởi tạo gì thêm.
void Screen1Presenter::activate()
{

}

// Framework gọi khi màn này bị thay thế. Không có gì phải dọn.
void Screen1Presenter::deactivate()
{

}

// Chuyển tiếp lựa chọn "1 người đấu máy" xuống Model.
void Screen1Presenter::selectVsCpu()
{
    model->setGameMode(GameMode::VS_CPU);
}

// Chuyển tiếp lựa chọn "2 người chơi" xuống Model.
void Screen1Presenter::selectTwoPlayers()
{
    model->setGameMode(GameMode::TWO_PLAYERS);
}
