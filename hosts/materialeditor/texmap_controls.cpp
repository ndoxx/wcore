#include <QVBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QFrame>
#include <QLineEdit>

#include "texmap_controls.h"
#include "editor_model.h"
#include "droplabel.h"
#include "color_picker_label.h"

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
    droplabel->setMinimumSize(QSize(128,128));
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

AlbedoControls::AlbedoControls():
TexMapControl(tr("Albedo"), ALBEDO),
color_picker_(new ColorPickerLabel)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Color:"), color_picker_);

    additional_controls->setLayout(addc_layout);

    layout->addWidget(additional_controls);
    layout->setAlignment(additional_controls, Qt::AlignTop);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void AlbedoControls::clear_additional()
{
    color_picker_->reset();
}

void AlbedoControls::write_entry_additional(TextureEntry& entry)
{
    AlbedoMap* albedo_map = static_cast<AlbedoMap*>(entry.texture_maps[texmap_index]);
    const QColor& uni_color = color_picker_->get_color();
    albedo_map->u_albedo = wcore::math::i32vec4(uni_color.red(),
                                                uni_color.green(),
                                                uni_color.blue(),
                                                255);
}

void AlbedoControls::read_entry_additional(const TextureEntry& entry)
{
    AlbedoMap* albedo_map = static_cast<AlbedoMap*>(entry.texture_maps[texmap_index]);
    const wcore::math::i32vec4& uni_color = albedo_map->u_albedo;
    color_picker_->set_color(QColor(uni_color.x(),
                                    uni_color.y(),
                                    uni_color.z(),
                                    255));
}

RoughnessControls::RoughnessControls():
TexMapControl(tr("Roughness"), ROUGHNESS),
roughness_edit_(new QLineEdit)
{
    QFormLayout* addc_layout = new QFormLayout();
    addc_layout->addRow(tr("Roughness:"), roughness_edit_);

    //roughness_edit_->setFixedWidth(50);
    additional_controls->setLayout(addc_layout);

    layout->addWidget(additional_controls);
    layout->setAlignment(additional_controls, Qt::AlignTop);

    // Add stretchable area at the bottom so that all controls are neatly packed to the top
    add_stretch();
}

void RoughnessControls::clear_additional()
{
    roughness_edit_->setText("");
}

void RoughnessControls::write_entry_additional(TextureEntry& entry)
{
    RoughnessMap* rough_map = static_cast<RoughnessMap*>(entry.texture_maps[texmap_index]);
    rough_map->u_roughness = roughness_edit_->text().toFloat();
}

void RoughnessControls::read_entry_additional(const TextureEntry& entry)
{
    RoughnessMap* rough_map = static_cast<RoughnessMap*>(entry.texture_maps[texmap_index]);
    roughness_edit_->setText(QString::number(rough_map->u_roughness));
}

} // namespace medit
