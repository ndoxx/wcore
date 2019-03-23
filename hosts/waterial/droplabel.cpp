#include <iostream>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPixmap>
#include <QBitmap>
#include <QMenu>

#include "droplabel.h"


namespace waterial
{

static QString ssIdle = "border-radius: 5px; border: none; background: white;";
static QString ssHoverDnD = "border-radius: 5px; border: none; background: rgb(0,204,255);";

DropLabel::DropLabel(QWidget* parent, Qt::WindowFlags f):
QLabel(parent, f),
current_path_(""),
current_tweak_path_(""),
initialized_(false)
{
    // Set style
    setStyleSheet(ssIdle);
    // Set contextual menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
                     this, SLOT(handle_context_menu(const QPoint&)));
}

int DropLabel::heightForWidth(int width) const
{
    //return pixmap_.isNull() ? width : ((qreal)pixmap_.height()*width)/pixmap_.width();
    return width;
}

/*QSize DropLabel::sizeHint() const
{
    int w = this->width();
    return QSize(w, heightForWidth(w));
}*/

QPixmap DropLabel::scaledPixmap() const
{
    return pixmap_.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void DropLabel::setPixmap(const QPixmap& pixmap)
{
    pixmap_ = pixmap;
    QLabel::setPixmap(scaledPixmap());
    setMask(pixmap_.mask());
    show();
    initialized_ = true;
    emit sig_texmap_changed(initialized_);
}

void DropLabel::setPixmap(const QString& pix_path, const QString& tweak_path)
{
    current_path_ = pix_path;
    current_tweak_path_ = tweak_path;
    if(!current_tweak_path_.isEmpty())
        setPixmap(QPixmap(current_tweak_path_));
    else
        setPixmap(QPixmap(current_path_));
}

void DropLabel::clear()
{
    QLabel::clear();
    current_path_ = "";
    current_tweak_path_ = "";
    initialized_ = false;
    emit sig_texmap_changed(initialized_);
}

void DropLabel::clear_tweaks()
{
    current_tweak_path_ = "";
    // Fallback to source image if possible
    if(!current_path_.isEmpty())
        setPixmap(current_path_);
    else
    {
        initialized_ = false;
        emit sig_texmap_changed(initialized_);
    }
}

void DropLabel::resizeEvent(QResizeEvent* event)
{
    if(!pixmap_.isNull() && initialized_)
        QLabel::setPixmap(scaledPixmap());
}

void DropLabel::dragEnterEvent(QDragEnterEvent* event)
{
    /*QStringList mime_formats = event->mimeData()->formats();
    for(int ii=0; ii<mime_formats.size(); ++ii)
         std::cout << "format: " << mime_formats.at(ii).toLocal8Bit().constData() << std::endl;*/

    // Accept drops from treeView entries
    if(event->mimeData()->hasFormat("text/uri-list"))
    {
        setStyleSheet(ssHoverDnD);
        event->acceptProposedAction();
    }
}

void DropLabel::dragLeaveEvent(QDragLeaveEvent* event)
{
    setStyleSheet(ssIdle);
}

void DropLabel::dropEvent(QDropEvent* event)
{
    //std::cout << "drop: " << event->mimeData()->text().toUtf8().constData() << std::endl;

    // Get path to file from mime data in event
    current_path_ = event->mimeData()->urls().first().toLocalFile();
    // Generate a pixmap and set label to use it (but rescaled)
    setPixmap(current_path_);
    setStyleSheet(ssIdle);

}

void DropLabel::handle_context_menu(const QPoint& pos)
{
    // tmp? nothing to show if label is empty
    if(!initialized_) return;

    QPoint globalPos = mapToGlobal(pos);

    QMenu context_menu;
    context_menu.addAction(QIcon(":/res/icons/clear.png"), tr("&Clear"), this, SLOT(clear()));
    if(!current_tweak_path_.isEmpty())
    {
        context_menu.addAction(tr("Clear &tweaks"), this, SLOT(clear_tweaks()));
    }

    context_menu.exec(globalPos);
}


} // namespace waterial

