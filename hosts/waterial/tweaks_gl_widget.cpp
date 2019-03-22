#include <iostream>
#include <vector>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>

#include "tweaks_gl_widget.h"

namespace waterial
{

TweaksGLWidget::TweaksGLWidget(QWidget* parent):
QOpenGLWidget(parent),
clear_color_(0.f,0.f,0.f),
source_tex_(nullptr),
program_(new QOpenGLShaderProgram),
vbo_(new QOpenGLBuffer)
{

}

TweaksGLWidget::~TweaksGLWidget()
{
    makeCurrent();
    delete source_tex_;
    delete program_;
    delete vbo_;
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

    vbo_->create();
    vbo_->bind();
    vbo_->allocate(v_data.data(), v_data.size() * sizeof(GLfloat));

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    const char* vsrc =
        "const vec2 madd=vec2(0.5,0.5);\n"
        "attribute highp vec4 vertex;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vertex;\n"
        "    texc.st = vertex.xy*madd+madd;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char* fsrc =
        "uniform sampler2D texture;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = vec4(texc.st,0,1)/*texture2D(texture, texc.st)*/;\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    program_->addShader(vshader);
    program_->addShader(fshader);

    #define PROGRAM_VERTEX_ATTRIBUTE 0
    program_->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    if(!program_->link())
    {
        std::cout << program_->log().toStdString() << std::endl;
    }

    program_->bind();
    program_->setUniformValue("texture", 0);
}

void TweaksGLWidget::paintGL()
{
    glClearColor(clear_color_.x(),clear_color_.y(),clear_color_.z(),1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(source_tex_==nullptr)
        return;

    program_->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program_->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3);

    source_tex_->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
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
    if(source_tex_)
        delete source_tex_;
    source_tex_ = new QOpenGLTexture(QImage(path)/*.mirrored()*/);
    update();
}

} // namespace waterial
