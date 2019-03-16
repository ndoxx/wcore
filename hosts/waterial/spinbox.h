#ifndef SPINBOX_H
#define SPINBOX_H

#include <QDoubleSpinBox>

namespace waterial
{

class DoubleSpinBox: public QDoubleSpinBox
{
    Q_OBJECT

public:
    explicit DoubleSpinBox(QWidget* parent = nullptr);

    void set_constrains(double minval, double maxval, double step=0.1, double value=0.0);
};


} // namespace waterial

#endif // SPINBOX_H
