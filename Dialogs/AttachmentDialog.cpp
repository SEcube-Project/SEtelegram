#include "AttachmentDialog.h"
#include "ui_AttachmentDialog.h"
#include "../global.h"
#include <QPushButton>

AttachmentDialog::AttachmentDialog(QWidget *parent, std::string file_name) :
        QDialog(parent),
        ui(new Ui::AttachmentDialog) {
    ui->setupUi(this);
    this->setFixedSize(this->size());

    QPixmap icon("../resources/document.png");
    QPixmap scaled = icon.scaled(60, 60,
            Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->iconLabel->setPixmap(scaled);
    ui->iconLabel->setStyleSheet("QLabel {padding: 0px 0px 0px 8px;}");

    if (file_name.length() > 40) {
        ui->fileNameLabel->setText(textOverflow(ui->fileNameLabel, file_name, ui->fileNameLabel->width()).c_str());
    }
    else {
        ui->fileNameLabel->setText(file_name.c_str());
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Send");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Cancel");
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &AttachmentDialog::accept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
            this, &AttachmentDialog::reject);
}

AttachmentDialog::~AttachmentDialog() {
    delete ui;
}

std::string AttachmentDialog::getCaption() {
    return ui->captionEdit->toPlainText().toStdString();
}
