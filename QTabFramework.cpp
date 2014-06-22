
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

QTabSplitter::QTabSplitter(Qt::Orientation orientation, QWidget* parent, QTabWindow* tabWindow) : QSplitter(orientation, parent), tabWindow(tabWindow)
{
  setChildrenCollapsible(false);
}

void QTabSplitter::writeLayout(QDataStream& stream)
{
  stream << saveState();
  stream << (quint32)orientation();
  for(int i = 0, count = this->count(); i < count; ++i)
  {
    QWidget* widget = this->widget(i);
    QTabSplitter* tabSplitter = dynamic_cast<QTabSplitter*>(widget);
    if(tabSplitter)
    {
      stream << (quint32)QTabFramework::SplitterType;
      tabSplitter->writeLayout(stream);
    }
    else
    {
      QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget);
      if(tabContainer)
      {
        stream << (quint32)QTabFramework::ContainerType;
        tabContainer->writeLayout(stream);
      }
    }
  }
  stream << (quint32)QTabFramework::NoneType;
}

void QTabSplitter::readLayout(QDataStream& stream)
{
  QByteArray state;
  stream >> state;
  quint32 orientation;
  stream >> orientation;
  setOrientation((Qt::Orientation)orientation);
  quint32 type;
  stream >> type;
  while(type != QTabFramework::NoneType)
  {
    switch((QTabFramework::LayoutType)type)
    {
    case QTabFramework::SplitterType:
      {
        QTabSplitter* tabSplitter = new QTabSplitter(Qt::Horizontal, this, tabWindow);
        addWidget(tabSplitter);
        tabSplitter->readLayout(stream);
      }
      break;
    case QTabFramework::ContainerType:
      {
        QTabContainer* tabContainer = new QTabContainer(this, tabWindow);
        addWidget(tabContainer);
        tabContainer->readLayout(stream);
      }
      break;
    default:
      Q_ASSERT(false);
      break;
    }
    stream >> type;
  }
  restoreState(state);
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

void QTabContainer::writeLayout(QDataStream& stream)
{
  stream << (quint32)currentIndex();
  for(int i = 0, count = this->count(); i < count; ++i)
  {
    stream << (quint32)QTabFramework::TabType;
    QWidget* widget = this->widget(i);
    QString tabObjectName = tabWindow->tabFramework->tabObjectName(widget);
    stream << tabObjectName;
  }
  stream << (quint32)QTabFramework::NoneType;
}

void QTabContainer::readLayout(QDataStream& stream)
{
  quint32 currentIndex;
  stream >> currentIndex;
  quint32 type;
  stream >> type;
  while(type != QTabFramework::NoneType)
  {
    if(type == QTabFramework::TabType)
    {
      QString objectName;
      stream >> objectName;
      tabWindow->tabFramework->unhideTab(objectName, this);
    }
    else
    {
      Q_ASSERT(false);
    }
    stream >> type;
  }
  setCurrentIndex(currentIndex);
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

void QTabWindow::writeLayout(QDataStream& stream)
{
  stream << saveGeometry();
  stream << saveState();
  QWidget* widget = centralWidget();
  QTabSplitter* tabSplitter = dynamic_cast<QTabSplitter*>(widget);
  if(tabSplitter)
  {
    stream << (quint32)QTabFramework::SplitterType;
    tabSplitter->writeLayout(stream);
  }
  else
  {
    QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget);
    if(tabContainer)
    {
      stream << (quint32)QTabFramework::ContainerType;
      tabContainer->writeLayout(stream);
    }
    else
      stream << (quint32)QTabFramework::NoneType;
  }
}

void QTabWindow::readLayout(QDataStream& stream)
{
  QByteArray geometry, state;
  stream >> geometry;
  stream >> state;
  quint32 type;
  stream >> type;
  switch((QTabFramework::LayoutType)type)
  {
  case QTabFramework::SplitterType:
    {
      QTabSplitter* tabSplitter = new QTabSplitter(Qt::Horizontal, this, this);
      setCentralWidget(tabSplitter);
      tabSplitter->readLayout(stream);
    }
    break;
  case QTabFramework::ContainerType:
    {
      QTabContainer* tabContainer = new QTabContainer(this, this);
      setCentralWidget(tabContainer);
      tabContainer->readLayout(stream);
    }
    break;
  default:
    Q_ASSERT(false);
    break;
  }
  restoreGeometry(geometry);
  restoreState(state);
}

void QTabWindow::hideAllTabs()
{
  QWidget* centralWidget = this->centralWidget();
  if(!centralWidget)
    return;

  QLinkedList<QWidget*> widgets;
  widgets.append(centralWidget);
  for(QLinkedList<QWidget*>::Iterator i = widgets.begin(); i != widgets.end(); ++i)
  {
    QWidget* widget = *i;
    QTabSplitter* tabSplitter = dynamic_cast<QTabSplitter*>(widget);
    if(tabSplitter)
    {
      for(int i = 0, count = tabSplitter->count(); i < count; ++i)
      {
        QWidget* widget = tabSplitter->widget(i);
        widgets.append(widget);
      }
    }
    else
    {
      QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget);
      if(tabContainer)
        for(int i = tabContainer->count() - 1; i >= 0; --i)
          tabFramework->hideTab(tabContainer->widget(i), false);
    }
  }
}

