#ifndef TEXMAP_CONTROLS_H
#define TEXMAP_CONTROLS_H

#include <vector>
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
QT_FORWARD_DECLARE_CLASS(TexmapControlPane)
struct TextureEntry;
class MainWindow;
// Groups all the controls for a given texture map
class TexMapControl: public QGroupBox
{
    Q_OBJECT

public:
    TexMapControl(const QString& title, int index);
    virtual ~TexMapControl() = default;

    virtual void connect_controls(TexmapControlPane* texmap_pane) { }
    void connect_all(MainWindow* main_window, TexmapControlPane* texmap_pane);

    void clear();
    void write_entry(TextureEntry& entry);
    void read_entry(const TextureEntry& entry);

    void add_stretch();

    void set_tweak(const QString& tweak_path);
    void set_source(const QString& source_path);

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
    virtual void connect_controls(TexmapControlPane* texmap_pane) override;
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    ColorPickerLabel* color_picker_;
    QPushButton* btn_tweak_;
};

// Specialized controls for roughness map
class RoughnessControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit RoughnessControl();
    virtual ~RoughnessControl() = default;

protected:
    virtual void connect_controls(TexmapControlPane* texmap_pane) override;
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    DoubleSpinBox* roughness_edit_;
    QPushButton* btn_tweak_;
};

// Specialized controls for metallic map
class MetallicControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit MetallicControl();
    virtual ~MetallicControl() = default;

protected:
    virtual void connect_controls(TexmapControlPane* texmap_pane) override;
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    DoubleSpinBox* metallic_edit_;
    QPushButton* btn_tweak_;
};

// Specialized controls for depth map
class DepthControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit DepthControl();
    virtual ~DepthControl() = default;

protected:
    virtual void connect_controls(TexmapControlPane* texmap_pane) override;
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    DoubleSpinBox* parallax_scale_edit_;
    QPushButton* btn_tweak_;
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

protected:
    virtual void connect_controls(TexmapControlPane* texmap_pane) override;
    virtual void clear_additional() override;
    virtual void write_entry_additional(TextureEntry& entry) override;
    virtual void read_entry_additional(const TextureEntry& entry) override;

private:
    DoubleSpinBox* ao_edit_;
    QPushButton* btn_generate_;
};

// Specialized controls for normal map
class NormalControl: public TexMapControl
{
    Q_OBJECT

public:
    explicit NormalControl();
    virtual ~NormalControl() = default;

protected:
    virtual void connect_controls(TexmapControlPane* texmap_pane) override;

private:
    QPushButton* btn_generate_;
};

QT_FORWARD_DECLARE_CLASS(TweaksDialog)
QT_FORWARD_DECLARE_CLASS(AOGenDialog)
QT_FORWARD_DECLARE_CLASS(NormalGenDialog)

class EditorModel;
class TexmapControlPane: public QWidget
{
    Q_OBJECT

public:
    TexmapControlPane(MainWindow* main_window, EditorModel* editor_model, QWidget* parent=nullptr);

    // Retrieve data from controls and update a given entry with this information
    void update_entry(TextureEntry& entry);
    // Retrieve data from controls and update current texture entry
    void update_current_entry();
    // Retrieve data from current texture entry and update view
    void update_texture_view();
    // Clear texmap views to default
    void clear_view();

public slots:
    void handle_gen_normal_map_gpu();
    void handle_gen_ao_map_gpu();
    void handle_save_current_texture();

    void handle_tweak_albedo();
    void handle_tweak_roughness();
    void handle_tweak_metallic();
    void handle_tweak_depth();
    void handle_tweak_finished(int result);
    void handle_ao_finished(int result);
    void handle_normal_finished(int result);

private:
    std::vector<TexMapControl*> texmap_controls_;
    EditorModel* editor_model_;
    TweaksDialog* tweaks_dialog_;
    AOGenDialog* ao_gen_dialog_;
    NormalGenDialog* normal_gen_dialog_;

    MainWindow* main_window_;
};

} // namespace waterial

#endif // TEXMAP_CONTROLS_H
