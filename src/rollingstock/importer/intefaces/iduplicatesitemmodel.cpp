#include "iduplicatesitemmodel.h"

#include "../model/duplicatesimporteditemsmodel.h"
#include "../model/duplicatesimportedrsmodel.h"

#include "utils/thread/iquittabletask.h"
#include "iduplicatesitemevent.h"

#include <QThreadPool>

IDuplicatesItemModel::IDuplicatesItemModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    mDb(db),
    mTask(nullptr),
    mState(Loaded),
    cachedCount(-1)
{

}

IDuplicatesItemModel::~IDuplicatesItemModel()
{
    if(mTask)
    {
        mTask->stop();
        mTask->cleanup();
        mTask = nullptr;
    }
}

bool IDuplicatesItemModel::event(QEvent *e)
{
    if(e->type() == IDuplicatesItemEvent::_Type)
    {
        IDuplicatesItemEvent *ev = static_cast<IDuplicatesItemEvent *>(e);
        if(mTask == ev->task)
        {
            QString errText;
            if(ev->max == IDuplicatesItemEvent::ProgressMaxFinished)
            {
                if(ev->progress == IDuplicatesItemEvent::ProgressError)
                {
                    //FIXME: add error QString in base IQuittableTask

                    //errText = loadTask->getErrorText();
                    cachedCount = -1;
                }
                else if(ev->progress != IDuplicatesItemEvent::ProgressAbortedByUser)
                {
                    handleResult(mTask);
                    cachedCount = ev->ducplicatesCount;
                }

                mState = Loaded;
                emit stateChanged(mState);

                if(ev->progress == IDuplicatesItemEvent::ProgressAbortedByUser)
                {
                    emit processAborted();
                }
                else
                {
                    emit progressFinished();
                }

                if(ev->progress == IDuplicatesItemEvent::ProgressError)
                {
                    //Emit error after finishing
                    emit error(errText);
                }

                //Delete task before handling event because otherwise it is detected as still running
                delete mTask;
                mTask = nullptr;
            }
            else
            {
                if(mState != ev->state)
                {
                    mState = State(ev->state);
                    cachedCount = ev->ducplicatesCount;
                    emit stateChanged(mState);
                }
                emit progressChanged(ev->progress, ev->max);
            }
        }
    }
    return QAbstractItemModel::event(e);
}

IDuplicatesItemModel *IDuplicatesItemModel::createModel(ModelModes::Mode mode, sqlite3pp::database &db, ICheckName *iface, QObject *parent)
{
    switch (mode)
    {
    case ModelModes::Owners:
    case ModelModes::Models:
        return new DuplicatesImportedItemsModel(mode, db, iface, parent);
    case ModelModes::Rollingstock:
        return new DuplicatesImportedRSModel(db, iface, parent);
    }

    return nullptr;
}

bool IDuplicatesItemModel::startLoading(int mode)
{
    if(mState != Loaded)
        return true; //Already started

    mTask = createTask(mode);
    if(!mTask)
        return false;

    cachedCount = -1;
    mState = Starting;
    emit stateChanged(mState);

    QThreadPool::globalInstance()->start(mTask);
    return true;
}

void IDuplicatesItemModel::cancelLoading()
{
    if(mState == Loaded)
        return; //No active process

    //TODO: wait finished?
    mTask->stop();
    mTask->cleanup();
    mTask = nullptr;

    emit processAborted();

    mState = Loaded;
    emit stateChanged(mState);
}
