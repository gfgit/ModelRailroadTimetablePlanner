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

#ifndef PRINTHELPER_H
#define PRINTHELPER_H

#include <QPageLayout>
#include <QPen>
#include <QFont>

class IGraphScene;

namespace Print {

//Standard resolution, set to all printers to calculate page size
static constexpr const double PrinterDefaultResolution = 72.0;

//Page Layout
struct PageLayoutOpt
{
    QRectF pageRectPoints;

    int pageCountHoriz = 0;
    int pageCountVert = 0;

    double sourceScaleFactor = 0.5;
    double marginWidthPoints = 20;

    double pageMarginsPenWidthPoints = 7;
    bool drawPageMargins = true;
};

//Scaled values useful for printing
struct PageLayoutScaled
{
    PageLayoutOpt lay;

    //Device page rect is multiplied by printerResolution
    QRectF devicePageRectPixels;
    double printerResolution = PrinterDefaultResolution;

    //Premultiplied originalScaleFactor for PrinterDefaultResolution/printerResolution
    //This is needed to compensate devicePageRect which depends on printer resolution
    double realScaleFactor = 1;

    double overlapMarginWidthScaled = 20;

    QPen pageMarginsPen;
};

//Page Numbers
struct PageNumberOpt
{
    QFont font;
    QString fmt;
    double fontSizePt = 20;
    bool enable = true;
};

//Device
class IPagedPaintDevice
{
public:
    virtual ~IPagedPaintDevice() {};
    virtual bool newPage(QPainter *painter, const QRectF& brect, bool isFirstPage) = 0;

    inline bool needsInitForEachPage() const { return m_needsInitForEachPage; }

protected:
    bool m_needsInitForEachPage;
};

class IProgress
{
public:
    virtual ~IProgress() {};
    virtual bool reportProgressAndContinue(int current, int max) = 0;

    static const int ProgressSetMaximum = -1;
};

} //namespace Print


class PrintHelper
{
public:
    static QPageSize fixPageSize(const QPageSize& pageSz, QPageLayout::Orientation &orient);

    static void applyPageSize(const QPageSize& pageSz, const QPageLayout::Orientation &orient, Print::PageLayoutOpt &pageLay);

    static void initScaledLayout(Print::PageLayoutScaled &scaledPageLay, const Print::PageLayoutOpt &pageLay);

    static void calculatePageCount(IGraphScene *scene, Print::PageLayoutOpt& pageLay,
                                   QSizeF &outEffectivePageSizePoints);

    static bool printPagedScene(QPainter *painter, Print::IPagedPaintDevice *dev, IGraphScene *scene, Print::IProgress *progress,
                                Print::PageLayoutScaled &pageLay, Print::PageNumberOpt& pageNumOpt);
};

#endif // PRINTHELPER_H
