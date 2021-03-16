#ifndef MEETINGINFORMATIONDIALOG_H
#define MEETINGINFORMATIONDIALOG_H

#include <QDialog>

class QLineEdit;

namespace Ui {
class MeetingInformationDialog;
}

class MeetingInformationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MeetingInformationDialog(QWidget *parent = nullptr);
    ~MeetingInformationDialog();

public:
    bool loadData();
    void saveData();

private slots:
    void showImage();
    void importImage();
    void removeImage();

    void toggleHeader();
    void toggleFooter();

    void updateMinumumDate();

private:
    void setSheetText(QLineEdit *lineEdit, QPushButton *but, const QString &text, bool isNull);

private:
    Ui::MeetingInformationDialog *ui;
    QImage img;

    bool needsToSaveImg;
    bool headerIsNull;
    bool footerIsNull;
};

#endif // MEETINGINFORMATIONDIALOG_H
