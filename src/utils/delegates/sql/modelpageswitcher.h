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
