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

#include "filterheaderview.h"
#include "filterheaderlineedit.h"

#include "pageditemmodel.h"

#include <QApplication>
#include <QTableView>
#include <QScrollBar>

#include <QToolTip>

FilterHeaderView::FilterHeaderView(QWidget *parent) :
    QHeaderView(Qt::Horizontal, parent)
{
    // Activate the click signals to allow sorting
    setSectionsClickable(true);
    setSortIndicatorShown(true);

    // Make sure to not automatically resize the columns according to the contents
    setSectionResizeMode(QHeaderView::Interactive);

    // Highlight column headers of selected cells to emulate spreadsheet behaviour
    setHighlightSections(true);

    // Do some connects: Basically just resize and reposition the input widgets whenever anything
    // changes
    connect(this, &FilterHeaderView::sectionResized, this, &FilterHeaderView::adjustPositions);

    connect(this, &FilterHeaderView::sectionClicked, this, &FilterHeaderView::updateSoring);

    // Set custom context menu handling
    setContextMenuPolicy(Qt::CustomContextMenu);
}

void FilterHeaderView::generateFilters()
{
    bool needsUpdateGeo = false;

    IPagedItemModel *m  = qobject_cast<IPagedItemModel *>(model());
    if (m)
    {
        int i                 = 0;
        const int columnCount = m->columnCount();
        for (int col = 0; col < columnCount; col++)
        {
            auto filter = m->getFilterAtCol(col);
            if (!filter.second.testFlag(IPagedItemModel::FilterFlag::BasicFiltering))
            {
                // No filtering for this column
                for (; i < filterWidgets.size(); i++)
                {
                    if (filterWidgets.at(i)->getColumn() > col)
                        break; // We went past

                    if (filterWidgets.at(i)->getColumn() == col)
                    {
                        // Remove old filter
                        delete filterWidgets.takeAt(i);
                        i--;
                    }
                }
                continue;
            }

            FilterHeaderLineEdit *filterEdit = nullptr;
            for (; i < filterWidgets.size(); i++)
            {
                if (filterWidgets.at(i)->getColumn() > col)
                    break; // We went past

                if (filterWidgets.at(i)->getColumn() == col)
                {
                    filterEdit = filterWidgets.at(i);
                    break;
                }
            }

            if (!filterEdit)
            {
                filterEdit = new FilterHeaderLineEdit(col, this);
                connect(filterEdit, &FilterHeaderLineEdit::delayedTextChanged, this,
                        &FilterHeaderView::inputChanged);
                connect(filterEdit, &FilterHeaderLineEdit::tooltipRequested, this,
                        &FilterHeaderView::showColumnTooltip);
                filterWidgets.insert(i, filterEdit);
                needsUpdateGeo = true;
            }

            filterEdit->updateTextWithoutEmitting(filter.first);
            filterEdit->setAllowNull(filter.second.testFlag(IPagedItemModel::ExplicitNULL));
        }

        // Update sorting
        setSortIndicator(m->getSortingColumn(), Qt::AscendingOrder);
    }
    else
    {
        qDeleteAll(filterWidgets);
        filterWidgets.clear();
    }

    if (needsUpdateGeo || filterWidgets.isEmpty())
    {
        // Position them correctly
        updateGeometries();
    }
}

QSize FilterHeaderView::sizeHint() const
{
    // For the size hint just take the value of the standard implementation and add the height of a
    // input widget to it if necessary
    QSize s = QHeaderView::sizeHint();
    if (filterWidgets.size())
        s.setHeight(s.height() + filterWidgets.at(0)->sizeHint().height()
                    + 4); // The 4 adds just adds some extra space
    return s;
}

void FilterHeaderView::updateGeometries()
{
    // If there are any input widgets add a viewport margin to the header to generate some empty
    // space for them which is not affected by scrolling
    if (filterWidgets.size())
        setViewportMargins(0, 0, 0, filterWidgets.at(0)->sizeHint().height());
    else
        setViewportMargins(0, 0, 0, 0);

    // Now just call the parent implementation and reposition the input widgets
    QHeaderView::updateGeometries();
    adjustPositions();
}

