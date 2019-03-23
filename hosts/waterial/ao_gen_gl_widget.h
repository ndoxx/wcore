#ifndef AO_GEN_GL_WIDGET_H
#define AO_GEN_GL_WIDGET_H

#include "shader_gen_gl_widget.h"

namespace waterial
{

class AOGenGLWidget: public ShaderGenGLWidget
{
    Q_OBJECT

public:
    explicit AOGenGLWidget(QWidget* parent=nullptr);
    virtual ~AOGenGLWidget() override;

public slots:

protected:
    virtual void update_uniforms() override;

private:
    // uniforms

};


} // namespace waterial

#endif // AO_GEN_GL_WIDGET_H
