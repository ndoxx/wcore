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
    entry.texture_maps[texmap_index]->path = droplabel->get_path();
    entry.texture_maps[texmap_index]->has_image = !entry.texture_maps[texmap_index]->path.isEmpty();
    entry.texture_maps[texmap_index]->use_image = map_enabled->checkState() == Qt::Checked;

    write_entry_additional(entry);
}

void TexMapControl::read_entry(const TextureEntry& entry)
{
    droplabel->clear();
    if(entry.texture_maps[texmap_index]->has_image)
    {
        droplabel->setPixmap(entry.texture_maps[texmap_index]->path);
    }
    map_enabled->setEnabled(entry.texture_maps[texmap_index]->has_image);
    map_enabled->setCheckState(entry.texture_maps[texmap_index]->use_image ? Qt::Checked : Qt::Unchecked);

    read_entry_additional(entry);
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

RoughnessControl::RoughnessControl():
TexMapControl(tr("Roughness"), ROUGHNESS),
roughness_edit_(new DoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), roughness_edit_);
    roughness_edit_->set_constrains(0.0, 1.0, 0.1, 0.0);

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


MetallicControl::MetallicControl():
TexMapControl(tr("Metallic"), METALLIC),
metallic_edit_(new DoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), metallic_edit_);
    metallic_edit_->set_constrains(0.0, 1.0, 0.1, 0.0);

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


DepthControl::DepthControl():
TexMapControl(tr("Depth"), DEPTH),
parallax_scale_edit_(new DoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Parallax Scale:"), parallax_scale_edit_);
    parallax_scale_edit_->set_constrains(0.0, 1.0, 0.1, 0.0);

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


AOControl::AOControl():
TexMapControl(tr("AO"), AO),
ao_edit_(new DoubleSpinBox),
gen_from_depth_btn_(new QPushButton(tr("From depthmap"))),
invert_cb_(new QCheckBox),
strength_edit_(new DoubleSpinBox),
mean_edit_(new DoubleSpinBox),
range_edit_(new DoubleSpinBox),
blursharp_edit_(new DoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), ao_edit_);

    // Add separator
    auto sep = new QFrame;
    sep->setObjectName("Separator");
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    addc_layout->addRow(sep);


    addc_layout->addRow(tr("Invert:"), invert_cb_);
    addc_layout->addRow(tr("Strength:"), strength_edit_);
    addc_layout->addRow(tr("Mean:"), mean_edit_);
    addc_layout->addRow(tr("Range:"), range_edit_);
    addc_layout->addRow(tr("Blur/Sharp:"), blursharp_edit_);
    addc_layout->addRow(gen_from_depth_btn_);

    ao_edit_->set_constrains(0.0, 1.0, 0.1, 0.0);
    strength_edit_->set_constrains(0.0, 1.0, 0.1, 0.5);
    mean_edit_->set_constrains(0.0, 1.0, 0.1, 1.0);
    range_edit_->set_constrains(0.0, 1.0, 0.1, 1.0);
    blursharp_edit_->set_constrains(-10.0, 10.0, 1.0, 0.0);

    additional_controls->setLayout(addc_layout);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void AOControl::clear_additional()
{
    ao_edit_->setValue(0);
    strength_edit_->setValue(0.5);
    range_edit_->setValue(1.0);
    blursharp_edit_->setValue(0.0);
    invert_cb_->setCheckState(Qt::Unchecked);
}

void AOControl::write_entry_additional(TextureEntry& entry)
{
    AOMap* ao_map = static_cast<AOMap*>(entry.texture_maps[texmap_index]);
    ao_map->u_ao = (float)ao_edit_->value();

    // Generator options
    ao_map->gen_invert    = invert_cb_->checkState() == Qt::Checked;
    ao_map->gen_strength  = (float)strength_edit_->value();
    ao_map->gen_mean      = (float)mean_edit_->value();
    ao_map->gen_range     = (float)range_edit_->value();
    ao_map->gen_blursharp = (float)blursharp_edit_->value();
}

void AOControl::read_entry_additional(const TextureEntry& entry)
{
    AOMap* ao_map = static_cast<AOMap*>(entry.texture_maps[texmap_index]);
    ao_edit_->setValue(ao_map->u_ao);

    // Generator options
    invert_cb_->setCheckState(ao_map->gen_invert ? Qt::Checked : Qt::Unchecked);
    strength_edit_->setValue(ao_map->gen_strength);
    mean_edit_->setValue(ao_map->gen_mean);
    range_edit_->setValue(ao_map->gen_range);
    blursharp_edit_->setValue(ao_map->gen_blursharp);
}

void AOControl::connect_controls(TexmapControlPane* texmap_pane)
{
    connect(gen_from_depth_btn_, &QPushButton::clicked,
            texmap_pane,         &TexmapControlPane::handle_gen_ao_map);
}

