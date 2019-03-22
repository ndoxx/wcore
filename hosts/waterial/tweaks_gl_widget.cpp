#include <iostream>
#include <vector>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include "tweaks_gl_widget.h"

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
initialized_(false)
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
    std::cout << "initializeGL" << std::endl;
    initializeOpenGLFunctions();

    vao_->create();
    vao_->bind();

    vbo_->create();
    vbo_->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vbo_->bind();
    vbo_->allocate(v_data.data(), v_data.size() * sizeof(GLfloat));

    source_tex_ = new QOpenGLTexture(QImage(source_path_).mirrored());

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char* vsrc =
        "const vec2 madd=vec2(0.5,0.5);\n"
        "attribute highp vec4 position;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = position;\n"
        "    texc.st = position.xy*madd+madd;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char* fsrc =
        "uniform sampler2D texture;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = texture2D(texture, texc.st);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    program_->addShader(vshader);
    program_->addShader(fshader);

    if(!program_->link())
    {
        std::cout << program_->log().toStdString() << std::endl;
    }
    attr_position_ = program_->attributeLocation("position");
    std::cout << "attr pos " << attr_position_ << std::endl;

    program_->bind();

    program_->enableAttributeArray(attr_position_);
    program_->setAttributeBuffer(attr_position_, GL_FLOAT, 0, 3);

    program_->setUniformValue("texture", 0);

    initialized_ = true;
}

void TweaksGLWidget::paintGL()
{
    glClearColor(clear_color_.x(),clear_color_.y(),clear_color_.z(),1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(source_tex_==nullptr)
        return;

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
    makeCurrent();
    source_path_ = path;
    if(initialized_)
    {
        source_tex_->release();
        source_tex_->destroy();
        delete source_tex_;
        source_tex_ = new QOpenGLTexture(QImage(source_path_).mirrored());
        source_tex_->bind();
    }
    doneCurrent();
}

} // namespace waterial
