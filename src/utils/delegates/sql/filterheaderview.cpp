#include "filterheaderview.h"
#include "filterheaderlineedit.h"

#include "pageditemmodel.h"

#include <QApplication>
#include <QTableView>
#include <QScrollBar>
#include <QLineEdit>

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

    // Do some connects: Basically just resize and reposition the input widgets whenever anything changes
    connect(this, &FilterHeaderView::sectionResized, this, &FilterHeaderView::adjustPositions);

    connect(this, &FilterHeaderView::sectionClicked, this, &FilterHeaderView::updateSoring);

    // Set custom context menu handling
    setContextMenuPolicy(Qt::CustomContextMenu);
}

void FilterHeaderView::generateFilters()
{
    // Delete all the current filter widgets
    qDeleteAll(filterWidgets);
    filterWidgets.clear();

    IPagedItemModel *m = qobject_cast<IPagedItemModel *>(model());
    if(m)
    {
        // And generate a bunch of new ones
        const int columnCount = m->columnCount();
        for(int i = 0; i < columnCount; i++)
        {
            auto filter = m->getFilterAtCol(i);
            if(!filter.second.testFlag(IPagedItemModel::FilterFlag::BasicFiltering))
                continue; //No filtering for this column

            FilterHeaderLineEdit* l = new FilterHeaderLineEdit(i, this);
            l->setText(filter.first);
            connect(l, &FilterHeaderLineEdit::delayedTextChanged, this, &FilterHeaderView::inputChanged);
            filterWidgets.push_back(l);
        }

        //Update sorting
        setSortIndicator(m->getSortingColumn(), Qt::AscendingOrder);
    }

    // Position them correctly
    updateGeometries();
}

QSize FilterHeaderView::sizeHint() const
{
    // For the size hint just take the value of the standard implementation and add the height of a input widget to it if necessary
    QSize s = QHeaderView::sizeHint();
    if(filterWidgets.size())
        s.setHeight(s.height() + filterWidgets.at(0)->sizeHint().height() + 4); // The 4 adds just adds some extra space
    return s;
}

void FilterHeaderView::updateGeometries()
{
    // If there are any input widgets add a viewport margin to the header to generate some empty space for them which is not affected by scrolling
    if(filterWidgets.size())
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

    //Sometimes it's 0 when recursion happen, skip moving widgets to avoid flickering
    if(y == 0)
        return;

    // The two adds some extra space between the header label and the input widget
    y += 2;

    // Loop through all widgets
    for(int i = 0; i < filterWidgets.size(); ++i)
    {
        // Get the current widget, move it and resize it
        FilterHeaderLineEdit* w = filterWidgets.at(i);
        const int col = w->getColumn();
        if (QApplication::layoutDirection() == Qt::RightToLeft)
            w->move(width() - (sectionPosition(col) + sectionSize(col) - offset()), y);
        else
            w->move(sectionPosition(col) - offset(), y);
        w->resize(sectionSize(col), w->sizeHint().height());
    }
}

void FilterHeaderView::inputChanged(FilterHeaderLineEdit *w, const QString& new_value)
{
    // Just get the column number and the new value and send them to anybody interested in filter changes
    emit filterChanged(w->getColumn(), new_value);

    IPagedItemModel *m = qobject_cast<IPagedItemModel *>(model());
    if(!m)
        return;

    m->setFilterAtCol(w->getColumn(), new_value);
    m->refreshData(true);
}

void FilterHeaderView::clearFilters()
{
    for(QLineEdit* filterLineEdit : qAsConst(filterWidgets))
        filterLineEdit->clear();
}

void FilterHeaderView::setFilter(int column, const QString& value)
{
    for(FilterHeaderLineEdit* filterLineEdit : qAsConst(filterWidgets))
    {
        if(filterLineEdit->getColumn() == column)
        {
            filterLineEdit->clear();
            break;
        }
    }
}

void FilterHeaderView::updateSoring(int col)
{
    IPagedItemModel *m = qobject_cast<IPagedItemModel *>(model());
    if(!m)
        return;

    m->setSortingColumn(col);
    setSortIndicator(m->getSortingColumn(), Qt::AscendingOrder);
}

QString FilterHeaderView::filterValue(int column) const
{
    for(FilterHeaderLineEdit* filterLineEdit : qAsConst(filterWidgets))
    {
        if(filterLineEdit->getColumn() == column)
        {
            return filterLineEdit->text();
        }
    }

    return QString();
}

void FilterHeaderView::installOnTable(QTableView *view)
{
    view->setHorizontalHeader(this);

    connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, this, &FilterHeaderView::adjustPositions);
    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, this, &FilterHeaderView::adjustPositions);

    // Disconnect clicking in header to select column, since we will use it for sorting.
    // Note that, in order to work, this cannot be converted to the standard C++11 format.
    disconnect(this, SIGNAL(sectionPressed(int)), view, SLOT(selectColumn(int)));
    disconnect(this, SIGNAL(sectionEntered(int)), view, SLOT(_q_selectColumn(int)));

    //Must be enabled again because QTableView disables it if sorting not enabled in QTableView
    setSortIndicatorShown(true);
}

void FilterHeaderView::setModel(QAbstractItemModel *m)
{
    QHeaderView::setModel(m);
    generateFilters();
}
