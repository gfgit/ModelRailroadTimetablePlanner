/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "odsimporter.h"

#include "utils/types.h"

#include "../importmodes.h"

#include <QXmlStreamReader>

#include <QDebug>

const QString offns   = QStringLiteral("xmlns:office");
const QString offvers = QStringLiteral("office:version");
const QString tbl     = QStringLiteral("table:table");
const QString tblname = QStringLiteral("table:name");
const QString offbody = QStringLiteral("office:body");

ODSImporter::ODSImporter(const int mode, const int firstRow, const int numColm, const int nameCol,
                         int defSpeed, RsType defType, sqlite3pp::database &db) :
    mDb(db),
    q_addOwner(
      mDb,
      "INSERT INTO imported_rs_owners(id, name, import, new_name, match_existing_id, sheet_idx)"
      " VALUES(NULL, ?, 1, NULL, ?, ?)"),
    q_unsetImported(mDb, "UPDATE imported_rs_owners SET import=0 WHERE id=?"),
    q_addModel(mDb, "INSERT INTO imported_rs_models(id, name, suffix, import, new_name, "
                    "match_existing_id, max_speed, axes, type, sub_type)"
                    " VALUES(NULL, ?, '', 1, NULL, ?, ?, ?, ?, ?)"),
    q_addRS(mDb, "INSERT INTO imported_rs_list(id, import, model_id, owner_id, number, new_number)"
                 " VALUES(NULL, 1, ?, ?, ?, NULL)"),

    q_findOwner(mDb, "SELECT id FROM rs_owners WHERE name=?"),
    q_findModel(mDb, "SELECT id FROM rs_models WHERE name=?"),
    q_findImportedModel(mDb, "SELECT id FROM imported_rs_models WHERE name=?"),

    tableFirstRow(firstRow),
    tableRSNumberCol(numColm),
    tableModelNameCol(nameCol),
    importMode(mode),

    sheetIdx(0),
    row(0),
    col(0),

    defaultSpeed(defSpeed),
    defaultType(defType)
{
    mutex = sqlite3_db_mutex(mDb.db());
}

bool ODSImporter::loadDocument(QXmlStreamReader &xml)
{
    xml.setNamespaceProcessing(false);
    sheetIdx = 0;

    if (!xml.readNextStartElement()
        || xml.qualifiedName() != QLatin1String("office:document-content"))
        return false;

    bool offNsFound            = false;
    bool offVersFound          = false;

    QXmlStreamAttributes attrs = xml.attributes();

    for (int i = 0; i < attrs.size(); i++)
    {
        const QXmlStreamAttribute &a = attrs[i];
        if (!offVersFound && a.qualifiedName() == offvers)
        {
            if (a.value().size() < 3 || a.value().at(1) != '.')
            {
                qWarning() << "WRONG OFFICE VERSION:" << a.value();
            }
            int major = a.value().at(0).digitValue();
            int minor = a.value().at(2).digitValue();
            qDebug() << "FOUND VERSION:" << a.value();
            if (major != 1 || minor < 2)
            {
                qDebug() << "Error: wrong office:version value";
            }
            offVersFound = true;
            if (offNsFound)
                break;
        }
        else if (!offNsFound && a.qualifiedName() == offns)
        {
            offNsFound = true;
            if (offVersFound)
                break;
        }
    }

    if (!offVersFound || !offNsFound)
        return false;

    while (xml.readNextStartElement() && xml.qualifiedName() != offbody)
    {
        xml.skipCurrentElement();
    }

    if (xml.hasError())
    {
        qDebug() << "XML Error:" << xml.error() << xml.errorString();
        return false;
    }

    if (!xml.readNextStartElement() || xml.qualifiedName() != QLatin1String("office:spreadsheet"))
        return false;

    if (xml.hasError())
    {
        return false;
    }

    return true;
}

bool ODSImporter::readNextTable(QXmlStreamReader &xml)
{
    while (xml.readNextStartElement())
    {
        if (xml.qualifiedName() != tbl)
        {
            xml.skipCurrentElement();
            continue; // Skip unknown elements
        }

        readTable(xml);
        sheetIdx++;
        return true;
    }

    return false;
}

