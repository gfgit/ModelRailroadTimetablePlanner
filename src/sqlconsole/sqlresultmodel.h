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

#ifndef SQLRESULTMODEL_H
#define SQLRESULTMODEL_H

#ifdef ENABLE_USER_QUERY

#    include <QAbstractTableModel>
#    include <QColor>

namespace sqlite3pp {
class query;
}

class SQLResultModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SQLResultModel(QObject *parent = nullptr);
    virtual ~SQLResultModel() override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setBackground(const QColor &col);
    bool initFromQuery(sqlite3pp::query *q);

signals:
    void error(int err, int extendedErr, const QString &msg);

private:
    QStringList colNames;
    QVector<QVariant> m_data;
    int colCount;
    QColor backGround;
};

#endif // ENABLE_USER_QUERY

#endif // SQLRESULTMODEL_H
