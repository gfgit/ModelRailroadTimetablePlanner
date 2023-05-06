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

#ifndef PRINTPREVIEWSCENEPROXY_H
#define PRINTPREVIEWSCENEPROXY_H

#include "utils/scene/igraphscene.h"

#include "printhelper.h"

class PrintPreviewSceneProxy : public IGraphScene
{
    Q_OBJECT
public:
    PrintPreviewSceneProxy(QObject *parent = nullptr);

    virtual void renderContents(QPainter *painter, const QRectF& sceneRect) override;
    virtual void renderHeader(QPainter *painter, const QRectF& sceneRect,
                              Qt::Orientation orient, double scroll) override;

    IGraphScene *getSourceScene() const;
    void setSourceScene(IGraphScene *newSourceScene);

    double getViewScaleFactor() const;
    void setViewScaleFactor(double newViewScaleFactor);

    Print::PageLayoutOpt getPageLay() const;
    void setPageLay(const Print::PageLayoutOpt& newPageLay);

signals:
    void pageCountChanged();

private slots:
    void onSourceSceneDestroyed();
    void updateSourceSizeAndRedraw();

private:
    void drawPageBorders(QPainter *painter, const QRectF& sceneRect,
                         bool isHeader, Qt::Orientation orient = Qt::Horizontal);

private:
    IGraphScene *m_sourceScene;
    Print::PageLayoutOpt m_pageLay;
    QSizeF effectivePageSize;

    double viewScaleFactor;
    QSizeF originalHeaderSize;
};

#endif // PRINTPREVIEWSCENEPROXY_H
