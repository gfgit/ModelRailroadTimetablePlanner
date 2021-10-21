#ifndef SCENESELECTIONMODEL_H
#define SCENESELECTIONMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include "utils/types.h"
#include "graph/linegraphtypes.h"

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

    explicit SceneSelectionModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void addEntry(const Entry& entry);
    void removeAt(int row);
    void moveRow(int row, bool up);

public slots:
    void removeAll();

private:
    QVector<Entry> entries;
    LineGraphType selectedType;
    SelectionMode selectionMode;
};

#endif // SCENESELECTIONMODEL_H
