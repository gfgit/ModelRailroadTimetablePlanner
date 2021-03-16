#ifndef KMUTILS_H
#define KMUTILS_H

#include <QString>

namespace utils {

/* Convinence functions:
 * Km in railways are expressed with fixed decimal point
 * Decimal separator: '+'
 * Deciaml digits: 3 (meters)
 * Format: X+XXX
 *
 * Example: 15.75 km -> KM 15+750
 */
QString kmNumToText(int kmInMeters);
int kmNumFromTextInMeters(const QString& str);

} //namespace utils

#endif // KMUTILS_H
