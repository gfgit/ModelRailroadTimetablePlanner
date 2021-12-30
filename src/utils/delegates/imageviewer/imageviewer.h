#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QDialog>
#include <QImage>

class QScrollArea;
class QSlider;
class QLabel;

class ImageViewer : public QDialog
{
    Q_OBJECT
public:
    ImageViewer(QWidget *parent = nullptr);
    virtual ~ImageViewer() {}

    void setImage(const QImage& img);

public slots:
    void setScale(int val);

private:
    QImage originalImg;
    QImage scaledImg;

    QSlider *slider;
    QScrollArea *scrollArea;
    QLabel *infoLabel;
    QLabel *imageLabel;
};

#endif // IMAGEVIEWER_H
