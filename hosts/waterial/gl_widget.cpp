#include <GL/glew.h>
#include <QSurfaceFormat>
#include <QTimer>
#include <QCheckBox>

#include "gl_widget.h"
#include "qt_context.h"
#include "wcore.h"
#include "logger.h"
#include "error.h"
#include "globals.h"
#include "model.h"
#include "material.h"

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
dphi_(0.0f),
dtheta_(0.5f),
dpsi_(0.0f),
model_x_(0.f),
model_y_(0.f),
model_z_(0.f),
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
        DLOGF("Failed to initialize GLEW.", "core", Severity::CRIT);
        fatal("Failed to initialize GLEW.");
    }
    glGetError();   // Mask an unavoidable error caused by GLEW

    GLB.START_LEVEL = "mv";
    GLB.SCR_W = 800;
    GLB.SCR_H = 600;

    engine_->Init(0, nullptr, nullptr, context_);
    engine_->scene_control->LoadStart();

    // Systems configuration
    engine_->pipeline_control->SetShadowMappingEnabled(false);

    connect(frame_timer_, SIGNAL(timeout()), this, SLOT(update()));
    frame_timer_->start(16);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(!active_) return;

    engine_->pipeline_control->SetDefaultFrameBuffer(defaultFramebufferObject());

    // Update model
    engine_->scene_control->VisitRefModel("the_model"_h, [&](Model& model)
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
        model.set_position(math::vec3(model_x_, model_y_, model_z_));

        // Swap material?
        if(new_material_)
        {
            model.set_material(new_material_);
            new_material_ = nullptr;
        }
    });

    engine_->Update(16.67/1000.f);
    engine_->RenderFrame();
    engine_->FinishFrame();
}

void GLWidget::resizeGL(int width, int height)
{
    GLB.WIN_W = width;
    GLB.WIN_H = height;
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
    model_x_ = (float)newvalue;
}

void GLWidget::handle_y_changed(double newvalue)
{
    model_y_ = (float)newvalue;
}

void GLWidget::handle_z_changed(double newvalue)
{
    model_z_ = (float)newvalue;
}

void GLWidget::handle_material_swap(const wcore::MaterialDescriptor& descriptor)
{
    new_material_ = new Material(descriptor);
}


} // namespace waterial
