#ifndef SETELEGRAM_DOCUMENTMESSAGEFRAME_H
#define SETELEGRAM_DOCUMENTMESSAGEFRAME_H

#include <QLabel>

namespace Ui {
    class DocumentMessageFrame;
}

class DocumentMessageFrame: public QFrame {
Q_OBJECT

public:
    explicit DocumentMessageFrame(QWidget *parent = nullptr,
            const std::string& sender_name = "",
            const std::string& document_name = "",
            const std::string& caption = "",
            bool caption_error = false,
            bool is_from_me = false);
    ~DocumentMessageFrame() override;
    QSize sizeHint()const override;

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

signals:
    void clicked();
    void release();
    void pressed();

private:
    Ui::DocumentMessageFrame *ui;
    std::string style_string;
    QSize* q = new QSize(500, 120);
};

#endif //SETELEGRAM_DOCUMENTMESSAGEFRAME_H
