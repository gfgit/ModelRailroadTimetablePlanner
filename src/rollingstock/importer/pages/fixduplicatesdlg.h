#ifndef FIXDUPLICATESDLG_H
#define FIXDUPLICATESDLG_H

#include <QDialog>

class IDuplicatesItemModel;
class QDialogButtonBox;
class QAbstractItemDelegate;
class QTableView;
class QProgressDialog;

class FixDuplicatesDlg : public QDialog
{
    Q_OBJECT
public:

    enum { GoBackToPrevPage = QDialog::Accepted + 1 };

    FixDuplicatesDlg(IDuplicatesItemModel *m, bool enableGoBack, QWidget *parent = nullptr);

    void setItemDelegateForColumn(int column, QAbstractItemDelegate *delegate);

    int blockingReloadCount(int mode);

public slots:
    void done(int res) override;

private slots:
    void showModelError(const QString &text);
    void handleProgress(int progress, int max);
    void handleModelState(int state);

private:
    int warnCancel(QWidget *w);

private:
    friend class ProgressDialog;
    IDuplicatesItemModel *model;
    QTableView *view;
    QDialogButtonBox *box;
    QProgressDialog *progressDlg;
    bool canGoBack;
};

#endif // FIXDUPLICATESDLG_H
