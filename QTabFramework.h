
#pragma once

class QTabWindow;
class QTabContainer;

class QTabFramework
{
public:
  enum InsertPolicy
  {
    InsertFloating,
    InsertOnTop,
    InsertLeft,
    InsertRight,
    InsertTop,
    InsertBottom
  };

public:
  ~QTabFramework();

  void addTab(const QString& title, QWidget* widget, InsertPolicy insertPolicy = InsertFloating, QWidget* position = 0);
  void moveTab(QWidget* widget, InsertPolicy insertPolicy = InsertFloating, QWidget* position = 0);
  void removeTab(QWidget* widget);

  void saveLayout();
  void loadLayout();

private:
  QList<QTabWindow*> windows;

private:
  void addTab(const QString& title, QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy);
  void moveTab(const QString& title, QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy);
  void removeWindow(QTabWindow* window);

  friend class QTabDrawer;
};

class QTabSplitter : public QSplitter
{
public:
  QTabSplitter(Qt::Orientation orientation, QWidget* parent);

private:
  QRect findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QTabContainer*& position);

  friend class QTabWindow;
};

class QTabWindow : public QMainWindow
{
public:
  QTabWindow(QTabFramework* tabFramework) : tabFramework(tabFramework), overlayWidget(0) {}

private:
  QTabFramework* tabFramework;
  QWidget* overlayWidget;

private:
  QRect findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QTabContainer*& position);
  void setDropOverlayRect(const QRect& globalRect);

  friend class QTabContainer;
  friend class QTabDrawer;
  friend class QTabFramework;
};

class QTabContainer : public QTabWidget
{
public:
  QTabContainer(QWidget* parent, QTabWindow* tabWindow);

private:
  QTabWindow* tabWindow;

private:
  QRect findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy);

  friend class QTabDrawer;
  friend class QTabFramework;
  friend class QTabWindow;
  friend class QTabSplitter;
};

class QTabDrawer : public QTabBar
{
public:
  QTabDrawer(QTabContainer* tabContainer);

private:
  QTabContainer* tabContainer;
  int pressedIndex;
  bool dragInProgress;
  QPoint dragStartPosition;
  QWidget* dragWidget;

private:
  QRect findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QTabContainer*& position);

protected:
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
};
