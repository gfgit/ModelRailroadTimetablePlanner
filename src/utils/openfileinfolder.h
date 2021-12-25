#ifndef OPENFILEINFOLDER_H
#define OPENFILEINFOLDER_H

#include <QString>

class QWidget;

namespace utils {

void openFileInFolder(const QString& title, const QString& filePath, QWidget *parent);

}

#endif // OPENFILEINFOLDER_H
