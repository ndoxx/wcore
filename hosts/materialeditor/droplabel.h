#ifndef DROPLABEL_H
#define DROPLABEL_H

#include <QLabel>

/*
    Custom label widget that:
    - handles drop actions with mime type "text/uri-list",
    - displays an image pointed to by said uri
    - preserves image aspect ratio during resize operations
*/

namespace medit
{

class DropLabel: public QLabel
{
    Q_OBJECT

public:
    DropLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    // Get preferred height given width (preserves pixmap aspect ratio)
    /*virtual int heightForWidth(int width) const override;
    // Get recommended size using heightForWidth
    virtual QSize sizeHint() const override;*/
    // Get a scaled version of member pixmap
    QPixmap scaledPixmap() const;
    // Set member pixmap
    void setPixmap(const QPixmap& pixmap);
    void setPixmap(const QString& pix_path);
    inline const QPixmap& getPixmap() const { return pixmap_; }

    // Gets the current file path to loaded image
    inline const QString& get_path() { return current_path_; }

public slots:
    void handle_context_menu(const QPoint& pos);
    void clear();

protected:
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent* event) override;

private:
    QPixmap pixmap_;
    QString current_path_;
    bool initialized_;
};

} // namespace medit

#endif // DROPLABEL_H
