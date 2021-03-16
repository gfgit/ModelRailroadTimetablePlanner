#ifndef ITEMSELECTIONPAGE_H
#define ITEMSELECTIONPAGE_H

#include <QWizardPage>

#include "utils/model_mode.h"

class RSImportWizard;
class QTableView;
class QLabel;

class IRsImportModel;
class IFKField;
class QItemEditorFactory;

class ItemSelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    ItemSelectionPage(RSImportWizard *w, IRsImportModel *m, QItemEditorFactory *edFactory, IFKField *ifaceDelegate, int delegateCol, ModelModes::Mode mode, QWidget *parent = nullptr);

    virtual void initializePage() override;
    virtual void cleanupPage() override;
    virtual bool validatePage() override;

private slots:
    void updateStatus();
    void sectionClicked(int col);
    void showModelError(const QString &text);

private:
    RSImportWizard *mWizard;
    IRsImportModel *model;
    QItemEditorFactory *editorFactory;

    QTableView *view;
    QLabel *statusLabel;

    ModelModes::Mode m_mode;
};

#endif // ITEMSELECTIONPAGE_H
