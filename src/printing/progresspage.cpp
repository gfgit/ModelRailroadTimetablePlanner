#include "progresspage.h"
#include "printwizard.h"

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

#include "printworker.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include "graph/graphmanager.h"

#include "lines/linestorage.h"

//BIG TODO: deselect jobs before printing because the dashed rectangle of the selection gets printed too!!!

ProgressPage::ProgressPage(PrintWizard *w, QWidget *parent) :
    QWizardPage(parent),
    mWizard(w),
    complete(false)
{
    m_label = new QLabel(tr("Printing..."));
    m_progressBar = new QProgressBar;
    m_progressBar->setMinimum(0);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(m_label);
    l->addWidget(m_progressBar);
    setLayout(l);

    setCommitPage(true);
    setFinalPage(true);

    setTitle(tr("Printing"));
}

void ProgressPage::initializePage()
{
    complete = false;
    m_progressBar->reset();

    m_worker = new PrintWorker;
    m_worker->setOutputType(mWizard->type);
    m_worker->setPrinter(mWizard->printer);
    m_worker->setFileOutput(mWizard->fileOutput, mWizard->differentFiles);

    auto grapgMgr = Session->getViewManager()->getGraphMgr();
    m_worker->setGraphMgr(grapgMgr);

    PrintWorker::Scenes scenes;

    //FIXME: allow selection of witch line to print
    query q(Session->m_Db, "SELECT id,name FROM lines");
    for(auto line : q)
    {
        db_id lineId = line.get<db_id>(0);
        QString name = line.get<QString>(1);
        QGraphicsScene *scene = Session->mLineStorage->sceneForLine(lineId);
        scenes.append({lineId, scene, name});
    }

// OLD CODE
//    LinesModel *model = mWizard->linesModel;

//    for(int row = 0; row < size; row++)
//    {
//        if(vec.at(row))
//        {
//            const LineObj *line = model->getLineObjForRow(row);
//            if(!line)
//                continue;

//            scenes.append({line->lineId, line->scene, line->name});
//        }
//    }

    m_worker->setScenes(scenes);

    m_progressBar->setMaximum(m_worker->getMaxProgress());

    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(&m_thread, &QThread::started, m_worker, &PrintWorker::doWork);
    connect(m_worker, &PrintWorker::finished, this, &ProgressPage::handleFinished);
    connect(m_worker, &PrintWorker::progress, this, &ProgressPage::handleProgress);
    connect(m_worker, &PrintWorker::description, this, &ProgressPage::handleDescription);

    m_thread.start();
}

bool ProgressPage::validatePage()
{
    if(m_thread.isRunning())
    {
        if(!m_thread.wait(2000))
            return false;
    }

    return complete;
}

bool ProgressPage::isComplete() const
{
    return complete;
}

void ProgressPage::handleFinished()
{
    m_thread.quit();
    m_thread.wait();

    m_label->setText(tr("Completed"));

    complete = true;
    emit completeChanged();
}

void ProgressPage::handleProgress(int val)
{
    m_progressBar->setValue(val);
}

void ProgressPage::handleDescription(const QString& text)
{
    m_label->setText(QStringLiteral("Printing '%1' ...").arg(text));
}
