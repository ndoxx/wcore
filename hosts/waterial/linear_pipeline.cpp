#include <QOpenGLFramebufferObject>

#include "linear_pipeline.h"

namespace waterial
{

LinearPipeline::LinearPipeline(const std::vector<std::pair<QString,QString>>& shader_sources,
                               int width,
                               int height,
                               QObject* parent):
width_(width),
height_(height),
fbo_(new QOpenGLFramebufferObject(width_,
                                  height_,
                                  QOpenGLFramebufferObject::CombinedDepthStencil))
{
    int stage = 0;
    int nstages = shader_sources.size();
    for(auto&& shader_source: shader_sources)
    {
        stages_.push_back(new ShaderStage(shader_source.first,
                                          shader_source.second,
                                          width,
                                          height,
                                          stage==(nstages-1),
                                          parent));
        ++stage;
    }

}

LinearPipeline::~LinearPipeline()
{
    for(auto* stage: stages_)
        delete stage;
    delete fbo_;
}

void LinearPipeline::set_uniform_updater(int stage, ShaderStage::UniformUpdater func)
{
    stages_[stage]->set_uniform_updater(func);
}

void LinearPipeline::render(int out_width, int out_height, bool is_export)
{
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, width_, height_);

    for(int ii=0; ii<stages_.size(); ++ii)
    {
        if(ii>0)
            stages_[ii-1]->bind_as_source();
        stages_[ii]->bind_as_target();

        if(ii==stages_.size()-1)
        {
            if(is_export)
                fbo_->bind();
            else
            {
                QOpenGLFramebufferObject::bindDefault();
                glViewport(0,0,out_width, out_height);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        stages_[ii]->release();

        if(is_export && ii==(stages_.size()-1))
        {
            fbo_->release();
            glFinish();
        }
    }
    glPopAttrib();
}

QImage LinearPipeline::get_image()
{
    return fbo_->toImage();
}


} // namespace waterial
