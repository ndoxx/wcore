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
    void set_invert(int state);
    void set_strength(double value);
    void set_mean(double value);
    void set_range(double value);
    void set_sigma(double value);

protected:
    virtual void init() override;

private:
    // uniforms
    bool invert_;
    float strength_;
    float mean_;
    float range_;
    float sigma_;
};


} // namespace waterial

#endif // AO_GEN_GL_WIDGET_H
