#include <GL/glew.h>
#include <QSurfaceFormat>
#include <QTimer>
#include <QCheckBox>
#include <QFileInfo>

#include <fstream>

#include "preview_gl_widget.h"
#include "qt_context.h"
#include "wcore.h"
#include "logger.h"
#include "error.h"
#include "model.h"
#include "material.h"
#include "texture.h"
#include "lights.h"
#include "camera.h"
#include "editor_model.h"
#include "mesh_factory.h"
#include "surface_mesh.h"
#include "wat_loader.h"

using namespace wcore;

namespace waterial
{

PreviewGLWidget::PreviewGLWidget(QWidget* parent):
QOpenGLWidget(parent),
engine_(new wcore::Engine),
context_(new QtContext),
frame_timer_(new QTimer(this)),
active_(true),
rotate_model_(true),
reset_orientation_(false),
light_proxy_cooldown_(0),
dphi_(0.0f),
dtheta_(0.5f),
dpsi_(0.0f),
cam_coords_(1.5f,M_PI/4.0f,M_PI),
model_pos_(0.f),
light_pos_(0.f,2.f,0.f),
light_color_(1.f,1.f,1.f),
light_brightness_(10.f),
light_amb_(0.03f),
light_radius_(5.f),
light_inclination_(85.f * M_PI / 180.f),
light_perihelion_(90.f * M_PI / 180.f),
light_type_(0),
new_material_(nullptr)
{

}

PreviewGLWidget::~PreviewGLWidget()
{
    makeCurrent();
    if(new_material_)
        delete new_material_;
    delete engine_;
    doneCurrent();
}

QSize PreviewGLWidget::minimumSizeHint() const
{
    return QSize(320, 240);
}

void PreviewGLWidget::initializeGL()
{
    //connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &PreviewGLWidget::cleanup);

    // Initialize GLEW
    glewExperimental = GL_TRUE; // If not set, segfault at glGenVertexArrays()
    if (glewInit() != GLEW_OK) {
        DLOGF("Failed to initialize GLEW.", "waterial");
        fatal("Failed to initialize GLEW.");
    }
    glGetError();   // Mask an unavoidable error caused by GLEW

    engine_->SetFrameSize(800, 600);
    engine_->Init(0, nullptr, nullptr, context_);
    // [HACK] Model is set not to be cullable in level mv, otherwise it will cull (strangely)
    // for some values of azimuth and inclination
    engine_->scene->LoadStart("mv");

    Camera& cam = engine_->scene->GetCamera();
    cam.set_perspective(800,600,0.1f,15.f);
    cam.set_view_policy(Camera::ViewPolicy::DIRECTIONAL); // View matrix is a look_at matrix
    cam.set_look_at(math::vec3(0.f));

    // Systems configuration
    engine_->pipeline->SetShadowMappingEnabled(false);
    engine_->pipeline->SetDirectionalLightEnabled(false);
    engine_->pipeline->SetDaylightSystemEnabled(false);

    connect(frame_timer_, SIGNAL(timeout()), this, SLOT(update()));
    frame_timer_->start(16);
}

void PreviewGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(!active_) return;

    engine_->pipeline->SetDefaultFrameBuffer(defaultFramebufferObject());

    // Update model
    engine_->scene->VisitModelRef("the_model"_h, [&](Model& model)
    {
        // Reset orientation?
        if(reset_orientation_)
        {
            model.reset_orientation();
            reset_orientation_ = false;
        }

        if(rotate_model_)
            model.rotate(dphi_,dtheta_,dpsi_);

        // Set model position
        model.set_position(model_pos_);
        model.update_bounding_boxes();

        // Swap material?
        if(new_material_)
        {
            model.set_material(new_material_);
            new_material_ = nullptr;
        }
    });

    // Update light
    if(light_type_==0) // Point light
    {
        engine_->scene->VisitLightRef("the_point_light"_h, [&](Light& light)
        {
            light.set_position(light_pos_);
            light.set_radius(light_radius_);
            light.set_color(light_color_);
            light.set_ambient_strength(light_amb_);
            light.set_brightness(light_brightness_);
        });

        if(--light_proxy_cooldown_>0)
            engine_->pipeline->dShowLightProxy(1,0.1f);
        else
            engine_->pipeline->dShowLightProxy(0);
    }
    else if(light_type_==1) // Directional light
    {
        Light& dirlight = engine_->scene->GetDirectionalLight();
        math::vec3 sun_pos(cos(light_inclination_),
                           sin(light_inclination_)*sin(light_perihelion_),
                           sin(light_inclination_)*cos(light_perihelion_));
        sun_pos.normalize();
        dirlight.set_position(sun_pos);
        dirlight.set_color(light_color_);
        dirlight.set_ambient_strength(light_amb_);
        dirlight.set_brightness(light_brightness_);
    }

    // Update camera
    Camera& cam = engine_->scene->GetCamera();
    cam.set_position(math::vec3(cam_coords_.x()*sin(cam_coords_.y())*sin(cam_coords_.z()),
                                cam_coords_.x()*cos(cam_coords_.y()),
                                cam_coords_.x()*sin(cam_coords_.y())*cos(cam_coords_.z())));

    engine_->Update(16.67/1000.f);
    engine_->RenderFrame();
    engine_->FinishFrame();
}

void PreviewGLWidget::resizeGL(int width, int height)
{
    engine_->SetWindowSize(width, height);
}

