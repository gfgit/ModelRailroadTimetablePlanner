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

#include "imageviewer.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QSlider>

ImageViewer::ImageViewer(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    infoLabel = new QLabel;
    lay->addWidget(infoLabel);

    slider = new QSlider(Qt::Horizontal);
    lay->addWidget(slider);

    scrollArea = new QScrollArea;
    lay->addWidget(scrollArea);

    imageLabel = new QLabel;
    imageLabel->setBackgroundRole(QPalette::Base);

    scrollArea->setWidget(imageLabel);
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setAlignment(Qt::AlignCenter);

    slider->setRange(1, 200);
    connect(slider, &QSlider::valueChanged, this, &ImageViewer::setScale);

    setMinimumSize(200, 200);
}

void ImageViewer::setImage(const QImage &img)
{
    originalImg = img;


    const int widthPx = img.width();
    const int heightPx = img.height();
    const int dotsX = img.dotsPerMeterX();
    const int dotsY = img.dotsPerMeterY();

    const double widthCm = 100.0 * double(widthPx) / double(dotsX);
    const double heightCm = 100.0 * double(heightPx) / double(dotsY);

    QString info;
    info.append(QStringLiteral("Size: %1px / %2px\n").arg(widthPx).arg(heightPx));
    info.append(QStringLiteral("Width:  %1 cm\n").arg(widthCm, 0, 'f', 2));
    info.append(QStringLiteral("Height: %1 cm\n").arg(heightCm, 0, 'f', 2));

    QStringList keys = img.textKeys();
    for(const QString& key : keys)
    {
        info.append("'");
        info.append(key);
        info.append("' = '");
        info.append(img.text(key));
        info.append("'\n");
    }
    infoLabel->setText(info);

    setScale(100);

    if(originalImg.isNull())
    {
        imageLabel->setPixmap(QPixmap());
        imageLabel->setText(tr("No image"));
        imageLabel->adjustSize();
    }
}

void ImageViewer::setScale(int val)
{
    if(originalImg.isNull())
        return;

    slider->setValue(val);
    QString tip = QStringLiteral("%1%").arg(val);
    imageLabel->setToolTip(tip);
    slider->setToolTip(tip);

    if(val == 100)
    {
        scaledImg = originalImg;
    }else{
        scaledImg = originalImg.scaledToWidth(originalImg.width() * val / 100, Qt::SmoothTransformation);
    }

    imageLabel->setPixmap(QPixmap::fromImage(scaledImg));
    imageLabel->adjustSize();
}
