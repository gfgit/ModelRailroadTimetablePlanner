#include "languageutils.h"

#include <QCoreApplication>
#include <QTranslator>

#include "app/session.h"

#include <QDebug>

constexpr const char *mrtpTranslationPrefix = "mrtp";

static inline QTranslator *loadTranslatorInternal(const QLocale &loc, const QString& path, const QString& prefix)
{
    QTranslator *translator = new QTranslator(qApp);
    if(!translator->load(loc, prefix, QStringLiteral("_"), path))
    {
        qDebug() << "Cannot load translations for:" << prefix.toUpper() << loc.name() << loc.uiLanguages();
        delete translator;
        return nullptr;
    }
    return translator;
}

QTranslator *utils::language::loadAppTranslator(const QLocale &loc)
{
    const QString path = qApp->applicationDirPath() + QStringLiteral("/translations");
    return ::loadTranslatorInternal(loc, path, QString::fromLatin1(mrtpTranslationPrefix));
}

bool utils::language::loadTranslationsFromSettings()
{
    QLocale loc = AppSettings.getLanguage();
    const QString path = qApp->applicationDirPath() + QStringLiteral("/translations");

    QTranslator *qtTransl = ::loadTranslatorInternal(loc, path, QLatin1String("qt"));
    if(qtTransl)
    {
        QCoreApplication::installTranslator(qtTransl);
    }

    QTranslator *mrtpTransl = ::loadTranslatorInternal(loc, path, QString::fromLatin1(mrtpTranslationPrefix));
    if(mrtpTransl)
    {
        QCoreApplication::installTranslator(mrtpTransl);
        return true;
    }

    return false;
}
