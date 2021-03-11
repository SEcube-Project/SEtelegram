#include "TextMessageFrame.h"
#include "ui_TextMessageFrame.h"
#include "../global.h"

TextMessageFrame::TextMessageFrame(QWidget *parent,
        const std::string& sender_name, const std::string& text,
        bool error, bool is_from_me)
        : QFrame(parent),
          ui(new Ui::TextMessageFrame) {
    ui->setupUi(this);

    style_string = "QFrame {"
                   "color: #272d30;"
                   "border-radius: 10px;"
                   "padding: 6px;";

    ui->textLabel->setText(text.c_str());
    ui->textLabel->setStyleSheet("QLabel {padding: 0px}");
    if (error) {
        QFont font = ui->textLabel->font();
        font.setBold(true); font.setItalic(true);
        ui->textLabel->setFont(font);
    }
    auto text_px_len = ui->textLabel->fontMetrics().width(text.c_str());

    ui->senderNameLabel->setText(sender_name.c_str());
    ui->senderNameLabel->setStyleSheet("color: #34baeb; "
                                       "font-weight: bold;"
                                       "padding: 0px;");
    auto sender_px_len = ui->senderNameLabel->fontMetrics().width(sender_name.c_str());

    if (sender_px_len > text_px_len) {
        ui->senderNameLabel->setFixedWidth(sender_px_len);
        ui->textLabel->setFixedWidth(text_px_len);
        q->setWidth((int)sender_px_len + 32);
    }
    else {
        if (text_px_len >= WIDE - 32) {
            std::string wrapped_text;
            int longest_line_len = 0;
            int num_rows = textWrap(text, wrapped_text, ui->textLabel->fontMetrics(),
                                    WIDE - 32, longest_line_len);
            ui->textLabel->setText(wrapped_text.c_str());
            ui->textLabel->setFixedHeight((num_rows * ui->textLabel->fontMetrics().lineSpacing()) + 8);
            ui->textLabel->setFixedWidth(longest_line_len);
            q->setWidth(longest_line_len + 32);
        }
        else {
            ui->textLabel->setFixedWidth(text_px_len);
            q->setWidth((int)text_px_len + 32);
        }
    }

    if (sender_name.empty()) {
        ui->senderNameLabel->hide();
        ui->textLabel->move(14, 7);
        q->setHeight(ui->textLabel->height() + 14);
    }
    else {
        ui->senderNameLabel->setText(sender_name.c_str());
        q->setHeight(ui->textLabel->height() + 14 + ui->senderNameLabel->height());
    }
    updateGeometry();

    if (is_from_me) {
        style_string.append("background-color: #d8f7cb;"
                            "border: 2px mediumseagreen;}");
    }
    else {
        style_string.append("background-color: #f7fafa;"
                            "border: 2px silver;}");
    }
    this->setStyleSheet(style_string.c_str());

    QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setSizePolicy(sp);
}

TextMessageFrame::~TextMessageFrame() {
    delete ui;
    delete q;
}

QSize TextMessageFrame::sizeHint() const {
    return *q;
}
