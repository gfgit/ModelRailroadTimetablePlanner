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

#ifndef SCENEPRINTPREVIEWDLG_H
#define SCENEPRINTPREVIEWDLG_H

#include <QDialog>

#include <QPageLayout>
#include "printing/helper/model/printhelper.h"

class BasicGraphView;

class IGraphScene;
class PrintPreviewSceneProxy;

class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;

class QPrinter;

class ScenePrintPreviewDlg : public QDialog
{
    Q_OBJECT
public:
    explicit ScenePrintPreviewDlg(QWidget *parent = nullptr);

    /*!
     * \brief listen to slider double click
     *
     * When \ref zoomSlider gets double clicked, reset zoom to 100%
     */
    bool eventFilter(QObject *watched, QEvent *ev) override;

    void setSourceScene(IGraphScene *sourceScene);

    void setSceneScale(double scaleFactor);

    QPrinter *printer() const;
    void setPrinter(QPrinter *newPrinter);

    inline QPageLayout getPrinterPageLay() const { return printerPageLay; }
    void setPrinterPageLay(const QPageLayout& pageLay);

    Print::PageLayoutOpt getScenePageLay() const;
    void setScenePageLay(const Print::PageLayoutOpt &newScenePageLay);

private slots:
    void updateZoomLevel(int zoom);
    void onScaleChanged(double zoom);
    void setMarginsWidth(double margins);

    void showPageSetupDlg();

    void updatePageCount();

private:
    void updateModelPageSize();

private:
    BasicGraphView *graphView;
    PrintPreviewSceneProxy *previewScene;

    QSlider *zoomSlider;
    QSpinBox *zoomSpinBox;

    QDoubleSpinBox *sceneScaleSpinBox;
    QDoubleSpinBox *marginSpinBox;

    QLabel *pageCountLabel;

    QPrinter *m_printer;
    QPageLayout printerPageLay;
};

#endif // SCENEPRINTPREVIEWDLG_H
