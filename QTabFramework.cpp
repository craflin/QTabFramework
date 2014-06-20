
#include "stdafx.h"

class QTabDropOverlay : public QWidget
{
public:
  QTabDropOverlay(QWidget* parent) : QWidget(parent)
  {
    setPalette(Qt::transparent);
    setAttribute(Qt::WA_TransparentForMouseEvents);
  }

private:
  void paintEvent(QPaintEvent *event)
  {
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(88, 88, 88, 88)));
    painter.drawRect(rect());
  }
};

QTabDrawer::QTabDrawer(QTabContainer* tabContainer) : QTabBar(tabContainer), tabContainer(tabContainer), pressedIndex(-1)
{
  //setMovable(true);
  setTabsClosable(true);
  setUsesScrollButtons(true);
  setElideMode(Qt::ElideRight);
  connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
}

void QTabDrawer::mousePressEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
  {
    event->ignore();
    return;
  }

  pressedIndex = tabAt(event->pos());
  if(pressedIndex >= 0)
  {
    dragStartPosition = event->pos();
  }

  QTabBar::mousePressEvent(event);
}

void QTabDrawer::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
  {
      event->ignore();
      return;
  }

  QTabBar::mouseReleaseEvent(event);
}

void QTabDrawer::mouseMoveEvent(QMouseEvent* event)
{
  // Start drag
  if(pressedIndex >= 0)
  {
    if ((event->pos() - dragStartPosition).manhattanLength() > QApplication::startDragDistance())
    {
      QWidget* dragWidget = tabContainer->widget(pressedIndex);

      QMimeData* mimeData = new QMimeData;
      mimeData->setData("application/x-tabwidget", QByteArray());

      QDrag* drag = new QDrag(dragWidget);
      drag->setMimeData(mimeData);

      QRect tabRect = this->tabRect(pressedIndex);
      QPixmap pixmap = QPixmap::grabWidget(this, tabRect);
      drag->setPixmap(pixmap);
      drag->setHotSpot(QPoint(event->pos().x() - tabRect.x(), event->pos().y() - tabRect.y()));
      //drag->setHotSpot(QPoint(dragStartPosition.x() - tabRect.x(), dragStartPosition.y() - tabRect.y()));

      Qt::DropAction ret = drag->exec(Qt::MoveAction);
      if(ret != Qt::MoveAction)
        tabContainer->tabWindow->tabFramework->moveTabLater(dragWidget, 0, QTabFramework::InsertFloating, -1);

      pressedIndex = -1;
      return;
    }
  }
  else
    QTabBar::mouseMoveEvent(event);
}

void QTabDrawer::closeTab(int index)
{
  QWidget* widget = tabContainer->widget(index);
  tabContainer->tabWindow->tabFramework->hideTab(widget);
}

QTabSplitter::QTabSplitter(Qt::Orientation orientation, QWidget* parent) : QSplitter(orientation, parent)
{
  setChildrenCollapsible(false);
}

QTabContainer::QTabContainer(QWidget* parent, QTabWindow* tabWindow) : QTabWidget(parent), tabWindow(tabWindow)
{
  setTabBar(new QTabDrawer(this));
  setDocumentMode(true);
  setAcceptDrops(true);
}

