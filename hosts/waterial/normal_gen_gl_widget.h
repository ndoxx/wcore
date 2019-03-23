#ifndef NORMAL_GEN_GL_WIDGET_H
#define NORMAL_GEN_GL_WIDGET_H

#include "shader_gen_gl_widget.h"

namespace waterial
{

class NormalGenGLWidget: public ShaderGenGLWidget
{
    Q_OBJECT

public:
    explicit NormalGenGLWidget(QWidget* parent=nullptr);
    virtual ~NormalGenGLWidget() override;

public slots:

protected:
    //virtual QString make_out_path(const QString& in_path) override;
    virtual void update_uniforms() override;

private:
    // uniforms

};

} // namespace waterial

#endif // NORMAL_GEN_GL_WIDGET_H
