#include "printhelper.h"

#include "utils/scene/igraphscene.h"

#include <QPainter>

#include "utils/font_utils.h"

#include <QtMath>

QPageSize PrintHelper::fixPageSize(const QPageSize &pageSz, QPageLayout::Orientation &orient)
{
    //NOTE: Sometimes QPageSetupDialog messes up page size
    //To express A4 Landascape, it swaps height and width
    //But now the page size is no longer recognized as "A4"
    //And other dialogs might show "Custom", try to fix it.

    if(pageSz.id() != QPageSize::Custom)
        return pageSz; //Already correct value, use it directly

    const QSizeF defSize = pageSz.definitionSize();

    QPageSize possibleMatchSwapped(defSize.transposed(),
                                   pageSz.definitionUnits());
    if(possibleMatchSwapped.id() != QPageSize::Custom)
        return possibleMatchSwapped; //Found a match

    //No match found, at least check orientation
    if(defSize.width() > defSize.height())
        orient = QPageLayout::Landscape;
    else
        orient = QPageLayout::Portrait;
    return pageSz;
}

void PrintHelper::calculatePageCount(IGraphScene *scene, PageLayoutOpt &pageLay, QSizeF &outEffectivePageSize)
{
    QSizeF srcContentsSize;
    if(scene)
        srcContentsSize = scene->getContentsSize() * pageLay.scaleFactor;

    //NOTE: do not shift by top left margin here
    //This is because it should not be counted when calculating page count
    //as it is out of effective page size (= page inside margins)
    //Overall size is then computed from page count so it's always correct

    //Apply overlap margin
    //Each page has 2 margin (left and right) and other 2 margins (top and bottom)
    //Calculate effective content size inside margins
    outEffectivePageSize = pageLay.devicePageRect.size();
    outEffectivePageSize.rwidth() -= 2 * pageLay.marginOriginalWidth;
    outEffectivePageSize.rheight() -= 2 * pageLay.marginOriginalWidth;

    //Calculate page count, at least 1 page
    pageLay.horizPageCnt = qMax(1, qCeil(srcContentsSize.width() / outEffectivePageSize.width()));
    pageLay.vertPageCnt = qMax(1, qCeil(srcContentsSize.height() / outEffectivePageSize.height()));
}

