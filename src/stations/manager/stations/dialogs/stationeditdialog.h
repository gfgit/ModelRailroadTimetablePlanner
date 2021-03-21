#ifndef STATIONEDITDIALOG_H
#define STATIONEDITDIALOG_H

#include <QDialog>

namespace Ui {
class StationEditDialog;
}

class StationEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StationEditDialog(QWidget *parent = nullptr);
    ~StationEditDialog();

private:
    Ui::StationEditDialog *ui;
};

#endif // STATIONEDITDIALOG_H
