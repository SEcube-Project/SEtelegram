#include "ui_ChatButtonFrame.h"
#include "ChatButtonFrame.h"
#include "../global.h"

ChatButtonFrame::ChatButtonFrame(QWidget *parent, const std::string& chat_name, std::int32_t unread_count)
        : QFrame(parent), ui(new Ui::ChatButtonFrame) {
    ui->setupUi(this);

    this->setCursor(Qt::PointingHandCursor);
    this->setStyleSheet("border: none; background-color: #f7fafa; width: available;");
    this->setFixedHeight(50);

    ui->unreadCountLabel->setFixedHeight(32);
    ui->chatNameLabel->setStyleSheet("color: black;");
    ui->unreadCountLabel->setStyleSheet("background-color: #2f9fe0;" //#bfbfbf"
                                        "color: white; border-radius: 16px;"
                                        "font-weight: 500;");
    chat_name_ = chat_name;
    setUnreadCounter(unread_count);
}

ChatButtonFrame::~ChatButtonFrame() {
    delete ui;
}

void ChatButtonFrame::mousePressEvent(QMouseEvent*) {
    emit pressed();
}

void ChatButtonFrame::mouseReleaseEvent(QMouseEvent*) {
    emit release();
    emit clicked();
}

void ChatButtonFrame::selected(bool selected) {
    selected_ = selected;
    if (selected_) {
        this->setStyleSheet("border: none; background-color: #2f9fe0;");
        ui->chatNameLabel->setStyleSheet("color: white;");
    }
    else {
        this->setStyleSheet("border: none; background-color: #f7fafa;");
        ui->chatNameLabel->setStyleSheet("color: black;");
    }
    setUnreadCounter(0);
}

void ChatButtonFrame::setUnreadCounter(std::int32_t count) {
    if (count == 0) {
        ui->unreadCountLabel->hide();
        setLabels(chat_name_, "");
    }
    else {
        ui->unreadCountLabel->show();
        setLabels(chat_name_, std::to_string(count));
    }
}

void ChatButtonFrame::setLabels(const std::string& chat_name, const std::string& unread_count_str) {
    ui->unreadCountLabel->setText(unread_count_str.c_str());
    auto unread_counter_px_len = ui->unreadCountLabel->fontMetrics().width(unread_count_str.c_str());
    ui->unreadCountLabel->setFixedWidth(unread_counter_px_len+14 > 32 ? unread_counter_px_len+14 : 32);
    ui->unreadCountLabel->move(this->width() - 10 - ui->unreadCountLabel->width(), 10);

    auto chat_name_px_len = ui->chatNameLabel->fontMetrics().width(chat_name.c_str());
    ui->chatNameLabel->setFixedWidth(this->width() - (unread_counter_px_len + 50));

    if(chat_name_px_len > ui->chatNameLabel->width()) {
        std::string trimmed = textOverflow(ui->chatNameLabel, chat_name, ui->chatNameLabel->width());
        ui->chatNameLabel->setText(trimmed.c_str());
    }
    else {
        ui->chatNameLabel->setText(chat_name.c_str());
    }
}
