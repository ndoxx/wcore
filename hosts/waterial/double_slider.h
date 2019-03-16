#ifndef DOUBLE_SLIDER_H
#define DOUBLE_SLIDER_H

#include <QSlider>

namespace waterial
{

class DoubleSlider : public QSlider
{
    Q_OBJECT

public:
    DoubleSlider(QWidget* parent=0);

    void set_range(double minval, double maxval);
    void set_value(double value);

signals:
    void doubleValueChanged(double value);

public slots:
    void notifyValueChanged(int value);

private:
    double double_value_;
    double minval_;
    double maxval_;
};

} // namespace waterial

#endif // DOUBLE_SLIDER_H
