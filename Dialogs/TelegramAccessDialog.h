#ifndef SETELEGRAM_TELEGRAMACCESSDIALOG_H
#define SETELEGRAM_TELEGRAMACCESSDIALOG_H

#include <QDialog>

namespace Ui { class TelegramAccessDialog; }

class TelegramAccessDialog: public QDialog {
Q_OBJECT

public:
    TelegramAccessDialog(QWidget *parent = nullptr, const std::string& text = "");
    ~TelegramAccessDialog();
    std::string input;

private slots:
    void okButtonClicked();

private:
    Ui::TelegramAccessDialog *ui;
};


#endif //SETELEGRAM_TELEGRAMACCESSDIALOG_H
