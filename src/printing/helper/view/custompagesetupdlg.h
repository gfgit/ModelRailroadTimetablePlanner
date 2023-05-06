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

#ifndef CUSTOMPAGESETUPDLG_H
#define CUSTOMPAGESETUPDLG_H

#include <QDialog>
#include <QPageLayout>

class QComboBox;
class QRadioButton;

/*!
 * \brief The CustomPageSetupDlg class
 *
 * Helper dialog to choose page layout for printing to PDF
 * This is needed because QPageSetupDialog works only on
 * native printers.
 *
 * Qt::Vertical is Portrait, Horizontal is Landscape
 */
class CustomPageSetupDlg : public QDialog
{
    Q_OBJECT
public:
    explicit CustomPageSetupDlg(QWidget *parent = nullptr);

    void setPageSize(const QPageSize& pageSz);
    inline QPageSize getPageSize() const { return m_pageSize; }

    void setPageOrient(QPageLayout::Orientation orient);
    inline QPageLayout::Orientation getPageOrient() const { return m_pageOrient; }

private slots:
    void onPageComboActivated(int idx);
    void onPagePortraitToggled(bool val);

private:
    QComboBox *pageSizeCombo;
    QRadioButton *portraitRadioBut;
    QRadioButton *landscapeRadioBut;

    QPageSize m_pageSize;
    QPageLayout::Orientation m_pageOrient;
};

#endif // CUSTOMPAGESETUPDLG_H
