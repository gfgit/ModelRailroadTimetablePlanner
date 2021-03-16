#include "optionsmodel.h"

#include "utils/file_format_names.h"

OptionsModel::OptionsModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

int OptionsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(RSImportWizard::ImportSource::NSources);
}

QVariant OptionsModel::data(const QModelIndex &idx, int role) const
{
    if(role != Qt::DisplayRole || idx.row() < 0 || idx.row() >= int(RSImportWizard::ImportSource::NSources) || idx.column() != 0)
        return QVariant();

    RSImportWizard::ImportSource source = RSImportWizard::ImportSource(idx.row());
    switch (source)
    {
    case RSImportWizard::ImportSource::OdsImport:
        return FileFormats::tr(FileFormats::odsFormat);
    case RSImportWizard::ImportSource::SQLiteImport:
        return FileFormats::tr(FileFormats::tttFormat);
    default:
        break;
    }
    return QVariant();
}
