#ifndef SQLFKFIELDDELEGATE_H
#define SQLFKFIELDDELEGATE_H

#include <QStyledItemDelegate>

class IFKField;
class IMatchModelFactory;
class CustomCompletionLineEdit;

class SqlFKFieldDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    SqlFKFieldDelegate(IMatchModelFactory *factory, IFKField *iface, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private slots:
    void handleCompletionDone(CustomCompletionLineEdit *editor);

private:
    IFKField *mIface;
    IMatchModelFactory *mFactory;
};

#endif // SQLFKFIELDDELEGATE_H
