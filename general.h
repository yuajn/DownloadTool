#ifndef GENERAL_H
#define GENERAL_H

#include <QDebug>
#define qOut                                         \
  qDebug() << "[" __FILE__ << ":" << __LINE__ << "]" \
           << "->"

#endif  // GENERAL_H
