#ifndef RSJOBVIEWER_H
#define RSJOBVIEWER_H

#include <QWidget>

#include "utils/types.h"

class QTimeEdit;
class QTableView;
class QLabel;
class RsPlanModel;

class RSJobViewer : public QWidget
{
    Q_OBJECT
public:
    RSJobViewer(QWidget *parent = nullptr);
    virtual ~RSJobViewer();

    void setRS(db_id id);
    db_id m_rsId;

    void updatePlan();
    void updateRsInfo();

public slots:
    void updateInfo();

private slots:
    void showContextMenu(const QPoint &pos);

private:
    QTableView *view;
    RsPlanModel *model;

    QTimeEdit *timeEdit1;
    QTimeEdit *timeEdit2;
    QLabel *infoLabel;
};

#endif // RSJOBVIEWER_H
