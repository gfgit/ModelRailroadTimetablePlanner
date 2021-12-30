#include "sqliteoptionswidget.h"

#include "utils/files/file_format_names.h"

#include <QVBoxLayout>
#include <QLabel>

SQLiteOptionsWidget::SQLiteOptionsWidget(QWidget *parent) :
    IOptionsWidget(parent)
{
    //SQLite Option
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel(tr("Import rollingstock pieces, models and owners from another MRTPlanner session file.\n"
                                 "The file must be a valid MRTPlanner session of recent version\n"
                                 "Extension: (*.ttt)")));
    lay->setAlignment(Qt::AlignTop | Qt::AlignRight);
}

void SQLiteOptionsWidget::loadSettings(const QMap<QString, QVariant> &/*settings*/)
{

}

void SQLiteOptionsWidget::saveSettings(QMap<QString, QVariant> &/*settings*/)
{

}

void SQLiteOptionsWidget::getFileDialogOptions(QString &title, QStringList &fileFormats)
{
    title = tr("Open Session");

    fileFormats.reserve(3);
    fileFormats << FileFormats::tr(FileFormats::tttFormat);
    fileFormats << FileFormats::tr(FileFormats::sqliteFormat);
    fileFormats << FileFormats::tr(FileFormats::allFiles);
}
