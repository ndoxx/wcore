#include <iostream>
#include <cassert>

#include "spinbox.h"

namespace waterial
{

DoubleSpinBox::DoubleSpinBox(QWidget* parent):
QDoubleSpinBox(parent)
{
    // * Restore "C" locale settings
    // Reject comma group separator, use dot as decimal separator
    QLocale qlocale(QLocale::C);
    qlocale.setNumberOptions(QLocale::RejectGroupSeparator);
    setLocale(qlocale);

    // * Style
    setMinimumWidth(50);
    setMaximumHeight(20);
    setFrame(true);
    setAlignment(Qt::AlignHCenter);

    QFont font = this->font();
    font.setPointSize(10);
    setFont(font);
}

void DoubleSpinBox::set_constrains(double minval, double maxval, double step, double value)
{
    assert(minval<maxval && "DoubleSpinBox: minimum value should be less than maximum value.");
    assert(step<(maxval-minval) && "DoubleSpinBox: step value should be less than whole range.");
    assert(value<=maxval && "DoubleSpinBox: initial value should be less than maximum value");
    assert(value>=minval && "DoubleSpinBox: initial value should be greater than minimum value");

    setRange(minval, maxval);
    setSingleStep(0.1);
    setValue(7.0);
}


} // namespace waterial
