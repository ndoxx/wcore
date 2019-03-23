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

#include "tweaks_gl_widget.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

TweaksGLWidget::TweaksGLWidget(QWidget* parent):
QOpenGLWidget(parent),
clear_color_(0.f,0.f,0.f),
source_tex_(nullptr),
program_(new QOpenGLShaderProgram),
vbo_(new QOpenGLBuffer),
vao_(new QOpenGLVertexArrayObject),
attr_position_(0),
img_width_(0),
img_height_(0),
initialized_(false),
hue_(0.f),
saturation_(0.f),
value_(0.f)
{

}

TweaksGLWidget::~TweaksGLWidget()
{
    makeCurrent();
    delete source_tex_;
    delete program_;
    vbo_->destroy();
    vao_->destroy();
    doneCurrent();
}

QSize TweaksGLWidget::minimumSizeHint() const
{
    return QSize(256, 256);
}

// Screen quad vertex data
std::vector<GLfloat> v_data =
{
    // vertex position
    -1.f, -1.f, 0.f,
     1.f, -1.f, 0.f,
     1.f,  1.f, 0.f,

    -1.f, -1.f, 0.f,
     1.f,  1.f, 0.f,
    -1.f,  1.f, 0.f
};

void TweaksGLWidget::initializeGL()
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

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Create shader program
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    vshader->compileSourceFile(":/res/shaders/tweaks.vert");
    fshader->compileSourceFile(":/res/shaders/tweaks.frag");
    program_->addShader(vshader);
    program_->addShader(fshader);

    if(!program_->link())
    {
        DLOGE("Tweaks shader failed to link:", "waterial", Severity::CRIT);
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

void TweaksGLWidget::paintGL()
{
    // Update uniforms
    program_->setUniformValue("f_hue", hue_);
    program_->setUniformValue("f_saturation", saturation_);
    program_->setUniformValue("f_value", value_);

    draw();
}

void TweaksGLWidget::draw()
{
    glClearColor(clear_color_.x(),clear_color_.y(),clear_color_.z(),1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw texture to quad
    vao_->bind();
    source_tex_->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    vao_->release();
}

void TweaksGLWidget::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
}

void TweaksGLWidget::mousePressEvent(QMouseEvent* event)
{

}

void TweaksGLWidget::mouseMoveEvent(QMouseEvent* event)
{

}

void TweaksGLWidget::set_source_image(const QString& path)
{
    source_path_ = path;
    out_path_ = make_out_path(source_path_);
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

QString TweaksGLWidget::make_out_path(const QString& in_path)
{
    QFileInfo info(in_path);
    QString out_name = info.completeBaseName() + "_twk.png";
    QDir out_dir = info.absoluteDir();
    return out_dir.filePath(out_name);
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

void TweaksGLWidget::handle_export()
{
    makeCurrent();

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, img_width_, img_height_);
    QOpenGLFramebufferObject fbo(img_width_, img_height_, QOpenGLFramebufferObject::CombinedDepthStencil);
    fbo.bind();

    draw();

    fbo.release();
    glPopAttrib();

    QImage fbo_img(fbo.toImage());
    QImage image(fbo_img.constBits(), fbo_img.width(), fbo_img.height(), QImage::Format_ARGB32);
    image.save(out_path_);

    doneCurrent();
}


} // namespace waterial