void AOControl::get_options(generator::AOGenOptions& options)
{
    options.range    = (float)range_edit_->value();
    options.mean     = (float)mean_edit_->value();
    options.strength = (float)strength_edit_->value();
    options.sigma    = (float)blursharp_edit_->value();
    options.invert   = invert_cb_->checkState() == Qt::Checked;;
}


NormalControl::NormalControl():
TexMapControl(tr("Normal"), NORMAL),
gen_from_depth_btn_(new QPushButton(tr("From depthmap"))),
filter_combo_(new QComboBox()),
invert_r_cb_(new QCheckBox()),
invert_g_cb_(new QCheckBox()),
invert_h_cb_(new QCheckBox()),
level_edit_(new DoubleSpinBox),
strength_edit_(new DoubleSpinBox),
blursharp_edit_(new DoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();

    QWidget* cboxes = new QWidget();
    QGridLayout* cboxes_layout = new QGridLayout();

    cboxes_layout->setAlignment(Qt::AlignHCenter);
    cboxes_layout->addWidget(new QLabel(tr("R")), 0, 0);
    cboxes_layout->addWidget(new QLabel(tr("G")), 0, 1);
    cboxes_layout->addWidget(new QLabel(tr("H")), 0, 2);
    cboxes_layout->addWidget(invert_r_cb_, 1, 0);
    cboxes_layout->addWidget(invert_g_cb_, 1, 1);
    cboxes_layout->addWidget(invert_h_cb_, 1, 2);
    cboxes->setLayout(cboxes_layout);
    cboxes->setMinimumWidth(50);
    cboxes->setMaximumHeight(55);
    cboxes->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    filter_combo_->addItems(QStringList()<<"Sobel"<<"Scharr");
    filter_combo_->setMaximumHeight(22);

    addc_layout->addRow(tr("Invert:"), cboxes);
    addc_layout->addRow(tr("Kernel:"), filter_combo_);
    addc_layout->addRow(tr("Level:"), level_edit_);
    addc_layout->addRow(tr("Strength:"), strength_edit_);
    addc_layout->addRow(tr("Blur/Sharp:"), blursharp_edit_);
    addc_layout->addRow(gen_from_depth_btn_);

    level_edit_->set_constrains(4.0, 10.0, 0.1, 7.0);
    strength_edit_->set_constrains(0.01, 5.0, 0.1, 0.6);
    blursharp_edit_->set_constrains(-10.0, 10.0, 1.0, 0.0);

    additional_controls->setLayout(addc_layout);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void NormalControl::clear_additional()
{
    filter_combo_->setCurrentIndex(0);
    invert_r_cb_->setCheckState(Qt::Unchecked);
    invert_g_cb_->setCheckState(Qt::Unchecked);
    invert_h_cb_->setCheckState(Qt::Unchecked);
    level_edit_->setValue(7.0);
    strength_edit_->setValue(0.61);
    blursharp_edit_->setValue(0.0);
}

void NormalControl::write_entry_additional(TextureEntry& entry)
{
    NormalMap* normal_map = static_cast<NormalMap*>(entry.texture_maps[texmap_index]);
    normal_map->gen_filter    = filter_combo_->currentIndex();
    normal_map->gen_invert_r  = invert_r_cb_->checkState() == Qt::Checked;
    normal_map->gen_invert_g  = invert_g_cb_->checkState() == Qt::Checked;
    normal_map->gen_invert_h  = invert_h_cb_->checkState() == Qt::Checked;
    normal_map->gen_level     = (float)level_edit_->value();
    normal_map->gen_strength  = (float)strength_edit_->value();
    normal_map->gen_blursharp = (float)blursharp_edit_->value();
}

void NormalControl::read_entry_additional(const TextureEntry& entry)
{
    NormalMap* normal_map = static_cast<NormalMap*>(entry.texture_maps[texmap_index]);
    filter_combo_->setCurrentIndex(normal_map->gen_filter);
    invert_r_cb_->setCheckState(normal_map->gen_invert_r ? Qt::Checked : Qt::Unchecked);
    invert_g_cb_->setCheckState(normal_map->gen_invert_g ? Qt::Checked : Qt::Unchecked);
    invert_h_cb_->setCheckState(normal_map->gen_invert_h ? Qt::Checked : Qt::Unchecked);
    level_edit_->setValue(normal_map->gen_level);
    strength_edit_->setValue(normal_map->gen_strength);
    blursharp_edit_->setValue(normal_map->gen_blursharp);
}

void NormalControl::connect_controls(TexmapControlPane* texmap_pane)
{
    connect(gen_from_depth_btn_, &QPushButton::clicked,
            texmap_pane,         &TexmapControlPane::handle_gen_normal_map);
}

void NormalControl::get_options(generator::NormalGenOptions& options)
{
    options.filter   = generator::FilterType(filter_combo_->currentIndex());
    options.invert_r = (invert_r_cb_->checkState() == Qt::Checked) ? -1.f : 1.f;
    options.invert_g = (invert_g_cb_->checkState() == Qt::Checked) ? -1.f : 1.f;
    options.invert_h = (invert_h_cb_->checkState() == Qt::Checked) ? -1.f : 1.f;
    options.level    = (float)level_edit_->value();
    options.strength = (float)strength_edit_->value();
    options.sigma    = (float)blursharp_edit_->value();
}


TexmapControlPane::TexmapControlPane(MainWindow* main_window, EditorModel* editor_model, QWidget* parent):
QWidget(parent),
editor_model_(editor_model)
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
    TextureEntry& entry = editor_model_->get_current_texture_entry();

    for(int ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        texmap_controls_[ii]->read_entry(entry);
}

void TexmapControlPane::clear_view()
{
    // Clear all texmap controls
    for(uint32_t ii=0; ii<TexMapControlIndex::N_CONTROLS; ++ii)
        texmap_controls_[ii]->clear();
}

void TexmapControlPane::get_options(generator::AOGenOptions& options)
{
    AOControl* ao_control = static_cast<AOControl*>(texmap_controls_[AO]);
    ao_control->get_options(options);
}

void TexmapControlPane::get_options(generator::NormalGenOptions& options)
{
    NormalControl* normal_control = static_cast<NormalControl*>(texmap_controls_[NORMAL]);
    normal_control->get_options(options);
}

void TexmapControlPane::handle_save_current_texture()
{
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        DLOGN("Saving texture <n>" + texname.toStdString() + "</n>", "waterial", Severity::LOW);

        TextureEntry& entry = editor_model_->get_current_texture_entry();
        entry.name = texname;
        update_entry(entry);
    }
}

