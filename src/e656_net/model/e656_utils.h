#ifndef E656_UTILS_H
#define E656_UTILS_H

#include <QString>
#include <QTime>

#include "utils/types.h"

struct ImportedJobItem
{
    db_id number = 0;
    QString catName;
    QString urlPath;
    QString originStation;
    QString destinationStation;
    QString platformName;

    QTime arrival;
    QTime departure;

    QTime originTime;
    QTime destinationTime;

    JobCategory matchingCategory = JobCategory::NCategories;
};

#endif // E656_UTILS_H
