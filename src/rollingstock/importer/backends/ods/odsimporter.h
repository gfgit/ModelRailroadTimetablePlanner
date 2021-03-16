#ifndef ODSIMPORTER_H
#define ODSIMPORTER_H

#include <sqlite3pp/sqlite3pp.h>

#include "utils/types.h"

class QXmlStreamReader;
typedef struct sqlite3_mutex sqlite3_mutex;

class ODSImporter
{
public:
    ODSImporter(const int mode, const int firstRow, const int numColm, const int nameCol,
                int defSpeed, RsType defType, sqlite3pp::database &db);

    bool loadDocument(QXmlStreamReader &xml);
    bool readNextTable(QXmlStreamReader &xml);

private:
    void readTable(QXmlStreamReader& xml);
    void readRow(QXmlStreamReader &xml, QByteArray &model, qint64 &number);
    QString readCell(QXmlStreamReader &xml);

private:
    sqlite3pp::database &mDb;
    sqlite3_mutex *mutex;
    sqlite3pp::command q_addOwner;
    sqlite3pp::command q_unsetImported;
    sqlite3pp::command q_addModel;
    sqlite3pp::command q_addRS;

    sqlite3pp::query q_findOwner;
    sqlite3pp::query q_findModel;
    sqlite3pp::query q_findImportedModel;

    const int tableFirstRow;     //Start from 1 (not 0)
    const int tableRSNumberCol;  //Start from 1 (not 0)
    const int tableModelNameCol; //Start from 1 (not 0)
    const int importMode;

    int sheetIdx;
    int row;
    int col;

    int defaultSpeed;
    RsType defaultType;
};

#endif // ODSIMPORTER_H
