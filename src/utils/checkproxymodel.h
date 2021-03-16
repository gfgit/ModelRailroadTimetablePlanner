#ifndef CHECKPROXYMODEL_H
#define CHECKPROXYMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QSet>

#include <QIdentityProxyModel>

class CheckProxyModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit CheckProxyModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setSourceModel(QAbstractItemModel *value);
    QAbstractItemModel *getSourceModel() const;

    bool hasAtLeastACheck() const;

    QVector<bool> checks() const;

    void setChecks(QSet<int> rowSet);

signals:
    void hasCheck(bool check);

public slots:
    void selectAll();
    void selectNone();

private slots:
    void onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void onSourceRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void onModelReset();
    //TODO: what about layoutChanged() ???

private:
    QAbstractItemModel *sourceModel;
    QVector<bool> m_checks;
    bool atLeastACheck;
};


//BIG TODO: new model crashes after invokeMethod 'resetInternalData()'

//TODO: maybe handle tree models???
//class CheckProxyModel : public QIdentityProxyModel
//{
//    Q_OBJECT
//public:
//    explicit CheckProxyModel(QObject *parent = nullptr);

//    QVariant data(const QModelIndex &index, int role) const override;
//    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

//    Qt::ItemFlags flags(const QModelIndex &index) const override;

//    bool hasAtLeastACheck() const;

//    void setChecks(QSet<int> rowSet);

//    QVector<bool> checks() const;

//signals:
//    void hasCheck(bool check);

//public slots:
//    void selectAll();
//    void selectNone();

//private slots:
//    void onSourceRowsIserted(QModelIndex, int first, int last);
//    void onSourceRowsMoved(QModelIndex, int start, int end, QModelIndex, int row);
//    void onSourceRowsRemoved(QModelIndex, int first, int last);

//private:
//    QVector<bool> m_checks;
//    bool atLeastACheck;
//};

#endif // CHECKPROXYMODEL_H