void TexmapControlPane::handle_gen_normal_map()
{
    // Get current entry
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        handle_save_current_texture();
        TextureEntry& entry = editor_model_->get_current_texture_entry();
        // Get depth map if any
        if(entry.texture_maps[DEPTH]->has_image && entry.texture_maps[DEPTH]->use_image)
        {
            QApplication::setOverrideCursor(Qt::WaitCursor);

            QString depth_path(entry.texture_maps[DEPTH]->path);
            QImage depthmap(depth_path);
            QImage normalmap(entry.width, entry.height, QImage::Format_RGBA8888);

            // Generate normal map
            generator::NormalGenOptions options;
            get_options(options);
            generator::normal_from_depth(depthmap, normalmap, options);
            // Blur/Sharpen
            generator::blur_sharp(normalmap, options.sigma);

            // Get directory of depth map
            QDir dir(QFileInfo(depth_path).absoluteDir());

            // Generate filename and save normal map
            QString filename = texname + "_norm_gen.png";
            QString normal_path(dir.filePath(filename));

            DLOGN("Saving generated <h>normal</h> map:", "waterial", Severity::LOW);
            DLOGI("<p>" + normal_path.toStdString() + "</p>", "waterial", Severity::LOW);

            normalmap.save(normal_path);

            // Display newly generated normal map
            entry.texture_maps[NORMAL]->has_image = true;
            entry.texture_maps[NORMAL]->use_image = true;
            entry.texture_maps[NORMAL]->path = normal_path;
            update_texture_view();

            QApplication::restoreOverrideCursor();
        }
    }
}

void TexmapControlPane::handle_gen_ao_map()
{
    // Get current entry
    const QString& texname = editor_model_->get_current_texture_name();
    if(!texname.isEmpty())
    {
        handle_save_current_texture();
        TextureEntry& entry = editor_model_->get_current_texture_entry();
        // Get depth map if any
        if(entry.texture_maps[DEPTH]->has_image && entry.texture_maps[DEPTH]->use_image)
        {
            QApplication::setOverrideCursor(Qt::WaitCursor);

            QString depth_path(entry.texture_maps[DEPTH]->path);
            QImage depthmap(depth_path);
            QImage aomap(entry.width, entry.height, QImage::Format_RGBA8888);

            // Generate normal map
            generator::AOGenOptions options;
            get_options(options);
            generator::ao_from_depth(depthmap, aomap, options);
            // Blur/Sharpen
            generator::blur_sharp(aomap, options.sigma);

            // Get directory of depth map
            QDir dir(QFileInfo(depth_path).absoluteDir());

            // Generate filename and save normal map
            QString filename = texname + "_ao_gen.png";
            QString ao_path(dir.filePath(filename));

            DLOGN("Saving generated <h>AO</h> map:", "waterial", Severity::LOW);
            DLOGI("<p>" + ao_path.toStdString() + "</p>", "waterial", Severity::LOW);

            aomap.save(ao_path);

            // Display newly generated normal map
            entry.texture_maps[AO]->has_image = true;
            entry.texture_maps[AO]->use_image = true;
            entry.texture_maps[AO]->path = ao_path;
            update_texture_view();

            QApplication::restoreOverrideCursor();
        }
    }
}

} // namespace waterial
