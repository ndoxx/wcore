#include <GL/glew.h>
#include <QSurfaceFormat>
#include <QTimer>
#include <QCheckBox>

#include "gl_widget.h"
#include "qt_context.h"
#include "wcore.h"
#include "logger.h"
#include "error.h"
#include "model.h"
#include "material.h"
#include "lights.h"

using namespace wcore;

namespace waterial
{

GLWidget::GLWidget(QWidget* parent):
QOpenGLWidget(parent),
engine_(new wcore::Engine),
context_(new QtContext),
frame_timer_(new QTimer(this)),
active_(true),
rotate_model_(true),
reset_orientation_(false),
show_light_proxy_(false),
dphi_(0.0f),
dtheta_(0.5f),
dpsi_(0.0f),
model_pos_(0.f),
light_pos_(0.f,2.f,0.f),
light_color_(1.f,1.f,1.f),
light_brightness_(10.f),
light_amb_(0.03f),
light_radius_(5.f),
light_inclination_(85.f * M_PI / 180.f),
light_perihelion_(90.f * M_PI / 180.f),
light_type_(0),
new_material_(nullptr)
{

}

GLWidget::~GLWidget()
{
    if(new_material_)
        delete new_material_;
    delete engine_;
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(320, 240);
}

void GLWidget::initializeGL()
{
    //connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

    // Initialize GLEW
    glewExperimental = GL_TRUE; // If not set, segfault at glGenVertexArrays()
    if (glewInit() != GLEW_OK) {
        DLOGF("Failed to initialize GLEW.", "waterial", Severity::CRIT);
        fatal("Failed to initialize GLEW.");
    }
    glGetError();   // Mask an unavoidable error caused by GLEW

    engine_->SetFrameSize(800, 600);
    engine_->Init(0, nullptr, nullptr, context_);
    engine_->scene->LoadStart("mv");

    // Systems configuration
    engine_->pipeline->SetShadowMappingEnabled(false);
    engine_->pipeline->SetDirectionalLightEnabled(false);
    engine_->pipeline->SetDaylightSystemEnabled(false);

    connect(frame_timer_, SIGNAL(timeout()), this, SLOT(update()));
    frame_timer_->start(16);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(!active_) return;

    engine_->pipeline->SetDefaultFrameBuffer(defaultFramebufferObject());

    // Update model
    engine_->scene->VisitModelRef("the_model"_h, [&](Model& model)
    {
        // Reset orientation?
        if(reset_orientation_)
        {
            model.reset_orientation();
            reset_orientation_ = false;
        }

        if(rotate_model_)
            model.rotate(dphi_,dtheta_,dpsi_);

        // Set model position
        model.set_position(model_pos_);

        // Swap material?
        if(new_material_)
        {
            model.set_material(new_material_);
            new_material_ = nullptr;
        }
    });

    // Update light
    engine_->scene->VisitLightRef("the_point_light"_h, [&](Light& light)
    {
        light.set_position(light_pos_);
        light.set_radius(light_radius_);
        light.set_color(light_color_);
        light.set_ambient_strength(light_amb_);
        light.set_brightness((light_type_==0) ? light_brightness_ : 0.f);
    });
    engine_->pipeline->dShowLightProxy(1);

    Light& dirlight = engine_->scene->GetDirectionalLight();
    math::vec3 sun_pos(cos(light_inclination_),
                       sin(light_inclination_)*sin(light_perihelion_),
                       sin(light_inclination_)*cos(light_perihelion_));
    sun_pos.normalize();
    dirlight.set_position(sun_pos);
    dirlight.set_color(light_color_);
    dirlight.set_ambient_strength(light_amb_);
    dirlight.set_brightness(light_brightness_);

    engine_->Update(16.67/1000.f);
    engine_->RenderFrame();
    engine_->FinishFrame();
}

void GLWidget::resizeGL(int width, int height)
{
    engine_->SetWindowSize(width, height);
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{

}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{

}


// Slots
void GLWidget::cleanup()
{
    makeCurrent();

    // do cleanup

    doneCurrent();
}

void GLWidget::handle_active_changed(int newstate)
{
    active_ = (newstate == Qt::Checked);
}

void GLWidget::handle_rotate_changed(int newstate)
{
    rotate_model_ = (newstate == Qt::Checked);
}

void GLWidget::handle_dphi_changed(double newvalue)
{
    dphi_ = (float)newvalue;
}

void GLWidget::handle_dtheta_changed(double newvalue)
{
    dtheta_ = (float)newvalue;
}

void GLWidget::handle_dpsi_changed(double newvalue)
{
    dpsi_ = (float)newvalue;
}

void GLWidget::handle_reset_orientation()
{
    reset_orientation_ = true;
}

void GLWidget::handle_x_changed(double newvalue)
{
    model_pos_[0] = -(float)newvalue;
}

void GLWidget::handle_y_changed(double newvalue)
{
    model_pos_[1] = (float)newvalue;
}

void GLWidget::handle_z_changed(double newvalue)
{
    model_pos_[2] = (float)newvalue;
}

void GLWidget::handle_material_swap(const wcore::MaterialDescriptor& descriptor)
{
    new_material_ = new Material(descriptor);
}

void GLWidget::handle_light_x_changed(double newvalue)
{
    light_pos_[0] = -(float)newvalue;
}

void GLWidget::handle_light_y_changed(double newvalue)
{
    light_pos_[1] = (float)newvalue;
}

void GLWidget::handle_light_z_changed(double newvalue)
{
    light_pos_[2] = (float)newvalue;
}

void GLWidget::handle_light_radius_changed(double newvalue)
{
    light_radius_ = (float)newvalue;
}

void GLWidget::handle_light_brightness_changed(double newvalue)
{
    light_brightness_ = (float)newvalue;
}

void GLWidget::handle_light_ambient_changed(double newvalue)
{
    light_amb_ = (float)newvalue;
}

void GLWidget::handle_light_inclination_changed(double newvalue)
{
    light_inclination_ = (float)newvalue;
}

void GLWidget::handle_light_perihelion_changed(double newvalue)
{
    light_perihelion_ = (float)newvalue;
}

void GLWidget::handle_light_color_changed(QColor newvalue)
{
    light_color_ = math::vec3(newvalue.red()/255.f,
                              newvalue.green()/255.f,
                              newvalue.blue()/255.f);
}

void GLWidget::handle_light_type_changed(int newvalue)
{
    light_type_ = newvalue;

    if(light_type_ == 0) // Point
    {
        engine_->pipeline->SetDirectionalLightEnabled(false);
    }
    else if(light_type_ == 1) // Directional
    {
        engine_->pipeline->SetDirectionalLightEnabled(true);
    }
}

void GLWidget::handle_bloom_changed(int newvalue)
{
    engine_->pipeline->SetBloomEnabled(newvalue == Qt::Checked);
}

} // namespace waterial
