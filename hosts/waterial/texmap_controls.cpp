#include <iostream>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QComboBox>
#include <QScrollArea>
#include <QApplication>

#include "texmap_controls.h"
#include "editor_model.h"
#include "droplabel.h"
#include "spinbox.h"
#include "color_picker_label.h"
#include "mainwindow.h"
#include "texmap_generator.h"
#include "tweaks_dialog.h"
#include "ao_gen_dialog.h"
#include "normal_gen_dialog.h"
#include "logger.h"

using namespace wcore;

namespace waterial
{

TexMapControl::TexMapControl(const QString& title, int index):
QGroupBox(title),
layout(new QVBoxLayout()),
droplabel(new DropLabel()),
map_enabled(new QCheckBox(tr("enable image map"))),
additional_controls(new QFrame),
scroll_area(new QScrollArea),
texmap_index(index)
{
    droplabel->setAcceptDrops(true);
    droplabel->setMinimumSize(QSize(100,100));
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    droplabel->setSizePolicy(policy);
    connect(droplabel, SIGNAL(sig_texmap_changed(bool)),
            this,      SLOT(handle_sig_texmap_changed(bool)));
    /*connect(droplabel, SIGNAL(sig_texmap_changed(bool)),
            this,      SLOT(handle_sig_something_changed()));*/

    // Checkbox to enable/disable texture map
    map_enabled->setEnabled(false);
    /*connect(map_enabled, SIGNAL(stateChanged(int)),
            this,        SLOT(handle_sig_something_changed()));*/

    layout->addSpacerItem(new QSpacerItem(20, 10));
    layout->addWidget(droplabel);
    layout->addWidget(map_enabled);
    layout->setAlignment(droplabel, Qt::AlignTop);
    layout->setAlignment(map_enabled, Qt::AlignTop);

    // Add separator
    auto sep = new QFrame;
    sep->setObjectName("Separator");
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep);
    layout->setAlignment(sep, Qt::AlignTop);

    // Additional controls are in a scrollable area
    scroll_area->setObjectName("TexmapScroll");
    scroll_area->setBackgroundRole(QPalette::Window);
    scroll_area->setFrameShadow(QFrame::Plain);
    scroll_area->setFrameShape(QFrame::NoFrame);
    scroll_area->setWidgetResizable(true);
    scroll_area->setWidget(additional_controls);
    layout->addWidget(scroll_area);

    this->setLayout(layout);
}

void TexMapControl::connect_all(MainWindow* main_window, TexmapControlPane* texmap_pane)
{
    connect(this,        &TexMapControl::sig_controls_changed,
            main_window, &MainWindow::handle_project_needs_saving);

    // Connect sub-controls
    connect_controls(texmap_pane);
}

void TexMapControl::write_entry(TextureEntry& entry)
{
    entry.texture_maps[texmap_index]->tweak_path = droplabel->get_tweak_path();
    entry.texture_maps[texmap_index]->has_tweak = !entry.texture_maps[texmap_index]->tweak_path.isEmpty();

    entry.texture_maps[texmap_index]->source_path = droplabel->get_path();
    entry.texture_maps[texmap_index]->has_image = !entry.texture_maps[texmap_index]->source_path.isEmpty();
    entry.texture_maps[texmap_index]->use_image = map_enabled->checkState() == Qt::Checked;

    write_entry_additional(entry);
}

void TexMapControl::read_entry(const TextureEntry& entry)
{
    droplabel->clear();
    if(entry.texture_maps[texmap_index]->has_image)
        droplabel->setPixmap(entry.texture_maps[texmap_index]->source_path,
                             entry.texture_maps[texmap_index]->tweak_path);

    map_enabled->setEnabled(entry.texture_maps[texmap_index]->has_image);
    map_enabled->setCheckState(entry.texture_maps[texmap_index]->use_image ? Qt::Checked : Qt::Unchecked);

    read_entry_additional(entry);
}

void TexMapControl::set_tweak(const QString& tweak_path)
{
    droplabel->setPixmap(droplabel->get_path(), tweak_path);
}

void TexMapControl::set_source(const QString& source_path)
{
    droplabel->setPixmap(source_path);
}

void TexMapControl::clear()
{
    droplabel->clear();
    map_enabled->setEnabled(false);
    map_enabled->setCheckState(Qt::Unchecked);

    clear_additional();
}

void TexMapControl::add_stretch()
{
    layout->addStretch();
}

void TexMapControl::handle_sig_something_changed()
{
    emit sig_controls_changed();
}

void TexMapControl::handle_sig_texmap_changed(bool initialized)
{
    if(initialized)
    {
        map_enabled->setEnabled(true);
        map_enabled->setCheckState(Qt::Checked);
    }
    else
    {
        map_enabled->setEnabled(false);
        map_enabled->setCheckState(Qt::Unchecked);
    }

    emit sig_controls_changed();
}

