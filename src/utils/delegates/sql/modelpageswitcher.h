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

#ifndef MODELPAGESWITCHER_H
#define MODELPAGESWITCHER_H

#include <QWidget>

class QLabel;
class QPushButton;
class QSpinBox;

class IPagedItemModel;

class ModelPageSwitcher : public QWidget
{
    Q_OBJECT
public:
    ModelPageSwitcher(bool autoHide, QWidget *parent = nullptr);

    void setModel(IPagedItemModel *m);

public slots:
    void prevPage();
    void nextPage();

private slots:
    void onModelDestroyed();
    void totalItemCountChanged();
    void pageCountChanged();
    void currentPageChanged();

    void goToPage();
    void refreshModel();

private:
    void recalcText();
    void recalcPages();

private:
    IPagedItemModel *model;
    QLabel *statusLabel;
    QPushButton *prevBut;
    QPushButton *nextBut;
    QSpinBox *pageSpin;
    QPushButton *goToPageBut;
    QPushButton *refreshModelBut;
    bool autoHideMode;
};

#endif // MODELPAGESWITCHER_H
