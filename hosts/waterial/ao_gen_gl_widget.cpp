#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLShaderProgram>

#include "ao_gen_gl_widget.h"
#include "linear_pipeline.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

AOGenGLWidget::AOGenGLWidget(QWidget* parent):
ShaderGenGLWidget({
                    {":/res/shaders/passthrough.vert", ":/res/shaders/gen_ao.frag"},
                    {":/res/shaders/passthrough.vert", ":/res/shaders/blur_h.frag"},
                    {":/res/shaders/passthrough.vert", ":/res/shaders/blur_v.frag"},
                  },
                  parent),
invert_(true),
strength_(0.5f),
mean_(1.f),
range_(1.f),
sigma_(0.f)
{

}

AOGenGLWidget::~AOGenGLWidget()
{

}

void AOGenGLWidget::init()
{
    pipeline_->set_uniform_updater(0, [&](QOpenGLShaderProgram* program)
    {
        program->setUniformValue("b_invert", invert_);
        program->setUniformValue("f_strength", strength_);
        program->setUniformValue("f_mean", mean_);
        program->setUniformValue("f_range", range_);
    });
    pipeline_->set_uniform_updater(1, [&](QOpenGLShaderProgram* program)
    {
        program->setUniformValue("f_sigma", (1.f/5.f) * (sigma_/img_width_));
    });
    pipeline_->set_uniform_updater(2, [&](QOpenGLShaderProgram* program)
    {
        program->setUniformValue("f_sigma", (1.f/5.f) * (sigma_/img_height_));
    });
}

void AOGenGLWidget::set_invert(int state)
{
    invert_ = (state == Qt::Checked);
    update();
}

void AOGenGLWidget::set_strength(double value)
{
    strength_ = (float)value;
    update();
}

void AOGenGLWidget::set_mean(double value)
{
    mean_ = (float)value;
    update();
}

void AOGenGLWidget::set_range(double value)
{
    range_ = (float)value;
    update();
}

void AOGenGLWidget::set_sigma(double value)
{
    sigma_ = (float)value;
    update();
}


} // namespace waterial
