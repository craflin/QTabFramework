
#include "stdafx.h"

class QTabDropOverlay : public QWidget
{
public:
  QTabDropOverlay(QWidget* parent) : QWidget(parent)
  {
    setPalette(Qt::transparent);
    setAttribute(Qt::WA_TransparentForMouseEvents);
  }

  void paintEvent(QPaintEvent *event) {
      QPainter painter(this);
      //painter.setRenderHint(QPainter::Antialiasing);
      painter.setBrush(QBrush(QColor(88, 88, 88, 88)));
      painter.drawRect(rect());
      //painter.setPen(QPen(Qt::red));
      //painter.drawLine(width()/8, height()/8, 7*width()/8, 7*height()/8);
      //painter.drawLine(width()/8, 7*height()/8, 7*width()/8, height()/8);
  }
};

QTabDrawer::QTabDrawer(QTabContainer* tabContainer) : QTabBar(tabContainer), tabContainer(tabContainer), pressedIndex(-1), dragInProgress(false) {}

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

  if(dragInProgress)
  {
    int draggedIndex = pressedIndex;
    pressedIndex = -1;
    dragInProgress = false;
    tabContainer->tabWindow->setDropOverlayRect(QRect());

    if(rect().contains(event->pos()))
      QTabBar::mouseReleaseEvent(event);
    else
    {
      QTabBar::mouseReleaseEvent(event);

      if(draggedIndex >= 0)
      {
        draggedIndex = tabContainer->indexOf(dragWidget); // index of the widget may change while its being dragged
        if(draggedIndex >= 0)
        {
          QTabFramework::InsertPolicy insertPolicy;
          QTabContainer* position;
          QRect rect = findDropRect(event->globalPos(), insertPolicy, position);
          tabContainer->removeTab(draggedIndex);
          tabContainer->tabWindow->tabFramework->addTab(dragWidgetTabText, dragWidget, position, insertPolicy);
          
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
      }
    }
  }
  else
    QTabBar::mouseReleaseEvent(event);
}

void QTabDrawer::mouseMoveEvent(QMouseEvent* event)
{
  // Start drag
  if (!dragInProgress && pressedIndex >= 0)
  {
      if ((event->pos() - dragStartPosition).manhattanLength() > QApplication::startDragDistance())
      {
          dragInProgress = true;
          dragWidget = tabContainer->widget(pressedIndex);
          dragWidgetTabText = tabContainer->tabText(pressedIndex);
          //qDebug("start drag: %s", dragWidgetTabText.toAscii().constData());
      }
  }

  if(dragInProgress)
  {
    if(rect().contains(event->pos()))
    {
      QTabBar::mouseMoveEvent(event);
      tabContainer->tabWindow->setDropOverlayRect(QRect());
    }
    else
    {
      QTabBar::mouseMoveEvent(event);

      QTabFramework::InsertPolicy insertPolicy;
      QTabContainer* position;
      QRect rect = findDropRect(event->globalPos(), insertPolicy, position);

      tabContainer->tabWindow->setDropOverlayRect(rect);
    }
  }
  else
    QTabBar::mouseMoveEvent(event);
}

QRect QTabDrawer::findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QTabContainer*& position)
{
  QRect rect = tabContainer->findDropRect(globalPos, insertPolicy);
  position = tabContainer;
  if(insertPolicy == QTabFramework::InsertOnTop)
  {
    insertPolicy = QTabFramework::InsertFloating;
    return QRect();
  }
  else if(insertPolicy == QTabFramework::InsertFloating)
  {
    rect = tabContainer->tabWindow->findDropRect(globalPos, insertPolicy, position);
    //if(insertPolicy == QTabFramework::InsertFloating)
    //  tabContainer->tabWindow->tabFramework->findDropRect(event->pos(), insertPolicy);
  
  }
  return rect;
}

QTabSplitter::QTabSplitter(Qt::Orientation orientation, QWidget* parent) : QSplitter(orientation, parent)
{
  setChildrenCollapsible(false);
}

