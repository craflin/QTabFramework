
#pragma once

class QTabFramework;
class QTabContainer;

class QTabWindow : public QMainWindow
{
  Q_OBJECT

public:
  QTabWindow(QTabFramework* tabFramework);

signals:
  void activated();

protected:
  virtual void closeEvent(QCloseEvent* event);
  virtual void changeEvent(QEvent* event);

private:
  enum LayoutType
  {
    WindowType,
    SplitterType,
    ContainerType,
  };

private:
  QTabFramework* tabFramework;
  QWidget* overlayWidget;
  QWidget* overlayWidgetTab;

private slots:
  void hideCurrent();

private:
  void setDropOverlayRect(const QRect& globalRect, const QRect& tabRect = QRect());
  void writeLayout(QByteArray& buffer);
  void hideAllTabs();

  friend class QTabContainer;
  friend class QTabDrawer;
  friend class QTabFramework;
  friend class QTabSplitter;
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
  QTabFramework();
  ~QTabFramework();

  void addTab(QWidget* widget, InsertPolicy insertPolicy = InsertFloating, QWidget* position = 0);
  void moveTab(QWidget* widget, InsertPolicy insertPolicy = InsertFloating, QWidget* position = 0);
  void removeTab(QWidget* widget);
  void hideTab(QWidget* widget);

  QAction* toggleViewAction(QWidget* widget);

  void restoreLayout(const QByteArray& layout);
  QByteArray saveLayout();

protected:
  virtual void closeEvent(QCloseEvent* event);

private:
  struct TabData
  {
    QString objectName;
    bool hidden;
    QAction* action;
  };

private:
  QList<QTabWindow*> floatingWindows;
  QHash<QWidget*, TabData> tabs;
  QHash<QString, QWidget*> tabsByName;
  QSignalMapper signalMapper;
  QSignalMapper activatedSignalMapper;

  QWidget* moveTabWidget;
  QTabContainer* moveTabPosition;
  InsertPolicy moveTabInsertPolicy;
  int moveTabTabIndex;

private slots:
  void executeMoveTab();
  void toggleVisibility(QWidget* widget);
  void updateWindowZOrder(QWidget* widget);

private:
  void addTab(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void moveTab(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void moveTabLater(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void removeContainerIfEmpty(QTabContainer* tabContainer);
  void removeWindow(QTabWindow* window);
  void hideTab(QWidget* widget, bool removeContainerIfEmpty);

  friend class QTabDrawer;
  friend class QTabContainer;
  friend class QTabWindow;
};

class QTabSplitter : public QSplitter
{
public:
  QTabSplitter(Qt::Orientation orientation, QWidget* parent);

private:
  void writeLayout(QByteArray& buffer);

  friend class QTabWindow;
};

class QTabContainer : public QTabWidget
{
public:
  QTabContainer(QWidget* parent, QTabWindow* tabWindow);

protected:
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragLeaveEvent(QDragLeaveEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent* event);
  virtual void dropEvent(QDropEvent* event);

private:
  QTabWindow* tabWindow;

private:
  QRect findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QRect& tabRect, int& tabIndex);

  void writeLayout(QByteArray& buffer);

  friend class QTabDrawer;
  friend class QTabFramework;
  friend class QTabWindow;
  friend class QTabSplitter;
};

class QTabDrawer : public QTabBar
{
  Q_OBJECT

public:
  QTabDrawer(QTabContainer* tabContainer);

protected:
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);

private:
  QTabContainer* tabContainer;
  int pressedIndex;
  QPoint dragStartPosition;

private slots:
  void closeTab(int index);
};