AlbedoControl::AlbedoControl():
TexMapControl(tr("Albedo"), ALBEDO),
color_picker_(new ColorPickerLabel)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), color_picker_);

    btn_tweak_ = new QPushButton(tr("Tweak"));
    btn_tweak_->setMaximumHeight(20);
    addc_layout->addRow(btn_tweak_);

    additional_controls->setLayout(addc_layout);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void AlbedoControl::clear_additional()
{
    color_picker_->reset();
}

void AlbedoControl::write_entry_additional(TextureEntry& entry)
{
    AlbedoMap* albedo_map = static_cast<AlbedoMap*>(entry.texture_maps[texmap_index]);
    const QColor& uni_color = color_picker_->get_color();
    albedo_map->u_albedo = wcore::math::i32vec4(uni_color.red(),
                                                uni_color.green(),
                                                uni_color.blue(),
                                                255);
}

void AlbedoControl::read_entry_additional(const TextureEntry& entry)
{
    AlbedoMap* albedo_map = static_cast<AlbedoMap*>(entry.texture_maps[texmap_index]);
    const wcore::math::i32vec4& uni_color = albedo_map->u_albedo;
    color_picker_->set_color(QColor(uni_color.x(),
                                    uni_color.y(),
                                    uni_color.z(),
                                    255));
}

void AlbedoControl::connect_controls(TexmapControlPane* texmap_pane)
{
    connect(btn_tweak_,  &QPushButton::clicked,
            texmap_pane, &TexmapControlPane::handle_tweak_albedo);
}

RoughnessControl::RoughnessControl():
TexMapControl(tr("Roughness"), ROUGHNESS),
roughness_edit_(new DoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), roughness_edit_);
    roughness_edit_->set_constrains(0.0, 1.0, 0.1, 0.0);

    btn_tweak_ = new QPushButton(tr("Tweak"));
    btn_tweak_->setMaximumHeight(20);
    addc_layout->addRow(btn_tweak_);

    additional_controls->setLayout(addc_layout);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void RoughnessControl::clear_additional()
{
    roughness_edit_->setValue(0);
}

void RoughnessControl::write_entry_additional(TextureEntry& entry)
{
    RoughnessMap* rough_map = static_cast<RoughnessMap*>(entry.texture_maps[texmap_index]);
    rough_map->u_roughness = (float)roughness_edit_->value();
}

void RoughnessControl::read_entry_additional(const TextureEntry& entry)
{
    RoughnessMap* rough_map = static_cast<RoughnessMap*>(entry.texture_maps[texmap_index]);
    roughness_edit_->setValue(rough_map->u_roughness);
}

void RoughnessControl::connect_controls(TexmapControlPane* texmap_pane)
{
    connect(btn_tweak_,  &QPushButton::clicked,
            texmap_pane, &TexmapControlPane::handle_tweak_roughness);
}

MetallicControl::MetallicControl():
TexMapControl(tr("Metallic"), METALLIC),
metallic_edit_(new DoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), metallic_edit_);
    metallic_edit_->set_constrains(0.0, 1.0, 0.1, 0.0);

    btn_tweak_ = new QPushButton(tr("Tweak"));
    btn_tweak_->setMaximumHeight(20);
    addc_layout->addRow(btn_tweak_);

    additional_controls->setLayout(addc_layout);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void MetallicControl::clear_additional()
{
    metallic_edit_->setValue(0);
}

void MetallicControl::write_entry_additional(TextureEntry& entry)
{
    MetallicMap* metal_map = static_cast<MetallicMap*>(entry.texture_maps[texmap_index]);
    metal_map->u_metallic = (float)metallic_edit_->value();
}

void MetallicControl::read_entry_additional(const TextureEntry& entry)
{
    MetallicMap* metal_map = static_cast<MetallicMap*>(entry.texture_maps[texmap_index]);
    metallic_edit_->setValue(metal_map->u_metallic);
}

void MetallicControl::connect_controls(TexmapControlPane* texmap_pane)
{
    connect(btn_tweak_,  &QPushButton::clicked,
            texmap_pane, &TexmapControlPane::handle_tweak_metallic);
}

