#include <iostream>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPixmap>
#include <QBitmap>
#include "droplabel.h"


DropLabel::DropLabel(QWidget* parent, Qt::WindowFlags f):
QLabel(parent, f)
{

}

DropLabel::DropLabel(const QString& text, QWidget* parent, Qt::WindowFlags f):
QLabel(parent, f)
{

}

int DropLabel::heightForWidth(int width) const
{
    return pixmap_.isNull() ? this->height() : ((qreal)pixmap_.height()*width)/pixmap_.width();
}

QSize DropLabel::sizeHint() const
{
    int w = this->width();
    return QSize(w, heightForWidth(w));
}

QPixmap DropLabel::scaledPixmap() const
{
    return pixmap_.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void DropLabel::setPixmap(const QPixmap& pixmap)
{
    pixmap_ = pixmap;
    QLabel::setPixmap(scaledPixmap());
}

void DropLabel::resizeEvent(QResizeEvent* event)
{
    if(!pixmap_.isNull())
        QLabel::setPixmap(scaledPixmap());
}

void DropLabel::dragEnterEvent(QDragEnterEvent* event)
{
    /*
    QStringList mime_formats = event->mimeData()->formats();
    for(int ii=0; ii<mime_formats.size(); ++ii)
         std::cout << "format: " << mime_formats.at(ii).toLocal8Bit().constData() << std::endl;
    */

    // Accept drops from treeView entries
    if(event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void DropLabel::dropEvent(QDropEvent* event)
{
    //std::cout << "drop: " << event->mimeData()->text().toUtf8().constData() << std::endl;

    // Get path to file from mime data in event
    auto file_path = QUrl(event->mimeData()->text()).toLocalFile();
    // Generate a pixmap and set label to use it (but rescaled)
    setPixmap(QPixmap(file_path));
    setMask(pixmap_.mask());
    show();
}
