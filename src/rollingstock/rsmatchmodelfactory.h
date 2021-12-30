#ifndef RSMATCHMODELFACTORY_H
#define RSMATCHMODELFACTORY_H

#include "utils/delegates/sql/imatchmodelfactory.h"

#include "utils/model_mode.h"

namespace sqlite3pp
{
class database;
}

class RSMatchModelFactory : public IMatchModelFactory
{
public:
    RSMatchModelFactory(ModelModes::Mode mode, sqlite3pp::database &db, QObject *parent);

    ISqlFKMatchModel *createModel() override;

private:
    sqlite3pp::database &mDb;
    ModelModes::Mode m_mode;
};

#endif // RSMATCHMODELFACTORY_H
