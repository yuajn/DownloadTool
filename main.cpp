#include "downloadtool.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  DownloadTool w;
  w.show();
  return a.exec();
}
