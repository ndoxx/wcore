#include <cmath>
#include <cassert>
#include "double_slider.h"

namespace medit
{

DoubleSlider::DoubleSlider(QWidget* parent):
QSlider(parent),
minval_(0.0),
maxval_(1.0)
{
    connect(this, SIGNAL(valueChanged(int)),
            this, SLOT(notifyValueChanged(int)));

    setOrientation(Qt::Horizontal);
    setRange(0, 100);
}

void DoubleSlider::set_range(double minval, double maxval)
{
    assert(minval<maxval && "[DoubleSlider] minimum value should be less than maximum value.");

    minval_ = minval;
    maxval_ = maxval;
    double range = maxval_-minval_;
    setRange((int)std::floor(minval_*100/range), (int)std::floor(maxval_*100/range));
}

void DoubleSlider::set_value(double value)
{
    assert(value<=maxval_ && "[DoubleSlider] value should be less than or equal to maximum value.");
    assert(value>=minval_ && "[DoubleSlider] value should be greater than or equal to minimum value.");

    setValue((int)std::floor(value*100/(maxval_-minval_)));
}

void DoubleSlider::notifyValueChanged(int value)
{
    double_value_ = (value * (maxval_-minval_)) / 100.0;
    emit doubleValueChanged(double_value_);
}


} // namespace medit