QRect QTabContainer::findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QRect& tabRectResult, int& tabIndex)
{
  QPoint pos = mapFromGlobal(globalPos);
  QRect containerRect = rect();
  QRect result;
  tabIndex = -1;
  if(containerRect.contains(pos))
  {
    if(count() == 0)
    {
      insertPolicy = QTabFramework::InsertOnTop;
      result = containerRect;
    }
    else if(tabBar()->geometry().contains(pos))
    {
      insertPolicy = QTabFramework::Insert;
      result = containerRect;
      QTabBar* tabBar = this->tabBar();
      for(int i = 0, count = tabBar->count(); i < count; ++i)
      {
        QRect tabRect = tabBar->tabRect(i);
        if(tabRect.contains(pos))
        {
          tabRectResult = tabRect;
          tabRectResult.setRight(tabRect.left() + tabRect.width() / 2);
          tabRectResult.translate(tabBar->mapToGlobal(QPoint(0, 0)));
          tabIndex = i;
          break;
        }
      }
    }
    else if(pos.x() < containerRect.x() + containerRect.width() / 3)
    {
      insertPolicy = QTabFramework::InsertLeft;
      result = QRect(containerRect.topLeft(), QPoint(containerRect.x() + containerRect.width() / 3, containerRect.bottom()));
    }
    else if(pos.x() >= containerRect.x() + containerRect.width() * 2 / 3)
    {
      insertPolicy = QTabFramework::InsertRight;
      result = QRect(QPoint(containerRect.x() + containerRect.width() * 2 / 3, containerRect.y()), containerRect.bottomRight());
    }
    else if(pos.y() < containerRect.y() + containerRect.height() / 3)
    {
      insertPolicy = QTabFramework::InsertTop;
      result = QRect(containerRect.topLeft(), QPoint(containerRect.right(), containerRect.y() + containerRect.height() / 3));
    }
    else if(pos.y() >= containerRect.y() + containerRect.height() * 2 / 3)
    {
      insertPolicy = QTabFramework::InsertBottom;
      result = QRect(QPoint(containerRect.x(), containerRect.y() + containerRect.height() * 2 / 3), containerRect.bottomRight());
    }
    else
    {
      insertPolicy = QTabFramework::InsertOnTop;
      result = containerRect;
    }
  }
  else
  {
    insertPolicy = QTabFramework::InsertFloating;
    return QRect();
  }
  result.translate(mapToGlobal(QPoint(0, 0)));
  return result;
}

void QTabContainer::dragEnterEvent(QDragEnterEvent* event)
{
  if(event->mimeData()->hasFormat("application/x-tabwidget"))
  {
    QTabContainer* sourceTabContainer = dynamic_cast<QTabContainer*>(event->source()->parent()->parent());
    QTabFramework::InsertPolicy insertPolicy;
    QRect tabRect;
    int tabIndex;
    QRect rect = findDropRect(mapToGlobal(event->pos()), insertPolicy, tabRect, tabIndex);
    if(sourceTabContainer == this && sourceTabContainer->count() == 1)
      tabRect = QRect();
    if(sourceTabContainer == this && (insertPolicy == QTabFramework::InsertOnTop || (sourceTabContainer->count() == 1 && insertPolicy != QTabFramework::Insert)) && !tabRect.isValid())
    {
      tabWindow->setDropOverlayRect(QRect());
      event->acceptProposedAction();
    }
    else
    {
      tabWindow->setDropOverlayRect(rect, tabRect);
      event->acceptProposedAction();
    }
    return;
  }
  event->ignore();
}

void QTabContainer::dragLeaveEvent(QDragLeaveEvent* event)
{
  tabWindow->setDropOverlayRect(QRect());
}

void QTabContainer::dragMoveEvent(QDragMoveEvent* event)
{
  if(event->mimeData()->hasFormat("application/x-tabwidget"))
  {
    QTabContainer* sourceTabContainer = dynamic_cast<QTabContainer*>(event->source()->parent()->parent());
    QTabFramework::InsertPolicy insertPolicy;
    QRect tabRect;
    int tabIndex;
    QRect rect = findDropRect(mapToGlobal(event->pos()), insertPolicy, tabRect, tabIndex);
    if(sourceTabContainer == this && sourceTabContainer->count() == 1)
      tabRect = QRect();
    if(sourceTabContainer == this && (insertPolicy == QTabFramework::InsertOnTop || (sourceTabContainer->count() == 1 && insertPolicy != QTabFramework::Insert)) && !tabRect.isValid())
    {
      tabWindow->setDropOverlayRect(QRect());
      event->acceptProposedAction();
    }
    else
    {
      tabWindow->setDropOverlayRect(rect, tabRect);
      event->acceptProposedAction();
    }
    return;
  }
  event->ignore();
}

void QTabContainer::dropEvent(QDropEvent* event)
{
  tabWindow->setDropOverlayRect(QRect());

  if(event->mimeData()->hasFormat("application/x-tabwidget"))
  {
    QTabContainer* sourceTabContainer = dynamic_cast<QTabContainer*>(event->source()->parent()->parent());
    QTabFramework::InsertPolicy insertPolicy;
    QRect tabRect;
    int tabIndex;
    QRect rect = findDropRect(mapToGlobal(event->pos()), insertPolicy, tabRect, tabIndex);
    if(sourceTabContainer == this && sourceTabContainer->count() == 1)
      tabRect = QRect();
    if(sourceTabContainer == this && (insertPolicy == QTabFramework::InsertOnTop || (sourceTabContainer->count() == 1  && insertPolicy != QTabFramework::Insert)) && !tabRect.isValid())
    {
      tabWindow->tabFramework->moveTabLater(event->source(), this, QTabFramework::InsertFloating, tabIndex);
      event->acceptProposedAction();
    }
    else
    {
      if(sourceTabContainer == this && tabIndex > indexOf(event->source()))
        --tabIndex;

      tabWindow->tabFramework->moveTabLater(event->source(), this, insertPolicy, tabIndex);
      event->acceptProposedAction();
    }
    tabWindow->setDropOverlayRect(QRect());
    return;
  }
  event->ignore();
}

