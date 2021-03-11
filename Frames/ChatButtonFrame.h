#ifndef SETELEGRAM_CHATBUTTONFRAME_H
#define SETELEGRAM_CHATBUTTONFRAME_H

#include <QtWidgets/QFrame>

namespace Ui {
    class ChatButtonFrame;
}

class ChatButtonFrame: public QFrame {
Q_OBJECT

public:
    explicit ChatButtonFrame(QWidget *parent = nullptr,
                             const std::string& chat_name = "",
                             std::int32_t unread_count = 0);
    ~ChatButtonFrame() override;
    void selected(bool selected);
    void setUnreadCounter(std::int32_t count);

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEvent *ev) override {
        if (!selected_) {
            setStyleSheet("border: none; background-color: #e6e8e8;");
        }
    }
    void leaveEvent(QEvent *ev) override {
        if (!selected_) {
            setStyleSheet("border: none; background-color: #f7fafa;");
        }
    }

signals:
    void clicked();
    void release();
    void pressed();

private:
    Ui::ChatButtonFrame *ui;
    std::string chat_name_;
    bool selected_{};
    void setLabels(const std::string& chat_name, const std::string& unread_count_str);
};


#endif //SETELEGRAM_CHATBUTTONFRAME_H
