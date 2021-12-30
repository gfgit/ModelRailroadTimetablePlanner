#ifndef LANGUAGEUTILS_H
#define LANGUAGEUTILS_H

class QTranslator;
class QLocale;

template<typename T>
class QVector;

namespace utils {

namespace language {

QTranslator *loadAppTranslator(const QLocale& loc);

bool loadTranslationsFromSettings();

QVector<QLocale> getAvailableTranslations();

} // namespace language

} // namespace utils

#endif // LANGUAGEUTILS_H
