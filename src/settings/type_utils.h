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
