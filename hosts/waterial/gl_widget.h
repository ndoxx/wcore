#ifndef GL_WIDGET_H
#define GL_WIDGET_H

#include <QOpenGLWidget>
//#include <QOpenGLFunctions_4_0_Core>

namespace wcore
{
class Engine;
}

namespace medit
{

class QtContext;
class GLWidget : public QOpenGLWidget//, protected QOpenGLFunctions_4_0_Core
{
    Q_OBJECT

public:
    explicit GLWidget(QWidget* parent = 0);
    ~GLWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;

public slots:
    void cleanup();

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

private:
    wcore::Engine* engine_;
    QtContext* context_;
};

} // namespace medit

#endif // GL_WIDGET_H
