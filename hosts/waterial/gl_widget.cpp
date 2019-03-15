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

using namespace wcore;

namespace medit
{

GLWidget::GLWidget(QWidget* parent):
QOpenGLWidget(parent),
engine_(new wcore::Engine),
context_(new QtContext),
frame_timer_(new QTimer(this)),
active_(true),
rotate_model_(true),
dphi_(0.1f),
dtheta_(0.1f),
dpsi_(0.1f)
{

}

GLWidget::~GLWidget()
{
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
    engine_->LoadStart();

    connect(frame_timer_, SIGNAL(timeout()), this, SLOT(update()));
    frame_timer_->start(16);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(!active_) return;

    engine_->SetDefaultFrameBuffer(defaultFramebufferObject());

    // Update model
    engine_->VisitRefModel("the_model"_h, [&](Model& model)
    {
        if(rotate_model_)
            model.rotate(dphi_,dtheta_,dpsi_);
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


} // namespace medit
