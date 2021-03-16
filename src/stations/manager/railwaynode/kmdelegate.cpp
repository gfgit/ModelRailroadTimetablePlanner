#include "kmdelegate.h"
#include "kmspinbox.h"

KmDelegate::KmDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

QWidget *KmDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    KmSpinBox *spin = new KmSpinBox(parent);
    return spin;
}

void KmDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KmSpinBox *spin = static_cast<KmSpinBox *>(editor);
    spin->setValue(index.data(Qt::EditRole).toInt());
}

void KmDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KmSpinBox *spin = static_cast<KmSpinBox *>(editor);
    model->setData(index, spin->value(), Qt::EditRole);
}

void KmDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}
