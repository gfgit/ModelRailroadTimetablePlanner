#include "rsnumberdelegate.h"

#include "utils/model_roles.h"

#include "rsnumspinbox.h"

//TODO: remove
RsNumberDelegate::RsNumberDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

QWidget *RsNumberDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &index) const
{
    QSpinBox *ed = nullptr;
    QVariant v = index.data(RS_IS_ENGINE);
    if(!v.toBool())
    {
        //Custom spinbox for Wagons 'XXX-X'
        ed = new RsNumSpinBox(parent);
    }
    else
    {
        //Normal spinbox for Engines
        ed = new QSpinBox(parent);
        ed->setRange(0, 9999);
    }

    return ed;
}

void RsNumberDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QSpinBox *ed = static_cast<QSpinBox *>(editor);
    QVariant v = index.data(RS_NUMBER);
    int val = 0;
    if(v.isValid())
    {
        val = v.toInt();
    }

    ed->setValue(val);
}

void RsNumberDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSpinBox *ed = static_cast<QSpinBox *>(editor);
    model->setData(index, ed->value(), RS_NUMBER);
}

void RsNumberDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}
