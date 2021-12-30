#include "languageutils.h"

#include <QCoreApplication>
#include <QTranslator>

#include "app/session.h"

#include <QDirIterator>
#include <QVector>

#include <QDebug>

constexpr const char *mrtpTranslationName = "mrtp";
static const QLatin1String translationsFolder = QLatin1String("/translations");

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
    const QString path = qApp->applicationDirPath() + translationsFolder;
    return ::loadTranslatorInternal(loc, path, QString::fromLatin1(mrtpTranslationName));
}

bool utils::language::loadTranslationsFromSettings()
{
    const QString path = qApp->applicationDirPath() + translationsFolder;
    QLocale loc = Session->getAppLanguage();

    //NOTE: If locale is English with default country we do not need translations
    //because they are already embedded in the executable strings so skip it
    if(loc == MeetingSession::embeddedLocale)
        return true;

    QTranslator *qtTransl = ::loadTranslatorInternal(loc, path, QLatin1String("qt"));
    if(qtTransl)
    {
        QCoreApplication::installTranslator(qtTransl);
    }

    QTranslator *mrtpTransl = ::loadTranslatorInternal(loc, path, QString::fromLatin1(mrtpTranslationName));
    if(mrtpTransl)
    {
        QCoreApplication::installTranslator(mrtpTransl);
        return true;
    }

    return false;
}

QVector<QLocale> utils::language::getAvailableTranslations()
{
    QVector<QLocale> vec;

    //NOTE: First add default laguage embedded in executable strings
    vec.append(MeetingSession::embeddedLocale);

    const QString path = qApp->applicationDirPath() + translationsFolder;
    const QString filter = mrtpTranslationName + QStringLiteral("_*.qm");
    const int startIdx = qstrlen(mrtpTranslationName) + 1; //Add the '_'

    //Get all installed translation files
    QDirIterator iter(path, {filter}, QDir::Files);
    while (iter.hasNext())
    {
        iter.next();
        const QString fullName = iter.fileName();
        int lastIdx = fullName.indexOf('.');
        if(lastIdx < 0)
            continue; //Skip invalid name

        //Extract language from file name
        QString language = fullName.mid(startIdx, lastIdx - startIdx);
        QLocale loc(language);
        if(loc == QLocale::c())
            continue; //This means it didn't parse locale

        vec.append(loc);
    }

    return vec;
}
