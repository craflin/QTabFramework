
#include "stdafx.h"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

   QTabFramework tabFramework;
   QLabel* firstWidget = new QLabel("Test.cpp");
   tabFramework.addTab("Test.cpp", firstWidget);
   tabFramework.addTab("Test2.cpp", new QLabel("Test2.cpp"), QTabFramework::InsertOnTop, firstWidget);
   tabFramework.addTab("Solution Explorer", new QLabel("Solution Explorer"), QTabFramework::InsertLeft, firstWidget);
   tabFramework.addTab("Output", new QLabel("Output"), QTabFramework::InsertBottom, firstWidget);

   //tabFramework.addTab("Test3.cpp", new QLabel("Test3.cpp"));

  //QMainWindow mainWindow;
  //QTabBar* tabBar = new QTabBar();
  //tabBar->addTab("tab hjk l  f hsdf dgfgdfgd jj k");
  //tabBar->addTab("tab das a asdhsa a a ");
  //tabBar->addTab("xxx tab das a asdhsa a a ");
  //tabBar->setMovable(true);
  //tabBar->setTabsClosable(true);
  //tabBar->setUsesScrollButtons(true);
  //tabBar->setElideMode(Qt::ElideRight);
  //mainWindow.setCentralWidget(tabBar);
  //mainWindow.show();
  //mainWindow.resize(200, 40);

  return app.exec();
}
