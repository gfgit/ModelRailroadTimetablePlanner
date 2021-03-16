#include "irsimportmodel.h"

IRsImportModel::IRsImportModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(500, db, parent)
{

}
