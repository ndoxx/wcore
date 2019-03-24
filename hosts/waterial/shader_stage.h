#ifndef SHADER_STAGE_H
#define SHADER_STAGE_H

#include <functional>

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)
QT_FORWARD_DECLARE_CLASS(QObject)

namespace waterial
{

class ShaderStage
{
public:
    typedef std::function<void(QOpenGLShaderProgram* program)> UniformUpdater;

    ShaderStage(const QString& vshader_path,
                const QString& fshader_path,
                int width,
                int height,
                QObject* parent=nullptr);
    ~ShaderStage();

    void init(bool is_input, bool is_output);
    inline void set_uniform_updater(UniformUpdater func) { update_uniforms_ = func; }
    void bind_as_source();
    void bind_as_target();
    void release();

private:
    QOpenGLShaderProgram* program_;
    QOpenGLFramebufferObject* fbo_;
    UniformUpdater update_uniforms_;

    int width_;
    int height_;
};

} // namespace waterial

#endif
