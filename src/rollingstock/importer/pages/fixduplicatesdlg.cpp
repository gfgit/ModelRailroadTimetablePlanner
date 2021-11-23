#include "fixduplicatesdlg.h"

#include <QTableView>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <QProgressDialog>
#include <QMessageBox>
#include "utils/owningqpointer.h"

#include "../intefaces/iduplicatesitemmodel.h"

#include "utils/model_mode.h"
#include "../rsimportstrings.h"

#include <QEventLoop>

class ProgressDialog : public QProgressDialog
{
public:
    ProgressDialog(FixDuplicatesDlg *parent) :
        QProgressDialog(parent),
        dlg(parent)
    {
        setAutoReset(false);
        //Manually handle cancel
        disconnect(this, SIGNAL(canceled()), this, SLOT(cancel()));
        connect(this, SIGNAL(canceled()), this, SLOT(reject()));
    }

    void done(int res) override
    {
        if(res != QDialog::Accepted)
        {
            res = dlg->warnCancel(this);
            if(res == QDialog::Accepted)
                return; //Give user a second chance
        }
        QDialog::done(res);
    }

private:
    FixDuplicatesDlg *dlg;
};

FixDuplicatesDlg::FixDuplicatesDlg(IDuplicatesItemModel *m, bool enableGoBack, QWidget *parent) :
    QDialog(parent),
    model(m),
    canGoBack(enableGoBack)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    QLabel *label = new QLabel(RsImportStrings::tr("The file constains some duplicates in item names wich need to be fixed in order to procced.\n"
                                                   "There also may be some items with empty name.\n"
                                                   "Please assign a custom name to them so that there are no duplicates"));
    lay->addWidget(label);

    view = new QTableView;
    //Prevent changing names by accidentally pressing a key
    view->setEditTriggers(QTableView::DoubleClicked);
    view->setModel(model);
    view->resizeColumnsToContents();
    lay->addWidget(view);


    box = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    lay->addWidget(box);

    progressDlg = new ProgressDialog(this);

    connect(model, &IDuplicatesItemModel::progressChanged, this, &FixDuplicatesDlg::handleProgress);
    connect(model, &IDuplicatesItemModel::progressFinished, progressDlg, &QProgressDialog::accept);
    connect(model, &IDuplicatesItemModel::stateChanged, this, &FixDuplicatesDlg::handleModelState);
    connect(model, &IDuplicatesItemModel::error, this, &FixDuplicatesDlg::showModelError);
    connect(progressDlg, &ProgressDialog::rejected, model, &IDuplicatesItemModel::cancelLoading);

    setWindowTitle(RsImportStrings::tr("Fix Item Names"));
    setMinimumSize(450, 200);
}

void FixDuplicatesDlg::setItemDelegateForColumn(int column, QAbstractItemDelegate *delegate)
{
    view->setItemDelegateForColumn(column, delegate);
}

void FixDuplicatesDlg::showModelError(const QString& text)
{
    QMessageBox::warning(this, RsImportStrings::tr("Invalid Operation"), text);
}

void FixDuplicatesDlg::done(int res)
{
    if(res == QDialog::Accepted)
    {
        //Check if all are fixed
        res = blockingReloadCount(IDuplicatesItemModel::LoadingData);
        if(res != QDialog::Accepted)
        {
            QDialog::done(res);
            return;
        }

        int count = model->getItemCount();
        if(count)
        {
            OwningQPointer<QMessageBox> msgBox = new QMessageBox(this);
            msgBox->setIcon(QMessageBox::Warning);
            msgBox->setWindowTitle(RsImportStrings::tr("Not yet!"));
            msgBox->setText(RsImportStrings::tr("There are still %1 items to be fixed").arg(count));
            QPushButton *okBut = msgBox->addButton(QMessageBox::Ok);
            QPushButton *backToPrevPage = nullptr;
            if(canGoBack)
                backToPrevPage = msgBox->addButton(RsImportStrings::tr("Previuos page"), QMessageBox::NoRole);
            msgBox->setDefaultButton(okBut);
            msgBox->setEscapeButton(okBut); //If dialog gets closed of Esc is pressed act ad if Ok was pressed
            msgBox->exec();

            const bool goBack = msgBox && msgBox->clickedButton() == backToPrevPage && backToPrevPage;

            if(goBack)
            {
                res = GoBackToPrevPage;
            }
            else
            {
                return; //Give user a second chance
            }
        }
    }
    else
    {
        res = warnCancel(this);
        if(res == QDialog::Accepted)
            return; //Give user a second chance
    }

    return QDialog::done(res);
}

void FixDuplicatesDlg::handleProgress(int progress, int max)
{
    progressDlg->setMaximum(max);
    progressDlg->setValue(progress);
}

void FixDuplicatesDlg::handleModelState(int state)
{
    QString label;
    switch (state)
    {
    case IDuplicatesItemModel::Loaded:
        label = tr("Loaded.");
        break;
    case IDuplicatesItemModel::Starting:
        label = tr("Starting...");
        break;
    case IDuplicatesItemModel::CountingItems:
        label = tr("Counting items...");
        break;
    case IDuplicatesItemModel::LoadingData:
        label = tr("Loading data...");
        break;
    default:
        label = tr("Unknown state.");
    }
    progressDlg->setLabelText(label);
}

int FixDuplicatesDlg::blockingReloadCount(int mode)
{
    if(!model->startLoading(mode))
        return canGoBack ? int(GoBackToPrevPage) : int(QDialog::Rejected);

    progressDlg->reset();
    progressDlg->setResult(0);
    progressDlg->setModal(true);

    QEventLoop loop;
    connect(progressDlg, &QDialog::finished, &loop, &QEventLoop::exit);
    int ret = loop.exec(QEventLoop::DialogExec);
    progressDlg->reset();
    progressDlg->hide();
    return ret;
}

int FixDuplicatesDlg::warnCancel(QWidget *w)
{
    //Warn user
    OwningQPointer<QMessageBox> msgBox = new QMessageBox(w);
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setWindowTitle(RsImportStrings::tr("Aborting RS Import"));
    msgBox->setText(RsImportStrings::tr("If you don't fix duplicated items you cannot proceed.\n"
                                        "Do you wish to Abort the process?"));
    QPushButton *abortBut = msgBox->addButton(QMessageBox::Abort);
    QPushButton *noBut = msgBox->addButton(QMessageBox::No);
    QPushButton *backToPrevPage = nullptr;
    if(canGoBack)
        backToPrevPage = msgBox->addButton(RsImportStrings::tr("Previuos page"), QMessageBox::NoRole);
    msgBox->setDefaultButton(noBut);
    msgBox->setEscapeButton(noBut);

    connect(model, &IDuplicatesItemModel::progressFinished, msgBox, &QMessageBox::accept);
    connect(model, &IDuplicatesItemModel::processAborted, msgBox, &QMessageBox::accept);
    msgBox->exec();

    if(!msgBox)
        return QDialog::Rejected;

    QAbstractButton *but = msgBox->clickedButton();
    int ret = QDialog::Accepted; //Default give second chance
    if(but == abortBut)
    {
        ret = QDialog::Rejected;
    }
    else if(but == backToPrevPage && backToPrevPage)
    {
        ret = FixDuplicatesDlg::GoBackToPrevPage;
    }

    return ret;
}
