#ifndef SETELEGRAM_IMAGEMESSAGEFRAME_H
#define SETELEGRAM_IMAGEMESSAGEFRAME_H

#include <QtWidgets/QFrame>

namespace Ui {
    class ImageMessageFrame;
}

class ImageMessageFrame: public QFrame {
Q_OBJECT

public:
    explicit ImageMessageFrame(QWidget *parent = nullptr,
                               const std::string& sender = "",
                               const std::string& capt = "",
                               bool caption_err = false,
                               bool is_from_me = false,
                               const std::string& image_id = "");
    ~ImageMessageFrame() override;
    QSize sizeHint()const override;
    void setImage(const std::string& path);
    std::string get_image_id();
    std::string image_path;

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

signals:
    void clicked();
    void release();
    void pressed();

private:
    Ui::ImageMessageFrame *ui;
    std::string style_string;
    std::string sender_name;
    std::string caption;
    std::string id;
    QSize* q = new QSize(0, 0);
};

#endif //SETELEGRAM_IMAGEMESSAGEFRAME_H
