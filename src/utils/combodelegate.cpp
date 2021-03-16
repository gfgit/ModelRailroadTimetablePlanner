#include "combodelegate.h"
#include "app/scopedebug.h"

#include <QComboBox>

ComboDelegate::ComboDelegate(QStringList list, int role, QObject *parent) :
    QStyledItemDelegate(parent),
    mList(list),
    mRole(role)
{
    //DEBUG_ENTRY;
    qDebug() << mList;
}

QWidget *ComboDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    //DEBUG_ENTRY;
    QComboBox *combo = new QComboBox(parent);
    connect(combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &ComboDelegate::onItemClicked);
    combo->addItems(mList);
    return combo;
}

void ComboDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    //DEBUG_ENTRY;
    QComboBox *combo = static_cast<QComboBox*>(editor);
    QVariant v = index.model()->data(index, mRole);
    if(v.isValid())
    {
        int val = v.toInt();
        combo->setCurrentIndex(val);
    }
    else
    {
        combo->setCurrentIndex(0); //TODO??? -1 --> 0 ?
    }
    combo->showPopup();
}

void ComboDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    //DEBUG_ENTRY;
    QComboBox *combo = static_cast<QComboBox*>(editor);
    int val = combo->currentIndex();
    model->setData(index, val, mRole);
    //QString text = combo->currentText();
    //model->setData(index, text, Qt::DisplayRole);
}

void ComboDelegate::onItemClicked()
{
    QComboBox *combo = qobject_cast<QComboBox *>(sender());
    if(combo)
    {
        commitData(combo);
        closeEditor(combo);
    }
}

void ComboDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}
