#include "sqlfkfielddelegate.h"

#include "customcompletionlineedit.h"

#include "imatchmodelfactory.h"
#include "isqlfkmatchmodel.h"
#include "IFKField.h"

SqlFKFieldDelegate::SqlFKFieldDelegate(IMatchModelFactory *factory, IFKField *iface, QObject *parent) :
    QStyledItemDelegate(parent),
    mIface(iface),
    mFactory(factory)
{

}

QWidget *SqlFKFieldDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    ISqlFKMatchModel *model = mFactory->createModel();
    CustomCompletionLineEdit *editor = new CustomCompletionLineEdit(model, parent);
    model->setParent(editor);
    connect(editor, &CustomCompletionLineEdit::completionDone, this, &SqlFKFieldDelegate::handleCompletionDone);
    return editor;
}

void SqlFKFieldDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    CustomCompletionLineEdit *ed = static_cast<CustomCompletionLineEdit *>(editor);
    db_id dataId = 0;
    QString name;
    if(mIface->getFieldData(index.row(), index.column(), dataId, name))
    {
        ed->setData(dataId, name);
    }
}

void SqlFKFieldDelegate::setModelData(QWidget *editor, QAbstractItemModel */*model*/, const QModelIndex &index) const
{
    CustomCompletionLineEdit *ed = static_cast<CustomCompletionLineEdit *>(editor);
    //FIXME: use also validateData() ?
    db_id dataId = 0;
    QString name;
    ed->getData(dataId, name);
    mIface->setFieldData(index.row(), index.column(), dataId, name);
}

void SqlFKFieldDelegate::handleCompletionDone(CustomCompletionLineEdit *editor)
{
    commitData(editor);
    closeEditor(editor);
}
