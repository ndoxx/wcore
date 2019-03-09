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

    void clear();
    void write_entry(TextureEntry& entry);
    void read_entry(const TextureEntry& entry);

    void add_stretch();

    // TMP
    inline DropLabel* get_droplabel() { return droplabel; }

public slots:
    void handle_sig_texmap_changed(bool init_state);

protected:
    virtual void clear_additional() {}
    virtual void write_entry_additional(TextureEntry& entry) {}
    virtual void read_entry_additional(const TextureEntry& entry) {}

protected:
    QVBoxLayout* layout;
    DropLabel* droplabel;
    QCheckBox* map_enabled;
    QFrame* additional_controls;
    int texmap_index;
};

class ColorPickerLabel;
// Specialized controls for albedo map
class AlbedoControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit AlbedoControl();
    virtual ~AlbedoControl() = default;

protected:
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    ColorPickerLabel* color_picker_;
};

// Specialized controls for roughness map
class RoughnessControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit RoughnessControl();
    virtual ~RoughnessControl() = default;

protected:
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    QDoubleSpinBox* roughness_edit_;
};

} // namespace medit

#endif // TEXMAP_CONTROLS_H
