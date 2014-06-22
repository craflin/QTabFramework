
#pragma once

class QTabFramework;
class QTabContainer;

class QTabWindow : public QMainWindow
{
  Q_OBJECT

public:
  QTabWindow(QTabFramework* tabFramework);

protected:
  virtual void closeEvent(QCloseEvent* event);

private:
  enum LayoutType
  {
    NoneType,
    WindowType,
    SplitterType,
    ContainerType,
    TabType,
  };

private:
  QTabFramework* tabFramework;
  QWidget* overlayWidget;
  QWidget* overlayWidgetTab;
  QWidget* focusTab;

private slots:
  void hideCurrent();

private:
  void setDropOverlayRect(const QRect& globalRect, const QRect& tabRect = QRect());
  void writeLayout(QDataStream& stream);
  void readLayout(QDataStream& stream);
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
  bool isVisible(QWidget* widget) const;

  QAction* toggleViewAction(QWidget* widget);

  void restoreLayout(const QByteArray& layout);
  QByteArray saveLayout();

protected:
  virtual void closeEvent(QCloseEvent* event);
  virtual void showEvent(QShowEvent* event);
  virtual bool eventFilter(QObject* obj, QEvent* event);

private:
  struct TabData
  {
    QWidget* widget;
    QString objectName;
    bool hidden;
    QAction* action;
  };

private:
  QList<QTabWindow*> floatingWindows;
  QList<QTabWindow*> floatingWindowsZOrder;
  QHash<QWidget*, TabData> tabs;
  QHash<QString, TabData*> tabsByName;
  QSignalMapper signalMapper;

  QWidget* moveTabWidget;
  QTabContainer* moveTabPosition;
  InsertPolicy moveTabInsertPolicy;
  int moveTabTabIndex;

private slots:
  void executeMoveTab();
  void toggleVisibility(QWidget* widget);
  void showFloatingWindows();
  void handleFocusChanged(QWidget* old, QWidget* now);

private:
  void addTab(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void moveTab(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void moveTabLater(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex);
  void removeContainerIfEmpty(QTabContainer* tabContainer);
  QTabWindow* createWindow();
  void removeWindow(QTabWindow* window);
  void hideTab(QWidget* widget, bool removeContainerIfEmpty);
  QString tabObjectName(QWidget* widget);
  void unhideTab(const QString& objectName, QTabContainer* position);

  friend class QTabDrawer;
  friend class QTabContainer;
  friend class QTabWindow;
};

class QTabSplitter : public QSplitter
{
public:
  QTabSplitter(Qt::Orientation orientation, QWidget* parent, QTabWindow* tabWindow);

private:
  QTabWindow* tabWindow;

private:
  void writeLayout(QDataStream& stream);
  void readLayout(QDataStream& stream);

  friend class QTabWindow;
};

class QTabContainer : public QTabWidget
{
  Q_OBJECT

public:
  QTabContainer(QWidget* parent, QTabWindow* tabWindow);

  int addTab(QWidget* widget, const QString& label);
  int insertTab(int index, QWidget* widget, const QString& label);
  void removeTab(int index);

protected:
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragLeaveEvent(QDragLeaveEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent* event);
  virtual void dropEvent(QDropEvent* event);

private:
  QTabWindow* tabWindow;

private slots:
  void handleCurrentChanged(int index);

private:
  QRect findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QRect& tabRect, int& tabIndex);

  void writeLayout(QDataStream& stream);
  void readLayout(QDataStream& stream);

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
