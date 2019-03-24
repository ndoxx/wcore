#ifndef AO_GEN_DIALOG_H
#define AO_GEN_DIALOG_H

#include <QDialog>

namespace waterial
{
QT_FORWARD_DECLARE_CLASS(AOGenGLWidget)

class AOGenDialog: public QDialog
{
    Q_OBJECT

public:
    explicit AOGenDialog(QWidget* parent=nullptr);
    ~AOGenDialog();

    const QString& get_output_image_path() const;

    void set_source_image(const QString& path);
    void set_output_image(const QString& path);

public slots:
    void handle_accept();

private:
    AOGenGLWidget* preview_;

private:

};

} // namespace waterial

#endif
