#ifndef TEXMAP_CONTROLS_H
#define TEXMAP_CONTROLS_H

#include <QGroupBox>

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QScrollArea)

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(DropLabel)
QT_FORWARD_DECLARE_CLASS(DoubleSpinBox)
struct TextureEntry;
class MainWindow;
// Groups all the controls for a given texture map
class TexMapControl: public QGroupBox
{
    Q_OBJECT

public:
    TexMapControl(const QString& title, int index);
    virtual ~TexMapControl() = default;

    virtual void connect_controls(MainWindow* main_window) { }
    void connect_all(MainWindow* main_window);

    void clear();
    void write_entry(TextureEntry& entry);
    void read_entry(const TextureEntry& entry);

    void add_stretch();

    // TMP
    inline DropLabel* get_droplabel() { return droplabel; }

signals:
    void sig_controls_changed();

public slots:
    void handle_sig_texmap_changed(bool initialized);
    void handle_sig_something_changed();

protected:
    virtual void clear_additional() {}
    virtual void write_entry_additional(TextureEntry& entry) {}
    virtual void read_entry_additional(const TextureEntry& entry) {}

protected:
    QVBoxLayout* layout;
    DropLabel* droplabel;
    QCheckBox* map_enabled;
    QFrame* additional_controls;
    QScrollArea* scroll_area;
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
    DoubleSpinBox* roughness_edit_;
};

// Specialized controls for metallic map
class MetallicControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit MetallicControl();
    virtual ~MetallicControl() = default;

protected:
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    DoubleSpinBox* metallic_edit_;
};

// Specialized controls for depth map
class DepthControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit DepthControl();
    virtual ~DepthControl() = default;

protected:
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    DoubleSpinBox* parallax_scale_edit_;
};

namespace generator
{
    struct NormalGenOptions;
    struct AOGenOptions;
}

// Specialized controls for AO map
class AOControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit AOControl();
    virtual ~AOControl() = default;

    virtual void connect_controls(MainWindow* main_window) override;
    void get_options(generator::AOGenOptions& options);

protected:
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    DoubleSpinBox* ao_edit_;
    QPushButton* gen_from_depth_btn_;
    QCheckBox* invert_cb_;
    DoubleSpinBox* strength_edit_;
    DoubleSpinBox* mean_edit_;
    DoubleSpinBox* range_edit_;
    DoubleSpinBox* blursharp_edit_;
};

// Specialized controls for normal map
class NormalControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit NormalControl();
    virtual ~NormalControl() = default;

    virtual void connect_controls(MainWindow* main_window) override;
    void get_options(generator::NormalGenOptions& options);

protected:
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    QPushButton* gen_from_depth_btn_;
    QComboBox* filter_combo_;
    QCheckBox* invert_r_cb_;
    QCheckBox* invert_g_cb_;
    QCheckBox* invert_h_cb_;
    DoubleSpinBox* level_edit_;
    DoubleSpinBox* strength_edit_;
    DoubleSpinBox* blursharp_edit_;
};

} // namespace waterial

#endif // TEXMAP_CONTROLS_H
