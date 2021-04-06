#ifndef KMDELEGATE_H
#define KMDELEGATE_H

#include <QStyledItemDelegate>

class KmDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    KmDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setMinAndPrefix(int min, const QString& pref);

private:
    int minimum;
    QString prefix;
};

#endif // KMDELEGATE_H
