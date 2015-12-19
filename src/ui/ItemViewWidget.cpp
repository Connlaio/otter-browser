/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2015 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#include "ItemViewWidget.h"
#include "../core/SessionsManager.h"
#include "../core/Settings.h"

#include <QtCore/QTimer>
#include <QtGui/QDropEvent>
#include <QtWidgets/QMenu>

namespace Otter
{

HeaderViewWidget::HeaderViewWidget(Qt::Orientation orientation, QWidget *parent) : QHeaderView(orientation, parent)
{
	connect(this, SIGNAL(sectionClicked(int)), this, SLOT(toggleColumnSort(int)));
}

void HeaderViewWidget::showEvent(QShowEvent *event)
{
	setSectionsMovable(true);
	setSectionsClickable(true);

	QHeaderView::showEvent(event);
}

void HeaderViewWidget::contextMenuEvent(QContextMenuEvent *event)
{
	QTreeView *view = qobject_cast<QTreeView*>(parent());
	QMenu menu(this);

	for (int i = 0; i < model()->columnCount(); ++i)
	{
		QAction *action = menu.addAction(model()->headerData(i, orientation()).toString());
		action->setData(i);
		action->setCheckable(true);
		action->setChecked(view && !view->isColumnHidden(i));
	}

	connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnVisibility(QAction*)));

	menu.exec(event->globalPos());
}

void HeaderViewWidget::toggleColumnSort(int column)
{
	ItemViewWidget *view = qobject_cast<ItemViewWidget*>(parent());

	if (!view)
	{
		return;
	}

	if (view->getSortColumn() != column)
	{
		setSorting(column, Qt::AscendingOrder);
	}
	else if (view->getSortOrder() == Qt::AscendingOrder)
	{
		setSorting(column, Qt::DescendingOrder);
	}
	else
	{
		setSorting(-1, Qt::AscendingOrder);
	}
}

void HeaderViewWidget::toggleColumnVisibility(QAction *action)
{
	if (action)
	{
		emit columnVisibilityChanged(action->data().toInt(), !action->isChecked());
	}
}

void HeaderViewWidget::setSorting(int column, Qt::SortOrder order)
{
	setSortIndicatorShown(true);
	setSortIndicator(column, order);

	emit sortingChanged(column, order);
}

int ItemViewWidget::m_treeIndentation = 0;

ItemViewWidget::ItemViewWidget(QWidget *parent) : QTreeView(parent),
	m_model(NULL),
	m_headerWidget(new HeaderViewWidget(Qt::Horizontal, this)),
	m_viewMode(ListViewMode),
	m_sortOrder(Qt::AscendingOrder),
	m_dragRow(-1),
	m_dropRow(-1),
	m_sortColumn(-1),
	m_canGatherExpanded(false),
	m_isModified(false),
	m_isInitialized(false)
{
	m_treeIndentation = indentation();

	optionChanged(QLatin1String("Interface/ShowScrollBars"), SettingsManager::getValue(QLatin1String("Interface/ShowScrollBars")));
	setHeader(m_headerWidget);
	setIndentation(0);
	setAllColumnsShowFocus(true);

	m_filterRoles.insert(Qt::DisplayRole);

	viewport()->setAcceptDrops(true);

	connect(SettingsManager::getInstance(), SIGNAL(valueChanged(QString,QVariant)), this, SLOT(optionChanged(QString,QVariant)));
	connect(this, SIGNAL(sortingChanged(int,Qt::SortOrder)), m_headerWidget, SLOT(setSorting(int,Qt::SortOrder)));
	connect(m_headerWidget, SIGNAL(sortingChanged(int,Qt::SortOrder)), this, SLOT(setSorting(int,Qt::SortOrder)));
	connect(m_headerWidget, SIGNAL(columnVisibilityChanged(int,bool)), this, SLOT(hideColumn(int,bool)));
	connect(m_headerWidget, SIGNAL(sectionMoved(int,int,int)), this, SLOT(saveState()));
}

