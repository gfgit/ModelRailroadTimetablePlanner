#ifndef PRINTHELPER_H
#define PRINTHELPER_H

#include <QRectF>
#include <QPen>
#include <QFont>


class PrintHelper
{
public:
    //Page Layout
    struct PageLayoutOpt
    {
        QRectF devicePageRect;
        QRectF scaledPageRect;

        double scaleFactor = 1;
        double marginOriginalWidth = 20;
        double overlapMarginWidthScaled = 20;

        int horizPageCnt = 0;
        int vertPageCnt = 0;

        bool drawPageMargins = true;
        double pageMarginsPenWidth = 5;
        QPen pageMarginsPen;

        bool isFirstPage = true;
    };

    //Page Numbers
    struct PageNumberOpt
    {
        QFont font;
        QString fmt;
        double fontSize = 20;
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

    class IRenderScene
    {
    public:
        virtual ~IRenderScene() {};
        virtual bool render(QPainter *painter, const QRectF& sceneRect) = 0;

        inline QSizeF getContentSize() const { return m_contentSize; }

    protected:
        QSizeF m_contentSize;
    };

    class IProgress
    {
    public:
        virtual ~IProgress() {};
        virtual bool reportProgressAndContinue(int current, int max) = 0;
    };

    static bool printPagedScene(QPainter *painter, IPagedPaintDevice *dev, IRenderScene *scene, IProgress *progress,
                                PageLayoutOpt& pageLay, PageNumberOpt& pageNumOpt);
};

#endif // PRINTHELPER_H