QRect QTabSplitter::findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QTabContainer*& position)
{
  QPoint pos = mapFromGlobal(globalPos);
  for(int i = 0, count = this->count(); i < count; ++i)
  {
    QWidget* widget = this->widget(i);
    if(widget->geometry().contains(pos))
    {
      QTabContainer* container = dynamic_cast<QTabContainer*>(widget);
      if(container)
      {
        position = container;
        return container->findDropRect(globalPos, insertPolicy);
      }
      else
      {
        QTabSplitter* splitter = dynamic_cast<QTabSplitter*>(widget);
        return splitter->findDropRect(globalPos, insertPolicy, position);
      }
    }
  }
  insertPolicy = QTabFramework::InsertFloating;
  position = 0;
  return QRect();
}

QTabContainer::QTabContainer(QWidget* parent, QTabWindow* tabWindow) : QTabWidget(parent), tabWindow(tabWindow)
{
  QTabDrawer* drawer = new QTabDrawer(this);
  drawer->setMovable(true);
  drawer->setTabsClosable(true);
  drawer->setUsesScrollButtons(true);
  drawer->setElideMode(Qt::ElideRight);
  setTabBar(drawer);
  setDocumentMode(true);
}

QRect QTabContainer::findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy)
{
  QPoint pos = mapFromGlobal(globalPos);
  QRect containerRect = rect();
  QRect result;
  if(containerRect.contains(pos))
  {
    if(tabBar()->geometry().contains(pos))
    {
      insertPolicy = QTabFramework::InsertOnTop;
      result = containerRect;
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

QRect QTabWindow::findDropRect(const QPoint& globalPos, QTabFramework::InsertPolicy& insertPolicy, QTabContainer*& position)
{
  QTabContainer* container = dynamic_cast<QTabContainer*>(centralWidget());
  if(container)
  {
    position = container;
    return container->findDropRect(globalPos, insertPolicy);
  }
  else
  {
    QTabSplitter* splitter = dynamic_cast<QTabSplitter*>(centralWidget());
    if(splitter)
      return splitter->findDropRect(globalPos, insertPolicy, position);
  }
  insertPolicy = QTabFramework::InsertFloating;
  position = 0;
  return QRect();
}

void QTabWindow::setDropOverlayRect(const QRect& globalRect)
{
  if(globalRect.isValid())
  {
    QRect rect = globalRect.translated(mapFromGlobal(QPoint(0, 0)));
    if(!overlayWidget)
    {
      overlayWidget = new QTabDropOverlay(this);
      overlayWidget->show();
    }
    overlayWidget->setGeometry(rect);
  }
  else
  {
    delete overlayWidget;
    overlayWidget = NULL;
  }
}

QTabFramework::~QTabFramework()
{
  qDeleteAll(windows);
}

void QTabFramework::addTab(const QString& title, QWidget* widget, InsertPolicy insertPolicy, QWidget* position)
{
  if(!position)
    addTab(title, widget, NULL, insertPolicy);
  else
  {
    addTab(title, widget, dynamic_cast<QTabContainer*>(position->parent()->parent()), insertPolicy);
  }
}

void QTabFramework::addTab(const QString& title, QWidget* widget, QTabContainer* container, InsertPolicy insertPolicy)
{
  bool createWindow = !container || insertPolicy == InsertFloating;
  if(createWindow)
  {
    QTabWindow* tabWindow = new QTabWindow(this);
    windows.append(tabWindow);
    QTabContainer* container = new QTabContainer(tabWindow, tabWindow);
    container->addTab(widget, title);
    tabWindow->setCentralWidget(container);
    tabWindow->show();
  }
  else
  {
    if(insertPolicy == InsertPolicy::InsertOnTop)
    {
      int index = container->addTab(widget, title);
      container->setCurrentIndex(index);
    }
    else if(insertPolicy == InsertPolicy::InsertLeft || insertPolicy == InsertPolicy::InsertRight ||
      insertPolicy == InsertPolicy::InsertTop || insertPolicy == InsertPolicy::InsertBottom)
    {
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
        newContainer->addTab(widget, title);
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
        newContainer->addTab(widget, title);
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

void QTabFramework::removeWindow(QTabWindow* window)
{
  windows.removeOne(window);
  delete window;
}
