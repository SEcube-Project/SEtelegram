#include "ErrorDialog.h"
#include "ui_ErrorDialog.h"
#include <QPushButton>

ErrorDialog::ErrorDialog(QWidget *parent) :
        QDialog(parent),
        ui(new Ui::ErrorDialog)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    ui->buttonBox->button(QDialogButtonBox::Cancel)->hide();
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &ErrorDialog::close);
}

ErrorDialog::~ErrorDialog()
{
    delete ui;
}

void ErrorDialog::setErrorLabel(std::string error_str) {
    ui->errorLabel->setText(error_str.c_str());
}

void ErrorDialog::setHintLabel(std::string hint_str) {
    if (hint_str.empty()) {
        ui->errorLabel->move(20, 100);
    }
    else {
        ui->hintLabel->setText(hint_str.c_str());
    }
}
