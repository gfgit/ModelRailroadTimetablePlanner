#ifndef PRINTWIZARD_H
#define PRINTWIZARD_H

#include <QWizard>
#include <QVector>

#include "printdefs.h"

class QPrinter;

class PrintWizard : public QWizard
{
    Q_OBJECT
public:

    explicit PrintWizard(QWidget *parent = nullptr);
    ~PrintWizard();

    void setOutputType(Print::OutputType out);
    Print::OutputType getOutputType() const;

    QPrinter *printer;
    QString fileOutput;
    bool differentFiles;

    Print::OutputType type;

    QVector<bool> m_checks;
};

#endif // PRINTWIZARD_H
