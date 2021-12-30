#ifndef LANGUAGEUTILS_H
#define LANGUAGEUTILS_H

class QTranslator;
class QLocale;

namespace utils {

namespace language {

QTranslator *loadAppTranslator(const QLocale& loc);

bool loadTranslationsFromSettings();

} // namespace language

} // namespace utils

#endif // LANGUAGEUTILS_H
