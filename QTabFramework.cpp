
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
      Qt::DropAction ret = drag->exec(Qt::MoveAction);
      if(ret != Qt::MoveAction)
      {
        tabContainer->tabWindow->tabFramework->moveTab(dragWidget, tabContainer, QTabFramework::InsertFloating, -1);
      }
      //delete drag ?

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
    if(tabBar()->geometry().contains(pos))
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
      tabWindow->tabFramework->moveTab(event->source(), this, QTabFramework::InsertFloating, tabIndex);
      event->acceptProposedAction();
    }
    else
    {
      tabWindow->tabFramework->moveTab(event->source(), this, insertPolicy, tabIndex);
      event->acceptProposedAction();
    }
    tabWindow->setDropOverlayRect(QRect());
    return;
  }
  event->ignore();
}

void QTabWindow::setDropOverlayRect(const QRect& globalRect, const QRect& globalTabRect)
{
  if(globalRect.isValid())
  {
    
    if(!overlayWidget)
    {
      overlayWidget = new QTabDropOverlay(this);
      overlayWidget->show();
    }
    if(globalTabRect.isValid())
    {
      QRect wrect = globalRect;
      wrect.setTop(globalTabRect.bottom() + 1);
      QRect rect = wrect.translated(mapFromGlobal(QPoint(0, 0)));
      overlayWidget->setGeometry(rect);
      if(!overlayWidgetTab)
      {
        overlayWidgetTab = new QTabDropOverlay(this);
        overlayWidgetTab->show();
      }
      QRect tabRect = globalTabRect.translated(mapFromGlobal(QPoint(0, 0)));
      overlayWidgetTab->setGeometry(tabRect);
    }
    else
    {
      QRect rect = globalRect.translated(mapFromGlobal(QPoint(0, 0)));
      overlayWidget->setGeometry(rect);
      if(overlayWidgetTab)
      {
        overlayWidgetTab->deleteLater();
        overlayWidgetTab = 0;
      }
    }
  }
  else
  {
    if(overlayWidgetTab)
    {
      overlayWidgetTab->deleteLater();
      overlayWidgetTab = NULL;
    }
    overlayWidget->deleteLater();
    overlayWidget = NULL;
  }
}

QTabFramework::~QTabFramework()
{
  qDeleteAll(hiddenTabs);
  qDeleteAll(windows);
}

void QTabFramework::addTab(QWidget* widget, InsertPolicy insertPolicy, QWidget* position)
{
  if(!position)
    addTab(widget, NULL, insertPolicy, -1);
  else
    addTab(widget, dynamic_cast<QTabContainer*>(position->parent()->parent()), insertPolicy, -1);
}

void QTabFramework::addTab(QWidget* widget, QTabContainer* container, InsertPolicy insertPolicy, int tabIndex)
{
  bool createWindow = !container || insertPolicy == InsertFloating;
  if(createWindow)
  {
    hiddenTabs.remove(widget);
    QTabWindow* tabWindow = new QTabWindow(this);
    windows.append(tabWindow);
    QTabContainer* container = new QTabContainer(tabWindow, tabWindow);
    container->addTab(widget, widget->windowTitle());
    tabWindow->setCentralWidget(container);
    tabWindow->show();
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
      hiddenTabs.remove(widget);
      container->insertTab(tabIndex, widget, widget->windowTitle());
      container->setCurrentIndex(tabIndex);
    }
    else if(insertPolicy == InsertPolicy::InsertLeft || insertPolicy == InsertPolicy::InsertRight ||
      insertPolicy == InsertPolicy::InsertTop || insertPolicy == InsertPolicy::InsertBottom)
    {
      hiddenTabs.remove(widget);
      Qt::Orientation orientation = (insertPolicy == InsertPolicy::InsertLeft || insertPolicy == InsertPolicy::InsertRight) ? Qt::Horizontal : Qt::Vertical;
      int widthOrHeight = 0;
      if(orientation == Qt::Horizontal)
        widthOrHeight = container->width() / 3;
      else
        widthOrHeight = container->height() / 3;
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
          sizes[containerIndex] -= widthOrHeight;
          sizes.insert(containerIndex + 1, widthOrHeight);
          splitter->setSizes(sizes);
        }
        else
        {
          QList<int> sizes = splitter->sizes();
          splitter->insertWidget(containerIndex, newContainer);
          sizes[containerIndex] -= widthOrHeight;
          sizes.insert(containerIndex, widthOrHeight);
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
        if(insertPolicy == InsertPolicy::InsertRight || insertPolicy == InsertPolicy::InsertBottom)
        {
          newSplitter->addWidget(container);
          newSplitter->addWidget(newContainer);
          QList<int> sizes = newSplitter->sizes();
          sizes[0] -= (widthOrHeight - sizes[1]);
          sizes[1] = widthOrHeight;
          newSplitter->setSizes(sizes);
        }
        else
        {
          newSplitter->addWidget(newContainer);
          newSplitter->addWidget(container);
          QList<int> sizes = newSplitter->sizes();
          sizes[1] -= (widthOrHeight - sizes[0]);
          sizes[0] = widthOrHeight;
          newSplitter->setSizes(sizes);
        }
        if(!splitter)
          container->tabWindow->setCentralWidget(newSplitter);
        else
        {
          splitter->insertWidget(containerIndex, newSplitter);
          splitter->setSizes(sizes);
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
  QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget->parent()->parent());
  int movedIndex = tabContainer->indexOf(widget);
  tabContainer->removeTab(movedIndex);
  addTab(widget, position, insertPolicy, tabIndex);
  removeContainerIfEmpty(tabContainer);
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

void QTabFramework::removeTab(QWidget* widget)
{
  QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget->parent()->parent());
  int movedIndex = tabContainer->indexOf(widget);
  tabContainer->removeTab(movedIndex);
  removeContainerIfEmpty(tabContainer);
  hiddenTabs.remove(widget);
}

void QTabFramework::hideTab(QWidget* widget)
{
  if(hiddenTabs.contains(widget))
    return;
  QTabContainer* tabContainer = dynamic_cast<QTabContainer*>(widget->parent()->parent());
  int movedIndex = tabContainer->indexOf(widget);
  tabContainer->removeTab(movedIndex);
  removeContainerIfEmpty(tabContainer);
  hiddenTabs.insert(widget);
}

void QTabFramework::removeWindow(QTabWindow* window)
{
  windows.removeOne(window);
  delete window;
}
