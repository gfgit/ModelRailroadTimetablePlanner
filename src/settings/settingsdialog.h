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
    void setSheetLanguage(const QLocale& appLoc, const QLocale& sheetLoc);

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
