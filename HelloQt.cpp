
#include "stdafx.h"

int main(int argc, char **argv)
{
  QApplication app (argc, argv);

  QMainWindow mainWindow;
  mainWindow.setCentralWidget(new QLabel("HelloQt"));
  mainWindow.show();

  return app.exec();
}
