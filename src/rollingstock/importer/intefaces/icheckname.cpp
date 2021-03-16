#include "icheckname.h"

ICheckName::~ICheckName()
{

}

bool ICheckName::checkCustomNameValid(db_id, const QString &, const QString &, QString *)
{
    return false;
}

bool ICheckName::checkNewNumberIsValid(db_id, db_id, db_id, RsType , int, int, QString *)
{
    return false;
}
