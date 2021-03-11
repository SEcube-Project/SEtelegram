#include "DocumentMessageFrame.h"
#include "ui_DocumentMessageFrame.h"
#include "../global.h"

DocumentMessageFrame::DocumentMessageFrame(QWidget *parent,
        const std::string& sender_name, const std::string& document_name,
        const std::string& caption, bool caption_error, bool is_from_me) :
        QFrame(parent),
        ui(new Ui::DocumentMessageFrame) {
    ui->setupUi(this);

    this->setCursor(Qt::PointingHandCursor);

    style_string = "QFrame {"
                   "color: #272d30;"
                   "border-radius: 10px;"
                   "padding: 6px;";

    QPixmap icon("../resources/icons8-regular-document-64.png");
    QPixmap scaled = icon.scaled(34, 34,
            Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->iconLabel->setPixmap(scaled);
    ui->iconLabel->setStyleSheet("QLabel {padding: 4px 0px 0px 4px;}");

    ui->senderNameLabel->setText(sender_name.c_str());
    ui->senderNameLabel->setStyleSheet("QLabel {"
                                       "padding: 0px;"
                                       "color: #34baeb;"
                                       "font-weight: bold;}");
    auto sender_px_len = ui->senderNameLabel->fontMetrics().width(document_name.c_str());

    ui->documentNameLabel->setText(document_name.c_str());
    ui->documentNameLabel->setStyleSheet("QLabel {padding: 0px;}");
    auto doc_px_len = ui->documentNameLabel->fontMetrics().width(document_name.c_str());

    ui->captionLabel->setText(caption.c_str());
    ui->captionLabel->setStyleSheet("QLabel {padding: 0px;}");
    if (caption_error) {
        QFont font = ui->captionLabel->font();
        font.setBold(true); font.setItalic(true);
        ui->captionLabel->setFont(font);
    }
    auto caption_px_len = ui->captionLabel->fontMetrics().width(caption.c_str());

    // Set elements and frame width
    if (sender_px_len > doc_px_len && sender_px_len > caption_px_len) {
        if (sender_px_len > WIDE - 32) {
            std::string trimmed = textOverflow(
                    ui->senderNameLabel, sender_name, WIDE - 32);
            ui->senderNameLabel->setText(trimmed.c_str());
            sender_px_len = ui->senderNameLabel->fontMetrics().width(trimmed.c_str());
        }
        ui->senderNameLabel->setFixedWidth(sender_px_len);
        ui->documentNameLabel->setFixedWidth(doc_px_len);
        ui->captionLabel->setFixedWidth(caption_px_len);
        q->setWidth((int)sender_px_len + 32);
    }
    else if (doc_px_len > sender_px_len && doc_px_len > caption_px_len) {
        if (doc_px_len > WIDE - 72) {
            std::string trimmed = textOverflow(
                    ui->documentNameLabel, document_name, WIDE - 72);
            ui->documentNameLabel->setText(trimmed.c_str());
            doc_px_len = ui->documentNameLabel->fontMetrics().width(trimmed.c_str());
        }
        ui->senderNameLabel->setFixedWidth(sender_px_len);
        ui->documentNameLabel->setFixedWidth(doc_px_len);
        ui->captionLabel->setFixedWidth(caption_px_len);
        q->setWidth((int)doc_px_len + 72);
    }
    else if (caption_px_len > sender_px_len && caption_px_len > doc_px_len) {
        if (caption_px_len >= WIDE - 32) {
            std::string wrapped_text;
            int longest_line_len = 0;
            int num_rows = textWrap(caption, wrapped_text,
                                    ui->captionLabel->fontMetrics(), WIDE - 32, longest_line_len);
            ui->captionLabel->setText(wrapped_text.c_str());
            ui->captionLabel->setFixedHeight((num_rows * ui->captionLabel->fontMetrics().lineSpacing()) + 8);
            ui->captionLabel->setFixedWidth(longest_line_len);
            q->setWidth(longest_line_len + 32);
            ui->senderNameLabel->setFixedWidth(sender_px_len);
            ui->documentNameLabel->setFixedWidth(doc_px_len);
        }
        else {
            ui->senderNameLabel->setFixedWidth(sender_px_len);
            ui->documentNameLabel->setFixedWidth(doc_px_len);
            ui->captionLabel->setFixedWidth(caption_px_len);
            q->setWidth((int)caption_px_len + 32);
        }
    }

    // Set elements and frame height
    q->setHeight(ui->documentNameLabel->height() + 14);
    if (sender_name.empty()) {
        ui->senderNameLabel->hide();
        ui->iconLabel->move(8, 7);
        ui->documentNameLabel->move(52, 7);
        ui->captionLabel->move(16, 40);
    }
    else {
        int new_height = q->height() + ui->senderNameLabel->height();
        q->setHeight(new_height);
    }
    if (caption.empty()) {
        ui->captionLabel->hide();
    }
    else {
        int new_height = q->height() + ui->captionLabel->height() - 14;
        q->setHeight(new_height);
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

DocumentMessageFrame::~DocumentMessageFrame() {
    delete ui;
    delete q;
}

QSize DocumentMessageFrame::sizeHint()const {
    return *q;
}

void DocumentMessageFrame::mousePressEvent(QMouseEvent*) {
    emit pressed();
}

void DocumentMessageFrame::mouseReleaseEvent(QMouseEvent*) {
    emit release();
    emit clicked();
}