QTabWindow::QTabWindow(QTabFramework* tabFramework) : tabFramework(tabFramework), overlayWidget(0), overlayWidgetTab(0)
{
  new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this, SLOT(hideCurrent()));
}

void QTabWindow::hideCurrent()
{
  QObject* widget = QApplication::focusWidget();
  while(widget)
  {
    QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget);
    if(tabContainer)
    {
      tabFramework->hideTab(tabContainer->currentWidget());
      return;
    }
    widget = widget->parent();
  }
}

void QTabWindow::setDropOverlayRect(const QRect& globalRect, const QRect& globalTabRect)
{
  if(globalRect.isValid())
  {
    if(!overlayWidget)
      overlayWidget = new QTabDropOverlay(this);
    overlayWidget->raise();
    overlayWidget->show();
    if(globalTabRect.isValid())
    {
      QRect wrect = globalRect;
      wrect.setTop(globalTabRect.bottom() + 1);
      QRect rect = wrect.translated(mapFromGlobal(QPoint(0, 0)));
      overlayWidget->setGeometry(rect);
      if(!overlayWidgetTab)
        overlayWidgetTab = new QTabDropOverlay(this);
      overlayWidgetTab->raise();
      overlayWidgetTab->show();
      QRect tabRect = globalTabRect.translated(mapFromGlobal(QPoint(0, 0)));
      overlayWidgetTab->setGeometry(tabRect);
    }
    else
    {
      QRect rect = globalRect.translated(mapFromGlobal(QPoint(0, 0)));
      overlayWidget->setGeometry(rect);
      if(overlayWidgetTab)
        overlayWidgetTab->hide();
    }
  }
  else if(overlayWidget)
  {
    if(overlayWidgetTab)
      overlayWidgetTab->hide();
    overlayWidget->hide();
  }
}

//void QTabWindow::removeDropOverlay()
//{
//  if(overlayWidget)
//  {
//    if(overlayWidgetTab)
//    {
//      delete overlayWidgetTab;
//      overlayWidgetTab = 0;
//    }
//    delete overlayWidget;
//    overlayWidget = 0;
//  }
//}

void QTabWindow::closeEvent(QCloseEvent* event)
{
  // "hide" all widgets
  QWidget* centralWidget = this->centralWidget();
  if(centralWidget)
  {
    QList<QWidget*> widgets;
    widgets.append(centralWidget);
    for(QList<QWidget*>::Iterator i = widgets.begin(); i != widgets.end(); ++i)
    {
      QWidget* widget = *i;
      QTabSplitter* tabSplitter = dynamic_cast<QTabSplitter*>(widget);
      if(tabSplitter)
      {
        for(int i = 0, count = tabSplitter->count(); i < count; ++i)
          widgets.append(tabSplitter->widget(i));
      }
      else
      {
        QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget);
        for(int i = 0, count = tabContainer->count(); i < count; ++i)
          tabFramework->hideTab(tabContainer->widget(i), false);
      }
    }
  }

  QMainWindow::closeEvent(event);
}

void QTabWindow::changeEvent(QEvent* event)
{
  if(event->type() == QEvent::ActivationChange && isActiveWindow())
  {
    emit activated();
  }

  QMainWindow::changeEvent(event);
}

QTabFramework::QTabFramework() : QTabWindow(this), moveTabWidget(0)
{
  connect(&signalMapper, SIGNAL(mapped(QWidget*)), this, SLOT(toggleVisibility(QWidget*)));
  floatingWindows.append(this);
  connect(&activatedSignalMapper, SIGNAL(mapped(QWidget*)), this, SLOT(updateWindowZOrder(QWidget*)));
  activatedSignalMapper.setMapping(this, this);
  connect(this, SIGNAL(activated()), &activatedSignalMapper, SLOT(map()));
}

