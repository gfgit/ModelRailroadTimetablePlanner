#ifndef JOBCATEGORYSTRINGS_H
#define JOBCATEGORYSTRINGS_H

#include <QCoreApplication>
#include "types.h"

static const char* JobCategoryFullNameTable[int(JobCategory::NCategories)] = { //NOTE: keep in sync wiht JobCategory
    QT_TRANSLATE_NOOP("JobCategoryName", "FREIGHT"),
    QT_TRANSLATE_NOOP("JobCategoryName", "LIS"),
    QT_TRANSLATE_NOOP("JobCategoryName", "POSTAL"),

    QT_TRANSLATE_NOOP("JobCategoryName", "REGIONAL"),
    QT_TRANSLATE_NOOP("JobCategoryName", "FAST REGIONAL"),
    QT_TRANSLATE_NOOP("JobCategoryName", "LOCAL"),
    QT_TRANSLATE_NOOP("JobCategoryName", "INTERCITY"),
    QT_TRANSLATE_NOOP("JobCategoryName", "EXPRESS"),
    QT_TRANSLATE_NOOP("JobCategoryName", "DIRECT"),
    QT_TRANSLATE_NOOP("JobCategoryName", "HIGH SPEED")
};

static const char* JobCategoryAbbrNameTable[int(JobCategory::NCategories)] = { //NOTE: keep in sync wiht JobCategory
    QT_TRANSLATE_NOOP("JobCategoryName", "FRG"),
    QT_TRANSLATE_NOOP("JobCategoryName", "LIS"),
    QT_TRANSLATE_NOOP("JobCategoryName", "P"),

    QT_TRANSLATE_NOOP("JobCategoryName", "R"),
    QT_TRANSLATE_NOOP("JobCategoryName", "RF"),
    QT_TRANSLATE_NOOP("JobCategoryName", "LOC"),
    QT_TRANSLATE_NOOP("JobCategoryName", "IC"),
    QT_TRANSLATE_NOOP("JobCategoryName", "EXP"),
    QT_TRANSLATE_NOOP("JobCategoryName", "DIR"),
    QT_TRANSLATE_NOOP("JobCategoryName", "HSP")
};

namespace JobCategoryName__ {
constexpr char unknownCatName[] = "Unknown";
}


class JobCategoryName
{
    Q_DECLARE_TR_FUNCTIONS(JobCategoryName)

public:
    static inline QString fullName(JobCategory cat)
    {
        if(cat >= JobCategory::NCategories)
            return JobCategoryName__::unknownCatName;
        return tr(JobCategoryFullNameTable[int(cat)]);
    }

    static inline QString shortName(JobCategory cat)
    {
        if(cat >= JobCategory::NCategories)
            return JobCategoryName__::unknownCatName;
        return tr(JobCategoryAbbrNameTable[int(cat)]);
    }

    static inline QString jobName(db_id jobId, JobCategory cat)
    {
        return shortName(cat) + QString::number(jobId); //Example: LIS1234
    }
};

#endif // JOBCATEGORYSTRINGS_H
