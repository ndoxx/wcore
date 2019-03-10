#include <iostream>
#include <QColorDialog>

#include "color_picker_label.h"

namespace medit
{

ColorPickerLabel::ColorPickerLabel(QWidget* parent, Qt::WindowFlags f):
QLabel(parent)
{
    QObject::connect(this, SIGNAL(clicked()),
                     this, SLOT(handle_albedo_color_edit()));

    QFont font = this->font();
    font.setPointSize(10);
    font.setBold(true);
    setFont(font);
    setAlignment(Qt::AlignCenter);
    setObjectName("ColorEdit");
    set_color(Qt::white);
    setMinimumHeight(23);
}

void ColorPickerLabel::mousePressEvent(QMouseEvent* event)
{
    emit clicked();
}

void ColorPickerLabel::handle_albedo_color_edit()
{
    QColor color = QColorDialog::getColor(Qt::white, this);
    if(color.isValid())
        set_color(color);
}

void ColorPickerLabel::set_color(const QColor& color)
{
    color_ = color;
    // Background is selected color, foreground is inverted color for better contrast
    setStyleSheet("QLabel#ColorEdit{ background-color: " + color_.name()
                + "; color: " + QColor(255-color_.red(), 255-color_.green(), 255-color_.blue(), 255).name()
                + "; border: 2px solid black; }");
    setText(color_.name());
}


} // namespace medit
