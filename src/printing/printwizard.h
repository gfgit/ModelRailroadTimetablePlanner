#ifndef PRINTWIZARD_H
#define PRINTWIZARD_H

#include <QWizard>
#include <QVector>

#include "printdefs.h"

class QPrinter;
class SceneSelectionModel;

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

    QPrinter *getPrinter() const;

private:
    QPrinter *printer;
    QString fileOutput;
    bool differentFiles;

    Print::OutputType type;

    SceneSelectionModel *selectionModel;
};

#endif // PRINTWIZARD_H
