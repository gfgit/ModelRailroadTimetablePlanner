#ifndef STATIONLABELSHEADER_H
#define STATIONLABELSHEADER_H

#include <QWidget>

class LineGraphScene;

class StationLabelsHeader : public QWidget
{
    Q_OBJECT
public:
    explicit StationLabelsHeader(QWidget *parent = nullptr);

    LineGraphScene *scene() const;
    void setScene(LineGraphScene *newScene);

public slots:
    void setScroll(int value);

private slots:
    void onSceneDestroyed();

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    LineGraphScene *m_scene;
    int horizontalScroll;
};

#endif // STATIONLABELSHEADER_H