void FilterHeaderView::adjustPositions()
{
    int y = QHeaderView::sizeHint().height();

    // Sometimes it's 0 when recursion happen, skip moving widgets to avoid flickering
    if (y == 0)
        return;

    // The two adds some extra space between the header label and the input widget
    y += 2;

    // Loop through all widgets
    for (int i = 0; i < filterWidgets.size(); ++i)
    {
        // Get the current widget, move it and resize it
        FilterHeaderLineEdit *w = filterWidgets.at(i);
        const int col           = w->getColumn();
        if (QApplication::layoutDirection() == Qt::RightToLeft)
            w->move(width() - (sectionPosition(col) + sectionSize(col) - offset()), y);
        else
            w->move(sectionPosition(col) - offset(), y);
        w->resize(sectionSize(col), w->sizeHint().height());
    }
}

void FilterHeaderView::inputChanged(FilterHeaderLineEdit *w, const QString &new_value)
{
    // Just get the column number and the new value and send them to anybody interested in filter
    // changes
    emit filterChanged(w->getColumn(), new_value);

    IPagedItemModel *m = qobject_cast<IPagedItemModel *>(model());
    if (!m)
        return;

    m->setFilterAtCol(w->getColumn(), new_value);
    m->refreshData(true);
}

void FilterHeaderView::showColumnTooltip(FilterHeaderLineEdit *w, const QPoint &globalPos)
{
    int col = w->getColumn();
    QString str;
    if (w->allowNull())
        str = tr("Type <b>#NULL</b> to filter empty cells.");

    QString modelTooltip;
    if (model()) // Get model header tooltip at requested column
        modelTooltip = model()->headerData(col, Qt::Horizontal, Qt::ToolTipRole).toString();

    if (!modelTooltip.isEmpty())
    {
        if (!str.isEmpty())
            str.append(QLatin1String("<br><br>"));         // Separate with a line between
        modelTooltip.replace('\n', QLatin1String("<br>")); // Use HTML line break
        str.append(modelTooltip);
    }

    if (str.isEmpty())
        QToolTip::hideText();
    else
        QToolTip::showText(globalPos, str, w);
}

void FilterHeaderView::clearFilters()
{
    for (FilterHeaderLineEdit *filterLineEdit : std::as_const(filterWidgets))
        filterLineEdit->clear();
}

void FilterHeaderView::setFilter(int column, const QString &value)
{
    Q_UNUSED(value)
    for (FilterHeaderLineEdit *filterLineEdit : std::as_const(filterWidgets))
    {
        if (filterLineEdit->getColumn() == column)
        {
            filterLineEdit->clear();
            break;
        }
    }
}

void FilterHeaderView::updateSoring(int col)
{
    IPagedItemModel *m = qobject_cast<IPagedItemModel *>(model());
    if (!m)
        return;

    m->setSortingColumn(col);
    setSortIndicator(m->getSortingColumn(), Qt::AscendingOrder);
}

QString FilterHeaderView::filterValue(int column) const
{
    for (FilterHeaderLineEdit *filterLineEdit : std::as_const(filterWidgets))
    {
        if (filterLineEdit->getColumn() == column)
        {
            return filterLineEdit->text();
        }
    }

    return QString();
}

void FilterHeaderView::installOnTable(QTableView *view)
{
    view->setHorizontalHeader(this);

    connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, this,
            &FilterHeaderView::adjustPositions);
    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, this,
            &FilterHeaderView::adjustPositions);

    // Disconnect clicking in header to select column, since we will use it for sorting.
    // Note that, in order to work, this cannot be converted to the standard C++11 format.
    disconnect(this, SIGNAL(sectionPressed(int)), view, SLOT(selectColumn(int)));
    disconnect(this, SIGNAL(sectionEntered(int)), view, SLOT(_q_selectColumn(int)));

    // Must be enabled again because QTableView disables it if sorting not enabled in QTableView
    setSortIndicatorShown(true);
}

void FilterHeaderView::setModel(QAbstractItemModel *m)
{
    IPagedItemModel *pagedModel = qobject_cast<IPagedItemModel *>(model());
    if (pagedModel)
    {
        disconnect(pagedModel, &IPagedItemModel::filterChanged, this,
                   &FilterHeaderView::generateFilters);
    }

    QHeaderView::setModel(m);

    pagedModel = qobject_cast<IPagedItemModel *>(model());
    if (pagedModel)
    {
        connect(pagedModel, &IPagedItemModel::filterChanged, this,
                &FilterHeaderView::generateFilters);
    }

    generateFilters();
}
