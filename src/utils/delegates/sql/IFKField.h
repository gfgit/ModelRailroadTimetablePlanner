#ifndef IFKFIELD_H
#define IFKFIELD_H

#include <QString>
#include "utils/types.h"

/* IOwnerField
 * Generic interface for RSOwnersSQLDelegate so it can be used by multiple models
 * Used for set a Foreign Key field: user types the name and the model gets the corresponding id
*/

class IFKField
{
public:
    virtual ~IFKField();

    virtual bool getFieldData(int row, int col, db_id &idOut, QString& nameOut) const = 0;
    virtual bool validateData(int row, int col, db_id id, const QString& name) = 0;
    virtual bool setFieldData(int row, int col, db_id id, const QString& name) = 0;
};

#endif // IFKFIELD_H
