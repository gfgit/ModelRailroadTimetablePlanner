#ifndef PRINTWIZARD_H
#define PRINTWIZARD_H

#include <QWizard>
#include <QVector>

#include "printing/printdefs.h"

#include "printing/helper/model/printhelper.h"

class QPrinter;
class SceneSelectionModel;
class PrintWorker;
class PrintProgressPage;

class IGraphScene;

namespace sqlite3pp {
class database;
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

    PrintHelper::PageLayoutOpt getScenePageLay() const;
    void setScenePageLay(const PrintHelper::PageLayoutOpt &newScenePageLay);

    Print::PrintBasicOptions getPrintOpt() const;
    void setPrintOpt(const Print::PrintBasicOptions &newPrintOpt);

    //Get first selected scene, ownership is passed to the caller
    IGraphScene *getFirstScene();

    inline sqlite3pp::database& getDb() const { return mDb; }

    inline SceneSelectionModel* getSelectionModel() const { return selectionModel; }

    inline bool taskRunning() const { return printTask; }

protected:
    bool event(QEvent *e) override;
    void done(int result) override;

private:
    void startPrintTask();
    void abortPrintTask();
    void handleProgressError(const QString& errMsg);

private:
    void validatePrintOptions();

private:
    sqlite3pp::database &mDb;
    SceneSelectionModel *selectionModel;

    QPrinter *printer;

    Print::PrintBasicOptions printOpt;
    PrintHelper::PageLayoutOpt scenePageLay;


    PrintProgressPage *progressPage;
    PrintWorker *printTask;
    bool isStoppingTask;
};

#endif // PRINTWIZARD_H