void ItemViewWidget::showEvent(QShowEvent *event)
{
	if (m_isInitialized)
	{
		QTreeView::showEvent(event);

		return;
	}

	const QString name = objectName();
	const QString suffix = QLatin1String("ViewWidget");
	const QString type = (name.endsWith(suffix) ? name.left(name.size() - suffix.size()) : name);

	if (type.isEmpty())
	{
		return;
	}

	Settings settings(SessionsManager::getReadableDataPath(QLatin1String("views.ini")));
	settings.beginGroup(type);

	const int order = settings.getValue(QLatin1String("order"), 0).toInt();

	setSorting((qAbs(order) - 1), ((order > 0) ? Qt::AscendingOrder : Qt::DescendingOrder));

	const QStringList columns = settings.getValue(QLatin1String("columns")).toString().split(QLatin1Char(','), QString::SkipEmptyParts);

	if (!columns.isEmpty())
	{
		for (int i = 0; i < model()->columnCount(); ++i)
		{
			setColumnHidden(i, true);
		}

		disconnect(m_headerWidget, SIGNAL(sectionMoved(int,int,int)), this, SLOT(saveState()));

		for (int i = 0; i < columns.count(); ++i)
		{
			setColumnHidden(columns[i].toInt(), false);

			if (m_headerWidget)
			{
				m_headerWidget->moveSection(m_headerWidget->visualIndex(columns[i].toInt()), i);
			}
		}

		connect(m_headerWidget, SIGNAL(sectionMoved(int,int,int)), this, SLOT(saveState()));
	}

	m_headerWidget->setStretchLastSection(true);

	m_isInitialized = true;

	QTreeView::showEvent(event);
}

void ItemViewWidget::dropEvent(QDropEvent *event)
{
	if (m_viewMode == TreeViewMode)
	{
		QTreeView::dropEvent(event);

		return;
	}

	QDropEvent mutableEvent(QPointF((visualRect(m_model->index(0, 0)).x() + 1), event->posF().y()), Qt::MoveAction, event->mimeData(), event->mouseButtons(), event->keyboardModifiers(), event->type());

	QTreeView::dropEvent(&mutableEvent);

	if (!mutableEvent.isAccepted())
	{
		return;
	}

	event->accept();

	m_dropRow = indexAt(event->pos()).row();

	if (m_dragRow <= m_dropRow)
	{
		--m_dropRow;
	}

	if (dropIndicatorPosition() == QAbstractItemView::BelowItem)
	{
		++m_dropRow;
	}

	m_isModified = true;

	emit modified();

	QTimer::singleShot(50, this, SLOT(updateDropSelection()));
}

void ItemViewWidget::startDrag(Qt::DropActions supportedActions)
{
	m_dragRow = currentIndex().row();

	QTreeView::startDrag(supportedActions);
}

