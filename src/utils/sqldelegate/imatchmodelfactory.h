#ifndef IMATCHMODELFACTORY_H
#define IMATCHMODELFACTORY_H

#include <QObject>

class ISqlFKMatchModel;

class IMatchModelFactory : public QObject
{
    Q_OBJECT
public:
    explicit IMatchModelFactory(QObject *parent = nullptr);

    virtual ISqlFKMatchModel *createModel() = 0;
};

#endif // IMATCHMODELFACTORY_H
