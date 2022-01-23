#ifndef PAGESIZEMODEL_H
#define PAGESIZEMODEL_H

#include <QAbstractListModel>

/*!
 * \brief The PageSizeModel class
 *
 * A small helper model to list available QPageSize
 * Use it for QComboBox
 */
class PageSizeModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PageSizeModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
};

#endif // PAGESIZEMODEL_H
