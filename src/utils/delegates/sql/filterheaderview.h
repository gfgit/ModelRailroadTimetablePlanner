#ifndef FILTERHEADERVIEW_H
#define FILTERHEADERVIEW_H

#include <QHeaderView>
#include <QVector>

class QTableView;
class FilterHeaderLineEdit;

class FilterHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit FilterHeaderView(QWidget* parent = nullptr);

    QSize sizeHint() const override;
    void setModel(QAbstractItemModel *m) override;

    bool hasFilters() const { return (filterWidgets.size() > 0); }
    QString filterValue(int column) const;

    void installOnTable(QTableView *view);

public slots:
    void generateFilters();
    void adjustPositions();
    void clearFilters();
    void setFilter(int column, const QString& value);
    void updateSoring(int col);

signals:
    void filterChanged(int column, QString value);

protected:
    void updateGeometries() override;

private slots:
    void inputChanged(FilterHeaderLineEdit *w, const QString& new_value);
    void showColumnTooltip(FilterHeaderLineEdit *w, const QPoint& globalPos);

private:
    QVector<FilterHeaderLineEdit *> filterWidgets;
};

#endif // FILTERHEADERVIEW_H
