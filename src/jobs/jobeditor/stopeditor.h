#ifndef STOPEDITOR_H
#define STOPEDITOR_H

#include <QFrame>
#include <QTime>

#include "utils/types.h"

class QComboBox;
class CustomCompletionLineEdit;
class QTimeEdit;
class QGridLayout;

class StationsMatchModel;
class StationLinesListModel;

namespace sqlite3pp {
class database;
}

class StopEditor : public QFrame
{
    Q_OBJECT
public:
    StopEditor(sqlite3pp::database &db, QWidget *parent = nullptr);

    virtual bool eventFilter(QObject *watched, QEvent *ev) override;

    void setPrevDeparture(const QTime& prevTime);
    void setArrival(const QTime& arr);
    void setDeparture(const QTime& dep);
    void setStation(db_id stId);
    void setCurLine(db_id lineId);
    void setNextLine(db_id lineId);
    void setStopType(int type);
    void setPrevSt(db_id stId);

    void calcInfo();

    QTime getArrival();
    QTime getDeparture();
    db_id getCurLine();

    db_id getNextLine();
    db_id getStation();

    bool closeOnLineChosen() const;
    void setCloseOnLineChosen(bool value);

signals:
    void lineChosen(StopEditor *ed);

public slots:
    void popupLinesCombo();

private slots:
    void onStationSelected(db_id stId);
    void onNextLineChanged(int index);

    void comboBoxViewContextMenu(const QPoint &pos);
    void linesLoaded();

    void arrivalChanged(const QTime &arrival);

private:
    QGridLayout *lay;
    CustomCompletionLineEdit *mLineEdit;
    QTimeEdit *arrEdit;
    QTimeEdit *depEdit;
    QComboBox *linesCombo;

    StationsMatchModel *stationsMatchModel;
    StationLinesListModel *linesModel;

    db_id curLineId;
    db_id nextLineId;
    db_id stopId;
    db_id stationId;
    db_id mPrevSt;

    QTime lastArrival;

    int stopType;
    bool m_closeOnLineChosen;
};

#endif // STOPEDITOR_H
