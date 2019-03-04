#include <vector>
#include <QListView>
#include <QImage>
#include <QSortFilterProxyModel>

#include "editor_model.h"
#include "texlist_model.h"
#include "logger.h"

using namespace wcore;

namespace medit
{


void TextureEntry::debug_display()
{
    DLOG("Texture <n>" + name.toStdString() + "</n>", "core", Severity::LOW);
    if(has_map[0]) DLOGI("albedo: <p>"    + paths[0].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[1]) DLOGI("roughness: <p>" + paths[1].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[2]) DLOGI("metallic: <p>"  + paths[2].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[3]) DLOGI("ao: <p>"        + paths[3].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[4]) DLOGI("depth: <p>"     + paths[4].toStdString() + "</p>", "core", Severity::LOW);
    if(has_map[5]) DLOGI("normal: <p>"    + paths[5].toStdString() + "</p>", "core", Severity::LOW);
}


EditorModel::EditorModel():
texlist_model_(new TexListModel),
texlist_sort_proxy_model_(new QSortFilterProxyModel)
{

}

EditorModel::~EditorModel()
{
    delete texlist_model_;
    delete texlist_sort_proxy_model_;
}

void EditorModel::set_output_folder(const QString& path)
{
    output_folder_ = QDir(path);
    if(!output_folder_.exists())
    {
        DLOGE("Output texture folder does not exist:", "core", Severity::CRIT);
        DLOGI(path.toStdString(), "core", Severity::CRIT);
    }
}

void EditorModel::set_current_texture_name(const QString& name)
{
    current_texname_ = name;
    DLOGN("Working on texture <n>" + current_texname_.toStdString() + "</n>", "core", Severity::LOW);
}

void EditorModel::setup_list_model(QListView* listview)
{
    // Custom model uses string list as data
    texlist_model_->setStringList(texlist_);
    // Proxy model for sorting the texture list view when a new item is added
    texlist_sort_proxy_model_->setDynamicSortFilter(true);
    texlist_sort_proxy_model_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    texlist_sort_proxy_model_->setFilterKeyColumn(0);
    texlist_sort_proxy_model_->setSourceModel(texlist_model_);
    listview->setModel(texlist_sort_proxy_model_);
}

QModelIndex EditorModel::add_texture(const QString& name)
{
    // Append texture name to list view data and sort
    QModelIndex index = texlist_model_->append(name);
    texlist_sort_proxy_model_->sort(0);

    // Add texture descriptor
    texture_descriptors_.insert(std::pair(H_(name.toUtf8().constData()), TextureEntry()));

    // index is a source index and needs to be remapped to proxy sorted index
    return texlist_sort_proxy_model_->mapFromSource(index);
}

bool EditorModel::has_entry(wcore::hash_t name)
{
    return (texture_descriptors_.find(name) != texture_descriptors_.end());
}

void EditorModel::delete_current_texture(QListView* tex_list)
{
    if(!current_texname_.isEmpty())
    {
        hash_t hname = H_(current_texname_.toUtf8().constData());
        auto it = texture_descriptors_.find(hname);
        if(it != texture_descriptors_.end())
        {
            current_texname_ = "";
            texture_descriptors_.erase(it);
            texlist_model_->removeRow(texlist_sort_proxy_model_->mapToSource(tex_list->currentIndex()).row());
        }
    }
}

void EditorModel::rename_texture(const QString& old_name, const QString& new_name)
{
    // Just remap the descriptor to the new name hash
    hash_t hname = H_(old_name.toUtf8().constData());
    auto it = texture_descriptors_.find(hname);
    if(it != texture_descriptors_.end())
    {
        hash_t new_hname = H_(new_name.toUtf8().constData());
        current_texname_ = new_name;
        texture_descriptors_.insert(std::pair(new_hname, it->second));
        texture_descriptors_.erase(it);
    }
}

void EditorModel::compile(const QString& texname)
{
    DLOGN("Compiling texture <n>" + texname.toStdString() + "</n>", "core", Severity::LOW);

    hash_t hname = H_(texname.toUtf8().constData());
    TextureEntry& entry = get_texture_entry(hname);

    if(entry.width && entry.height)
    {
        DLOGI("Size: WxH= <v>" + std::to_string(entry.width) + "x" + std::to_string(entry.height) + "</v>", "core", Severity::LOW);

        QImage block0(entry.width, entry.height, QImage::Format_RGBA8888);
        QImage block1(entry.width, entry.height, QImage::Format_RGBA8888);
        QImage block2(entry.width, entry.height, QImage::Format_RGBA8888);

        QImage** texmaps = new QImage*[NTEXMAPS];
        for(int ii=0; ii<NTEXMAPS; ++ii)
        {
            if(entry.has_map[ii])
                texmaps[ii] = new QImage(entry.paths[ii]);
            else
                texmaps[ii] = nullptr;
        }

        // Block 0
        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.width; ++yy)
            {
                QRgb albedo   = texmaps[0] ? texmaps[0]->pixel(xx,yy) : qRgb(0,0,0);
                int roughness = texmaps[1] ? qRed(texmaps[1]->pixel(xx,yy)) : 0;

                QRgb out_color = qRgba(qRed(albedo), qGreen(albedo), qBlue(albedo), roughness);
                block0.setPixel(xx, yy, out_color);
            }
        }

        // Block 1
        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.width; ++yy)
            {
                QRgb normal = texmaps[5] ? texmaps[5]->pixel(xx,yy) : qRgb(0,0,0);
                int depth   = texmaps[4] ? qRed(texmaps[4]->pixel(xx,yy)) : 0;

                QRgb out_color = qRgba(qRed(normal), qGreen(normal), qBlue(normal), depth);
                block1.setPixel(xx, yy, out_color);
            }
        }

        // Block 2
        for(int xx=0; xx<entry.width; ++xx)
        {
            for(int yy=0; yy<entry.width; ++yy)
            {
                int metallic = texmaps[2] ? qRed(texmaps[2]->pixel(xx,yy)) : 0;
                int ao       = texmaps[3] ? qRed(texmaps[3]->pixel(xx,yy)) : 0;

                QRgb out_color = qRgba(metallic, ao, 0, 0);
                block2.setPixel(xx, yy, out_color);
            }
        }

        // Save blocks
        QString block0_name = texname + "_block0.png";
        QString block1_name = texname + "_block1.png";
        QString block2_name = texname + "_block2.png";

        DLOGI("block0: <p>" + block0_name.toStdString() + "</p>", "core", Severity::LOW);
        DLOGI("block1: <p>" + block1_name.toStdString() + "</p>", "core", Severity::LOW);
        DLOGI("block2: <p>" + block2_name.toStdString() + "</p>", "core", Severity::LOW);

        block0.save(output_folder_.filePath(block0_name));
        block1.save(output_folder_.filePath(block1_name));
        block2.save(output_folder_.filePath(block2_name));

        delete[] texmaps;
    }


}

} // namespace medit
