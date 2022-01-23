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
