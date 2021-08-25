#ifndef LINEGRAPHVIEW_H
#define LINEGRAPHVIEW_H

#include <QAbstractScrollArea>

class LineGraphScene;

class StationLabelsHeader;
class HourPanel;

/*!
 * \brief Widget to display a LineGraphScene
 *
 * A scrollable widget which renders LineGraphScene contents
 * Moving the mouse cursor on the contents, tooltips will show
 *
 * \sa LineGraphWidget
 * \sa BackgroundHelper
 */
class LineGraphView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit LineGraphView(QWidget *parent = nullptr);

    LineGraphScene *scene() const;
    void setScene(LineGraphScene *newScene);

    void ensureVisible(int x, int y, int xmargin, int ymargin);

signals:
    void syncToolbarToScene();

public slots:
    void redrawGraph();

protected:
    bool event(QEvent *e) override;
    bool viewportEvent(QEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private slots:
    void onSceneDestroyed();
    void resizeHeaders();

private:
    void updateScrollBars();

private:
    StationLabelsHeader *stationHeader;
    HourPanel *hourPanel;

    LineGraphScene *m_scene;
};

#endif // LINEGRAPHVIEW_H
