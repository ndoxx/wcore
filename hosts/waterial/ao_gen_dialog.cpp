#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

#include "ao_gen_dialog.h"
#include "ao_gen_gl_widget.h"
#include "spinbox.h"

namespace waterial
{

AOGenDialog::AOGenDialog(QWidget* parent):
QDialog(parent),
preview_(new AOGenGLWidget(this))
{
    setWindowTitle(tr("AO map generator"));

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    QHBoxLayout* ctl_view_layout = new QHBoxLayout();
    QFormLayout* ctl_layout = new QFormLayout();
    QVBoxLayout* preview_layout = new QVBoxLayout();

    QCheckBox* invert_cb = new QCheckBox();
    DoubleSpinBox* strength_edit = new DoubleSpinBox();
    DoubleSpinBox* mean_edit = new DoubleSpinBox();
    DoubleSpinBox* range_edit = new DoubleSpinBox();
    DoubleSpinBox* blursharp_edit = new DoubleSpinBox();

    invert_cb->setCheckState(Qt::Checked);
    strength_edit->set_constrains(0.0, 1.0, 0.05, 0.5);
    mean_edit->set_constrains(0.0, 1.0, 0.05, 1.0);
    range_edit->set_constrains(0.0, 1.0, 0.05, 1.0);
    blursharp_edit->set_constrains(-10.0, 10.0, 1.0, 0.0);

    ctl_layout->addRow(new QLabel(tr("Thresholding")));
    ctl_layout->addRow(tr("Invert:"), invert_cb);
    ctl_layout->addRow(tr("Strength:"), strength_edit);
    ctl_layout->addRow(tr("Mean:"), mean_edit);
    ctl_layout->addRow(tr("Range:"), range_edit);

    ctl_layout->addRow(new QLabel(tr("Blur")));
    ctl_layout->addRow(tr("Blur/Sharp:"), blursharp_edit);

    connect(invert_cb, SIGNAL(stateChanged(int)),
            preview_, SLOT(set_invert(int)));
    connect(strength_edit, SIGNAL(valueChanged(double)),
            preview_,      SLOT(set_strength(double)));
    connect(mean_edit, SIGNAL(valueChanged(double)),
            preview_,  SLOT(set_mean(double)));
    connect(range_edit, SIGNAL(valueChanged(double)),
            preview_,   SLOT(set_range(double)));
    connect(blursharp_edit, SIGNAL(valueChanged(double)),
            preview_,       SLOT(set_sigma(double)));

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

    connect(ok_btn,     SIGNAL(clicked()), this, SLOT(handle_accept()));
    connect(cancel_btn, SIGNAL(clicked()), this, SLOT(reject()));
}

AOGenDialog::~AOGenDialog()
{

}

void AOGenDialog::handle_accept()
{
    preview_->handle_export();
    accept();
}

void AOGenDialog::set_source_image(const QString& path)
{
    preview_->set_source_image(path);
}

void AOGenDialog::set_output_image(const QString& path)
{
    preview_->set_output_image(path);
}

const QString& AOGenDialog::get_output_image_path() const
{
    return preview_->get_output_image_path();
}

}
