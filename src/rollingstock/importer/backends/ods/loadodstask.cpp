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

#include "loadodstask.h"
#include "odsimporter.h"
#include "../loadprogressevent.h"
#include "options.h"

#include <QTemporaryFile>
#include <QXmlStreamReader>

#include <zip.h>

#include <QDebug>

LoadODSTask::LoadODSTask(const QMap<QString, QVariant> &arguments, sqlite3pp::database &db,
                         int mode, int defSpeed, RsType defType, const QString &fileName,
                         QObject *receiver) :
    ILoadRSTask(db, fileName, receiver),
    importMode(mode),
    defaultSpeed(defSpeed),
    defaultType(defType)
{
    m_tblFirstRow     = arguments.value(odsFirstRowKey, 3).toInt();
    m_tblRSNumberCol  = arguments.value(odsNumColKey, 1).toInt();
    m_tblModelNameCol = arguments.value(odsNameColKey, 3).toInt();
}

void LoadODSTask::run()
{
    if (wasStopped())
    {
        sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressAbortedByUser,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }

    int max      = 4;
    int progress = 0;
    sendEvent(new LoadProgressEvent(this, progress++, max), false);

    int err       = 0;
    zip_t *zipper = zip_open(mFileName.toUtf8(), ZIP_RDONLY, &err);

    if (!zipper)
    {
        zip_error_t ziperror;
        zip_error_init_with_code(&ziperror, err);
        const char *msg = zip_error_strerror(&ziperror);
        qDebug() << "Failed to open output file" << mFileName << "Err:" << msg;

        errText = QString::fromUtf8(msg);
        sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressError,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }

    // Search for the file of given name
    const char *name = "content.xml";
    struct zip_stat st;
    zip_stat_init(&st);
    if (zip_stat(zipper, name, 0, &st) < 0)
    {
        const char *msg = zip_strerror(zipper);
        errText         = QString::fromUtf8(msg);

        // Close archive
        zip_close(zipper);

        sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressError,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }

    QTemporaryFile mContentFile;

    if (mContentFile.isOpen())
        mContentFile.close();

    if (!mContentFile.open() || !mContentFile.resize(qint64(st.size)))
    {
        errText = mContentFile.errorString();
        // Close archive
        zip_close(zipper);

        sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressError,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }

    // Read the compressed file
    zip_file *f = zip_fopen(zipper, name, 0);
    if (!f)
    {
        const char *msg = zip_strerror(zipper);
        errText         = QString::fromUtf8(msg);
        // Close archive
        zip_close(zipper);

        sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressError,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }

    zip_uint64_t sum = 0;
    char buf[4096];
    while (sum != st.size)
    {
        zip_int64_t len = zip_fread(f, buf, 4096);
        if (len < 0)
        {
            const char *msg = zip_file_strerror(f);
            errText         = QString::fromUtf8(msg);
            // Close file and archive
            zip_fclose(f);
            zip_close(zipper);

            sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressError,
                                            LoadProgressEvent::ProgressMaxFinished),
                      true);
            return;
        }
        mContentFile.write(buf, len);
        sum += zip_uint64_t(len);
    }

    // Close file and archive
    zip_fclose(f);
    zip_close(zipper);

    if (wasStopped())
    {
        sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressAbortedByUser,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }

    sendEvent(new LoadProgressEvent(this, progress++, max), false);

    mContentFile.reset(); // Seek to start

    QXmlStreamReader xml(&mContentFile);

    ODSImporter importer(importMode, m_tblFirstRow, m_tblRSNumberCol, m_tblModelNameCol,
                         defaultSpeed, defaultType, mDb);
    if (!importer.loadDocument(xml))
    {
        errText = xml.errorString();
        sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressError,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }

    do
    {
        if (wasStopped())
        {
            sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressAbortedByUser,
                                            LoadProgressEvent::ProgressMaxFinished),
                      true);
            return;
        }
        sendEvent(new LoadProgressEvent(this, progress++, max++), false);
    } while (importer.readNextTable(xml));

    if (xml.hasError())
    {
        progress = LoadProgressEvent::ProgressError;
        errText  = xml.errorString();
    }

    sendEvent(new LoadProgressEvent(this, progress, LoadProgressEvent::ProgressMaxFinished), true);
}
