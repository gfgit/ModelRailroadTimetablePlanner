#ifndef PRINTHELPER_H
#define PRINTHELPER_H

#include <QPageLayout>
#include <QPen>
#include <QFont>

class IGraphScene;

class PrintHelper
{
public:

    //Standard resolution, set to all printers to calculate page size
    static constexpr const int PrinterResolution = 1000;

    //Page Layout
    struct PageLayoutOpt
    {
        QRectF devicePageRect;

        double scaleFactor = 1;
        double marginOriginalWidth = 20;

        int horizPageCnt = 0;
        int vertPageCnt = 0;

        bool drawPageMargins = true;
        double pageMarginsPenWidth = 7;
        QPen pageMarginsPen;
    };

    //Scaled values useful for printing
    struct PageLayoutScaled
    {
        PageLayoutOpt lay;
        QRectF scaledPageRect;
        double overlapMarginWidthScaled;
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

    class IProgress
    {
    public:
        virtual ~IProgress() {};
        virtual bool reportProgressAndContinue(int current, int max) = 0;
    };

    static QPageSize fixPageSize(const QPageSize& pageSz, QPageLayout::Orientation &orient);

    static void calculatePageCount(IGraphScene *scene, PageLayoutOpt& pageLay,
                                   QSizeF &outEffectivePageSize);

    static bool printPagedScene(QPainter *painter, IPagedPaintDevice *dev, IGraphScene *scene, IProgress *progress,
                                PageLayoutScaled &pageLay, PageNumberOpt& pageNumOpt);
};

#endif // PRINTHELPER_H
