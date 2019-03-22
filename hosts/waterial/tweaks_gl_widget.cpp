#include "tweaks_gl_widget.h"

namespace waterial
{

TweaksGLWidget::TweaksGLWidget(QWidget* parent):
QOpenGLWidget(parent)
{

}

TweaksGLWidget::~TweaksGLWidget()
{

}

QSize TweaksGLWidget::minimumSizeHint() const
{
    return QSize(256, 256);
}

void TweaksGLWidget::initializeGL()
{

}

void TweaksGLWidget::paintGL()
{
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void TweaksGLWidget::resizeGL(int width, int height)
{

}

void TweaksGLWidget::mousePressEvent(QMouseEvent* event)
{

}

void TweaksGLWidget::mouseMoveEvent(QMouseEvent* event)
{

}

} // namespace waterial
