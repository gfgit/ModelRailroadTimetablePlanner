#ifndef OWNINGQPOINTER_H
#define OWNINGQPOINTER_H

#include <QPointer>

/*!
 * QPointer tracks QObject instances and resets itself if they are deleted
 * Meaning you will not get a dangling pointer.
 * OwningQPointer adds ownership of tracked object by deleting it if still valid
 * when the pointer goes out of scope
 */
template <typename T>
class OwningQPointer : public QPointer<T>
{
public:
    inline OwningQPointer(T *p) : QPointer<T>(p) { }
    ~OwningQPointer();
};

template<typename T>
OwningQPointer<T>::~OwningQPointer()
{
    if(this->data())
        delete this->data();
}

#endif // OWNINGQPOINTER_H
