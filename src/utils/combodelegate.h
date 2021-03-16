#ifndef COMBODELEGATE_H
#define COMBODELEGATE_H

#include <QStyledItemDelegate>
#include <QStringList>

class ComboDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ComboDelegate(QStringList list, int role, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private slots:
    void onItemClicked();

private:
    QStringList mList;
    int mRole;
};

#endif // COMBODELEGATE_H
