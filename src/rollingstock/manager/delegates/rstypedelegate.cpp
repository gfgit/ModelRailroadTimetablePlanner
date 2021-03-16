#include "rstypedelegate.h"

#include "utils/types.h"
#include "utils/rs_types_names.h"

#include "utils/model_roles.h"

#include <QComboBox>

RSTypeDelegate::RSTypeDelegate(bool subType, QObject *parent) :
    QStyledItemDelegate (parent),
    m_subType(subType)
{
    QStringList list;
    if(subType)
    {
        list.reserve(int(RsEngineSubType::NTypes));
        for(int i = 0; i < int(RsEngineSubType::NTypes); i++)
            list.append(RsTypeNames::name(RsEngineSubType(i)));
    }
    else
    {
        list.reserve(int(RsType::NTypes));
        for(int i = 0; i < int(RsType::NTypes); i++)
            list.append(RsTypeNames::name(RsType(i)));
    }

    comboModel.setStringList(list);
}

QWidget *RSTypeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*options*/, const QModelIndex &/*idx*/) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->setModel(const_cast<QStringListModel*>(&comboModel));
    return combo;
}

void RSTypeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox *>(editor);
    int type = index.data(m_subType ? RS_SUB_TYPE_ROLE : RS_TYPE_ROLE).toInt();
    connect(combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &RSTypeDelegate::onItemClicked);
    combo->setCurrentIndex(type);
    combo->showPopup();
}

void RSTypeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox *>(editor);
    int type = combo->currentIndex();
    model->setData(index, type, m_subType ? RS_SUB_TYPE_ROLE : RS_TYPE_ROLE);
}

void RSTypeDelegate::onItemClicked()
{
    QComboBox *combo = qobject_cast<QComboBox *>(sender());
    if(combo)
    {
        commitData(combo);
        closeEditor(combo);
    }
}
