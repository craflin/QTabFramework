
#pragma once

class QTabFramework;
class QTabContainer;

class QTabWindow : public QMainWindow
{
public:
  QTabWindow(QTabFramework* tabFramework) : tabFramework(tabFramework), overlayWidget(0), overlayWidgetTab(0) {}

private:
  QTabFramework* tabFramework;
  QWidget* overlayWidget;
  QWidget* overlayWidgetTab;

private:
  void setDropOverlayRect(const QRect& globalRect, const QRect& tabRect = QRect());
  //void removeDropOverlay();

  virtual void closeEvent(QCloseEvent* event);

  friend class QTabContainer;
  friend class QTabDrawer;
  friend class QTabFramework;
};


class QTabFramework : public QTabWindow
{
  Q_OBJECT

public:
  enum InsertPolicy
  {
    InsertFloating,
    InsertOnTop,
    Insert,
    InsertLeft,
    InsertRight,
    InsertTop,
    InsertBottom
  };

public:
  QTabFramework() : QTabWindow(this), moveTabWidget(0) {}
  ~QTabFramework();

  void addTab(QWidget* widget, InsertPolicy insertPolicy = InsertFloating, QWidget* position = 0);
  //void moveTab(QWidget* widget, InsertPolicy insertPolicy = InsertFloating, QWidget* position = 0);
  void removeTab(QWidget* widget);
  void hideTab(QWidget* widget);

  void saveLayout();
  void loadLayout();


private:

  struct TabData
  {
    bool hidden;
    QAction* action;
  };

private:
  QList<QTabWindow*> floatingWindows;
  QHash<QWidget*, TabData> tabs;

  QWidget* moveTabWidget;
  QTabContainer* moveTabPosition;
  InsertPolicy moveTabInsertPolicy;
  int moveTabTabIndex;

private slots:
  void executeMoveTab();

private:
  void addTab(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void moveTab(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void moveTabLater(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void removeContainerIfEmpty(QTabContainer* tabContainer);
  void removeWindow(QTabWindow* window);
  void hideTab(QWidget* widget, bool removeContainerIfEmpty);

  virtual void closeEvent(QCloseEvent* event);

  friend class QTabDrawer;
  friend class QTabContainer;
  friend class QTabWindow;
};

class QTabSplitter : public QSplitter
{
public:
  QTabSplitter(Qt::Orientation orientation, QWidget* parent);
};

class QTabContainer : public QTabWidget
{
public:
  QTabContainer(QWidget* parent, QTabWindow* tabWindow);

private:
  QTabWindow* tabWindow;

private:
  QRect findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QRect& tabRect, int& tabIndex);

  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragLeaveEvent(QDragLeaveEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent* event);
  virtual void dropEvent(QDropEvent* event);

  friend class QTabDrawer;
  friend class QTabFramework;
};

class QTabDrawer : public QTabBar
{
  Q_OBJECT

public:
  QTabDrawer(QTabContainer* tabContainer);

private:
  QTabContainer* tabContainer;
  int pressedIndex;
  QPoint dragStartPosition;

protected:
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);

private slots:
  void closeTab(int index);
};
