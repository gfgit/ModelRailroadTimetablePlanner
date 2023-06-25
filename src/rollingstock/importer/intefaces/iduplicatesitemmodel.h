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

#ifndef IDUPLICATESITEMMODEL_H
#define IDUPLICATESITEMMODEL_H

#include <QAbstractTableModel>

#include "utils/model_mode.h"

namespace sqlite3pp {
class database;
}

class ICheckName;
class IQuittableTask;

class IDuplicatesItemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum State
    {
        Loaded = 0,
        Starting,
        CountingItems,
        LoadingData,
        FixingSameValues
    };

    IDuplicatesItemModel(sqlite3pp::database &db, QObject *parent = nullptr);
    ~IDuplicatesItemModel();

    bool event(QEvent *e) override;

    static IDuplicatesItemModel *createModel(ModelModes::Mode mode, sqlite3pp::database &db,
                                             ICheckName *iface, QObject *parent = nullptr);

    inline State getState() const
    {
        return mState;
    }
    inline int getItemCount() const
    {
        if (mState == Loaded || mState == LoadingData)
            return cachedCount;
        return -1;
    }

    bool startLoading(int mode);
    void cancelLoading();

signals:
    void error(const QString &text);
    void stateChanged(int state);
    void progressChanged(int progress, int max);
    void progressFinished();
    void processAborted();

protected:
    virtual IQuittableTask *createTask(int mode)    = 0;
    virtual void handleResult(IQuittableTask *task) = 0;

protected:
    sqlite3pp::database &mDb;

private:
    IQuittableTask *mTask;
    State mState;
    int cachedCount;
};

#endif // IDUPLICATESITEMMODEL_H
