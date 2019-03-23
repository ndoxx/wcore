#include <vector>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>

#include "shader_gen_gl_widget.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

ShaderGenGLWidget::ShaderGenGLWidget(const QString& vshader_path,
                                     const QString& fshader_path,
                                     QWidget* parent):
QOpenGLWidget(parent),
program_(new QOpenGLShaderProgram),
clear_color_(0.f,0.f,0.f),
vshader_path_(vshader_path),
fshader_path_(fshader_path),
source_tex_(nullptr),
fbo_(nullptr),
vbo_(new QOpenGLBuffer),
vao_(new QOpenGLVertexArrayObject),
attr_position_(0),
img_width_(0),
img_height_(0),
initialized_(false),
export_(false)
{

}

ShaderGenGLWidget::~ShaderGenGLWidget()
{
    makeCurrent();
    delete source_tex_;
    delete program_;
    delete fbo_;
    vbo_->destroy();
    vao_->destroy();
    doneCurrent();
}

QSize ShaderGenGLWidget::minimumSizeHint() const
{
    return QSize(256, 256);
}

// Screen quad vertex data
static std::vector<GLfloat> v_data =
{
    // vertex position
    -1.f, -1.f, 0.f,
     1.f, -1.f, 0.f,
     1.f,  1.f, 0.f,

    -1.f, -1.f, 0.f,
     1.f,  1.f, 0.f,
    -1.f,  1.f, 0.f
};

void ShaderGenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // Create VAO & VBO
    vao_->create();
    vao_->bind();

    vbo_->create();
    vbo_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo_->bind();
    vbo_->allocate(v_data.data(), v_data.size() * sizeof(GLfloat));

    // Texture from source image
    QImage source_image = QImage(source_path_).mirrored();
    img_width_ = source_image.width();
    img_height_ = source_image.height();
    source_tex_ = new QOpenGLTexture(source_image);
    fbo_ = new QOpenGLFramebufferObject(img_width_, img_height_, QOpenGLFramebufferObject::CombinedDepthStencil);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Create shader program
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    vshader->compileSourceFile(vshader_path_);
    fshader->compileSourceFile(fshader_path_);
    program_->addShader(vshader);
    program_->addShader(fshader);

    if(!program_->link())
    {
        DLOGE("Shader failed to link:", "waterial", Severity::CRIT);
        std::cout << program_->log().toStdString() << std::endl;
    }
    attr_position_ = program_->attributeLocation("position");

    program_->bind();

    // Setup attribute array
    program_->enableAttributeArray(attr_position_);
    program_->setAttributeBuffer(attr_position_, GL_FLOAT, 0, 3);

    // Texture is unit 0
    program_->setUniformValue("texture", 0);

    initialized_ = true;
}

void ShaderGenGLWidget::paintGL()
{
    if(export_)
    {
        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, img_width_, img_height_);
        fbo_->bind();
    }

    glClearColor(clear_color_.x(),clear_color_.y(),clear_color_.z(),1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update uniforms
    update_uniforms();

    // Draw texture to quad
    vao_->bind();
    source_tex_->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    vao_->release();

    if(export_)
    {
        fbo_->release();
        glPopAttrib();
        glFinish();
        QImage fbo_img(fbo_->toImage());
        QImage image(fbo_img.constBits(), fbo_img.width(), fbo_img.height(), QImage::Format_ARGB32);
        image.save(out_path_);
        export_ = false;
    }
}

void ShaderGenGLWidget::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
}

void ShaderGenGLWidget::mousePressEvent(QMouseEvent* event)
{

}

void ShaderGenGLWidget::mouseMoveEvent(QMouseEvent* event)
{

}

void ShaderGenGLWidget::set_source_image(const QString& path)
{
    source_path_ = path;
    //out_path_ = make_out_path(source_path_);
    if(initialized_)
    {
        // If initializeGL() already called, we need to replace texture
        makeCurrent();
        source_tex_->release();
        source_tex_->destroy();
        delete source_tex_;
        QImage source_image = QImage(source_path_).mirrored();
        img_width_ = source_image.width();
        img_height_ = source_image.height();
        source_tex_ = new QOpenGLTexture(source_image);
        source_tex_->bind();
        doneCurrent();
    }
}

void ShaderGenGLWidget::handle_export()
{
    export_ = true;
    makeCurrent();
    paintGL(); // calling update() would fail
    doneCurrent();
}


} // namespace waterial
