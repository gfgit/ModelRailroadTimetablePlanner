#ifndef LINESMATCHMODEL_H
#define LINESMATCHMODEL_H

#include "utils/delegates/sql/isqlfkmatchmodel.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QElapsedTimer>

class LinesMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT
public:
    LinesMatchModel(database &db, bool useTimer, QObject *parent = nullptr);
    ~LinesMatchModel();

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    void clearCache() override;

    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

protected:
    void timerEvent(QTimerEvent *e) override;

private:
    int timerId = 0;
    QElapsedTimer timer;

    struct LineItem
    {
        db_id lineId;
        QString name;
        char padding[4];
    };
    LineItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;
    QByteArray mQuery;
};

#endif // LINESMATCHMODEL_H