void ODSImporter::readTable(QXmlStreamReader &xml)
{
    db_id importedOwnerId = 0;
    if (importMode & RSImportMode::ImportRSOwners)
    {
        QString name;
        for (const QXmlStreamAttribute &a : xml.attributes())
        {
            if (a.qualifiedName() == tblname)
            {
                name = a.value().toString().simplified();
                break;
            }
        }

        // qDebug() << "OWNER:" << name;

        // Try to match an existing owner, if returns 0 -> no match -> create new owner
        db_id existingOwnerId = 0;
        q_findOwner.bind(1, name);
        if (q_findOwner.step() == SQLITE_ROW)
            existingOwnerId = q_findOwner.getRows().get<db_id>(0);
        q_findOwner.reset();

        if (name.isEmpty())
            q_addOwner.bind(1); // Bind NULL, will be handled by SelectOwnersPage
        else
            q_addOwner.bind(1, name);

        if (existingOwnerId)
            q_addOwner.bind(2, existingOwnerId);
        else
            q_addOwner.bind(2); // bind NULL

        q_addOwner.bind(3, sheetIdx);

        sqlite3_mutex_enter(mutex);
        int ret         = q_addOwner.execute();
        importedOwnerId = mDb.last_insert_rowid();
        sqlite3_mutex_leave(mutex);
        q_addOwner.reset();

        if (ret != SQLITE_OK)
        {
            // FIXME: tell the user
            qWarning() << "Error importing owner:" << name << "skipping...";
            xml.skipCurrentElement();
            return;
        }
    }

    row           = 0;
    col           = 0;

    int rsCount   = 0;
    bool finished = false;

    QByteArray model;
    qint64 number = -1;

    while (!finished && xml.readNext() != QXmlStreamReader::Invalid)
    {
        switch (xml.tokenType())
        {
        case QXmlStreamReader::StartElement:
        {
            if (importMode & RSImportMode::ImportRSModels
                && xml.qualifiedName() == QLatin1String("table:table-row"))
            {
                readRow(xml, model, number);

                if (row < tableFirstRow || model.isEmpty() || number == -1)
                    break; // First n rows are table header / empty

                // qDebug() << "RS:" << model << number;

                db_id importedModelId = 0;

                sqlite3_bind_text(q_findImportedModel.stmt(), 1, model, model.size(),
                                  SQLITE_STATIC);
                if (q_findImportedModel.step() == SQLITE_ROW)
                {
                    importedModelId = q_findImportedModel.getRows().get<db_id>(0);
                }
                q_findImportedModel.reset();

                if (!importedModelId)
                {
                    // Create new one
                    db_id existingModelId   = 0;
                    int maxSpeedKm          = defaultSpeed;
                    int axes                = 4;
                    RsType type             = defaultType;
                    RsEngineSubType subType = RsEngineSubType::Invalid;

                    // Try filling with matched name model infos
                    sqlite3_bind_text(q_findModel.stmt(), 1, model, model.size(), SQLITE_STATIC);
                    if (q_findModel.step() == SQLITE_ROW)
                    {
                        auto m          = q_findModel.getRows();
                        existingModelId = m.get<db_id>(0);
                        maxSpeedKm      = m.get<int>(1);
                        axes            = m.get<int>(2);
                        type            = RsType(m.get<int>(3));
                        subType         = RsEngineSubType(m.get<int>(4));
                    }
                    q_findModel.reset();

                    sqlite3_bind_text(q_addModel.stmt(), 1, model, model.size(), SQLITE_STATIC);
                    if (existingModelId)
                        q_addModel.bind(2, existingModelId);
                    else
                        q_addModel.bind(2); // bind NULL
                    q_addModel.bind(3, maxSpeedKm);
                    q_addModel.bind(4, axes);
                    q_addModel.bind(5, int(type));
                    q_addModel.bind(6, int(subType));

                    sqlite3_mutex_enter(mutex);
                    q_addModel.execute();
                    importedModelId = mDb.last_insert_rowid();
                    sqlite3_mutex_leave(mutex);

                    q_addModel.reset();
                }

                if (importMode & RSImportMode::ImportRSPieces)
                {
                    int num = number % 10000; // Cut at 4 digits

                    // Finally register RS
                    q_addRS.bind(1, importedModelId);
                    q_addRS.bind(2, importedOwnerId);
                    q_addRS.bind(3, num);
                    q_addRS.execute();
                    q_addRS.reset();

                    rsCount++;
                }
            }
            else
            {
                xml.skipCurrentElement();
            }
            break;
        }
        case QXmlStreamReader::EndElement:
        {
            finished = true;
            break;
        }
        default:
            break;
        }
    }

    if (rsCount == 0 && importMode & RSImportMode::ImportRSPieces)
    {
        q_unsetImported.bind(1, importedOwnerId);
        q_unsetImported.execute();
        q_unsetImported.reset();
    }
}

