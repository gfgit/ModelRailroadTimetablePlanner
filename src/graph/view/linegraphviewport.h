#ifndef LINEGRAPHVIEWPORT_H
#define LINEGRAPHVIEWPORT_H

#include <QWidget>

class LineGraphScene;

class LineGraphViewport : public QWidget
{
    Q_OBJECT
public:
    explicit LineGraphViewport(QWidget *parent = nullptr);

    LineGraphScene *scene() const;
    void setScene(LineGraphScene *newScene);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void paintStations(QPainter *painter);

private:
    LineGraphScene *m_scene;
};

#endif // LINEGRAPHVIEWPORT_H
