#ifndef LINEAR_PIPELINE_H
#define LINEAR_PIPELINE_H

#include <initializer_list>
#include <QString>

#include "shader_stage.h"

QT_FORWARD_DECLARE_CLASS(QObject)

namespace waterial
{

class ShaderStage;
class LinearPipeline
{
public:
    LinearPipeline(std::initializer_list<std::pair<QString,QString>> shader_sources,
                   int width,
                   int height,
                   QObject* parent=nullptr);

    ~LinearPipeline();

    void set_uniform_updater(int stage, ShaderStage::UniformUpdater func);

    void render(int out_width, int out_height, bool is_export);

    inline QOpenGLFramebufferObject* get_fbo() { return fbo_; }

private:
    QOpenGLFramebufferObject* fbo_;
    std::vector<ShaderStage*> stages_;
    int width_;
    int height_;
};

} // namespace waterial

#endif
