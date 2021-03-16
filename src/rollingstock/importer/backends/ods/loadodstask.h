#ifndef LOADODSTASK_H
#define LOADODSTASK_H

#include "../loadtaskutils.h"

#include "utils/types.h"

/* LoadODSTask
 *
 * Loads rollingstock pieces/models/owners from an ODS spreadsheet
 * Open Document Format V1.2 (*.ods)
 *
 * Table Characteristics
 * 1) tblFirstRow: first non-empty RS row (starting from 1, not 0) DEFAULT: 3
 * 2) tblRSNumberCol: column from which number is extracted  (starting from 1, not 0) DEFAULT: 1
 * 3) tblFirstRow: column from which model name is extracted (starting from 1, not 0) DEFAULT: 3
 */
class LoadODSTask : public ILoadRSTask
{
public:
    LoadODSTask(const QMap<QString, QVariant>& arguments, sqlite3pp::database &db,
                int mode, int defSpeed, RsType defType,
                const QString& fileName, QObject *receiver);

    void run() override;

private:
    int m_tblFirstRow;     //Start from 1 (not 0)
    int m_tblRSNumberCol;  //Start from 1 (not 0)
    int m_tblModelNameCol; //Start from 1 (not 0)
    int importMode;

    int defaultSpeed;
    RsType defaultType;
};

#endif // LOADODSTASK_H
