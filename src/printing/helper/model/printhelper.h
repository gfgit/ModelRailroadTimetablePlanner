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

    static QPageSize fixPageSize(const QPageSize& pageSz, QPageLayout::Orientation &orient);
    static void applyPageSize(const QPageSize& pageSz, const QPageLayout::Orientation &orient, PageLayoutOpt& pageLay);

    static void initScaledLayout(PageLayoutScaled &scaledPageLay, const PageLayoutOpt &pageLay);

    static void calculatePageCount(IGraphScene *scene, PageLayoutOpt& pageLay,
                                   QSizeF &outEffectivePageSizePoints);

    static bool printPagedScene(QPainter *painter, IPagedPaintDevice *dev, IGraphScene *scene, IProgress *progress,
                                PageLayoutScaled &pageLay, PageNumberOpt& pageNumOpt);
};

#endif // PRINTHELPER_H
