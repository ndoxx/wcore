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
    void set_invert_r(int state);
    void set_invert_g(int state);
    void set_invert_h(int state);
    void set_filter(int state);
    void set_level(double value);
    void set_strength(double value);
    void set_sigma(double value);

protected:
    virtual void init() override;

private:
    // uniforms
    bool invert_r_;
    bool invert_g_;
    bool invert_h_;
    int filter_;
    float level_;
    float strength_;
    float sigma_;
};

} // namespace waterial

#endif // NORMAL_GEN_GL_WIDGET_H
