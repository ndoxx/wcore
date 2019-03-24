#include <vector>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>

#include "shader_gen_gl_widget.h"
#include "linear_pipeline.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

ShaderGenGLWidget::ShaderGenGLWidget(std::initializer_list<std::pair<QString,QString>> shader_sources,
                                     QWidget* parent):
QOpenGLWidget(parent),
pipeline_(nullptr),
img_width_(0),
img_height_(0),
clear_color_(0.f,0.f,0.f),
shader_sources_(shader_sources),
source_tex_(nullptr),
vbo_(new QOpenGLBuffer),
vao_(new QOpenGLVertexArrayObject),
attr_position_(0),
export_(false)
{

}

ShaderGenGLWidget::~ShaderGenGLWidget()
{
    makeCurrent();
    delete source_tex_;
    delete pipeline_;
    delete vbo_;
    delete vao_;
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
    source_tex_->setWrapMode(QOpenGLTexture::Repeat);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    pipeline_ = new LinearPipeline(shader_sources_, img_width_, img_height_, this);

    init();
}

void ShaderGenGLWidget::paintGL()
{
    glClearColor(clear_color_.x(),clear_color_.y(),clear_color_.z(),1);
    // Geometry is a simple quad
    vao_->bind();
    source_tex_->bind();

    pipeline_->render(width(), height(), export_);

    // If export is needed, get end pipeline FBO as image and save it
    if(export_)
    {
        QImage fbo_img(pipeline_->get_image());
        QImage image(fbo_img.constBits(), fbo_img.width(), fbo_img.height(), QImage::Format_ARGB32);
        image.save(out_path_);
        export_ = false;
    }

    vao_->release();
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
}

void ShaderGenGLWidget::handle_export()
{
    export_ = true;
    makeCurrent();
    paintGL(); // calling update() would fail
    doneCurrent();
}


} // namespace waterial
