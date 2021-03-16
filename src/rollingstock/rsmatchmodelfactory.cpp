#include "rsmatchmodelfactory.h"

#include "rsmodelsmatchmodel.h"
#include "rsownersmatchmodel.h"
#include "rollingstockmatchmodel.h"

RSMatchModelFactory::RSMatchModelFactory(ModelModes::Mode mode, sqlite3pp::database &db, QObject *parent) :
    IMatchModelFactory(parent),
    mDb(db),
    m_mode(mode)
{

}

ISqlFKMatchModel *RSMatchModelFactory::createModel()
{
    switch (m_mode)
    {
    case ModelModes::Models:
        return new RSModelsMatchModel(mDb);
    case ModelModes::Owners:
        return new RSOwnersMatchModel(mDb);
    case ModelModes::Rollingstock:
        return new RollingstockMatchModel(mDb);
    }
    return nullptr;
}
