#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLShaderProgram>

#include "tweaks_gl_widget.h"
#include "linear_pipeline.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

TweaksGLWidget::TweaksGLWidget(QWidget* parent):
ShaderGenGLWidget(":/res/shaders/tweaks.vert",
                  ":/res/shaders/tweaks.frag",
                  parent),
hue_(0.f),
saturation_(0.f),
value_(0.f)
{

}

TweaksGLWidget::~TweaksGLWidget()
{

}

void TweaksGLWidget::init()
{
    pipeline_->set_uniform_updater(0, [&](QOpenGLShaderProgram* program)
    {
        program->setUniformValue("f_hue", hue_);
        program->setUniformValue("f_saturation", saturation_);
        program->setUniformValue("f_value", value_);
    });
}

void TweaksGLWidget::handle_hue_changed(double newvalue)
{
    hue_ = float(newvalue);
    update();
}

void TweaksGLWidget::handle_saturation_changed(double newvalue)
{
    saturation_ = float(newvalue);
    update();
}

void TweaksGLWidget::handle_value_changed(double newvalue)
{
    value_ = float(newvalue);
    update();
}


} // namespace waterial
