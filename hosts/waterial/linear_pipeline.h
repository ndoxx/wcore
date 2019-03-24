#ifndef LINEAR_PIPELINE_H
#define LINEAR_PIPELINE_H

#include <vector>
#include <QString>
#include <QImage>

#include "shader_stage.h"

QT_FORWARD_DECLARE_CLASS(QObject)
QT_FORWARD_DECLARE_CLASS(QOpenGLFramebufferObject)

namespace waterial
{

class ShaderStage;
class LinearPipeline
{
public:
    LinearPipeline(const std::vector<std::pair<QString,QString>>& shader_sources,
                   int width,
                   int height,
                   QObject* parent=nullptr);

    ~LinearPipeline();

    void set_uniform_updater(int stage, ShaderStage::UniformUpdater func);
    void render(int out_width, int out_height, bool is_export);
    QImage get_image();

private:
    int width_;
    int height_;

    QOpenGLFramebufferObject* fbo_;
    std::vector<ShaderStage*> stages_;
};

} // namespace waterial

#endif