QTabFramework::~QTabFramework()
{
  for(QHash<QWidget*, TabData>::Iterator i = tabs.begin(), end = tabs.end(); i != end; ++i)
  {
    TabData& tabData = i.value();
    if(tabData.hidden)
      delete i.key();
  }

  floatingWindows.removeOne(this);
  qDeleteAll(floatingWindows);
}

void QTabFramework::addTab(QWidget* widget, InsertPolicy insertPolicy, QWidget* position)
{
  if(tabs.contains(widget))
    return;

  TabData& tabData = *tabs.insert(widget, TabData());
  tabData.hidden = false;
  tabData.action = 0;

  addTab(widget, position ? dynamic_cast<QTabContainer*>(position->parent()->parent()) : 0, insertPolicy, -1);
}

void QTabFramework::moveTab(QWidget* widget, InsertPolicy insertPolicy, QWidget* position)
{
  QHash<QWidget*, TabData>::Iterator it =  tabs.find(widget);
  if(it == tabs.end())
    return;
  TabData& tabData = it.value();
  if(tabData.hidden)
  {
    addTab(widget, position ? dynamic_cast<QTabContainer*>(position->parent()->parent()) : 0, insertPolicy, -1);
    tabData.hidden = false;
    if(tabData.action)
      tabData.action->setChecked(true);
  }
  else
  {
    moveTab(widget, position ? dynamic_cast<QTabContainer*>(position->parent()->parent()) : 0, insertPolicy, -1);
  }
}

void QTabFramework::removeTab(QWidget* widget)
{
  QHash<QWidget*, TabData>::Iterator it = tabs.find(widget);
  if(it == tabs.end())
    return;
  TabData& tabData = it.value();
  if(!tabData.hidden)
  {
    QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget->parent()->parent());
    int movedIndex = tabContainer->indexOf(widget);
    tabContainer->removeTab(movedIndex);
    widget->setParent(NULL);
    removeContainerIfEmpty(tabContainer);
  }
  if(tabData.action)
    delete tabData.action;
  tabs.erase(it);
}

void QTabFramework::hideTab(QWidget* widget)
{
  QHash<QWidget*, TabData>::Iterator it = tabs.find(widget);
  if(it == tabs.end())
    return;
  TabData& tabData = it.value();
  if(!tabData.hidden)
    hideTab(widget, true);
}

QAction* QTabFramework::toggleViewAction(QWidget* widget)
{
  QHash<QWidget*, TabData>::Iterator it = tabs.find(widget);
  if(it == tabs.end())
    return 0;
  TabData& tabData = it.value();
  if(!tabData.action)
  {
    tabData.action = new QAction(widget->windowTitle(), this);
    tabData.action->setCheckable(true);
    tabData.action->setChecked(!tabData.hidden);
    connect(tabData.action, SIGNAL(triggered()), &signalMapper, SLOT(map()));
    signalMapper.setMapping(tabData.action, widget);
  }
  return tabData.action;
}

void QTabFramework::restoreLayout(const QByteArray& layout)
{
}

QByteArray QTabFramework::saveLayout()
{
  QByteArray result;

  // save window list
  // save window z-order
  // save layout
  // save tab order
  // save focus tab

  return result;
}

