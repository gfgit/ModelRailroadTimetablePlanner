#ifndef OPTIONSMODEL_H
#define OPTIONSMODEL_H

#include <QAbstractListModel>
#include "../rsimportwizard.h"

class OptionsModel : public QAbstractListModel
{
public:
    OptionsModel(QObject *parent);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const;
};

#endif // OPTIONSMODEL_H
