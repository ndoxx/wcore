#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>

#include "shader_stage.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

ShaderStage::ShaderStage(const QString& vshader_path,
                         const QString& fshader_path,
                         int width,
                         int height,
                         bool is_output,
                         QObject* parent):
program_(new QOpenGLShaderProgram),
fbo_(nullptr),
update_uniforms_([](QOpenGLShaderProgram*){}),
width_(width),
height_(height)
{
    // Create shader program
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, parent);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, parent);
    vshader->compileSourceFile(vshader_path);
    fshader->compileSourceFile(fshader_path);
    program_->addShader(vshader);
    program_->addShader(fshader);

    if(!program_->link())
    {
        DLOGE("Shader failed to link:", "waterial", Severity::CRIT);
        std::cout << program_->log().toStdString() << std::endl;
    }
    unsigned int attr_position = program_->attributeLocation("position");

    program_->bind();

    // Setup attribute array
    program_->enableAttributeArray(attr_position);
    program_->setAttributeBuffer(attr_position, GL_FLOAT, 0, 3);

    // Generate intermediate FBO if current stage is not end stage
    if(!is_output)
        fbo_ = new QOpenGLFramebufferObject(width_, height_, QOpenGLFramebufferObject::CombinedDepthStencil);
}

ShaderStage::~ShaderStage()
{
    delete program_;
    delete fbo_;
}

void ShaderStage::bind_as_source()
{
    release();
    if(fbo_)
        glBindTexture(GL_TEXTURE_2D, fbo_->texture());
}

void ShaderStage::bind_as_target()
{
    program_->bind();
    update_uniforms_(program_);
    // Texture is unit 0
    program_->setUniformValue("texture", 0);
    if(fbo_)
        fbo_->bind();
}

void ShaderStage::release()
{
    program_->release();
    if(fbo_)
        fbo_->release();
}

} // namespace waterial
