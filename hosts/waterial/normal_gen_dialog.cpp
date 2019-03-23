#include "normal_gen_dialog.h"
#include "normal_gen_gl_widget.h"

namespace waterial
{

NormalGenDialog::NormalGenDialog(QWidget* parent):
QDialog(parent),
preview_(new NormalGenGLWidget(this))
{

}

NormalGenDialog::~NormalGenDialog()
{

}

void NormalGenDialog::handle_accept()
{
    preview_->handle_export();
    accept();
}

}
