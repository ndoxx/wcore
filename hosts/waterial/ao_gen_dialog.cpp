#include "ao_gen_dialog.h"
#include "ao_gen_gl_widget.h"

namespace waterial
{

AOGenDialog::AOGenDialog(QWidget* parent):
QDialog(parent),
preview_(new AOGenGLWidget(this))
{

}

AOGenDialog::~AOGenDialog()
{

}

void AOGenDialog::handle_accept()
{
    preview_->handle_export();
    accept();
}


}