void ODSImporter::readRow(QXmlStreamReader &xml, QByteArray &model, qint64 &number)
{
    row++;
    col = 0;

    for (const QXmlStreamAttribute &a : xml.attributes())
    {
        if (a.qualifiedName() == QLatin1String("table:number-rows-repeated"))
        {
            int rowsRepeated = a.value().toInt();
            row += rowsRepeated - 1;
            break;
        }
    }

    while (xml.readNext() != QXmlStreamReader::Invalid)
    {
        switch (xml.tokenType())
        {
        case QXmlStreamReader::StartElement:
        {
            if (xml.qualifiedName() == QLatin1String("table:table-cell"))
            {
                int oldCol = col;

                col++;
                for (const QXmlStreamAttribute &a : xml.attributes())
                {
                    if (a.qualifiedName() == QLatin1String("table:number-columns-repeated"))
                    {
                        int colsRepeated = a.value().toInt();
                        col += colsRepeated - 1;
                        break;
                    }
                }

                // Read current cell
                int depth                         = 1;
                bool cellEmpty                    = true;
                QXmlStreamReader::TokenType token = QXmlStreamReader::NoToken;
                while (depth && (token = xml.readNext()) != QXmlStreamReader::Invalid)
                {
                    switch (token)
                    {
                    case QXmlStreamReader::StartElement:
                        depth++;
                        break;
                    case QXmlStreamReader::EndElement:
                        depth--;
                        break;
                    case QXmlStreamReader::Characters:
                    {
                        if (xml.isWhitespace())
                            break;

                        // Convert to QString immidiately because xml reader changes buffer contents
                        // when reading next token
                        QStringView val = xml.text();
                        cellEmpty      = val.isEmpty();
                        // qDebug() << "CELL:" << row << col << val;

                        // Avoid allocating a QString copy of QStringView, directly convert to
                        // QByteArray
                        if (oldCol < tableRSNumberCol && col >= tableRSNumberCol && val.size())
                        {
                            // Do not use toInt(), we must tolerate dashes and other non-digit
                            // characters in the middle
                            qint64 tmp = 0;
                            for (int i = 0; i < val.size(); i++)
                            {
                                int d = val.at(i).digitValue();
                                if (d != -1) //-1 means it's not a digit so skip it
                                {
                                    tmp *= 10;
                                    tmp += d;
                                }
                            }
                            number = tmp;
                        }

                        if (oldCol < tableModelNameCol && col >= tableModelNameCol)
                            model = val.toUtf8().simplified();

                        break;
                    }
                    default:
                        break;
                    }
                }

                if (cellEmpty)
                {
                    if (oldCol < tableRSNumberCol && col >= tableRSNumberCol)
                        number = -1;
                    if (oldCol < tableModelNameCol && col >= tableModelNameCol)
                        model.clear();
                }
            }
            else
            {
                xml.skipCurrentElement();
            }
            break;
        }
        case QXmlStreamReader::EndElement:
            return;
        default:
            break;
        }
    }
}

QString ODSImporter::readCell(QXmlStreamReader &xml)
{
    col++;
    for (const QXmlStreamAttribute &a : xml.attributes())
    {
        if (a.qualifiedName() == QLatin1String("table:number-columns-repeated"))
        {
            int colsRepeated = a.value().toInt();
            col += colsRepeated - 1;
            break;
        }
    }

    QString val;

    // Read current cell
    int depth                         = 1;
    QXmlStreamReader::TokenType token = QXmlStreamReader::NoToken;
    while (depth && (token = xml.readNext()) != QXmlStreamReader::Invalid)
    {
        switch (token)
        {
        case QXmlStreamReader::StartElement:
            depth++;
            break;
        case QXmlStreamReader::EndElement:
            depth--;
            break;
        case QXmlStreamReader::Characters:
        {
            if (xml.isWhitespace())
                break;

            val = xml.text().toString(); // Convert to QString immidiately because xml reader
                                         // changes buffer contents when reading next token
            // qDebug() << "CELL:" << row << col << val;
            break;
        }
        default:
            break;
        }
    }

    return val;
}
