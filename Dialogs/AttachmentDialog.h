#ifndef SETELEGRAM_ATTACHMENTDIALOG_H
#define SETELEGRAM_ATTACHMENTDIALOG_H

#include <QInputDialog>

namespace Ui {
    class AttachmentDialog;
}

class AttachmentDialog : public QDialog {
    Q_OBJECT
public:
    explicit AttachmentDialog(QWidget *parent = nullptr, std::string file_name = "");
    ~AttachmentDialog();
    std::string getCaption();

private:
    Ui::AttachmentDialog *ui;
};

#endif //SETELEGRAM_ATTACHMENTDIALOG_H
