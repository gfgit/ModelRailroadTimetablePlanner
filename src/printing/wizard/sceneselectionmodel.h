/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SCENESELECTIONMODEL_H
#define SCENESELECTIONMODEL_H

#include <QAbstractTableModel>
#include "printing/helper/model/igraphscenecollection.h"

#include <QVector>

#include "utils/types.h"
#include "graph/linegraphtypes.h"

#include <sqlite3pp/sqlite3pp.h>

class SceneSelectionModel : public QAbstractTableModel, public IGraphSceneCollection
{
    Q_OBJECT

public:
    enum SelectionMode
    {
        UseSelectedEntries = 0,
        AllOfTypeExceptSelected,
        NModes
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
    inline SelectionMode getMode() const { return selectionMode; }
    inline LineGraphType getSelectedType() const { return selectedType; }

    // IGraphSceneCollection
    qint64 getItemCount() override;
    bool startIteration() override;
    SceneItem getNextItem() override;

    static QString getModeName(SelectionMode mode);

signals:
    void selectionModeChanged(int mode, int type);
    void selectionCountChanged();

public slots:
    void removeAll();

private:
    Entry getNextEntry();
    void keepOnlyType(LineGraphType type);

private:
    sqlite3pp::database &mDb;
    sqlite3pp::query mQuery;

    QVector<Entry> entries;
    LineGraphType selectedType;
    SelectionMode selectionMode;

    qint64 cachedCount;
    int iterationIdx;
};

#endif // SCENESELECTIONMODEL_H
