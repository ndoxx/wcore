#ifndef COLOR_PICKER_LABEL_H
#define COLOR_PICKER_LABEL_H

#include <QLabel>

/*
    Custom label widget that:
    - is clickable
    - shows color picker dialog on click
    - updates its color when a color is choosen
*/

namespace waterial
{

class ColorPickerLabel: public QLabel
{
    Q_OBJECT

public:
    explicit ColorPickerLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~ColorPickerLabel() = default;

    void set_color(const QColor& color);
    inline const QColor& get_color() const { return color_; }
    inline void reset() { set_color(Qt::white); }

signals:
    void clicked();
    void sig_value_changed(QColor newvalue);

public slots:
    void handle_albedo_color_edit();

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;

private:
    QColor color_;
};

} // namespace waterial

#endif // COLOR_PICKER_LABEL_H
