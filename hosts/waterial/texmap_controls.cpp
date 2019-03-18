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

#include "texmap_controls.h"
#include "editor_model.h"
#include "droplabel.h"
#include "spinbox.h"
#include "color_picker_label.h"
#include "mainwindow.h"
#include "texmap_generator.h"

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
    droplabel->setMinimumSize(QSize(200,200));
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    droplabel->setSizePolicy(policy);
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

void TexMapControl::connect_all(MainWindow* main_window)
{
    connect(this,        &TexMapControl::sig_controls_changed,
            main_window, &MainWindow::handle_project_needs_saving);

    // Connect sub-controls
    connect_controls(main_window);
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

void AOControl::connect_controls(MainWindow* main_window)
{
    connect(gen_from_depth_btn_, &QPushButton::clicked,
            main_window,         &MainWindow::handle_gen_ao_map);
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

    cboxes_layout->addWidget(new QLabel(tr("R")), 0, 0);
    cboxes_layout->addWidget(new QLabel(tr("G")), 0, 1);
    cboxes_layout->addWidget(new QLabel(tr("H")), 0, 2);
    cboxes_layout->addWidget(invert_r_cb_, 1, 0);
    cboxes_layout->addWidget(invert_g_cb_, 1, 1);
    cboxes_layout->addWidget(invert_h_cb_, 1, 2);
    cboxes->setLayout(cboxes_layout);
    cboxes->setMinimumWidth(50);
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

void NormalControl::connect_controls(MainWindow* main_window)
{
    connect(gen_from_depth_btn_, &QPushButton::clicked,
            main_window,         &MainWindow::handle_gen_normal_map);
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


} // namespace waterial
