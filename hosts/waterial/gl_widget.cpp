#include <GL/glew.h>
#include <QSurfaceFormat>

#include "gl_widget.h"
#include "qt_context.h"
#include "wcore.h"
#include "logger.h"
#include "error.h"
#include "globals.h"

using namespace wcore;

namespace medit
{

GLWidget::GLWidget(QWidget* parent):
QOpenGLWidget(parent),
engine_(new wcore::Engine),
context_(new QtContext)
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

void GLWidget::cleanup()
{
    makeCurrent();

    // do cleanup

    doneCurrent();
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
    GLB.WIN_W = width();
    GLB.WIN_H = height();
    GLB.SCR_W = width();
    GLB.SCR_H = height();

    engine_->Init(0, nullptr, nullptr, context_);
    engine_->LoadStart();
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    engine_->SetDefaultFrameBuffer(defaultFramebufferObject());
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


} // namespace medit
