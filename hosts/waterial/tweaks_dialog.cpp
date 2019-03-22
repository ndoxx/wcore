#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

#include "tweaks_dialog.h"
#include "tweaks_gl_widget.h"

namespace waterial
{

TweaksDialog::TweaksDialog(QWidget* parent):
QDialog(parent, Qt::CustomizeWindowHint),
preview_(new TweaksGLWidget())
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    QHBoxLayout* ctl_view_layout = new QHBoxLayout();
    QFormLayout* ctl_layout = new QFormLayout();
    QVBoxLayout* preview_layout = new QVBoxLayout();

    ctl_layout->addRow(new QLabel("plop0"));
    ctl_layout->addRow(new QLabel("plop1"));
    ctl_layout->addRow(new QLabel("plop2"));
    ctl_layout->addRow(new QLabel("plop3"));
    ctl_view_layout->addLayout(ctl_layout);

    preview_layout->addWidget(preview_);
    ctl_view_layout->addLayout(preview_layout);
    main_layout->addLayout(ctl_view_layout);

    // Add ok/cancel buttons
    QPushButton* ok_btn = new QPushButton(tr("OK"));
    QPushButton* cancel_btn = new QPushButton(tr("Cancel"));
    QHBoxLayout* btn_layout = new QHBoxLayout();
    btn_layout->addWidget(ok_btn);
    btn_layout->addWidget(cancel_btn);
    main_layout->addLayout(btn_layout);

    connect(ok_btn,     SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel_btn, SIGNAL(clicked()), this, SLOT(reject()));
}

TweaksDialog::~TweaksDialog()
{

}

} // namespace waterial


