#include "printhelper.h"

#include <QPainter>

#include <QtMath>

#include "utils/font_utils.h"

bool PrintHelper::printPagedScene(QPainter *painter, IPagedPaintDevice *dev, IRenderScene *scene, IProgress *progress,
                                  PageLayoutOpt &pageLay, PageNumberOpt &pageNumOpt)
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
    pageLay.pageMarginsPen.setWidth(pageLay.pageMarginsPenWidth / pageLay.scaleFactor);

    //Apply overlap margin
    //Each page has 2 margin (left and right) and other 2 margins (top and bottom)
    //Calculate effective content size inside margins
    QPointF effectivePageOrigin(pageLay.overlapMarginWidthScaled, pageLay.overlapMarginWidthScaled);
    QSizeF effectivePageSize = pageLay.scaledPageRect.size();
    effectivePageSize.rwidth() -= 2 * pageLay.overlapMarginWidthScaled;
    effectivePageSize.rheight() -= 2 * pageLay.overlapMarginWidthScaled;

    //Page info rect above top margin to draw page numbers
    QRectF pageNumbersRect(QPointF(pageLay.overlapMarginWidthScaled, 0),
                           QSizeF(effectivePageSize.width(), pageLay.overlapMarginWidthScaled));

    //Calculate page count
    pageLay.horizPageCnt = qCeil(scene->getContentSize().width() / effectivePageSize.width());
    pageLay.vertPageCnt = qCeil(scene->getContentSize().height() / effectivePageSize.height());

    if(!progress->reportProgressAndContinue(0, pageLay.horizPageCnt + pageLay.vertPageCnt))
        return false;

    //Rect to paint on each page
    QRectF sourceRect = pageLay.scaledPageRect;

    for(int y = 0; y < pageLay.vertPageCnt; y++)
    {
        for(int x = 0; x < pageLay.horizPageCnt; x++)
        {
            //To avoid calling newPage() at end of loop
            //which would result in an empty extra page after last drawn page
            dev->newPage(painter, pageLay.devicePageRect, pageLay.isFirstPage);
            if(pageLay.isFirstPage || dev->needsInitForEachPage())
            {
                //Apply scaling
                painter->scale(pageLay.scaleFactor, pageLay.scaleFactor);
                painter->translate(sourceRect.topLeft());

                //Inverse scale for page numbers font to keep it independent
                setFontPointSizeDPI(pageNumOpt.font, pageNumOpt.fontSize / pageLay.scaleFactor, painter);
            }
            pageLay.isFirstPage = false;

            //Clipping must be set on every new page
            //Since clipping works in logical (QPainter) coordinates
            //we use sourceRect which is already transformed
            painter->setClipRect(sourceRect);

            //Render scene
            scene->render(painter, sourceRect);

            if(pageLay.drawPageMargins)
            {
                //Draw a frame to help align printed pages
                painter->setPen(pageLay.pageMarginsPen);
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
                                        .arg(y + 1).arg(pageLay.vertPageCnt)
                                        .arg(x + 1).arg(pageLay.horizPageCnt);

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
            if(!progress->reportProgressAndContinue(y * pageLay.horizPageCnt + x,
                                                     pageLay.horizPageCnt + pageLay.vertPageCnt))
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