bool PrintHelper::printPagedScene(QPainter *painter, IPagedPaintDevice *dev, IGraphScene *scene, IProgress *progress,
                                  PageLayoutScaled &pageLay, PageNumberOpt &pageNumOpt)
{
    //Send initial progress
    if(!progress->reportProgressAndContinue(0, -1))
        return false;

    if(!dev->needsInitForEachPage())
    {
        //Device was already inited
        //Reset transformation on new scene
        painter->resetTransform();
    }

    //Inverse scale for margins pen to keep it independent
    pageLay.lay.pageMarginsPen.setWidthF(pageLay.lay.pageMarginsPenWidth / pageLay.lay.scaleFactor);

    QSizeF effectivePageSize;
    calculatePageCount(scene, pageLay.lay, effectivePageSize);

    //Page info rect above top margin to draw page numbers
    QRectF pageNumbersRect(QPointF(pageLay.overlapMarginWidthScaled, 0),
                           QSizeF(effectivePageSize.width(), pageLay.overlapMarginWidthScaled));

    const QSizeF headerSize = scene->getHeaderSize();

    if(!progress->reportProgressAndContinue(0, pageLay.lay.horizPageCnt + pageLay.lay.vertPageCnt))
        return false;

    //Rect to paint on each page
    QRectF sourceRect = pageLay.scaledPageRect;

    for(int y = 0; y < pageLay.lay.vertPageCnt; y++)
    {
        for(int x = 0; x < pageLay.lay.horizPageCnt; x++)
        {
            //To avoid calling newPage() at end of loop
            //which would result in an empty extra page after last drawn page
            dev->newPage(painter, pageLay.lay.devicePageRect, pageLay.isFirstPage);
            if(pageLay.isFirstPage || dev->needsInitForEachPage())
            {
                //Apply scaling
                painter->scale(pageLay.lay.scaleFactor, pageLay.lay.scaleFactor);
                painter->translate(sourceRect.topLeft());

                //Inverse scale for page numbers font to keep it independent
                setFontPointSizeDPI(pageNumOpt.font, pageNumOpt.fontSize / pageLay.lay.scaleFactor, painter);
            }
            pageLay.isFirstPage = false;

            //Clipping must be set on every new page
            //Since clipping works in logical (QPainter) coordinates
            //we use sourceRect which is already transformed
            painter->setClipRect(sourceRect);

            //Render scene contets
            scene->renderContents(painter, sourceRect);

            //Render horizontal header
            QRectF horizHeaderRect = sourceRect;
            horizHeaderRect.moveTop(0);
            horizHeaderRect.setBottom(headerSize.height());
            scene->renderHeader(painter, horizHeaderRect, Qt::Horizontal, 0);

            //Render vertical header
            QRectF vertHeaderRect = sourceRect;
            vertHeaderRect.moveLeft(0);
            vertHeaderRect.setRight(headerSize.width());
            scene->renderHeader(painter, vertHeaderRect, Qt::Vertical, 0);

            if(pageLay.lay.drawPageMargins)
            {
                //Draw a frame to help align printed pages
                painter->setPen(pageLay.lay.pageMarginsPen);
                QLineF arr[4] = {
                    //Top
                    QLineF(sourceRect.left(),  sourceRect.top() + pageLay.overlapMarginWidthScaled,
                           sourceRect.right(), sourceRect.top() + pageLay.overlapMarginWidthScaled),
                    //Bottom
                    QLineF(sourceRect.left(),  sourceRect.bottom() - pageLay.overlapMarginWidthScaled,
                           sourceRect.right(), sourceRect.bottom() - pageLay.overlapMarginWidthScaled),
                    //Left
                    QLineF(sourceRect.left() + pageLay.overlapMarginWidthScaled, sourceRect.top(),
                           sourceRect.left() + pageLay.overlapMarginWidthScaled, sourceRect.bottom()),
                    //Right
                    QLineF(sourceRect.right() - pageLay.overlapMarginWidthScaled, sourceRect.top(),
                           sourceRect.right() - pageLay.overlapMarginWidthScaled, sourceRect.bottom())
                };
                painter->drawLines(arr, 4);
            }

            if(pageNumOpt.enable)
            {
                //Draw page numbers to help laying out printed pages
                //Add +1 because loop starts from 0
                const QString str = pageNumOpt.fmt
                                        .arg(y + 1).arg(pageLay.lay.vertPageCnt)
                                        .arg(x + 1).arg(pageLay.lay.horizPageCnt);

                //Move to top left but separate a bit from left margin
                pageNumbersRect.moveTop(sourceRect.top());
                pageNumbersRect.moveLeft(sourceRect.left() + pageLay.overlapMarginWidthScaled * 1.5);
                painter->setFont(pageNumOpt.font);
                painter->drawText(pageNumbersRect, Qt::AlignVCenter | Qt::AlignLeft, str);
            }

            //Go left
            sourceRect.moveLeft(sourceRect.left() + effectivePageSize.width());
            painter->translate(-effectivePageSize.width(), 0);

            //Report progress
            if(!progress->reportProgressAndContinue(y * pageLay.lay.horizPageCnt + x,
                                                     pageLay.lay.horizPageCnt + pageLay.lay.vertPageCnt))
                return false;
        }

        //Go down and back to left most column
        sourceRect.moveTop(sourceRect.top() + effectivePageSize.height());
        painter->translate(sourceRect.left(), -effectivePageSize.height());
        sourceRect.moveLeft(0);
    }

    //Reset to top most and left most
    painter->translate(sourceRect.topLeft());
    sourceRect.moveTopLeft(QPointF());

    return true;
}
