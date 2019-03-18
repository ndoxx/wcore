#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

#include "preview_controls.h"
#include "gl_widget.h"
#include "double_slider.h"
#include "color_picker_label.h"

namespace waterial
{

PreviewControlWidget::PreviewControlWidget(GLWidget* preview, QWidget* parent):
QGroupBox(parent)
{
    QGridLayout* layout = new QGridLayout();

    setTitle(tr("Preview controls"));

    // * General controls
    auto* gb_general = create_general_controls(preview);
    layout->addWidget(gb_general, 0, 0, 1, 3); // Span all columns
    layout->setRowStretch(0, 0);
    layout->setColumnStretch(0, 1);

    // * Model controls
    auto* gb_model = create_model_controls(preview);
    layout->addWidget(gb_model, 1, 0);
    layout->setRowStretch(1, 1);

    // * Light controls
    auto* gb_light = create_light_controls(preview);
    layout->addWidget(gb_light, 1, 1);
    layout->setColumnStretch(1, 1);

    // * Camera controls
    auto* gb_camera = create_camera_controls(preview);
    layout->addWidget(gb_camera, 1, 2);
    layout->setColumnStretch(2, 1);

    setLayout(layout);
}

QGroupBox* PreviewControlWidget::create_general_controls(GLWidget* preview)
{
    QHBoxLayout* layout_gen = new QHBoxLayout();
    QGroupBox* gb_general = new QGroupBox(tr("General"));

    // Active preview
    QCheckBox* cb_active = new QCheckBox(tr("Active"));
    cb_active->setCheckState(Qt::Checked);
    connect(cb_active, SIGNAL(stateChanged(int)),
            preview,   SLOT(handle_active_changed(int)));
    layout_gen->addWidget(cb_active);

    gb_general->setLayout(layout_gen);

    return gb_general;
}

QGroupBox* PreviewControlWidget::create_model_controls(GLWidget* preview)
{
    QFormLayout* layout_model = new QFormLayout();
    QGroupBox* gb_model = new QGroupBox(tr("Model"));

    // Enable rotation
    QCheckBox* cb_rotate = new QCheckBox();
    cb_rotate->setCheckState(Qt::Checked);
    connect(cb_rotate, SIGNAL(stateChanged(int)),
            preview,   SLOT(handle_rotate_changed(int)));
    layout_model->addRow(new QLabel(tr("Rotate")), cb_rotate);

    // Reset orientation
    QPushButton* btn_reset_ori = new QPushButton(tr("Reset orientation"));
    btn_reset_ori->setMaximumHeight(20);
    connect(btn_reset_ori, SIGNAL(clicked()),
            preview,       SLOT(handle_reset_orientation()));
    layout_model->addWidget(btn_reset_ori);

    // Rotation parameters
    DoubleSlider* sld_dphi = new DoubleSlider();
    DoubleSlider* sld_dtheta = new DoubleSlider();
    DoubleSlider* sld_dpsi = new DoubleSlider();
    sld_dphi->setMaximumHeight(20);
    sld_dphi->set_value(0.0);
    sld_dtheta->setMaximumHeight(20);
    sld_dtheta->set_value(0.5);
    sld_dpsi->setMaximumHeight(20);
    sld_dpsi->set_value(0.0);
    connect(sld_dphi,   SIGNAL(doubleValueChanged(double)),
            preview,    SLOT(handle_dphi_changed(double)));
    connect(sld_dtheta, SIGNAL(doubleValueChanged(double)),
            preview,    SLOT(handle_dtheta_changed(double)));
    connect(sld_dpsi,   SIGNAL(doubleValueChanged(double)),
            preview,    SLOT(handle_dpsi_changed(double)));

    layout_model->addRow(new QLabel(QString::fromUtf8("\u03C6")), sld_dphi);
    layout_model->addRow(new QLabel(QString::fromUtf8("\u03B8")), sld_dtheta);
    layout_model->addRow(new QLabel(QString::fromUtf8("\u03C8")), sld_dpsi);

    // Position parameters
    DoubleSlider* sld_x = new DoubleSlider();
    DoubleSlider* sld_y = new DoubleSlider();
    DoubleSlider* sld_z = new DoubleSlider();
    sld_x->setMaximumHeight(20);
    sld_x->set_range(-2.0,2.0);
    sld_x->set_value(0.0);
    sld_y->setMaximumHeight(20);
    sld_y->set_range(-2.0,2.0);
    sld_y->set_value(0.0);
    sld_z->setMaximumHeight(20);
    sld_z->set_range(-1.0,2.0);
    sld_z->set_value(0.0);
    connect(sld_x,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_x_changed(double)));
    connect(sld_y,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_y_changed(double)));
    connect(sld_z,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_z_changed(double)));

    layout_model->addRow(new QLabel(tr("x")), sld_x);
    layout_model->addRow(new QLabel(tr("y")), sld_y);
    layout_model->addRow(new QLabel(tr("z")), sld_z);

    gb_model->setLayout(layout_model);

    return gb_model;
}

QGroupBox* PreviewControlWidget::create_light_controls(GLWidget* preview)
{
    QFormLayout* layout_light = new QFormLayout();
    QGroupBox* gb_light = new QGroupBox(tr("Light"));

    DoubleSlider* sld_r = new DoubleSlider();
    DoubleSlider* sld_b = new DoubleSlider();
    DoubleSlider* sld_x = new DoubleSlider();
    DoubleSlider* sld_y = new DoubleSlider();
    DoubleSlider* sld_z = new DoubleSlider();

    sld_r->setMaximumHeight(20);
    sld_r->set_range(0.0,10.0);
    sld_r->set_value(5.0);
    sld_b->setMaximumHeight(20);
    sld_b->set_range(0.0,50.0);
    sld_b->set_value(10.0);
    sld_x->setMaximumHeight(20);
    sld_x->set_range(-3.0,3.0);
    sld_x->set_value(0.0);
    sld_y->setMaximumHeight(20);
    sld_y->set_range(-5.0,5.0);
    sld_y->set_value(2.0);
    sld_z->setMaximumHeight(20);
    sld_z->set_range(-3.0,3.0);
    sld_z->set_value(0.0);
    connect(sld_r,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_radius_changed(double)));
    connect(sld_b,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_brightness_changed(double)));
    connect(sld_x,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_x_changed(double)));
    connect(sld_y,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_y_changed(double)));
    connect(sld_z,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_z_changed(double)));

    ColorPickerLabel* color_picker = new ColorPickerLabel();
    connect(color_picker, SIGNAL(sig_value_changed(QColor)),
            preview,      SLOT(handle_light_color_changed(QColor)));

    layout_light->addRow(new QLabel(tr("Color")), color_picker);
    layout_light->addRow(new QLabel(tr("Radius")), sld_r);
    layout_light->addRow(new QLabel(tr("Brightness")), sld_b);
    layout_light->addRow(new QLabel(tr("x")), sld_x);
    layout_light->addRow(new QLabel(tr("y")), sld_y);
    layout_light->addRow(new QLabel(tr("z")), sld_z);

    gb_light->setLayout(layout_light);

    return gb_light;
}

QGroupBox* PreviewControlWidget::create_camera_controls(GLWidget* preview)
{
    QFormLayout* layout_camera = new QFormLayout();
    QGroupBox* gb_camera = new QGroupBox(tr("Camera"));

    gb_camera->setLayout(layout_camera);

    return gb_camera;
}



} // namespace waterial
