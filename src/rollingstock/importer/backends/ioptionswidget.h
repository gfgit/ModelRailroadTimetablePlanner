#ifndef IOPTIONSWIDGET_H
#define IOPTIONSWIDGET_H

#include <QWidget>

#include <QMap>

class IOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IOptionsWidget(QWidget *parent = nullptr);

    virtual void loadSettings(const QMap<QString, QVariant> &settings) = 0;
    virtual void saveSettings(QMap<QString, QVariant> &settings) = 0;

    virtual void getFileDialogOptions(QString &title, QStringList &fileFormats) = 0;
};

#endif // IOPTIONSWIDGET_H
