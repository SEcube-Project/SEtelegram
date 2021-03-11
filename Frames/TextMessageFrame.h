#ifndef SETELEGRAM_TEXTMESSAGEFRAME_H
#define SETELEGRAM_TEXTMESSAGEFRAME_H

#include <QtWidgets/QFrame>

namespace Ui {
    class TextMessageFrame;
}

class TextMessageFrame: public QFrame {
    Q_OBJECT

public:
    explicit TextMessageFrame(QWidget *parent = nullptr,
            const std::string& sender_name = "",
            const std::string& text = "",
            bool error = false,
            bool is_from_me = false);
    ~TextMessageFrame() override;
    QSize sizeHint()const override;

private:
    Ui::TextMessageFrame *ui;
    std::string style_string;
    QSize* q = new QSize(500, 120);
};


#endif //SETELEGRAM_TEXTMESSAGEFRAME_H
