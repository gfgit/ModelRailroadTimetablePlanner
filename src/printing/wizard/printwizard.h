#ifndef PRINTWIZARD_H
#define PRINTWIZARD_H

#include <QWizard>
#include <QVector>

#include "printing/printdefs.h"

class QPrinter;
class SceneSelectionModel;
class PrintWorker;
class PrintProgressPage;

namespace sqlite3pp {
class database;
}

class PrintWizard : public QWizard
{
    Q_OBJECT
public:
    PrintWizard(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~PrintWizard();

    Print::OutputType getOutputType() const;
    void setOutputType(Print::OutputType out);

    inline SceneSelectionModel* getSelectionModel() const { return selectionModel; }

    QString getOutputFile() const;
    void setOutputFile(const QString& fileName);

    bool getDifferentFiles() const;
    void setDifferentFiles(bool newDifferentFiles);

    const QString &getFilePattern() const;
    void setFilePattern(const QString &newFilePattern);

    QPrinter *getPrinter() const;

    inline sqlite3pp::database& getDb() const { return mDb; }

    inline bool taskRunning() const { return printTask; }

protected:
    bool event(QEvent *e) override;
    void done(int result) override;

public:
    void startPrintTask();
    void abortPrintTask();
    void handleProgressError(const QString& errMsg);

private:
    sqlite3pp::database &mDb;

    QPrinter *printer;
    QString fileOutput;
    QString filePattern;
    bool differentFiles;

    Print::OutputType type;

    SceneSelectionModel *selectionModel;

    PrintProgressPage *progressPage;
    PrintWorker *printTask;
    bool isStoppingTask;
};

#endif // PRINTWIZARD_H
