#ifndef TWEAKS_GL_WIDGET_H
#define TWEAKS_GL_WIDGET_H

#include <QOpenGLWidget>

namespace waterial
{

class TweaksGLWidget: public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit TweaksGLWidget(QWidget* parent=nullptr);
    ~TweaksGLWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

private:

};

} // namespace waterial

#endif
