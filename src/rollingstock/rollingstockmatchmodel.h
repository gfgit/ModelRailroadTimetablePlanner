#ifndef ROLLINGSTOCKMATCHMODEL_H
#define ROLLINGSTOCKMATCHMODEL_H

#include "utils/sqldelegate/isqlfkmatchmodel.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QRegularExpression>

class RollingstockMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    enum Columns
    {
        ModelCol = 0,
        NumberCol,
        SuffixCol,
        OwnerCol,
        NCols
    };
    RollingstockMatchModel(database &db, QObject *parent = nullptr);

    // QAbstractTableModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

private:
    struct RSItem
    {
        db_id rsId;
        QString modelName;
        QString modelSuffix;
        QString ownerName;
        int number;
        RsType type;
    };
    RSItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;

    QRegularExpression regExp;
    QByteArray model,owner,number;
};

#endif // ROLLINGSTOCKMATCHMODEL_H
