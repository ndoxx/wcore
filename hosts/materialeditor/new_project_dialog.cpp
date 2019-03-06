#include <iostream>

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>

#include "new_project_dialog.h"

namespace medit
{

NewProjectDialog::NewProjectDialog(QWidget* parent):
QDialog(parent, Qt::CustomizeWindowHint),
name_input_(new QLineEdit())
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    QFormLayout* form_layout = new QFormLayout();
    QHBoxLayout* btn_layout = new QHBoxLayout();

    QPushButton* ok_btn = new QPushButton("OK");
    QPushButton* cancel_btn = new QPushButton("Cancel");

    form_layout->addRow("project name:", name_input_);

    btn_layout->addWidget(ok_btn);
    btn_layout->addWidget(cancel_btn);

    main_layout->addLayout(form_layout);
    main_layout->addLayout(btn_layout);

    connect(ok_btn,     SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel_btn, SIGNAL(clicked()), this, SLOT(reject()));
}

QString NewProjectDialog::get_project_name() const
{
    return name_input_->text();
}


} // namespace medit