void QTabFramework::addTab(QWidget* widget, QTabContainer* container, InsertPolicy insertPolicy, int tabIndex)
{
  bool createWindow = !container || insertPolicy == InsertFloating;
  if(createWindow)
  {
    if(centralWidget())
    {
      QTabWindow* tabWindow = new QTabWindow(this);
      floatingWindows.append(tabWindow);
      activatedSignalMapper.setMapping(tabWindow, tabWindow);
      connect(tabWindow, SIGNAL(activated()), &activatedSignalMapper, SLOT(map()));
      QTabContainer* container = new QTabContainer(tabWindow, tabWindow);
      container->addTab(widget, widget->windowTitle());
      tabWindow->setCentralWidget(container);
      tabWindow->show();
    }
    else
    {
      QTabContainer* container = new QTabContainer(this, this);
      container->addTab(widget, widget->windowTitle());
      setCentralWidget(container);
    }
  }
  else
  {
    if(insertPolicy == InsertPolicy::InsertOnTop || (insertPolicy == InsertPolicy::Insert && tabIndex < 0))
    {
      insertPolicy = InsertPolicy::Insert;
      tabIndex = container->count();
    }
    if(insertPolicy == InsertPolicy::Insert)
    {
      container->insertTab(tabIndex, widget, widget->windowTitle());
      container->setCurrentIndex(tabIndex);
    }
    else if(insertPolicy == InsertPolicy::InsertLeft || insertPolicy == InsertPolicy::InsertRight ||
      insertPolicy == InsertPolicy::InsertTop || insertPolicy == InsertPolicy::InsertBottom)
    {
      Qt::Orientation orientation = (insertPolicy == InsertPolicy::InsertLeft || insertPolicy == InsertPolicy::InsertRight) ? Qt::Horizontal : Qt::Vertical;
      int widthOrHeight = 0;
      if(orientation == Qt::Horizontal)
        widthOrHeight = container->width();
      else
        widthOrHeight = container->height();
      QTabSplitter* splitter = dynamic_cast<QTabSplitter*>(container->parent());
      if(splitter && splitter->orientation() == orientation)
      {
        QTabContainer* newContainer = new QTabContainer(splitter, container->tabWindow);
        newContainer->addTab(widget, widget->windowTitle());
        int containerIndex = splitter->indexOf(container);
        if(insertPolicy == InsertPolicy::InsertRight || insertPolicy == InsertPolicy::InsertBottom)
        {
          QList<int> sizes = splitter->sizes();
          splitter->insertWidget(containerIndex + 1, newContainer);
          sizes[containerIndex] -= widthOrHeight / 3;
          sizes.insert(containerIndex + 1, widthOrHeight / 3 - splitter->handleWidth());
          splitter->setSizes(sizes);
        }
        else
        {
          QList<int> sizes = splitter->sizes();
          splitter->insertWidget(containerIndex, newContainer);
          sizes[containerIndex] -= widthOrHeight / 3;
          sizes.insert(containerIndex, widthOrHeight / 3 - splitter->handleWidth());
          splitter->setSizes(sizes);
        }
      }
      else
      {
        QTabSplitter* newSplitter = new QTabSplitter(orientation, splitter ? (QWidget*)splitter : (QWidget*)container->tabWindow);
        QTabContainer* newContainer = new QTabContainer(newSplitter, container->tabWindow);
        int containerIndex = -1;
        QList<int> sizes;
        if(splitter)
        {
          containerIndex = splitter->indexOf(container);
          sizes = splitter->sizes();
        }
        newContainer->addTab(widget, widget->windowTitle());
        container->setParent(NULL); // remove container from splitter or tabWindow
        if(!splitter)
        {
          container->tabWindow->setCentralWidget(newSplitter);
          //container->tabWindow->removeDropOverlay();
        }
        else
        {
          splitter->insertWidget(containerIndex, newSplitter);
          splitter->setSizes(sizes);
        }
        if(insertPolicy == InsertPolicy::InsertRight || insertPolicy == InsertPolicy::InsertBottom)
        {
          newSplitter->addWidget(container);
          newSplitter->addWidget(newContainer);
          QList<int> sizes;
          sizes.append(widthOrHeight - widthOrHeight / 3);
          sizes.append(widthOrHeight / 3 - newSplitter->handleWidth());
          newSplitter->setSizes(sizes);
        }
        else
        {
          newSplitter->addWidget(newContainer);
          newSplitter->addWidget(container);
          QList<int> sizes;
          sizes.append(widthOrHeight / 3 - newSplitter->handleWidth());
          sizes.append(widthOrHeight - widthOrHeight / 3);
          newSplitter->setSizes(sizes);
        }
      }
    }
    else
    {
      Q_ASSERT(false);
    }
  }
}

void QTabFramework::moveTab(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex)
{
  //QHash<QWidget*, TabData>::Iterator it =  tabs.find(widget);
  //if(it == tabs.end())
  //  return;
  //TabData& tabData = it.value();
  //if(tabData.hidden)
  //{
  //  addTab(widget, position, insertPolicy, tabIndex);
  //  tabData.hidden = false;
  //}
  //else
  //{
    QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget->parent()->parent());
    int movedIndex = tabContainer->indexOf(widget);
    tabContainer->removeTab(movedIndex);
    addTab(widget, position, insertPolicy, tabIndex);
    removeContainerIfEmpty(tabContainer);
  //}
}

