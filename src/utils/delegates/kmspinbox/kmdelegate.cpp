#include "kmdelegate.h"
#include "kmspinbox.h"

KmDelegate::KmDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    minimum(0)
{

}

QWidget *KmDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    KmSpinBox *spin = new KmSpinBox(parent);
    spin->setMinimum(minimum);
    spin->setPrefix(prefix);
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

void KmDelegate::setMinAndPrefix(int min, const QString &pref)
{
    minimum = min;
    prefix = pref;
}
