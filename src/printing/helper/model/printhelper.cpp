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

#include "printhelper.h"

#include "utils/scene/igraphscene.h"

#include <QPainter>

#include <QtMath>

QPageSize PrintHelper::fixPageSize(const QPageSize &pageSz, QPageLayout::Orientation &orient)
{
    // NOTE: Sometimes QPageSetupDialog messes up page size
    // To express A4 Landascape, it swaps height and width
    // But now the page size is no longer recognized as "A4"
    // And other dialogs might show "Custom", try to fix it.

    if (pageSz.id() != QPageSize::Custom)
        return pageSz; // Already correct value, use it directly

    const QSizeF defSize = pageSz.definitionSize();

    QPageSize possibleMatchSwapped(defSize.transposed(), pageSz.definitionUnits());
    if (possibleMatchSwapped.id() != QPageSize::Custom)
        return possibleMatchSwapped; // Found a match

    // No match found, at least check orientation
    if (defSize.width() > defSize.height())
        orient = QPageLayout::Landscape;
    else
        orient = QPageLayout::Portrait;
    return pageSz;
}

void PrintHelper::applyPageSize(const QPageSize &pageSz, const QPageLayout::Orientation &orient,
                                Print::PageLayoutOpt &pageLay)
{
    QRect pointsRect = pageSz.rectPoints();

    const bool shouldTranspose =
      (orient == QPageLayout::Portrait && pointsRect.width() > pointsRect.height())
      || (orient == QPageLayout::Landscape && pointsRect.width() < pointsRect.height());

    if (shouldTranspose)
        pointsRect = pointsRect.transposed();

    pageLay.pageRectPoints = pointsRect;
}

void PrintHelper::initScaledLayout(Print::PageLayoutScaled &scaledPageLay,
                                   const Print::PageLayoutOpt &pageLay)
{
    scaledPageLay.lay = pageLay;

    // Update printer resolution
    double resolutionFactor = scaledPageLay.printerResolution / Print::PrinterDefaultResolution;

    scaledPageLay.realScaleFactor = pageLay.sourceScaleFactor * resolutionFactor;

    // Inverse scale for margins and pen to keep them independent
    scaledPageLay.overlapMarginWidthScaled = pageLay.marginWidthPoints / pageLay.sourceScaleFactor;
    scaledPageLay.pageMarginsPen.setWidthF(pageLay.pageMarginsPenWidthPoints
                                           / pageLay.sourceScaleFactor);
}

void PrintHelper::calculatePageCount(IGraphScene *scene, Print::PageLayoutOpt &pageLay,
                                     QSizeF &outEffectivePageSizePoints)
{
    QSizeF srcContentsSize;
    if (scene)
        srcContentsSize = scene->getContentsSize() * pageLay.sourceScaleFactor;

    // NOTE: do not shift by top left margin here
    // This is because it should not be counted when calculating page count
    // as it is out of effective page size (= page inside margins)
    // Overall size is then computed from page count so it's always correct

    // Apply overlap margin
    // Each page has 2 margin (left and right) and other 2 margins (top and bottom)
    // Calculate effective content size inside margins
    const double sizeShrink    = 2 * pageLay.marginWidthPoints;
    outEffectivePageSizePoints = pageLay.pageRectPoints.size();
    outEffectivePageSizePoints.rwidth() -= sizeShrink;
    outEffectivePageSizePoints.rheight() -= sizeShrink;

    // Calculate page count, at least 1 page
    pageLay.pageCountHoriz =
      qMax(1, qCeil(srcContentsSize.width() / outEffectivePageSizePoints.width()));
    pageLay.pageCountVert =
      qMax(1, qCeil(srcContentsSize.height() / outEffectivePageSizePoints.height()));
}

