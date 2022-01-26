#ifndef PRINTWIZARD_H
#define PRINTWIZARD_H

#include <QWizard>
#include <QVector>

#include "printing/printdefs.h"

class QPrinter;
class SceneSelectionModel;

class PrintWorkerHandler;
class PrintProgressPage;

class IGraphScene;

namespace sqlite3pp {
class database;
}

namespace Print {
struct PageLayoutOpt;
}

class PrintWizard : public QWizard
{
    Q_OBJECT
public:
    PrintWizard(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~PrintWizard();

    bool validateCurrentPage() override;

    QPrinter *getPrinter() const;

    void setOutputType(Print::OutputType type);

    Print::PageLayoutOpt getScenePageLay() const;
    void setScenePageLay(const Print::PageLayoutOpt &newScenePageLay);

    Print::PrintBasicOptions getPrintOpt() const;
    void setPrintOpt(const Print::PrintBasicOptions &newPrintOpt);

    //Get first selected scene, ownership is passed to the caller
    IGraphScene *getFirstScene();

    inline sqlite3pp::database& getDb() const { return mDb; }

    inline SceneSelectionModel* getSelectionModel() const { return selectionModel; }

    bool taskRunning() const;

protected:
    void done(int result) override;

private slots:
    void progressMaxChanged(int max);
    void progressChanged(int val, const QString& msg);
    void handleProgressFinished(bool success, const QString& errMsg);

private:
    sqlite3pp::database &mDb;
    SceneSelectionModel *selectionModel;

    QPrinter *printer;

    PrintProgressPage *progressPage;
    PrintWorkerHandler *printTaskHandler;
};

#endif // PRINTWIZARD_H
