#ifndef MODEL_ROLES_H
#define MODEL_ROLES_H

#include <qnamespace.h>

//Useful constants to set/retrive data from models

#define STOP_ID             (Qt::UserRole)
#define STATION_ID          (Qt::UserRole + 1)

#define LINE_ID             (Qt::UserRole)

#define PLATF_ID            (Qt::UserRole + 4)

#define JOB_SHIFT_ID        (Qt::UserRole + 6)

#define JOB_ID_ROLE         (Qt::UserRole + 10)

#define JOB_CATEGORY_ROLE   (Qt::UserRole + 12)

#define STOP_TYPE_ROLE      (Qt::UserRole + 20)
#define ARR_ROLE            (Qt::UserRole + 21)
#define DEP_ROLE            (Qt::UserRole + 22)
#define CUR_LINE_ROLE       (Qt::UserRole + 24)
#define SEGMENT_ROLE        (Qt::UserRole + 26)
#define OTHER_SEG_ROLE      (Qt::UserRole + 27)
#define NEXT_LINE_ROLE      (Qt::UserRole + 28)

#define ADDHERE_ROLE        (Qt::UserRole + 30)

#define STOP_DESCR_ROLE     (Qt::UserRole + 31)


#define RS_MODEL_ID         (Qt::UserRole + 40)
#define RS_TYPE_ROLE        (Qt::UserRole + 41)
#define RS_SUB_TYPE_ROLE    (Qt::UserRole + 42)
#define RS_OWNER_ID         (Qt::UserRole + 43)
#define RS_NUMBER           (Qt::UserRole + 44)
#define RS_IS_ENGINE        (Qt::UserRole + 45)

#define COLOR_ROLE          (Qt::UserRole + 46)

#endif // MODEL_ROLES_H
