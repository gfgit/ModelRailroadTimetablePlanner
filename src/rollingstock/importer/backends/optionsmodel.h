#ifndef OPTIONSMODEL_H
#define OPTIONSMODEL_H

#include <QAbstractListModel>

#include <QVector>
#include "rsimportbackend.h"

class RSImportBackendsModel : public QAbstractListModel
{
public:
    RSImportBackendsModel(QObject *parent);
    ~RSImportBackendsModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const;

    void addBackend(RSImportBackend *backend);

    RSImportBackend *getBackend(int idx) const;

private:
    QVector<RSImportBackend *> backends;
};

#endif // OPTIONSMODEL_H
