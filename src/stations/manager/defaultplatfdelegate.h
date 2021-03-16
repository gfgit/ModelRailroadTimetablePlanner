#ifndef DEFAULTPLATFDELEGATE_H
#define DEFAULTPLATFDELEGATE_H

#include <QStyledItemDelegate>
#include <QSpinBox>

class PlatformSpinBox : public QSpinBox
{
public:
    PlatformSpinBox(QWidget *parent = nullptr);

protected:
    QString textFromValue(int val) const override;
    int valueFromText(const QString &text) const override;
};

class DefaultPlatfDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DefaultPlatfDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};

#endif // DEFAULTPLATFDELEGATE_H
