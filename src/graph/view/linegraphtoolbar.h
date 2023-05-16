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

#ifndef LINEGRAPHTOOLBAR_H
#define LINEGRAPHTOOLBAR_H

#include <QWidget>

#include "utils/types.h"

class LineGraphScene;
class LineGraphSelectionWidget;
class QPushButton;
class QSlider;
class QSpinBox;

/*!
 * \brief Toolbar to select and refresh graph
 *
 * \sa LineGraphSelectionWidget
 */
class LineGraphToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit LineGraphToolbar(QWidget *parent = nullptr);
    ~LineGraphToolbar();

    void setScene(LineGraphScene *scene);

    /*!
     * \brief listen to focus events and to slider double click
     *
     * If a children widget gets focus events we do not know it
     * So we install an event filter on every children and listen
     *
     * When \ref zoomSlider gets double clicked, reset zoom to 100%
     *
     * \sa focusInEvent()
     */
    bool eventFilter(QObject *watched, QEvent *ev) override;

signals:
    void requestRedraw();
    void requestZoom(int zoom);

public slots:
    void resetToolbarToScene();
    void updateZoomLevel(int zoom);

private slots:
    void onWidgetGraphChanged(int type, db_id objectId);
    void onSceneGraphChanged(int type, db_id objectId);
    void onSceneDestroyed();

protected:
    /*!
     * \brief Activate scene
     *
     * Activate scene, it will receive requests to show items
     *
     * \sa LineGraphScene::activateScene()
     * \sa LineGraphManager::setActiveScene()
     */
    void focusInEvent(QFocusEvent *e) override;

private:
    LineGraphScene *m_scene;

    LineGraphSelectionWidget *selectionWidget;
    QPushButton *redrawBut;

    QSlider *zoomSlider;
    QSpinBox *zoomSpinBox;
    int mZoom;
};

#endif // LINEGRAPHTOOLBAR_H
