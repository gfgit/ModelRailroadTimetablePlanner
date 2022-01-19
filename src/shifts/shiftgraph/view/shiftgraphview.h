#ifndef SHIFTGRAPHVIEW_H
#define SHIFTGRAPHVIEW_H

#include <QAbstractScrollArea>

class ShiftGraphScene;

class ShiftGraphNameHeader;
class ShiftGraphHourPanel;

class ShiftGraphView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit ShiftGraphView(QWidget *parent = nullptr);

    ShiftGraphScene *scene() const;
    void setScene(ShiftGraphScene *newScene);

    inline int getZoomLevel() const { return mZoom; }

    QPointF mapToScene(const QPointF& pos, bool *ok = nullptr);

signals:
    void zoomLevelChanged(int zoom);

public slots:
    /*!
     * \brief Triggers graph redrawing
     *
     * \sa updateScrollBars()
     */
    void redrawGraph();

    void setZoomLevel(int zoom);

protected:
    /*!
     * \brief React to layout and style change
     *
     * On layout and style change update scroll bars
     * \sa updateScrollBars()
     */
    bool event(QEvent *e) override;

    /*!
     * \brief Show Tooltips
     *
     * Show tooltips on viewport
     * \sa LineGraphScene::getJobAt()
     */
    bool viewportEvent(QEvent *e) override;

    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *) override;

private slots:
    void onSceneDestroyed();
    void resizeHeaders();

private:
    /*!
     * \brief Update scrollbar size
     */
    void updateScrollBars();

private:
    ShiftGraphNameHeader *shiftHeader;
    ShiftGraphHourPanel *hourPanel;

    ShiftGraphScene *m_scene;

    int mZoom;
};

#endif // SHIFTGRAPHVIEW_H
