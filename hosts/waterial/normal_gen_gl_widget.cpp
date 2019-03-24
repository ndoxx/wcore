#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLShaderProgram>

#include "normal_gen_gl_widget.h"
#include "linear_pipeline.h"
#include "algorithms.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

NormalGenGLWidget::NormalGenGLWidget(QWidget* parent):
ShaderGenGLWidget({
                    {":/res/shaders/passthrough.vert", ":/res/shaders/gen_normal.frag"},
                    {":/res/shaders/passthrough.vert", ":/res/shaders/blur_h.frag"},
                    {":/res/shaders/passthrough.vert", ":/res/shaders/blur_v.frag"},
                  },
                  parent),
invert_r_(false),
invert_g_(false),
invert_h_(false),
filter_(0),
level_(7.0f),
strength_(0.6f),
sigma_(0.f)
{

}

NormalGenGLWidget::~NormalGenGLWidget()
{

}

static float dz_from_level_strength(float level, float strength)
{
    return math::clamp<float>(1.f/strength * (1.f + pow(2.f, level)) * (1.f/255.f), 0.f, 1.f);
}

void NormalGenGLWidget::init()
{
    pipeline_->set_uniform_updater(0, [&](QOpenGLShaderProgram* program)
    {
        program->setUniformValue("f_invert_r", invert_r_ ? -1.f : 1.f);
        program->setUniformValue("f_invert_g", invert_g_ ? -1.f : 1.f);
        program->setUniformValue("f_invert_h", invert_h_ ? -1.f : 1.f);
        program->setUniformValue("i_filter", filter_);
        program->setUniformValue("f_step_x", 1.f/img_width_);
        program->setUniformValue("f_step_y", 1.f/img_height_);
        program->setUniformValue("f_dz", dz_from_level_strength(level_, strength_));
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

void NormalGenGLWidget::set_invert_r(int state)
{
    invert_r_ = (state == Qt::Checked);
    update();
}

void NormalGenGLWidget::set_invert_g(int state)
{
    invert_g_ = (state == Qt::Checked);
    update();
}

void NormalGenGLWidget::set_invert_h(int state)
{
    invert_h_ = (state == Qt::Checked);
    update();
}

void NormalGenGLWidget::set_filter(int state)
{
    filter_ = state;
    update();
}

void NormalGenGLWidget::set_level(double value)
{
    level_ = (float)value;
    update();
}

void NormalGenGLWidget::set_strength(double value)
{
    strength_ = (float)value;
    update();
}

void NormalGenGLWidget::set_sigma(double value)
{
    sigma_ = (float)value;
    update();
}


} // namespace waterial
