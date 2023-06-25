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

#ifndef FILTERHEADERVIEW_H
#define FILTERHEADERVIEW_H

#include <QHeaderView>
#include <QVector>

class QTableView;
class FilterHeaderLineEdit;

/*!
 * \brief The FilterHeaderView class
 *
 * Header view which allows to filter on columns
 *
 * \sa IPagedItemModel
 * \sa FilterHeaderLineEdit
 */
class FilterHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit FilterHeaderView(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    void setModel(QAbstractItemModel *m) override;

    bool hasFilters() const
    {
        return (filterWidgets.size() > 0);
    }
    QString filterValue(int column) const;

    void installOnTable(QTableView *view);

public slots:
    void generateFilters();
    void adjustPositions();
    void clearFilters();
    void setFilter(int column, const QString &value);
    void updateSoring(int col);

signals:
    void filterChanged(int column, const QString &value);

protected:
    void updateGeometries() override;

private slots:
    void inputChanged(FilterHeaderLineEdit *w, const QString &new_value);
    void showColumnTooltip(FilterHeaderLineEdit *w, const QPoint &globalPos);

private:
    QVector<FilterHeaderLineEdit *> filterWidgets;
};

#endif // FILTERHEADERVIEW_H
