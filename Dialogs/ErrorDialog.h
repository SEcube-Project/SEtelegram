#ifndef SETELEGRAM_ERRORDIALOG_H
#define SETELEGRAM_ERRORDIALOG_H

#include <QDialog>

namespace Ui {
    class ErrorDialog;
}

class ErrorDialog : public QDialog {
    Q_OBJECT

    public:
        explicit ErrorDialog(QWidget *parent = nullptr);
        ~ErrorDialog();

        void setErrorLabel(std::string error_str);
        void setHintLabel(std::string hint_str);

    private:
        Ui::ErrorDialog *ui;

};


#endif //SETELEGRAM_ERRORDIALOG_H