void ItemViewWidget::optionChanged(const QString &option, const QVariant &value)
{
	if (option == QLatin1String("Interface/ShowScrollBars"))
	{
		setHorizontalScrollBarPolicy(value.toBool() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(value.toBool() ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	}
}

void ItemViewWidget::moveRow(bool up)
{
	if (!m_model)
	{
		return;
	}

	const int sourceRow = currentIndex().row();
	const int destinationRow = (up ? (sourceRow - 1) : (sourceRow + 1));

	if ((up && sourceRow > 0) || (!up && sourceRow < (m_model->rowCount() - 1)))
	{
		m_model->insertRow(sourceRow, m_model->takeRow(destinationRow));

		setCurrentIndex(getIndex(destinationRow, 0));
		notifySelectionChanged();

		m_isModified = true;

		emit modified();
	}
}

void ItemViewWidget::insertRow(const QList<QStandardItem*> &items)
{
	if (!m_model)
	{
		return;
	}

	const int row = (currentIndex().row() + 1);

	if (items.count() > 0)
	{
		m_model->insertRow(row, items);
	}
	else
	{
		m_model->insertRow(row);
	}

	setCurrentIndex(getIndex(row, 0));

	m_isModified = true;

	emit modified();
}

void ItemViewWidget::insertRow(QStandardItem *item)
{
	QList<QStandardItem*> items;
	items.append(item);

	insertRow(items);
}

void ItemViewWidget::removeRow()
{
	if (!m_model)
	{
		return;
	}

	const int row = currentIndex().row();
	QStandardItem *parent = m_model->itemFromIndex(currentIndex().parent());

	if (row >= 0)
	{
		if (parent)
		{
			parent->removeRow(row);
		}
		else
		{
			m_model->removeRow(row);
		}

		m_isModified = true;

		emit modified();
	}
}

void ItemViewWidget::moveUpRow()
{
	moveRow(true);
}

void ItemViewWidget::moveDownRow()
{
	moveRow(false);
}

void ItemViewWidget::saveState()
{
	if (!m_isInitialized)
	{
		return;
	}

	const QString name = objectName();
	const QString suffix = QLatin1String("ViewWidget");
	const QString type = (name.endsWith(suffix) ? name.left(name.size() - suffix.size()) : name);

	if (type.isEmpty())
	{
		return;
	}

	Settings settings(SessionsManager::getWritableDataPath(QLatin1String("views.ini")));
	settings.beginGroup(type);

	QStringList columns;

	for(int i = 0; i < getColumnCount(); ++i)
	{
		int section = m_headerWidget->logicalIndex(i);

		if (section >= 0 && !isColumnHidden(section))
		{
			columns.append(QString::number(section));
		}
	}

	settings.setValue(QLatin1String("columns"), columns.join(QLatin1Char(',')));
	settings.setValue(QLatin1String("order"), ((m_sortColumn + 1) * ((m_sortOrder == Qt::AscendingOrder) ? 1 : -1)));
	settings.save();
}

void ItemViewWidget::hideColumn(int column, bool hide)
{
	setColumnHidden(column, hide);
	saveState();
}

void ItemViewWidget::notifySelectionChanged()
{
	if (m_model)
	{
		m_previousIndex = m_currentIndex;
		m_currentIndex = getIndex(getCurrentRow());

		emit canMoveUpChanged(canMoveUp());
		emit canMoveDownChanged(canMoveDown());
	}

	emit needsActionsUpdate();
}

void ItemViewWidget::updateDropSelection()
{
	setCurrentIndex(getIndex(qBound(0, m_dropRow, getRowCount()), 0));

	m_dropRow = -1;
}

void ItemViewWidget::updateFilter()
{
	applyFilter(m_model->invisibleRootItem());
}

void ItemViewWidget::setSorting(int column, Qt::SortOrder order)
{
	if (column == m_sortColumn && order == m_sortOrder)
	{
		return;
	}

	m_sortColumn = column;
	m_sortOrder = order;

	sortByColumn(column, order);
	update();
	saveState();

	emit sortingChanged(column, order);
}

void ItemViewWidget::setFilterString(const QString filter)
{
	if (!m_model)
	{
		return;
	}

	if (filter != m_filterString)
	{
		if (m_filterString.isEmpty())
		{
			connect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateFilter()));
			connect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(updateFilter()));
			connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(updateFilter()));
		}

		m_canGatherExpanded = m_filterString.isEmpty();
		m_filterString = filter;

		applyFilter(m_model->invisibleRootItem());

		if (m_filterString.isEmpty())
		{
			m_expandedBranches.clear();

			disconnect(m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateFilter()));
			disconnect(m_model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(updateFilter()));
			disconnect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(updateFilter()));
		}
	}
}

void ItemViewWidget::setFilterRoles(const QSet<int> &roles)
{
	m_filterRoles = roles;
}

void ItemViewWidget::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (m_model)
	{
		m_model->setData(index, value, role);
	}
}

void ItemViewWidget::setModel(QAbstractItemModel *model)
{
	if (!model)
	{
		QTreeView::setModel(NULL);

		return;
	}

	QTreeView::setModel(model);

	if (!model->parent())
	{
		model->setParent(this);
	}

	if (model->inherits(QStringLiteral("QStandardItemModel").toLatin1()))
	{
		m_model = qobject_cast<QStandardItemModel*>(model);

		connect(m_model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(notifySelectionChanged()));
	}

	connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(notifySelectionChanged()));
	connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(modified()));
}

