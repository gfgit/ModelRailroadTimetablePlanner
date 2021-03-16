#ifndef MODEL_ROLES_H
#define MODEL_ROLES_H

#include <qnamespace.h>

//Useful constants to set/retrive data from models

#define STOP_ID             (Qt::UserRole)
#define STATION_ID          (Qt::UserRole + 1)
#define STATION_NAME        (Qt::DisplayRole)

#define ROW_ID              (Qt::UserRole + 2)

#define RS_ID               (Qt::UserRole + 3)

#define LINE_ID             (Qt::UserRole)
#define START_STATION_ID    (Qt::UserRole + 1)
#define END_STATION_ID      (Qt::UserRole + 2)

#define PLATF_ID            (Qt::UserRole + 4)

#define DIRECTION_ROLE        (Qt::UserRole + 5)

#define JOB_SHIFT_ID        (Qt::UserRole + 6)

#define DISTANCE_ROLE       (Qt::UserRole + 7)

#define INDEX_ROLE          (Qt::UserRole + 8)

#define KMPOS_ROLE          (Qt::UserRole + 9)

#define JOB_ID_ROLE         (Qt::UserRole + 10)

#define MAX_SPEED_ROLE      (Qt::UserRole + 11)

#define JOB_CATEGORY_ROLE   (Qt::UserRole + 12)

#define JOB_FIRST_ST_ROLE      (Qt::UserRole + 13)
#define JOB_FIRST_ST_NAME_ROLE (Qt::UserRole + 14)
#define JOB_LAST_ST_ROLE       (Qt::UserRole + 15)
#define JOB_LAST_ST_NAME_ROLE  (Qt::UserRole + 16)
#define JOB_SHIFT_NAME         (Qt::UserRole + 17)
#define JOB_CATEGORY_NAME_ROLE (Qt::UserRole + 18)

#define STOP_TYPE_ROLE      (Qt::UserRole + 20)
#define ARR_ROLE            (Qt::UserRole + 21)
#define DEP_ROLE            (Qt::UserRole + 22)
#define STATION_ROLE        (Qt::UserRole + 23)
#define CUR_LINE_ROLE       (Qt::UserRole + 24)
#define LINES_ROLE          (Qt::UserRole + 25)
#define SEGMENT_ROLE        (Qt::UserRole + 26)
#define OTHER_SEG_ROLE      (Qt::UserRole + 27)
#define NEXT_LINE_ROLE      (Qt::UserRole + 28)

#define POSSIBLE_LINE_ROLE  (Qt::UserRole + 29)
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