bool PrintHelper::printPagedScene(QPainter *painter, Print::IPagedPaintDevice *dev,
                                  IGraphScene *scene, Print::IProgress *progress,
                                  Print::PageLayoutScaled &pageLay,
                                  Print::PageNumberOpt &pageNumOpt)
{
    QSizeF effectivePageSize;
    calculatePageCount(scene, pageLay.lay, effectivePageSize);

    // NOTE: effectivePageSize is in points, we need to scale by inverse of source scene
    effectivePageSize /= pageLay.lay.sourceScaleFactor;

    // Fix margins by half pen with so pen does not draw on over contents
    const double marginCorrection = pageLay.pageMarginsPen.widthF() / 2.0;

    // Page info rect above top margin to draw page numbers
    QRectF pageNumbersRect(QPointF(pageLay.overlapMarginWidthScaled, 0),
                           QSizeF(effectivePageSize.width(), pageLay.overlapMarginWidthScaled));
    if (pageLay.lay.marginWidthPoints < 8)
        pageNumbersRect.setHeight(8.0 / pageLay.lay.sourceScaleFactor); // Minimum height

    // Avoid overflowing text outside margins
    double fontSizePt = qMin(pageNumOpt.fontSizePt, pageLay.lay.marginWidthPoints * 0.8);
    if (fontSizePt < 5)
        fontSizePt = 5; // Minimum font size

    pageNumOpt.font.setPointSizeF(fontSizePt / pageLay.realScaleFactor);

    const QSizeF headerSize = scene->getHeaderSize();

    // Set maximum progress (= total page count)
    if (progress
        && !progress->reportProgressAndContinue(Print::IProgress::ProgressSetMaximum,
                                                pageLay.lay.pageCountHoriz
                                                  * pageLay.lay.pageCountVert))
        return false;

    // Rect to paint on each page (inverse scale of source)
    QRectF sourceRect =
      QRectF(QPointF(), pageLay.lay.pageRectPoints.size() / pageLay.lay.sourceScaleFactor);

    // Shift by top left page margins
    const QPointF origin(pageLay.overlapMarginWidthScaled, pageLay.overlapMarginWidthScaled);
    sourceRect.moveTopLeft(sourceRect.topLeft() - origin);

    for (int y = 0; y < pageLay.lay.pageCountVert; y++)
    {
        for (int x = 0; x < pageLay.lay.pageCountHoriz; x++)
        {
            const bool onFirstPage = x + y == 0;
            // To avoid calling newPage() at end of loop
            // which would result in an empty extra page after last drawn page
            dev->newPage(painter, pageLay.devicePageRectPixels, onFirstPage);

            if (dev->needsInitForEachPage() || onFirstPage)
            {
                // If on first page of current scene or need init for every page
                // Reset painter transform because device might be already inited
                painter->resetTransform();

                // Apply scaling
                painter->scale(pageLay.realScaleFactor, pageLay.realScaleFactor);

                // Shift by inverse of top left page margins
                painter->translate(origin);
            }

            // Clipping must be set on every new page
            // Since clipping works in logical (QPainter) coordinates
            // we use sourceRect which is already transformed
            painter->setClipRect(sourceRect);

            QRectF sceneRect = sourceRect;
            if (sceneRect.left() < 0)
                sceneRect.setLeft(0);
            if (sceneRect.top() < 0)
                sceneRect.setTop(0);

            // Render scene contets
            scene->renderContents(painter, sceneRect);

            // Render horizontal header
            QRectF horizHeaderRect = sceneRect;
            horizHeaderRect.moveTop(0);
            horizHeaderRect.setBottom(headerSize.height());
            scene->renderHeader(painter, horizHeaderRect, Qt::Horizontal, 0);

            // Render vertical header
            QRectF vertHeaderRect = sceneRect;
            vertHeaderRect.moveLeft(0);
            vertHeaderRect.setRight(headerSize.width());
            scene->renderHeader(painter, vertHeaderRect, Qt::Vertical, 0);

            if (pageLay.lay.drawPageMargins)
            {
                // Draw a frame to help align printed pages
                painter->setPen(pageLay.pageMarginsPen);
                QLineF arr[4] = {
                  // Top
                  QLineF(sourceRect.left(),
                         sourceRect.top() + pageLay.overlapMarginWidthScaled - marginCorrection,
                         sourceRect.right(),
                         sourceRect.top() + pageLay.overlapMarginWidthScaled - marginCorrection),
                  // Bottom
                  QLineF(sourceRect.left(),
                         sourceRect.bottom() - pageLay.overlapMarginWidthScaled + marginCorrection,
                         sourceRect.right(),
                         sourceRect.bottom() - pageLay.overlapMarginWidthScaled + marginCorrection),
                  // Left
                  QLineF(sourceRect.left() + pageLay.overlapMarginWidthScaled - marginCorrection,
                         sourceRect.top(),
                         sourceRect.left() + pageLay.overlapMarginWidthScaled - marginCorrection,
                         sourceRect.bottom()),
                  // Right
                  QLineF(sourceRect.right() - pageLay.overlapMarginWidthScaled + marginCorrection,
                         sourceRect.top(),
                         sourceRect.right() - pageLay.overlapMarginWidthScaled + marginCorrection,
                         sourceRect.bottom())};
                painter->drawLines(arr, 4);
            }

            if (pageNumOpt.enable)
            {
                // Draw page numbers to help laying out printed pages
                // Add +1 because loop starts from 0
                const QString str = pageNumOpt.fmt.arg(y + 1)
                                      .arg(pageLay.lay.pageCountVert)
                                      .arg(x + 1)
                                      .arg(pageLay.lay.pageCountHoriz);

                // Move to top left but separate a bit from left margin
                pageNumbersRect.moveTop(sourceRect.top());
                pageNumbersRect.moveLeft(sourceRect.left()
                                         + pageLay.overlapMarginWidthScaled * 1.5);
                painter->setFont(pageNumOpt.font);
                painter->drawText(pageNumbersRect, Qt::AlignVCenter | Qt::AlignLeft, str);
            }

            // Go left
            sourceRect.moveLeft(sourceRect.left() + effectivePageSize.width());
            painter->translate(-effectivePageSize.width(), 0);

            // Report progress
            if (progress
                && !progress->reportProgressAndContinue(y * pageLay.lay.pageCountHoriz + x,
                                                        pageLay.lay.pageCountHoriz
                                                          * pageLay.lay.pageCountVert))
                return false;
        }

        // Go down and back to left most column
        sourceRect.moveTop(sourceRect.top() + effectivePageSize.height());
        painter->translate(sourceRect.left() + origin.x(), -effectivePageSize.height());
        sourceRect.moveLeft(-origin.x());
    }

    // Reset to top most and left most
    painter->translate(sourceRect.topLeft());
    sourceRect.moveTopLeft(QPointF());

    return true;
}
