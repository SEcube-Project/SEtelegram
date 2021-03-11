#include "TelegramAccessDialog.h"
#include "ui_TelegramAccessDialog.h"
#include <QStyle>
#include <QDesktopWidget>
#include "../Td.h"
#include <iostream>

TelegramAccessDialog::TelegramAccessDialog(QWidget *parent, const std::string& text)
        : QDialog(parent)
        , ui(new Ui::TelegramAccessDialog) {

    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                    this->size(), qApp->desktop()->availableGeometry()));

    ui->instructionLabel->setText(text.c_str());

    ui->okButton->setEnabled(false);
    connect(ui->inputEdit, &QLineEdit::textChanged, [this](){
        ui->okButton->setEnabled(!ui->inputEdit->text().isEmpty());});
    connect(ui->okButton, &QPushButton::clicked, [this](){okButtonClicked();});
}

void TelegramAccessDialog::okButtonClicked() {
    input = ui->inputEdit->text().toStdString();
    TelegramAccessDialog::accept();
}

TelegramAccessDialog::~TelegramAccessDialog() {
    delete ui;
}

