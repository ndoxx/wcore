#include <iostream>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QFrame>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QGridLayout>
#include <QComboBox>

#include "texmap_controls.h"
#include "editor_model.h"
#include "droplabel.h"
#include "color_picker_label.h"
#include "mainwindow.h"
#include "texmap_generator.h"

namespace medit
{

TexMapControl::TexMapControl(const QString& title, int index):
QGroupBox(title),
layout(new QVBoxLayout()),
droplabel(new DropLabel()),
map_enabled(new QCheckBox(tr("enable image map"))),
additional_controls(new QFrame),
texmap_index(index)
{
    droplabel->setAcceptDrops(true);
    droplabel->setMinimumSize(QSize(200,200));
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    droplabel->setSizePolicy(policy);
    QObject::connect(droplabel, SIGNAL(sig_texmap_changed(bool)),
                     this,      SLOT(handle_sig_texmap_changed(bool)));

    // Checkbox to enable/disable texture map
    map_enabled->setEnabled(false);

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

    this->setLayout(layout);
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

void TexMapControl::handle_sig_texmap_changed(bool init_state)
{
    map_enabled->setEnabled(init_state);
    map_enabled->setCheckState(init_state ? Qt::Checked : Qt::Unchecked);
    setWindowModified(true); // DNW does not propagate to parent
}

AlbedoControl::AlbedoControl():
TexMapControl(tr("Albedo"), ALBEDO),
color_picker_(new ColorPickerLabel)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), color_picker_);

    additional_controls->setLayout(addc_layout);

    layout->addWidget(additional_controls);
    layout->setAlignment(additional_controls, Qt::AlignTop);

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
roughness_edit_(new QDoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), roughness_edit_);
    roughness_edit_->setRange(0.0, 1.0);
    roughness_edit_->setSingleStep(0.1);

    // Reject comma group separator, use dot as decimal separator
    QLocale qlocale(QLocale::C);
    qlocale.setNumberOptions(QLocale::RejectGroupSeparator);
    roughness_edit_->setLocale(qlocale);

    roughness_edit_->setMinimumWidth(50);
    additional_controls->setLayout(addc_layout);

    layout->addWidget(additional_controls);
    layout->setAlignment(additional_controls, Qt::AlignTop);

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
metallic_edit_(new QDoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), metallic_edit_);
    metallic_edit_->setRange(0.0, 1.0);
    metallic_edit_->setSingleStep(0.1);

    // Reject comma group separator, use dot as decimal separator
    QLocale qlocale(QLocale::C);
    qlocale.setNumberOptions(QLocale::RejectGroupSeparator);
    metallic_edit_->setLocale(qlocale);

    metallic_edit_->setMinimumWidth(50);
    additional_controls->setLayout(addc_layout);

    layout->addWidget(additional_controls);
    layout->setAlignment(additional_controls, Qt::AlignTop);

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


AOControl::AOControl():
TexMapControl(tr("AO"), AO),
ao_edit_(new QDoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Uniform value:"), ao_edit_);
    ao_edit_->setRange(0.0, 1.0);
    ao_edit_->setSingleStep(0.1);

    // Reject comma group separator, use dot as decimal separator
    QLocale qlocale(QLocale::C);
    qlocale.setNumberOptions(QLocale::RejectGroupSeparator);
    ao_edit_->setLocale(qlocale);

    ao_edit_->setMinimumWidth(50);
    additional_controls->setLayout(addc_layout);

    layout->addWidget(additional_controls);
    layout->setAlignment(additional_controls, Qt::AlignTop);

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


DepthControl::DepthControl():
TexMapControl(tr("Depth"), DEPTH),
parallax_scale_edit_(new QDoubleSpinBox)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Parallax Scale:"), parallax_scale_edit_);
    parallax_scale_edit_->setRange(0.0, 1.0);
    parallax_scale_edit_->setSingleStep(0.1);

    // Reject comma group separator, use dot as decimal separator
    QLocale qlocale(QLocale::C);
    qlocale.setNumberOptions(QLocale::RejectGroupSeparator);
    parallax_scale_edit_->setLocale(qlocale);

    parallax_scale_edit_->setMinimumWidth(50);
    additional_controls->setLayout(addc_layout);

    layout->addWidget(additional_controls);
    layout->setAlignment(additional_controls, Qt::AlignTop);

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


NormalControl::NormalControl():
TexMapControl(tr("Normal"), NORMAL),
gen_from_depth_btn_(new QPushButton(tr("From depthmap"))),
filter_combo_(new QComboBox()),
invert_r_cb_(new QCheckBox()),
invert_g_cb_(new QCheckBox()),
invert_h_cb_(new QCheckBox()),
level_edit_(new QDoubleSpinBox),
strength_edit_(new QDoubleSpinBox),
blursharp_edit_(new QDoubleSpinBox)
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

    addc_layout->addRow(tr("Invert:"), cboxes);
    addc_layout->addRow(tr("Kernel:"), filter_combo_);
    addc_layout->addRow(tr("Level:"), level_edit_);
    addc_layout->addRow(tr("Strength:"), strength_edit_);
    addc_layout->addRow(tr("Blur/Sharp:"), blursharp_edit_);
    addc_layout->addRow(gen_from_depth_btn_);

    level_edit_->setRange(4.0, 10.0);
    level_edit_->setSingleStep(0.1);
    level_edit_->setValue(7.0);
    level_edit_->setMinimumWidth(50);

    strength_edit_->setRange(0.01, 5.0);
    strength_edit_->setSingleStep(0.1);
    strength_edit_->setValue(0.61);
    strength_edit_->setMinimumWidth(50);

    blursharp_edit_->setRange(-10.0, 10.0);
    blursharp_edit_->setSingleStep(1.0);
    blursharp_edit_->setValue(0.0);
    blursharp_edit_->setMinimumWidth(50);

    QLocale qlocale(QLocale::C);
    qlocale.setNumberOptions(QLocale::RejectGroupSeparator);
    level_edit_->setLocale(qlocale);
    strength_edit_->setLocale(qlocale);
    blursharp_edit_->setLocale(qlocale);

    additional_controls->setLayout(addc_layout);

    layout->addWidget(additional_controls);
    layout->setAlignment(additional_controls, Qt::AlignTop);

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
    QObject::connect(gen_from_depth_btn_, &QPushButton::clicked,
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


} // namespace medit
