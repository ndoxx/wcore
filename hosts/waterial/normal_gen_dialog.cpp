#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

#include "normal_gen_dialog.h"
#include "normal_gen_gl_widget.h"
#include "spinbox.h"

namespace waterial
{

NormalGenDialog::NormalGenDialog(QWidget* parent):
QDialog(parent),
preview_(new NormalGenGLWidget(this))
{
    setWindowTitle(tr("Normal map generator"));

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    QHBoxLayout* ctl_view_layout = new QHBoxLayout();
    QFormLayout* ctl_layout = new QFormLayout();
    QVBoxLayout* preview_layout = new QVBoxLayout();

    QCheckBox* invert_r_cb = new QCheckBox();
    QCheckBox* invert_g_cb = new QCheckBox();
    QCheckBox* invert_h_cb = new QCheckBox();
    invert_r_cb->setCheckState(Qt::Unchecked);
    invert_g_cb->setCheckState(Qt::Unchecked);
    invert_h_cb->setCheckState(Qt::Unchecked);

    QWidget* cboxes = new QWidget();
    QGridLayout* cboxes_layout = new QGridLayout();

    cboxes_layout->setAlignment(Qt::AlignHCenter);
    cboxes_layout->addWidget(new QLabel(tr("R")), 0, 0);
    cboxes_layout->addWidget(new QLabel(tr("G")), 0, 1);
    cboxes_layout->addWidget(new QLabel(tr("H")), 0, 2);
    cboxes_layout->addWidget(invert_r_cb, 1, 0);
    cboxes_layout->addWidget(invert_g_cb, 1, 1);
    cboxes_layout->addWidget(invert_h_cb, 1, 2);
    cboxes->setLayout(cboxes_layout);
    cboxes->setMinimumWidth(50);
    cboxes->setMaximumHeight(55);
    cboxes->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    QComboBox* filter_combo = new QComboBox();
    filter_combo->addItems(QStringList()<<"Sobel"<<"Scharr");
    filter_combo->setMaximumHeight(22);

    DoubleSpinBox* strength_edit = new DoubleSpinBox();
    DoubleSpinBox* level_edit = new DoubleSpinBox();
    DoubleSpinBox* blursharp_edit = new DoubleSpinBox();

    strength_edit->set_constrains(0.01, 5.0, 0.1, 0.6);
    level_edit->set_constrains(4.0, 10.0, 0.1, 7.0);
    blursharp_edit->set_constrains(-10.0, 10.0, 1.0, 0.0);

    ctl_layout->addRow(new QLabel(tr("Filtering")));
    ctl_layout->addRow(tr("Invert:"), cboxes);
    ctl_layout->addRow(tr("Kernel:"), filter_combo);
    ctl_layout->addRow(tr("Level:"), level_edit);
    ctl_layout->addRow(tr("Strength:"), strength_edit);

    ctl_layout->addRow(new QLabel(tr("Blur")));
    ctl_layout->addRow(tr("Blur/Sharp:"), blursharp_edit);

    connect(invert_r_cb, SIGNAL(stateChanged(int)),
            preview_,    SLOT(set_invert_r(int)));
    connect(invert_g_cb, SIGNAL(stateChanged(int)),
            preview_,    SLOT(set_invert_g(int)));
    connect(invert_h_cb, SIGNAL(stateChanged(int)),
            preview_,    SLOT(set_invert_h(int)));

    connect(filter_combo, SIGNAL(currentIndexChanged(int)),
            preview_,     SLOT(set_filter(int)));

    connect(strength_edit, SIGNAL(valueChanged(double)),
            preview_,      SLOT(set_strength(double)));
    connect(level_edit, SIGNAL(valueChanged(double)),
            preview_,   SLOT(set_level(double)));
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

NormalGenDialog::~NormalGenDialog()
{

}

void NormalGenDialog::handle_accept()
{
    preview_->handle_export();
    accept();
}

void NormalGenDialog::set_source_image(const QString& path)
{
    preview_->set_source_image(path);
}

void NormalGenDialog::set_output_image(const QString& path)
{
    preview_->set_output_image(path);
}

const QString& NormalGenDialog::get_output_image_path() const
{
    return preview_->get_output_image_path();
}

}
