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
    void handle_hue_changed(double newvalue);
    void handle_saturation_changed(double newvalue);
    void handle_value_changed(double newvalue);

protected:
    virtual void update_uniforms() override;

private:
    // uniforms
    float hue_;
    float saturation_;
    float value_;
};

} // namespace waterial

#endif
