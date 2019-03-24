#ifndef TWEAKS_GL_WIDGET_H
#define TWEAKS_GL_WIDGET_H

#include "shader_gen_gl_widget.h"

namespace waterial
{

class TweaksGLWidget: public ShaderGenGLWidget
{
    Q_OBJECT

public:
    explicit TweaksGLWidget(QWidget* parent=nullptr);
    virtual ~TweaksGLWidget() override;

public slots:
    void set_hue(double newvalue);
    void set_saturation(double newvalue);
    void set_value(double newvalue);

protected:
    virtual void init() override;

private:
    // uniforms
    float hue_;
    float saturation_;
    float value_;
};

} // namespace waterial

#endif