void QTabFramework::moveTabLater(QWidget* widget, QTabContainer* position, InsertPolicy insertPolicy, int tabIndex)
{
  moveTabWidget = widget;
  moveTabPosition = position;
  moveTabInsertPolicy = insertPolicy;
  moveTabTabIndex = tabIndex;
  QTimer::singleShot(0, this, SLOT(executeMoveTab()));
}

void QTabFramework::executeMoveTab()
{
  if(!moveTabWidget)
    return;
  moveTab(moveTabWidget, moveTabPosition, moveTabInsertPolicy, moveTabTabIndex);
  moveTabWidget = 0;
}

void QTabFramework::toggleVisibility(QWidget* widget)
{
  QHash<QWidget*, TabData>::Iterator it = tabs.find(widget);
  if(it == tabs.end())
    return;
  TabData& tabData = it.value();
  if(tabData.hidden)
    moveTab(widget, InsertFloating, 0);
  else
    hideTab(widget);
}

void QTabFramework::updateWindowZOrder(QWidget* widget)
{
  QTabWindow* tabWindow = dynamic_cast<QTabWindow*>(widget);
  if(!tabWindow)
    return;
  floatingWindows.removeOne(tabWindow);
  floatingWindows.append(tabWindow);
}

void QTabFramework::removeContainerIfEmpty(QTabContainer* tabContainer)
{
  if(tabContainer->count() == 0)
  {
    QObject* parent = tabContainer->parent();
    QTabSplitter* splitter = dynamic_cast<QTabSplitter*>(parent);
    if(splitter)
    {
      if(splitter->count() == 2)
      { // the splitter will be pointless, remove it
        QWidget* sibling = splitter->widget(0);
        if(sibling == tabContainer)
          sibling = splitter->widget(1);
        sibling->setParent(NULL);

        QTabSplitter* parentSplitter = dynamic_cast<QTabSplitter*>(splitter->parent());
        if(parentSplitter)
        {
          QList<int> sizes = parentSplitter->sizes();
          int splitterIndex = parentSplitter->indexOf(splitter);
          splitter->setParent(NULL);
          QTabSplitter* siblingSplitter = dynamic_cast<QTabSplitter*>(sibling);
          if(!siblingSplitter || siblingSplitter->orientation() != parentSplitter->orientation())
          {
            parentSplitter->insertWidget(splitterIndex, sibling);
            parentSplitter->setSizes(sizes);
          }
          else
          {
            QList<int> sibSizes = siblingSplitter->sizes();
            while(siblingSplitter->count() > 0)
            {
              QWidget* widget = siblingSplitter->widget(0);
              widget->setParent(NULL);
              parentSplitter->insertWidget(splitterIndex, widget);
              sizes.insert(splitterIndex, sibSizes[0]);
              sibSizes.removeFirst();
              ++splitterIndex;
            }
            delete siblingSplitter;
          }
          delete splitter;
        }
        else
        {
          splitter->setParent(NULL);
          tabContainer->tabWindow->setCentralWidget(sibling);
          delete splitter;
        }
      }
      else
      {
        // remove container
        int splitCount = splitter->count();
        tabContainer->setParent(NULL);
        delete tabContainer;
        Q_ASSERT(splitter->count() == splitCount - 1);
      }
    }
    else
    {
      tabContainer->tabWindow->tabFramework->removeWindow(tabContainer->tabWindow);
    }
  }
}

void QTabFramework::hideTab(QWidget* widget, bool removeContainerIfEmpty)
{
  QHash<QWidget*, TabData>::Iterator it =  tabs.find(widget);
  if(it == tabs.end())
    return;
  TabData& tabData = it.value();
  if(tabData.hidden)
    return;

  QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget->parent()->parent());
  int movedIndex = tabContainer->indexOf(widget);
  tabContainer->removeTab(movedIndex);
  widget->setParent(NULL);
  if(removeContainerIfEmpty)
    this->removeContainerIfEmpty(tabContainer);

  tabData.hidden = true;
  if(tabData.action)
    tabData.action->setChecked(!tabData.hidden);
}

void QTabFramework::removeWindow(QTabWindow* window)
{
  if(window == this)
    return;
  floatingWindows.removeOne(window);
  delete window;
}

void QTabFramework::closeEvent(QCloseEvent* event)
{
  floatingWindows.removeOne(this);
  qDeleteAll(floatingWindows);
  floatingWindows.clear();

  QMainWindow::closeEvent(event); // skip QTabWindow closeEvent handler
}
