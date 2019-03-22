#include <iostream>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>

#include "preview_controls.h"
#include "preview_gl_widget.h"
#include "double_slider.h"
#include "color_picker_label.h"

namespace waterial
{

PreviewControlWidget::PreviewControlWidget(PreviewGLWidget* preview, QWidget* parent):
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

QGroupBox* PreviewControlWidget::create_general_controls(PreviewGLWidget* preview)
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

QGroupBox* PreviewControlWidget::create_model_controls(PreviewGLWidget* preview)
{
    QFormLayout* layout_model = new QFormLayout();
    QGroupBox* gb_model = new QGroupBox(tr("Model"));

    // Enable rotation
    QCheckBox* cb_rotate = new QCheckBox();
    cb_rotate->setCheckState(Qt::Checked);
    connect(cb_rotate, SIGNAL(stateChanged(int)),
            preview,   SLOT(handle_rotate_changed(int)));
    layout_model->addRow(tr("Rotate"), cb_rotate);

    // Reset orientation
    QPushButton* btn_reset_ori = new QPushButton(tr("Reset orientation"));
    btn_reset_ori->setMaximumHeight(20);
    connect(btn_reset_ori, SIGNAL(clicked()),
            preview,       SLOT(handle_reset_orientation()));
    layout_model->addRow(btn_reset_ori);

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

    layout_model->addRow(QString::fromUtf8("\u03C6"), sld_dphi);
    layout_model->addRow(QString::fromUtf8("\u03B8"), sld_dtheta);
    layout_model->addRow(QString::fromUtf8("\u03C8"), sld_dpsi);

    // Reset position
    QPushButton* btn_reset_pos = new QPushButton(tr("Reset position"));
    btn_reset_pos->setMaximumHeight(20);
    connect(btn_reset_pos, SIGNAL(clicked()),
            this,          SLOT(handle_reset_model_position()));
    layout_model->addRow(btn_reset_pos);

    // Position parameters
    sld_mod_x_ = new DoubleSlider();
    sld_mod_y_ = new DoubleSlider();
    sld_mod_z_ = new DoubleSlider();

    sld_mod_x_->setMaximumHeight(20);
    sld_mod_x_->set_range(-2.0,2.0);
    sld_mod_x_->set_value(0.0);
    sld_mod_y_->setMaximumHeight(20);
    sld_mod_y_->set_range(-2.0,2.0);
    sld_mod_y_->set_value(0.0);
    sld_mod_z_->setMaximumHeight(20);
    sld_mod_z_->set_range(-1.0,2.0);
    sld_mod_z_->set_value(0.0);
    connect(sld_mod_x_, SIGNAL(doubleValueChanged(double)),
            preview,    SLOT(handle_x_changed(double)));
    connect(sld_mod_y_, SIGNAL(doubleValueChanged(double)),
            preview,    SLOT(handle_y_changed(double)));
    connect(sld_mod_z_, SIGNAL(doubleValueChanged(double)),
            preview,    SLOT(handle_z_changed(double)));

    layout_model->addRow(tr("x"), sld_mod_x_);
    layout_model->addRow(tr("y"), sld_mod_y_);
    layout_model->addRow(tr("z"), sld_mod_z_);

    gb_model->setLayout(layout_model);

    return gb_model;
}

QGroupBox* PreviewControlWidget::create_light_controls(PreviewGLWidget* preview)
{
    QFormLayout* layout_light = new QFormLayout();
    QGroupBox* gb_light = new QGroupBox(tr("Light"));

    // Light type combo box
    combo_light_type_ = new QComboBox();
    combo_light_type_->addItems(QStringList()<<"Point"<<"Directional");
    combo_light_type_->setMaximumHeight(22);
    connect(combo_light_type_, SIGNAL(currentIndexChanged(int)),
            this,              SLOT(handle_light_type_changed(int)));
    connect(this,    SIGNAL(sig_light_type_changed(int)),
            preview, SLOT(handle_light_type_changed(int)));
    layout_light->addRow(tr("Type"), combo_light_type_);

    DoubleSlider* sld_bright = new DoubleSlider();
    DoubleSlider* sld_ambient = new DoubleSlider();

    point_light_controls_ = new QWidget();
    dir_light_controls_ = new QWidget();
    QFormLayout* layout_point_light = new QFormLayout();
    QFormLayout* layout_dir_light = new QFormLayout();

    point_light_controls_->setObjectName("InternalWidget");
    dir_light_controls_->setObjectName("InternalWidget");

    DoubleSlider* sld_r = new DoubleSlider();
    DoubleSlider* sld_x = new DoubleSlider();
    DoubleSlider* sld_y = new DoubleSlider();
    DoubleSlider* sld_z = new DoubleSlider();

    layout_point_light->addRow(tr("Radius"), sld_r);
    layout_point_light->addRow(tr("x"), sld_x);
    layout_point_light->addRow(tr("y"), sld_y);
    layout_point_light->addRow(tr("z"), sld_z);

    DoubleSlider* sld_incl = new DoubleSlider();
    DoubleSlider* sld_peri = new DoubleSlider();

    layout_dir_light->addRow(tr("Inclination"), sld_incl);
    layout_dir_light->addRow(tr("Arg. perihelion"), sld_peri);

    point_light_controls_->setLayout(layout_point_light);
    dir_light_controls_->setLayout(layout_dir_light);
    dir_light_controls_->hide();

    sld_bright->setMaximumHeight(20);
    sld_bright->set_range(0.0,50.0);
    sld_bright->set_value(10.0);
    sld_ambient->setMaximumHeight(20);
    sld_ambient->set_range(0.0,0.5);
    sld_ambient->set_value(0.03);
    sld_r->setMaximumHeight(20);
    sld_r->set_range(0.0,10.0);
    sld_r->set_value(5.0);
    sld_x->setMaximumHeight(20);
    sld_x->set_range(-3.0,3.0);
    sld_x->set_value(0.0);
    sld_y->setMaximumHeight(20);
    sld_y->set_range(-5.0,5.0);
    sld_y->set_value(2.0);
    sld_z->setMaximumHeight(20);
    sld_z->set_range(-3.0,3.0);
    sld_z->set_value(0.0);
    sld_incl->setMaximumHeight(20);
    sld_incl->set_range(0.0,M_PI);
    sld_incl->set_value(85.0 * M_PI / 180.0);
    sld_peri->setMaximumHeight(20);
    sld_peri->set_range(0.0,2*M_PI);
    sld_peri->set_value(90.0 * M_PI / 180.0);
    connect(sld_r,   SIGNAL(doubleValueChanged(double)),
            preview,    SLOT(handle_light_radius_changed(double)));
    connect(sld_bright, SIGNAL(doubleValueChanged(double)),
            preview,     SLOT(handle_light_brightness_changed(double)));
    connect(sld_ambient, SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_ambient_changed(double)));
    connect(sld_x,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_x_changed(double)));
    connect(sld_y,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_y_changed(double)));
    connect(sld_z,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_light_z_changed(double)));
    connect(sld_incl, SIGNAL(doubleValueChanged(double)),
            preview,  SLOT(handle_light_inclination_changed(double)));
    connect(sld_peri, SIGNAL(doubleValueChanged(double)),
            preview,  SLOT(handle_light_perihelion_changed(double)));

    ColorPickerLabel* color_picker = new ColorPickerLabel();
    connect(color_picker, SIGNAL(sig_value_changed(QColor)),
            preview,      SLOT(handle_light_color_changed(QColor)));

    layout_light->addRow(tr("Color"), color_picker);
    layout_light->addRow(tr("Brightness"), sld_bright);
    layout_light->addRow(tr("Ambient"), sld_ambient);

    layout_light->addRow(point_light_controls_);
    layout_light->addRow(dir_light_controls_);

    // Add separator
    auto sep = new QFrame;
    sep->setObjectName("Separator");
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    layout_light->addRow(sep);

    // Bloom control
    QCheckBox* cb_bloom = new QCheckBox();
    cb_bloom->setCheckState(Qt::Checked);
    connect(cb_bloom, SIGNAL(stateChanged(int)),
            preview,  SLOT(handle_bloom_changed(int)));

    layout_light->addRow(tr("Enable bloom"), cb_bloom);

    gb_light->setLayout(layout_light);

    return gb_light;
}

