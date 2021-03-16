#ifndef SHIFTBUSYBOX_H
#define SHIFTBUSYBOX_H

#include <QDialog>

#include <QVector>

class QLabel;
class QTableView;

class ShiftBusyModel;

class ShiftBusyDlg : public QDialog
{
    Q_OBJECT
public:
    explicit ShiftBusyDlg(QWidget *parent = nullptr);

    void setModel(ShiftBusyModel *m);
private:
    QLabel *m_label;
    QTableView *view;

    ShiftBusyModel *model;
};

#endif // SHIFTBUSYBOX_H
