
#include "stdafx.h"

class MainWindow : public QTabFramework
{
public:
  MainWindow() : settings(QSettings::IniFormat, QSettings::UserScope, "QTabFrameworkTest", "QTabFrameworkTest")
  {
    resize(QSize(800, 600));

    QLineEdit* testWidget1 = new QLineEdit("Test.cpp");
    testWidget1->setWindowTitle(tr("Test.cpp"));
    QLineEdit* testWidget2 = new QLineEdit("Test2.cpp");
    testWidget2->setWindowTitle(tr("Test2.cpp"));
    QLineEdit* testWidget3 = new QLineEdit("Solution Explorer");
    testWidget3->setWindowTitle(tr("Solution Explorer"));
    QLineEdit* testWidget4 = new QLineEdit("Output");
    testWidget4->setWindowTitle(tr("Output"));

    addTab(testWidget1);
    addTab(testWidget2, QTabFramework::InsertOnTop, testWidget1);
    addTab(testWidget3, QTabFramework::InsertLeft, testWidget1);
    addTab(testWidget4, QTabFramework::InsertBottom, testWidget1);

    QMenuBar* menuBar = this->menuBar();
    QMenu* menu = menuBar->addMenu(tr("&View"));
    menu->addAction(toggleViewAction(testWidget1));
    menu->addAction(toggleViewAction(testWidget2));
    menu->addAction(toggleViewAction(testWidget3));
    menu->addAction(toggleViewAction(testWidget4));

    restoreLayout(settings.value("layout").toByteArray());
  }

private:
  void closeEvent(QCloseEvent* event)
  {
    settings.setValue("layout", saveLayout());

    QTabFramework::closeEvent(event);
  }

private:
  QSettings settings;
};

int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  MainWindow mainWindow;
  mainWindow.show();
  return app.exec();
}
