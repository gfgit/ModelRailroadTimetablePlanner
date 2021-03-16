#include "defaultplatfdelegate.h"

#include "utils/platform_utils.h"

#include "utils/model_roles.h"

#include "stations/stationssqlmodel.h"

DefaultPlatfDelegate::DefaultPlatfDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

QWidget *DefaultPlatfDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &index) const
{
    const StationsSQLModel *stModel = qobject_cast<const StationsSQLModel *>(index.model());
    if(!stModel)
        return nullptr;

    auto pair = stModel->getPlatfCountAtRow(index.row());
    if(pair.first < 0)
        return nullptr;

    PlatformSpinBox *spinBox = new PlatformSpinBox(parent);
    spinBox->setRange(-pair.second, pair.first - 1);

    return spinBox;
}

void DefaultPlatfDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    PlatformSpinBox *spinBox = static_cast<PlatformSpinBox *>(editor);
    spinBox->setValue(index.data(PLATF_ID).toInt());
}

void DefaultPlatfDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    PlatformSpinBox *spinBox = static_cast<PlatformSpinBox *>(editor);
    model->setData(index, spinBox->value(), Qt::EditRole);
}

PlatformSpinBox::PlatformSpinBox(QWidget *parent) :
    QSpinBox(parent)
{

}

QString PlatformSpinBox::textFromValue(int val) const
{
    return utils::shortPlatformName(val);
}

int PlatformSpinBox::valueFromText(const QString &text) const
{
    int space = text.indexOf(' ');
    if(space == -1)
    {
        bool ok = false;
        int platf = text.toInt(&ok);
        if(platf > 0)
            platf--; //Convert main platf to zero-based
        return ok ? platf : value();
    }

    bool ok = false;
    int platf = QStringRef(&text, 0, space).toInt(&ok);
    if(!ok)
        return value();

    if(QStringRef(&text, space, text.size() - space).startsWith('D', Qt::CaseInsensitive))
        platf = -platf; //Convert to depot
    else
        platf = qMax(0, platf - 1); //Convert main platf to zero-based

    return platf;
}
