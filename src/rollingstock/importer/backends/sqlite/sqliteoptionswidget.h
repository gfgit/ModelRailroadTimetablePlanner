#ifndef SQLITEOPTIONSWIDGET_H
#define SQLITEOPTIONSWIDGET_H

#include "../ioptionswidget.h"

class SQLiteOptionsWidget : public IOptionsWidget
{
    Q_OBJECT
public:
    SQLiteOptionsWidget(QWidget *parent);

    void loadSettings(const QMap<QString, QVariant> &settings) override;
    void saveSettings(QMap<QString, QVariant> &settings) override;
};

#endif // SQLITEOPTIONSWIDGET_H
