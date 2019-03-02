#ifndef DROPLABEL_H
#define DROPLABEL_H

#include <QLabel>

namespace Ui {
class DropLabel;
}

class DropLabel: public QLabel
{
    Q_OBJECT

public:
    DropLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    DropLabel(const QString& text, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    // Get preferred height given width (preserves pixmap aspect ratio)
    virtual int heightForWidth(int width) const override;
    // Get recommended size using heightForWidth
    virtual QSize sizeHint() const override;
    // Get a scaled version of member pixmap
    QPixmap scaledPixmap() const;
    // Set member pixmap
    void setPixmap(const QPixmap& pixmap);

protected:
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent* event) override;

private:
    QPixmap pixmap_;
};

#endif // DROPLABEL_H
