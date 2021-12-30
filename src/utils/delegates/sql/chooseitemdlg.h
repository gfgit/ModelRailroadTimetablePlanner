#ifndef CHOOSEITEMDLG_H
#define CHOOSEITEMDLG_H

#include <QDialog>

#include "utils/types.h"

#include <functional>

class QDialogButtonBox;
class QLabel;
class CustomCompletionLineEdit;
class ISqlFKMatchModel;

class ChooseItemDlg : public QDialog
{
    Q_OBJECT
public:
    typedef std::function<bool(db_id, QString&)> Callback;

    ChooseItemDlg(ISqlFKMatchModel *matchModel, QWidget *parent);

    void setDescription(const QString& text);
    void setPlaceholder(const QString& text);

    void setCallback(const Callback &callback);

    inline db_id getItemId() const { return itemId; }

public slots:
    void done(int res) override;

private slots:
    void itemChosen(db_id id);

private:
    QLabel *label;
    CustomCompletionLineEdit *lineEdit;
    QDialogButtonBox *buttonBox;
    db_id itemId;
    Callback m_callback;
};

#endif // CHOOSEITEMDLG_H
