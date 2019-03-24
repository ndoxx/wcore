#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QOpenGLShaderProgram>

#include "ao_gen_gl_widget.h"
#include "shader_stage.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

AOGenGLWidget::AOGenGLWidget(QWidget* parent):
ShaderGenGLWidget({
                    {":/res/shaders/gen_ao.vert", ":/res/shaders/gen_ao.frag"},
                  },
                  parent)
{

}

AOGenGLWidget::~AOGenGLWidget()
{

}

void AOGenGLWidget::init()
{

}


} // namespace waterial
