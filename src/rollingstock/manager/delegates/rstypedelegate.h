#ifndef RSTYPEDELEGATE_H
#define RSTYPEDELEGATE_H

#include <QStyledItemDelegate>
#include <QStringListModel>

class RSTypeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit RSTypeDelegate(bool subType, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &options, const QModelIndex &idx) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private slots:
    void onItemClicked();

private:
    QStringListModel comboModel;
    bool m_subType;
};

#endif // RSTYPEDELEGATE_H
