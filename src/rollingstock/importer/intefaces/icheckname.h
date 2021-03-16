#ifndef ICHECKNAME_H
#define ICHECKNAME_H

#include <QString>
#include "utils/types.h"

class ICheckName
{
public:
    virtual ~ICheckName();

    //For models and Owners
    virtual bool checkCustomNameValid(db_id importedId, const QString& originalName, const QString& newCustomName, QString *errMsgOut);

    //For RS
    virtual bool checkNewNumberIsValid(db_id importedRsId, db_id importedModelId, db_id matchExistingModelId, RsType rsType, int number, int newNumber, QString *errMsgOut);
};

#endif // ICHECKNAME_H
