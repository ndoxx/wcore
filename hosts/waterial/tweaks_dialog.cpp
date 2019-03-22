#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

#include "tweaks_dialog.h"
#include "tweaks_gl_widget.h"
#include "double_slider.h"

namespace waterial
{

TweaksDialog::TweaksDialog(QWidget* parent):
QDialog(parent),
preview_(new TweaksGLWidget()),
sld_hue_(new DoubleSlider),
sld_saturation_(new DoubleSlider),
sld_value_(new DoubleSlider)
{
    setWindowTitle(tr("Image tweaks"));

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    QHBoxLayout* ctl_view_layout = new QHBoxLayout();
    QFormLayout* ctl_layout = new QFormLayout();
    QVBoxLayout* preview_layout = new QVBoxLayout();

    sld_hue_->setMaximumHeight(20);
    sld_hue_->setMinimumWidth(200);
    sld_hue_->set_range(0.0,1.0);
    sld_hue_->set_value(0.0);
    sld_saturation_->setMaximumHeight(20);
    sld_saturation_->setMinimumWidth(200);
    sld_saturation_->set_range(-1.0,1.0);
    sld_saturation_->set_value(0.0);
    sld_value_->setMaximumHeight(20);
    sld_value_->setMinimumWidth(200);
    sld_value_->set_range(-1.0,1.0);
    sld_value_->set_value(0.0);

    ctl_layout->addRow(new QLabel(tr("HSV Adjust")));
    ctl_layout->addRow(tr("Hue"), sld_hue_);
    ctl_layout->addRow(tr("Saturation"), sld_saturation_);
    ctl_layout->addRow(tr("Value"), sld_value_);

    // Add separator
    auto sep = new QFrame;
    sep->setObjectName("Separator");
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    ctl_layout->addRow(sep);

    connect(sld_hue_, SIGNAL(doubleValueChanged(double)),
            preview_, SLOT(handle_hue_changed(double)));
    connect(sld_saturation_, SIGNAL(doubleValueChanged(double)),
            preview_,        SLOT(handle_saturation_changed(double)));
    connect(sld_value_, SIGNAL(doubleValueChanged(double)),
            preview_,   SLOT(handle_value_changed(double)));

    ctl_view_layout->addLayout(ctl_layout);

    preview_layout->addWidget(preview_);
    ctl_view_layout->addLayout(preview_layout);
    main_layout->addLayout(ctl_view_layout);

    // Add ok/cancel buttons
    QPushButton* ok_btn = new QPushButton(tr("OK"));
    QPushButton* cancel_btn = new QPushButton(tr("Cancel"));
    QHBoxLayout* btn_layout = new QHBoxLayout();
    btn_layout->addWidget(ok_btn);
    btn_layout->addWidget(cancel_btn);
    main_layout->addLayout(btn_layout);

    connect(ok_btn,     SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel_btn, SIGNAL(clicked()), this, SLOT(reject()));
}

TweaksDialog::~TweaksDialog()
{

}

void TweaksDialog::set_clear_color(const wcore::math::vec3& value)
{
    preview_->set_clear_color(value);
}

void TweaksDialog::set_source_image(const QString& path)
{
    preview_->set_source_image(path);
}

void TweaksDialog::reset()
{
    sld_hue_->set_value(0.0);
    sld_saturation_->set_value(0.0);
    sld_value_->set_value(0.0);
}

} // namespace waterial


