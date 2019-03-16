#ifndef TEXLIST_DELEGATE_H
#define TEXLIST_DELEGATE_H

#include <functional>
#include <QItemDelegate>
#include <QComboBox>

namespace waterial
{

QT_FORWARD_DECLARE_CLASS(EditorModel)
class TexlistDelegate: public QItemDelegate
{
    Q_OBJECT

public:
    typedef std::function<bool(const QString&)> NameValidator;

    explicit TexlistDelegate(QObject* parent=0);

    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const Q_DECL_OVERRIDE;

    inline void set_editor_model(EditorModel* model) { editor_model_ = model; }
    inline void set_item_name_validator(NameValidator validator) { item_name_validator_ = validator; }

signals:
    void sig_data_changed() const;

private:
    NameValidator item_name_validator_;
    EditorModel* editor_model_;
};

} // namespace waterial

#endif // TEXLIST_DELEGATE_H