void QTabWindow::closeEvent(QCloseEvent* event)
{
  hideAllTabs();

  QMainWindow::closeEvent(event);
  
  tabFramework->removeWindow(this);
}

void QTabWindow::changeEvent(QEvent* event)
{
  if(event->type() == QEvent::ActivationChange && isActiveWindow())
    tabFramework->updateWindowZOrder(this);

  QMainWindow::changeEvent(event);
}

QTabFramework::QTabFramework() : QTabWindow(this), moveTabWidget(0)
{
  connect(&signalMapper, SIGNAL(mapped(QWidget*)), this, SLOT(toggleVisibility(QWidget*)));
  floatingWindows.append(this);
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

  // determine object name
  QString objectName = widget->objectName();
  if(objectName.isEmpty())
  {
    QString className = widget->metaObject()->className();
    for(int num = 0;; ++num)
    {
      objectName = className + "_" + QString::number(num);
      if(!tabsByName.contains(objectName))
        break;
    }
  }
  else
  {
    QString objectClassName = objectName;
    if(tabsByName.contains(objectName))
      for(int num = 0;; ++num)
      {
        objectName = objectClassName + "_" + QString::number(num);
        if(!tabsByName.contains(objectName))
          break;
      }
  }

  // create tab data
  TabData& tabData = *tabs.insert(widget, TabData());
  tabData.widget = widget;
  tabData.objectName = objectName;
  tabData.hidden = false;
  tabData.action = 0;
  tabsByName.insert(objectName, &tabData);
  widget->installEventFilter(this);

  //
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
  tabsByName.remove(tabData.objectName);
  tabs.erase(it);
  widget->removeEventFilter(this);
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

bool QTabFramework::isVisible(QWidget* widget) const
{
  QHash<QWidget*, TabData>::ConstIterator it = tabs.find(widget);
  if(it == tabs.end())
    return false;
  const TabData& tabData = it.value();
  return !tabData.hidden;
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
  if(layout.isEmpty())
    return;

  // check format version
  QDataStream dataStream(layout);
  quint32 version;
  dataStream >> version;
  if(version != 100)
    return;

  // close all floating windows
  for(QList<QTabWindow*>::Iterator i = floatingWindows.begin(), end = floatingWindows.end(); i != end; ++i)
  {
    QTabWindow* tabWindow = *i;
    if(tabWindow != this)
    {
      tabWindow->hideAllTabs();
      delete tabWindow;
    }
  }
  floatingWindows.clear();
  floatingWindows.append(this);

  // hide all tabs
  hideAllTabs();
  QWidget* centralWidget = this->centralWidget();
  if(centralWidget)
  {
    setCentralWidget(0);
    delete centralWidget;
  }

  // restore local layout
  quint32 type;
  dataStream >> type;
  if(type == WindowType)
    readLayout(dataStream);

  // create floating windows
  dataStream >> type;
  while(type != NoneType)
  {
    if(type == WindowType)
    {
      QTabWindow* tabWindow = createWindow();
      tabWindow->readLayout(dataStream);
      tabWindow->show();
    }
    else
    {
      Q_ASSERT(false);
    }
    dataStream >> type;
  }
}

QByteArray QTabFramework::saveLayout()
{
  QByteArray result;
  QDataStream dataStream(&result, QIODevice::WriteOnly);
  quint32 version = 100;
  dataStream << version;
  dataStream << (quint32)WindowType;
  writeLayout(dataStream);
  for(QList<QTabWindow*>::Iterator i = floatingWindows.begin(), end = floatingWindows.end(); i != end; ++i)
    if(*i != this)
    {
      dataStream << (quint32)WindowType;
      (*i)->writeLayout(dataStream);
    }
  dataStream << (quint32)NoneType;
  return result;
}

void QTabFramework::addTab(QWidget* widget, QTabContainer* container, InsertPolicy insertPolicy, int tabIndex)
{
  QWidget* centralWidget = this->centralWidget();
  bool createWindow = !container || insertPolicy == InsertFloating || !centralWidget;
  if(createWindow)
  {
    if(centralWidget)
    {
      QTabWindow* tabWindow = this->createWindow();
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
        QTabSplitter* newSplitter = new QTabSplitter(orientation, splitter ? (QWidget*)splitter : (QWidget*)container->tabWindow, container->tabWindow);
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

QString QTabFramework::tabObjectName(QWidget* widget)
{
  QHash<QWidget*, TabData>::Iterator it = tabs.find(widget);
  if(it == tabs.end())
    return QString();
  TabData& tabData = it.value();
  return tabData.objectName;
}

void QTabFramework::unhideTab(const QString& objectName, QTabContainer* position)
{
  QHash<QString, TabData*>::Iterator it = tabsByName.find(objectName);
  if(it == tabsByName.end())
    return;
  TabData& tabData = *it.value();
  if(!tabData.hidden)
    return;
  position->addTab(tabData.widget, tabData.widget->windowTitle());
  tabData.hidden = false;
  if(tabData.action)
    tabData.action->setChecked(!tabData.hidden);
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

QTabWindow* QTabFramework::createWindow()
{
  QTabWindow* tabWindow = new QTabWindow(this);
  tabWindow->setAttribute(Qt::WA_DeleteOnClose);
  floatingWindows.append(tabWindow);
  return tabWindow;
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
  QWidget* centralWidget = this->centralWidget();
  if(centralWidget)
  {
    setCentralWidget(0);
    delete centralWidget;
  }
  for(QHash<QWidget*, TabData>::Iterator i = tabs.begin(), end = tabs.end(); i != end; ++i)
  {
    TabData& tabData = i.value();
    if(tabData.hidden)
      delete i.key();
  }
  tabs.clear();
  tabsByName.clear();

  QMainWindow::closeEvent(event); // skip QTabWindow closeEvent handler
}

bool QTabFramework::eventFilter(QObject* obj, QEvent* event)
{
  if(event->type() == QEvent::WindowTitleChange)
  {
    QWidget* widget = dynamic_cast<QWidget*>(obj);
    QHash<QWidget*, TabData>::Iterator it = tabs.find(widget);
    if(it != tabs.end())
    {
      QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget->parent()->parent());
      if(tabContainer)
      {
        int index = tabContainer->indexOf(widget);
        tabContainer->setTabText(index, widget->windowTitle());
      }
    }
  }

  return QMainWindow::eventFilter(obj, event);
}
