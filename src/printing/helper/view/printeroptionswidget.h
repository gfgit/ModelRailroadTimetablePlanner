#ifndef PRINTEROPTIONSWIDGET_H
#define PRINTEROPTIONSWIDGET_H

#include <QWidget>

#include "printing/printdefs.h"
#include "printing/helper/model/printhelper.h"

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QPushButton;

class QPrinter;

class PrinterOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PrinterOptionsWidget(QWidget *parent = nullptr);

    void setOptions(const Print::PrintBasicOptions &printOpt);
    Print::PrintBasicOptions getOptions() const;

    bool validateOptions();
    bool isComplete() const;

    QPrinter *printer() const;
    void setPrinter(QPrinter *newPrinter);

    PrintHelper::PageLayoutOpt getScenePageLay() const;
    void setScenePageLay(const PrintHelper::PageLayoutOpt &newScenePageLay);

    IGraphScene *sourceScene() const;
    void setSourceScene(IGraphScene *newSourceScene);

signals:
    void completeChanged();

private slots:
    void updateOutputType();
    void onChooseFile();
    void updateDifferentFiles();
    void onOpenPageSetup();
    void onShowPreviewDlg();

private:
    void createFilesBox();
    void createPageLayoutBox();

private:
    QGroupBox *fileBox;
    QCheckBox *differentFilesCheckBox;
    QCheckBox *sceneInOnePageCheckBox;
    QLineEdit *pathEdit;
    QLineEdit *patternEdit;
    QPushButton *fileBut;

    QGroupBox *pageLayoutBox;
    QPushButton *pageSetupDlgBut;
    QPushButton *previewDlgBut;

    QComboBox *outputTypeCombo;

    QPrinter *m_printer;

    IGraphScene *m_sourceScene;

    PrintHelper::PageLayoutOpt scenePageLay;
};

#endif // PRINTEROPTIONSWIDGET_H
