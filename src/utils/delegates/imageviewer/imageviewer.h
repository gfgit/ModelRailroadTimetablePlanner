/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
    virtual ~ImageViewer()
    {
    }

    void setImage(const QImage &img);

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
