#ifndef SHIFTGRAPHNAMEHEADER_H
#define SHIFTGRAPHNAMEHEADER_H

#include <QWidget>

class ShiftGraphScene;

class ShiftGraphNameHeader : public QWidget
{
    Q_OBJECT
public:
    explicit ShiftGraphNameHeader(QWidget *parent = nullptr);

    void setScene(ShiftGraphScene *newScene);

public slots:
    void setScroll(int value);
    void setZoom(int val);

protected:
    void paintEvent(QPaintEvent *e);

private:
    ShiftGraphScene *m_scene;
    int verticalScroll;
    int mZoom;

};

#endif // SHIFTGRAPHNAMEHEADER_H
