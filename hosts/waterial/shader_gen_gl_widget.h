#ifndef SHADER_GEN_GL_WIDGET_H
#define SHADER_GEN_GL_WIDGET_H

#include <initializer_list>
#include <QString>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_0_Core>

#include "math3d.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLBuffer)
QT_FORWARD_DECLARE_CLASS(QOpenGLVertexArrayObject)
QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)

namespace waterial
{

class LinearPipeline;
class ShaderGenGLWidget: public QOpenGLWidget, protected QOpenGLFunctions_4_0_Core
{
    Q_OBJECT

public:
    explicit ShaderGenGLWidget(std::initializer_list<std::pair<QString,QString>> shader_sources,
                               QWidget* parent=nullptr);
    virtual ~ShaderGenGLWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    inline void set_clear_color(const wcore::math::vec3& value) { clear_color_ = value; }
    inline const QString& get_output_image_path() const { return out_path_; }
    void set_source_image(const QString& path);
    inline void set_output_image(const QString& path) { out_path_ = path; }

public slots:
    void handle_export();

protected:
    virtual void initializeGL() Q_DECL_OVERRIDE;
    virtual void paintGL() Q_DECL_OVERRIDE;
    virtual void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    virtual void init() = 0;

protected:
    LinearPipeline* pipeline_;
    int img_width_;
    int img_height_;

private:
    wcore::math::vec3 clear_color_;
    std::vector<std::pair<QString,QString>> shader_sources_;

    QString source_path_;
    QString out_path_;
    QOpenGLTexture* source_tex_;
    QOpenGLBuffer* vbo_;
    QOpenGLVertexArrayObject* vao_;

    unsigned int attr_position_;
    bool export_;
};

} // namespace waterial

#endif
