#ifndef SCENESELECTIONMODEL_H
#define SCENESELECTIONMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include "utils/types.h"
#include "graph/linegraphtypes.h"

#include <sqlite3pp/sqlite3pp.h>

class SceneSelectionModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum SelectionMode
    {
        UseSelectedEntries = 0,
        AllOfTypeExceptSelected
    };

    enum Columns
    {
        TypeCol = 0,
        NameCol,
        NCols
    };

    struct Entry
    {
        db_id objectId;
        QString name;
        LineGraphType type;
    };

    SceneSelectionModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    bool addEntry(const Entry& entry);
    void removeAt(int row);
    void moveRow(int row, bool up);

    void setMode(SelectionMode mode, LineGraphType type);

    qint64 getSelectionCount() const;
    bool startIteration();
    Entry getNextEntry();

public slots:
    void removeAll();

private:
    void keepOnlyType(LineGraphType type);

private:
    sqlite3pp::database &mDb;
    sqlite3pp::query mQuery;

    QVector<Entry> entries;
    LineGraphType selectedType;
    SelectionMode selectionMode;
    int iterationIdx;
};

#endif // SCENESELECTIONMODEL_H
