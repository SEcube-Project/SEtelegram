#include "ImageMessageFrame.h"
#include "ui_ImageMessageFrame.h"
#include "../global.h"
#include <QtGui/QDesktopServices>
#include <QCloseEvent>
#include <QSizePolicy>

ImageMessageFrame::ImageMessageFrame(QWidget *parent, const std::string& sender,
        const std::string& capt, bool caption_err, bool is_from_me, const std::string& image_id)
        : QFrame(parent), ui(new Ui::ImageMessageFrame) {
    ui->setupUi(this);
    this->setCursor(Qt::PointingHandCursor);

    style_string = "QFrame {"
                   "color: #272d30;"
                   "border-radius: 10px;";

    sender_name = sender;
    caption = capt;
    id = image_id;

    if (caption_err) {
        QFont font = ui->captionLabel->font();
        font.setBold(true); font.setItalic(true);
        ui->captionLabel->setFont(font);
    }
    if (is_from_me) {
        style_string.append("background-color: #d8f7cb;}");
    }
    else {
        style_string.append("background-color: #f7fafa;}");
    }
    this->setStyleSheet(style_string.c_str());
}

ImageMessageFrame::~ImageMessageFrame() {
    delete ui;
    delete q;
}

QSize ImageMessageFrame::sizeHint()const {
    return *q;
}

void ImageMessageFrame::mousePressEvent(QMouseEvent*) {
    emit pressed();
}

void ImageMessageFrame::mouseReleaseEvent(QMouseEvent*) {
    emit release();
    emit clicked();
}

void ImageMessageFrame::setImage(const std::string &path) {
    image_path = path;

    // Set elements and frame width
    QPixmap image(image_path.c_str());
    QPixmap scaled = image.scaled(500, 500,
            Qt::KeepAspectRatio,Qt::SmoothTransformation);
    QIcon image_icon(scaled);
    int image_width = scaled.rect().size().width();

    ui->imageToolButton->setIcon(image_icon);
    ui->imageToolButton->setIconSize(scaled.rect().size());
    ui->imageToolButton->setFixedWidth(image_width);

    ui->captionLabel->setFixedWidth(image_width);
    ui->senderNameLabel->setFixedWidth(image_width);

    ui->imageToolButton->setFixedHeight(scaled.rect().size().height());
    ui->imageToolButton->setStyleSheet("QToolButton {"
                                       "border-radius: 8px;"
                                       "border: 0px;}");
    q->setWidth(image_width + 32);

    // Set elements and frame height
    q->setHeight(ui->imageToolButton->height() + 32);
    if (sender_name.empty()) {
        ui->senderNameLabel->hide();
        ui->imageToolButton->move(16, 16);
    }
    else {
        int sender_px_len = ui->senderNameLabel->fontMetrics().width(sender_name.c_str());
        if (sender_px_len > q->width()-32) {
            std::string trimmed = textOverflow(
                    ui->senderNameLabel, sender_name, q->width() - 32);
            ui->senderNameLabel->setText(trimmed.c_str());
            sender_px_len = ui->senderNameLabel->fontMetrics().width(trimmed.c_str());
            ui->senderNameLabel->setFixedWidth(sender_px_len);
        }
        else {
            ui->senderNameLabel->setText(sender_name.c_str());
        }
        ui->senderNameLabel->setStyleSheet("color: #34baeb;");
        int new_height = q->height() + ui->senderNameLabel->height();
        q->setHeight(new_height);
    }

    if (caption.empty()) {
        ui->captionLabel->hide();
    }
    else {
        ui->captionLabel->move(16,
                               ui->imageToolButton->pos().y() +
                               ui->imageToolButton->height() + 2);

        int caption_px_len = ui->captionLabel->fontMetrics().width(caption.c_str());
        if (caption_px_len > image_width) {
            std::string wrapped_text;
            int longest_line_len = 0;
            int num_rows = textWrap(caption, wrapped_text,
                                    ui->captionLabel->fontMetrics(), image_width, longest_line_len);
            ui->captionLabel->setText(wrapped_text.c_str());
            ui->captionLabel->setFixedHeight((num_rows * ui->captionLabel->fontMetrics().lineSpacing()) + 8);

            int new_height = q->height() + ui->captionLabel->height();
            q->setHeight(new_height);
        }
        else {
            ui->captionLabel->setText(caption.c_str());
            int new_height = q->height() + ui->captionLabel->height();
            q->setHeight(new_height);
        }
    }

    updateGeometry();

    QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setSizePolicy(sp);

    connect(ui->imageToolButton, &QToolButton::clicked,
            [this]() { QDesktopServices::openUrl(QUrl::fromLocalFile(image_path.c_str())); });
}

std::string ImageMessageFrame::get_image_id() {
    return id;
}