void ItemViewWidget::setViewMode(ItemViewWidget::ViewMode mode)
{
	m_viewMode = mode;

	setIndentation((mode == TreeViewMode) ? m_treeIndentation : 0);
}

QStandardItemModel* ItemViewWidget::getModel()
{
	return m_model;
}

QStandardItem* ItemViewWidget::getItem(const QModelIndex &index) const
{
	return(m_model ? m_model->itemFromIndex(index) : NULL);
}

QStandardItem* ItemViewWidget::getItem(int row, int column) const
{
	return(m_model ? m_model->item(row, column) : NULL);
}

QModelIndex ItemViewWidget::getIndex(int row, int column) const
{
	return (m_model ? m_model->index(row, column) : QModelIndex());
}

QSize ItemViewWidget::sizeHint() const
{
	const QSize size = QTreeView::sizeHint();

	if (m_model && m_model->columnCount() == 1)
	{
		return QSize((sizeHintForColumn(0) + (frameWidth() * 2)), size.height());
	}

	return size;
}

ItemViewWidget::ViewMode ItemViewWidget::getViewMode() const
{
	return m_viewMode;
}

Qt::SortOrder ItemViewWidget::getSortOrder() const
{
	return m_sortOrder;
}

int ItemViewWidget::getSortColumn() const
{
	return m_sortColumn;
}

int ItemViewWidget::getCurrentRow() const
{
	return (selectionModel()->hasSelection() ? currentIndex().row() : -1);
}

int ItemViewWidget::getPreviousRow() const
{
	return m_previousIndex.row();
}

int ItemViewWidget::getRowCount() const
{
	return (m_model ? m_model->rowCount() : 0);
}

int ItemViewWidget::getColumnCount() const
{
	return (m_model ? m_model->columnCount() : 0);
}

bool ItemViewWidget::canMoveUp() const
{
	return (currentIndex().row() > 0 && m_model->rowCount() > 1);
}

bool ItemViewWidget::applyFilter(QStandardItem *item)
{
	bool hasFound = m_filterString.isEmpty();
	const bool isFolder = !item->flags().testFlag(Qt::ItemNeverHasChildren);

	if (isFolder)
	{
		if (m_canGatherExpanded && isExpanded(item->index()))
		{
			m_expandedBranches.insert(item);
		}

		for (int i = 0; i < item->rowCount(); ++i)
		{
			QStandardItem *child = item->child(i, 0);

			if (child && applyFilter(child))
			{
				hasFound = true;
			}
		}
	}
	else
	{
		const int columnCount = (item->parent() ? item->parent()->columnCount() : m_model->columnCount());

		for (int i = 0; i < columnCount; ++i)
		{
			QStandardItem *child = m_model->itemFromIndex(item->index().sibling(item->row(), i));

			if (!child)
			{
				continue;
			}

			QSet<int>::iterator iterator;

			for (iterator = m_filterRoles.begin(); iterator != m_filterRoles.end(); ++iterator)
			{
				if (child->data(*iterator).toString().contains(m_filterString, Qt::CaseInsensitive))
				{
					hasFound = true;

					break;
				}
			}

			if (hasFound)
			{
				break;
			}
		}
	}

	setRowHidden(item->row(), item->index().parent(), !hasFound);

	if (isFolder)
	{
		setExpanded(item->index(), ((hasFound && !m_filterString.isEmpty()) || (m_filterString.isEmpty() && m_expandedBranches.contains(item))));
	}

	return hasFound;
}

bool ItemViewWidget::canMoveDown() const
{
	const int currentRow = currentIndex().row();

	return (currentRow >= 0 && m_model->rowCount() > 1 && currentRow < (m_model->rowCount() - 1));
}

bool ItemViewWidget::isModified() const
{
	return m_isModified;
}

}
