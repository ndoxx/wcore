#ifndef TEXMAP_CONTROLS_H
#define TEXMAP_CONTROLS_H

#include <QGroupBox>

class QVBoxLayout;
class QCheckBox;
class QFrame;
class QDoubleSpinBox;

namespace medit
{

enum TexMapControlIndex: uint32_t
{
    ALBEDO,
    ROUGHNESS,
    METALLIC,
    AO,
    DEPTH,
    NORMAL,
    N_CONTROLS
};

class DropLabel;
struct TextureEntry;
// Groups all the controls for a given texture map
class TexMapControl: public QGroupBox
{
    Q_OBJECT

public:
    TexMapControl(const QString& title, int index);
    virtual ~TexMapControl() = default;

    virtual void clear_additional() {}
    virtual void write_entry_additional(TextureEntry& entry) {}
    virtual void read_entry_additional(const TextureEntry& entry) {}

    void clear();
    void write_entry(TextureEntry& entry);
    void read_entry(const TextureEntry& entry);

    void add_stretch();

    // TMP
    inline DropLabel* get_droplabel() { return droplabel; }


public slots:
    void handle_sig_texmap_changed(bool init_state);

protected:
    QVBoxLayout* layout    = nullptr;
    DropLabel* droplabel   = nullptr;
    QCheckBox* map_enabled = nullptr;
    QFrame* additional_controls = nullptr;
    int texmap_index;
};

class ColorPickerLabel;
// Specialized controls for albedo map
class AlbedoControls: public TexMapControl
{
    Q_OBJECT

public:
    explicit AlbedoControls();
    virtual ~AlbedoControls() = default;

    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

    ColorPickerLabel* color_picker_;
};

// Specialized controls for roughness map
class RoughnessControls: public TexMapControl
{
    Q_OBJECT

public:
    explicit RoughnessControls();
    virtual ~RoughnessControls() = default;

    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

    QDoubleSpinBox* roughness_edit_;
};

} // namespace medit

#endif // TEXMAP_CONTROLS_H
