/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef REF_TYPE_H
#define REF_TYPE_H

#include <type_traits>

#include <QVariant>
#include <QColor>
#include <QLocale>

namespace utils {

template <typename T, typename T2=void> struct const_ref_t
{
    typedef const T& Type;
};

//Don't pass arithmetic types as reference
template <typename T>
struct const_ref_t<T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
    typedef T Type;
};

//Specialize QColor and QLocale so they are more legible in the .ini settigs file, instead of Qt own's representation

template <typename T>
inline QVariant toVariant(typename const_ref_t<T>::Type val)
{
    return QVariant(val);
}

template <>
inline QVariant toVariant<QColor>(typename const_ref_t<QColor>::Type val)
{
    return QVariant(val.name(QColor::HexRgb));
}

template <>
inline QVariant toVariant<QLocale>(typename const_ref_t<QLocale>::Type val)
{
    return QVariant(val.name());
}

template <typename T>
inline T fromVariant(const QVariant& v)
{
    return v.value<T>();
}

template <>
inline QColor fromVariant<QColor>(const QVariant& v)
{
    return QColor(v.toString());
}

template <>
inline QLocale fromVariant<QLocale>(const QVariant& v)
{
    return QLocale(v.toString());
}

} // namespace utils

#endif // REF_TYPE_H
