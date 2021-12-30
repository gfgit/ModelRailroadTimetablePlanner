#ifndef LANGUAGEMODEL_H
#define LANGUAGEMODEL_H

#include <QAbstractListModel>

#include <QLocale>
#include <QVector>

class LanguageModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit LanguageModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &p = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void loadAvailableLanguages();

    QLocale getLocaleAt(int idx);

    int findMatchingRow(const QLocale& loc);

private:
    QVector<QLocale> m_data;
};

#endif // LANGUAGEMODEL_H