DepthControl::DepthControl():
TexMapControl(tr("Depth"), DEPTH),
parallax_scale_edit_(new DoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Parallax Scale:"), parallax_scale_edit_);
    parallax_scale_edit_->set_constrains(0.0, 1.0, 0.1, 0.0);

    btn_tweak_ = new QPushButton(tr("Tweak"));
    btn_tweak_->setMaximumHeight(20);
    addc_layout->addRow(btn_tweak_);

    additional_controls->setLayout(addc_layout);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void DepthControl::clear_additional()
{
    parallax_scale_edit_->setValue(0);
}

void DepthControl::write_entry_additional(TextureEntry& entry)
{
    DepthMap* depth_map = static_cast<DepthMap*>(entry.texture_maps[texmap_index]);
    depth_map->u_parallax_scale = (float)parallax_scale_edit_->value();
}

void DepthControl::read_entry_additional(const TextureEntry& entry)
{
    DepthMap* depth_map = static_cast<DepthMap*>(entry.texture_maps[texmap_index]);
    parallax_scale_edit_->setValue(depth_map->u_parallax_scale);
}

void DepthControl::connect_controls(TexmapControlPane* texmap_pane)
{
    connect(btn_tweak_,  &QPushButton::clicked,
            texmap_pane, &TexmapControlPane::handle_tweak_depth);
}

AOControl::AOControl():
TexMapControl(tr("AO"), AO),
ao_edit_(new DoubleSpinBox),
btn_generate_(new QPushButton(tr("Generate")))
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), ao_edit_);
    ao_edit_->set_constrains(0.0, 1.0, 0.1, 0.0);

    btn_generate_->setMaximumHeight(20);
    addc_layout->addRow(btn_generate_);

    additional_controls->setLayout(addc_layout);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void AOControl::clear_additional()
{
    ao_edit_->setValue(0);
}

void AOControl::write_entry_additional(TextureEntry& entry)
{
    AOMap* ao_map = static_cast<AOMap*>(entry.texture_maps[texmap_index]);
    ao_map->u_ao = (float)ao_edit_->value();
}

void AOControl::read_entry_additional(const TextureEntry& entry)
{
    AOMap* ao_map = static_cast<AOMap*>(entry.texture_maps[texmap_index]);
    ao_edit_->setValue(ao_map->u_ao);
}

void AOControl::connect_controls(TexmapControlPane* texmap_pane)
{
    connect(btn_generate_, &QPushButton::clicked,
            texmap_pane,   &TexmapControlPane::handle_gen_ao_map_gpu);
}


NormalControl::NormalControl():
TexMapControl(tr("Normal"), NORMAL),
btn_generate_(new QPushButton(tr("Generate")))
{
    QFormLayout* addc_layout = new QFormLayout();

    btn_generate_->setMaximumHeight(20);
    addc_layout->addRow(btn_generate_);

    additional_controls->setLayout(addc_layout);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void NormalControl::connect_controls(TexmapControlPane* texmap_pane)
{
    connect(btn_generate_, &QPushButton::clicked,
            texmap_pane,   &TexmapControlPane::handle_gen_normal_map_gpu);
}


TexmapControlPane::TexmapControlPane(MainWindow* main_window, EditorModel* editor_model, QWidget* parent):
QWidget(parent),
editor_model_(editor_model),
tweaks_dialog_(nullptr),
ao_gen_dialog_(nullptr),
normal_gen_dialog_(nullptr),
main_window_(main_window)
{
    QGridLayout* layout_texmap_page = new QGridLayout();
    setObjectName("pageWidget");

    // Texture maps
    texmap_controls_.push_back(new AlbedoControl());
    texmap_controls_.push_back(new RoughnessControl());
    texmap_controls_.push_back(new MetallicControl());
    texmap_controls_.push_back(new DepthControl());
    texmap_controls_.push_back(new AOControl());
    texmap_controls_.push_back(new NormalControl());

    for(int ii=0; ii<texmap_controls_.size(); ++ii)
    {
        int col = ii%3;
        int row = ii/3;
        layout_texmap_page->addWidget(texmap_controls_[ii], row, col);
        layout_texmap_page->setRowStretch(row, 1); // So that all texmap controls will stretch the same way
        layout_texmap_page->setColumnStretch(col, 1);
        texmap_controls_[ii]->connect_all(main_window, this);
    }

    setLayout(layout_texmap_page);
}

void TexmapControlPane::update_entry(TextureEntry& entry)
{
    // Retrieve texture map info from controls and write to texture entry
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        texmap_controls_[ii]->write_entry(entry);

    // TMP Set dimensions to first defined texture dimensions
    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
    {
        if(entry.texture_maps[ii]->has_image)
        {
            entry.width  = texmap_controls_[ii]->get_droplabel()->getPixmap().width();
            entry.height = texmap_controls_[ii]->get_droplabel()->getPixmap().height();
            break;
        }
    }
}

void TexmapControlPane::update_texture_view()
{
    // * Update drop labels
    if(!editor_model_->get_current_texture_name().isEmpty())
    {
        TextureEntry& entry = editor_model_->get_current_texture_entry();

        for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
            texmap_controls_[ii]->read_entry(entry);
    }
    else
        clear_view();
}

void TexmapControlPane::update_current_entry()
{
    if(!editor_model_->get_current_texture_name().isEmpty())
    {
        TextureEntry& entry = editor_model_->get_current_texture_entry();

        for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
            texmap_controls_[ii]->write_entry(entry);
    }
}

void TexmapControlPane::clear_view()
{
    // Clear all texmap controls
    for(uint32_t ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        texmap_controls_[ii]->clear();
}

void TexmapControlPane::handle_save_current_texture()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        DLOGN("Saving texture <n>" + texname.toStdString() + "</n>", "waterial");

        TextureEntry& entry = editor_model_->get_current_texture_entry();
        entry.name = texname;
        update_entry(entry);
    }
}

