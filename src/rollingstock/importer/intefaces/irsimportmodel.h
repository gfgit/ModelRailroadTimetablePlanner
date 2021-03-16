#ifndef IRSIMPORTMODEL_H
#define IRSIMPORTMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"
#include "icheckname.h"

class IRsImportModel : public IPagedItemModel, public ICheckName
{
    Q_OBJECT
public:
    IRsImportModel(sqlite3pp::database &db, QObject *parent = nullptr);

    virtual int countImported() = 0;

signals:
    void importCountChanged();
};

#endif // IRSIMPORTMODEL_H
