#ifndef SHIFTGRAPHPRINTDLG_H
#define SHIFTGRAPHPRINTDLG_H

#include <QDialog>

#include "printing/printdefs.h"

class PrinterOptionsWidget;
class QDialogButtonBox;

class QGroupBox;
class QLabel;
class QProgressBar;

class QPrinter;
class ShiftGraphSceneCollection;

class PrintWorkerHandler;

namespace sqlite3pp {
class database;
}

namespace Print {
struct PageLayoutOpt;
}

class ShiftGraphPrintDlg : public QDialog
{
    Q_OBJECT
public:
    explicit ShiftGraphPrintDlg(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~ShiftGraphPrintDlg();

    void done(int res) override;

    void setOutputType(Print::OutputType type);

private slots:
    void updatePrintButton();
    void progressMaxChanged(int max);
    void progressChanged(int val, const QString& msg);
    void handleProgressFinished(bool success, const QString& errMsg);

private:
    sqlite3pp::database &mDb;

    PrinterOptionsWidget *optionsWidget;
    QDialogButtonBox *buttonBox;

    QGroupBox *progressBox;
    QLabel *progressLabel;
    QProgressBar *progressBar;

    ShiftGraphSceneCollection *m_collection;
    QPrinter *m_printer;

    PrintWorkerHandler *printTaskHandler;
};

#endif // SHIFTGRAPHPRINTDLG_H