void TexmapControlPane::handle_gen_ao_map_gpu()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(texname.isEmpty())
        return;

    TextureEntry& entry = editor_model_->get_current_texture_entry();
    update_entry(entry);
    if(entry.texture_maps[DEPTH]->has_image)
    {
        const QString& source = entry.texture_maps[DEPTH]->source_path;
        QDir out_dir = QFileInfo(source).absoluteDir();
        QString output = out_dir.filePath(texname + "_ao_gen.png");

        ao_gen_dialog_ = new AOGenDialog(main_window_);
        connect(ao_gen_dialog_, SIGNAL(finished(int)),
                this,           SLOT(handle_ao_finished(int)));
        ao_gen_dialog_->set_source_image(source);
        ao_gen_dialog_->set_output_image(output);
        ao_gen_dialog_->open();
    }
}

void TexmapControlPane::handle_gen_normal_map_gpu()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(texname.isEmpty())
        return;

    TextureEntry& entry = editor_model_->get_current_texture_entry();
    update_entry(entry);
    if(entry.texture_maps[DEPTH]->has_image)
    {
        const QString& source = entry.texture_maps[DEPTH]->source_path;
        QDir out_dir = QFileInfo(source).absoluteDir();
        QString output = out_dir.filePath(texname + "_norm_gen.png");

        normal_gen_dialog_ = new NormalGenDialog(main_window_);
        connect(normal_gen_dialog_, SIGNAL(finished(int)),
                this,               SLOT(handle_normal_finished(int)));
        normal_gen_dialog_->set_source_image(source);
        normal_gen_dialog_->set_output_image(output);
        normal_gen_dialog_->open();
    }
}

void TexmapControlPane::handle_tweak_albedo()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(texname.isEmpty())
        return;

    TextureEntry& entry = editor_model_->get_current_texture_entry();
    update_entry(entry);
    if(entry.texture_maps[ALBEDO]->has_image)
    {
        const QString& source = entry.texture_maps[ALBEDO]->source_path;
        QDir out_dir = QFileInfo(source).absoluteDir();
        QString output = out_dir.filePath(texname + "_albedo_twk.png");

        tweaks_dialog_ = new TweaksDialog(main_window_);
        connect(tweaks_dialog_, SIGNAL(finished(int)),
                this,           SLOT(handle_tweak_finished(int)));
        tweaks_dialog_->set_source_image(source);
        tweaks_dialog_->set_output_image(output);
        tweaks_dialog_->open();
    }
}

void TexmapControlPane::handle_tweak_roughness()
{
    //tweaks_dialog_->open();
}
void TexmapControlPane::handle_tweak_metallic()
{
    //tweaks_dialog_->open();
}
void TexmapControlPane::handle_tweak_depth()
{
    //tweaks_dialog_->open();
}

void TexmapControlPane::handle_tweak_finished(int result)
{
    if(result == QDialog::Accepted)
    {
        const QString& tweak_path = tweaks_dialog_->get_output_image_path();
        DLOGN("Saved generated <h>tweak</h> :", "waterial");
        DLOGI("<p>" + tweak_path.toStdString() + "</p>", "waterial");
        texmap_controls_[ALBEDO]->set_tweak(tweak_path);
        update_current_entry();
    }
    delete tweaks_dialog_;
}

void TexmapControlPane::handle_ao_finished(int result)
{
    if(result == QDialog::Accepted)
    {
        const QString& source_path = ao_gen_dialog_->get_output_image_path();
        DLOGN("Saved generated <h>AO</h> map:", "waterial");
        DLOGI("<p>" + source_path.toStdString() + "</p>", "waterial");
        texmap_controls_[AO]->set_source(source_path);
        update_current_entry();
    }
    delete ao_gen_dialog_;
}

void TexmapControlPane::handle_normal_finished(int result)
{
    if(result == QDialog::Accepted)
    {
        const QString& source_path = normal_gen_dialog_->get_output_image_path();
        DLOGN("Saved generated <h>normal</h> map:", "waterial");
        DLOGI("<p>" + source_path.toStdString() + "</p>", "waterial");
        texmap_controls_[NORMAL]->set_source(source_path);
        update_current_entry();
    }
    delete normal_gen_dialog_;
}

} // namespace waterial