void PreviewGLWidget::mousePressEvent(QMouseEvent* event)
{

}

void PreviewGLWidget::mouseMoveEvent(QMouseEvent* event)
{

}

void PreviewGLWidget::reset_light_proxy_cooldown()
{
    light_proxy_cooldown_ = 60;
}

// Slots
void PreviewGLWidget::cleanup()
{
    makeCurrent();

    // do cleanup

    doneCurrent();
}

void PreviewGLWidget::handle_active_changed(int newstate)
{
    active_ = (newstate == Qt::Checked);
}

void PreviewGLWidget::handle_rotate_changed(int newstate)
{
    rotate_model_ = (newstate == Qt::Checked);
}

void PreviewGLWidget::handle_dphi_changed(double newvalue)
{
    dphi_ = (float)newvalue;
}

void PreviewGLWidget::handle_dtheta_changed(double newvalue)
{
    dtheta_ = (float)newvalue;
}

void PreviewGLWidget::handle_dpsi_changed(double newvalue)
{
    dpsi_ = (float)newvalue;
}

void PreviewGLWidget::handle_reset_orientation()
{
    reset_orientation_ = true;
}

void PreviewGLWidget::handle_cam_radius_changed(double newvalue)
{
    cam_coords_[0] = (float)newvalue;
}

void PreviewGLWidget::handle_cam_inclination_changed(double newvalue)
{
    cam_coords_[1] = (float)newvalue;
}

void PreviewGLWidget::handle_cam_azimuth_changed(double newvalue)
{
    cam_coords_[2] = (float)newvalue;
}

void PreviewGLWidget::handle_x_changed(double newvalue)
{
    model_pos_[0] = -(float)newvalue;
}

void PreviewGLWidget::handle_y_changed(double newvalue)
{
    model_pos_[1] = (float)newvalue;
}

void PreviewGLWidget::handle_z_changed(double newvalue)
{
    model_pos_[2] = (float)newvalue;
}

void PreviewGLWidget::handle_material_swap(EditorModel* edmodel)
{
    // Locate watfile and check existence
    QString watfilename(edmodel->get_current_texture_name());
    watfilename += ".wat";

    auto filepath = edmodel->get_output_folder().filePath(watfilename);

    QFileInfo check_file(filepath);
    if(check_file.exists() && check_file.isFile())
    {
        // Read descriptor
        wcore::WatLoader wat_loader;
        std::ifstream ifs(filepath.toStdString(), std::ios::in | std::ios::binary);
        wcore::MaterialDescriptor descriptor;
        wat_loader.read(ifs, descriptor, true);
        descriptor.texture_descriptor.wat_location  = watfilename.toStdString();
        descriptor.texture_descriptor.sampler_group = 1;
        new_material_ = new Material(descriptor);
    }
}

void PreviewGLWidget::handle_mesh_swap(int newvalue)
{
    engine_->scene->VisitModelRef("the_model"_h, [&](Model& model)
    {
        switch(newvalue)
        {
            case 0: // Cube
                model.set_mesh(factory::make_cube_uniface());
                break;
            case 1: // Plane
                model.set_mesh(factory::make_plane());
                model.set_orientation(math::vec3(270,0,0)); // WTF? otherwise plane is vertical
                break;
        }
        model.update_bounding_boxes();
    });
}

void PreviewGLWidget::handle_light_x_changed(double newvalue)
{
    light_pos_[0] = -(float)newvalue;
    reset_light_proxy_cooldown();
}

void PreviewGLWidget::handle_light_y_changed(double newvalue)
{
    light_pos_[1] = (float)newvalue;
    reset_light_proxy_cooldown();
}

void PreviewGLWidget::handle_light_z_changed(double newvalue)
{
    light_pos_[2] = (float)newvalue;
    reset_light_proxy_cooldown();
}

void PreviewGLWidget::handle_light_radius_changed(double newvalue)
{
    light_radius_ = (float)newvalue;
}

void PreviewGLWidget::handle_light_brightness_changed(double newvalue)
{
    light_brightness_ = (float)newvalue;
}

void PreviewGLWidget::handle_light_ambient_changed(double newvalue)
{
    light_amb_ = (float)newvalue;
}

void PreviewGLWidget::handle_light_inclination_changed(double newvalue)
{
    light_inclination_ = (float)newvalue;
}

void PreviewGLWidget::handle_light_perihelion_changed(double newvalue)
{
    light_perihelion_ = (float)newvalue;
}

void PreviewGLWidget::handle_light_color_changed(QColor newvalue)
{
    light_color_ = math::vec3(newvalue.red()/255.f,
                              newvalue.green()/255.f,
                              newvalue.blue()/255.f);
}

void PreviewGLWidget::handle_light_type_changed(int newvalue)
{
    light_type_ = newvalue;

    if(light_type_ == 0) // Point
    {
        engine_->pipeline->SetDirectionalLightEnabled(false);
    }
    else if(light_type_ == 1) // Directional
    {
        engine_->pipeline->SetDirectionalLightEnabled(true);
        // Shutdown point light
        engine_->scene->VisitLightRef("the_point_light"_h, [&](Light& light)
        {
            light.set_brightness(0.f);
        });
    }
}

void PreviewGLWidget::handle_bloom_changed(int newvalue)
{
    engine_->pipeline->SetBloomEnabled(newvalue == Qt::Checked);
}


} // namespace waterial
