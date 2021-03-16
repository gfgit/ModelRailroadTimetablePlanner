#ifndef STATIONFREERSVIEWER_H
#define STATIONFREERSVIEWER_H

#include <QWidget>

#include "utils/types.h"

class StationFreeRSModel;
class QTimeEdit;
class QTableView;
class QPushButton;

class StationFreeRSViewer : public QWidget
{
    Q_OBJECT
public:
    explicit StationFreeRSViewer(QWidget *parent = nullptr);

    void setStation(db_id stId);
    void updateTitle();
    void updateData();

public slots:
    void goToNext();
    void goToPrev();

private slots:
    void onTimeEditingFinished();

    void showContextMenu(const QPoint &pos);

    void sectionClicked(int col);

private:
    StationFreeRSModel *model;
    QTableView *view;

    QTimeEdit *timeEdit;
    QPushButton *refreshBut;
    QPushButton *nextOpBut;
    QPushButton *prevOpBut;
};

#endif // STATIONFREERSVIEWER_H
