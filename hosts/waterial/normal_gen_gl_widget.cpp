#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLShaderProgram>

#include "normal_gen_gl_widget.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

NormalGenGLWidget::NormalGenGLWidget(QWidget* parent):
ShaderGenGLWidget(":/res/shaders/gen_normal.vert",
                  ":/res/shaders/gen_normal.frag",
                  parent)
{

}

NormalGenGLWidget::~NormalGenGLWidget()
{

}

void NormalGenGLWidget::update_uniforms()
{

}


} // namespace waterial
