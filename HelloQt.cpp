
#include "stdafx.h"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  QTabFramework tabFramework;
  QLabel* testWidget1 = new QLabel("Test.cpp");
  testWidget1->setWindowTitle("Test.cpp");
  QLabel* testWidget2 = new QLabel("Test2.cpp");
  testWidget2->setWindowTitle("Test2.cpp");
  QLabel* testWidget3 = new QLabel("Solution Explorer");
  testWidget3->setWindowTitle("Solution Explorer");
  QLabel* testWidget4 = new QLabel("Output");
  testWidget4->setWindowTitle("Output");

  tabFramework.resize(QSize(800, 600));
  tabFramework.addTab(testWidget1);
  tabFramework.addTab(testWidget2, QTabFramework::InsertOnTop, testWidget1);
  tabFramework.addTab(testWidget3, QTabFramework::InsertLeft, testWidget1);
  tabFramework.addTab(testWidget4, QTabFramework::InsertBottom, testWidget1);

  //QMenuBar* menuBar = new QMenuBar;
  //tabFramework.setMenuBar(menuBar);
  QMenuBar* menuBar = tabFramework.menuBar();
  QMenu* menu = menuBar->addMenu("&View");
  menu->addAction(tabFramework.toggleViewAction(testWidget1));
  menu->addAction(tabFramework.toggleViewAction(testWidget2));
  menu->addAction(tabFramework.toggleViewAction(testWidget3));
  menu->addAction(tabFramework.toggleViewAction(testWidget4));



  //QMenuBar* menuBar = tabFramework.menuBar();
  //QMenu* menu = menuBar->addMenu(QObject::tr("&View"));
  //menu->addAction();


  tabFramework.show();

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
