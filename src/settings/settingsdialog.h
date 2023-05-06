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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "utils/types.h"

#include <array>

namespace Ui {
class SettingsDialog;
}

class QSpinBox;
class ColorView;
class LanguageModel;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

public slots:
    void loadSettings();
    void saveSettings();
    void restoreDefaults();

    void onRestore();

    void resetSheetLanguage();

protected:
    void closeEvent(QCloseEvent *e);

private slots:
    void onJobColorChanged();
    void onJobGraphOptionsChanged();
    void onShiftGraphOptionsChanged();

private:
    void setupLanguageBox();
    void setSheetLanguage(const QLocale& sheetLoc);

private:
    Ui::SettingsDialog *ui;
    LanguageModel *languageModel;

    bool updateJobsColors;
    bool updateJobGraphOptions;
    bool updateShiftGraphOptions;

    QSpinBox* m_timeSpinBoxArr[int(JobCategory::NCategories)];
    ColorView *m_colorViews[int(JobCategory::NCategories)];
    void setupJobColorsPage();
};

#endif // SETTINGSDIALOG_H