QGroupBox* PreviewControlWidget::create_camera_controls(PreviewGLWidget* preview)
{
    QFormLayout* layout_camera = new QFormLayout();
    QGroupBox* gb_camera = new QGroupBox(tr("Camera"));

    // Reset orientation
    QPushButton* btn_reset_ori = new QPushButton(tr("Reset orientation"));
    btn_reset_ori->setMaximumHeight(20);
    connect(btn_reset_ori, SIGNAL(clicked()),
            this,          SLOT(handle_reset_cam_orientation()));
    layout_camera->addRow(btn_reset_ori);

    sld_cam_r_ = new DoubleSlider();
    sld_cam_theta_ = new DoubleSlider();
    sld_cam_phi_ = new DoubleSlider();

    sld_cam_r_->setMaximumHeight(20);
    sld_cam_r_->set_range(0.0,5.0);
    sld_cam_r_->set_value(1.5);
    sld_cam_theta_->setMaximumHeight(20);
    sld_cam_theta_->set_range(5.0*M_PI/180.f,M_PI);
    sld_cam_theta_->set_value(M_PI / 4.0f);
    sld_cam_phi_->setMaximumHeight(20);
    sld_cam_phi_->set_range(0.0,2*M_PI);
    sld_cam_phi_->set_value(M_PI);

    connect(sld_cam_r_,   SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_cam_radius_changed(double)));
    connect(sld_cam_theta_, SIGNAL(doubleValueChanged(double)),
            preview,   SLOT(handle_cam_inclination_changed(double)));
    connect(sld_cam_phi_, SIGNAL(doubleValueChanged(double)),
            preview, SLOT(handle_cam_azimuth_changed(double)));

    layout_camera->addRow(tr("Radius"), sld_cam_r_);
    layout_camera->addRow(tr("Inclination"), sld_cam_theta_);
    layout_camera->addRow(tr("Azimuth"), sld_cam_phi_);

    gb_camera->setLayout(layout_camera);

    return gb_camera;
}

void PreviewControlWidget::handle_light_type_changed(int newvalue)
{
    if(newvalue == 0) // Point light
    {
        point_light_controls_->show();
        dir_light_controls_->hide();
    }
    else if(newvalue == 1) // Directional light
    {
        dir_light_controls_->show();
        point_light_controls_->hide();
    }
    emit sig_light_type_changed(newvalue);
}

void PreviewControlWidget::handle_reset_cam_orientation()
{
    sld_cam_r_->set_value(2.0);
    sld_cam_theta_->set_value(M_PI / 4.0f);
    sld_cam_phi_->set_value(M_PI);
}

void PreviewControlWidget::handle_reset_model_position()
{
    sld_mod_x_->set_value(0.0);
    sld_mod_y_->set_value(0.0);
    sld_mod_z_->set_value(0.0);
}


} // namespace waterial
