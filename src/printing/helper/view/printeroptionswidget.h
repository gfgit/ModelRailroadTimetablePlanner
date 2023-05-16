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

#ifndef PRINTEROPTIONSWIDGET_H
#define PRINTEROPTIONSWIDGET_H

#include <QWidget>

#include "printing/helper/model/printdefs.h"
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

    Print::PageLayoutOpt getScenePageLay() const;
    void setScenePageLay(const Print::PageLayoutOpt &newScenePageLay);

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

    Print::PageLayoutOpt scenePageLay;
};

#endif // PRINTEROPTIONSWIDGET_H
