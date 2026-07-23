#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

/*
 * ModelListener - lớp cơ sở mà MỌI Presenter kế thừa.
 *
 * Vai trò:
 * 1) Cấp cho Presenter con trỏ `model` (protected bên dưới) - nhờ kế thừa
 *    lớp này mà Presenter viết được `model->getPlayer1Input()`...
 * 2) Là kênh để Model gọi NGƯỢC lên Presenter khi có sự kiện bất đồng bộ
 *    (khai báo hàm virtual ở đây rồi Presenter override). Dự án dùng
 *    mô hình "kéo" (View chủ động hỏi mỗi tick) nên chiều ngược này
 *    chưa cần dùng - lớp để trống nhưng giữ đúng khung MVP chuẩn.
 */
class ModelListener
{
public:
    ModelListener() : model(0) {}

    virtual ~ModelListener() {}

    // Framework gọi lúc chuyển màn: đưa địa chỉ Model duy nhất
    // cho Presenter mới, hoàn tất cặp con trỏ hai chiều Model <-> Presenter.
    void bind(Model* m)
    {
        model = m;
    }
protected:
    Model* model;   // con trỏ tới Model duy nhất của ứng dụng
};

#endif // MODELLISTENER_HPP
