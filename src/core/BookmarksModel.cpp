/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "BookmarksModel.h"
#include "Utils.h"
#include "WebBackend.h"
#include "WebBackendsManager.h"

namespace Otter
{

QHash<QString, QList<BookmarksItem*> > BookmarksItem::m_urls;
QHash<QString, BookmarksItem*> BookmarksItem::m_keywords;

BookmarksItem::BookmarksItem(BookmarkType type, const QUrl &url, const QString &title) : QStandardItem()
{
	setData(type, BookmarksModel::TypeRole);
	setData(url, BookmarksModel::UrlRole);
	setData(title, BookmarksModel::TitleRole);

	if (type == RootBookmark || type == FolderBookmark)
	{
		setData(Utils::getIcon(QLatin1String("inode-directory")), Qt::DecorationRole);
	}
	else if (type == TrashBookmark)
	{
		setData(Utils::getIcon(QLatin1String("user-trash")), Qt::DecorationRole);
		setEnabled(false);
	}
	else if (type == SeparatorBookmark)
	{
		setData(QLatin1String("separator"), Qt::AccessibleDescriptionRole);
	}
}

BookmarksItem::~BookmarksItem()
{
	if (!data(BookmarksModel::UrlRole).toString().isEmpty())
	{
		const QString url = data(BookmarksModel::UrlRole).toUrl().toString();

		if (m_urls.contains(url))
		{
			m_urls[url].removeAll(this);

			if (m_urls[url].isEmpty())
			{
				m_urls.remove(url);
			}
		}
	}

	if (!data(BookmarksModel::KeywordRole).toString().isEmpty() && m_keywords.contains(data(BookmarksModel::UrlRole).toString()))
	{
		m_keywords.remove(data(BookmarksModel::UrlRole).toString());
	}
}

void BookmarksItem::setData(const QVariant &value, int role)
{
	if (role == BookmarksModel::UrlRole && value.toUrl() != data(BookmarksModel::UrlRole).toUrl())
	{
		const QString oldUrl = data(BookmarksModel::UrlRole).toUrl().toString();
		const QString newUrl = value.toUrl().toString();

		if (!oldUrl.isEmpty() && m_urls.contains(oldUrl))
		{
			m_urls[oldUrl].removeAll(this);

			if (m_urls[oldUrl].isEmpty())
			{
				m_urls.remove(oldUrl);
			}
		}

		if (!newUrl.isEmpty())
		{
			if (!m_urls.contains(newUrl))
			{
				m_urls[newUrl] = QList<BookmarksItem*>();
			}

			m_urls[newUrl].append(this);
		}
	}
	else if (role == BookmarksModel::KeywordRole && value.toString() != data(BookmarksModel::KeywordRole).toString())
	{
		const QString oldKeyword = data(BookmarksModel::KeywordRole).toString();
		const QString newKeyword = value.toString();

		if (!oldKeyword.isEmpty() && m_keywords.contains(oldKeyword))
		{
			m_keywords.remove(oldKeyword);
		}

		if (!newKeyword.isEmpty())
		{
			m_keywords[newKeyword] = this;
		}
	}

	QStandardItem::setData(value, role);
}

QList<BookmarksItem*> BookmarksItem::getBookmarks(const QString &url)
{
	if (m_urls.contains(url))
	{
		return m_urls[url];
	}

	return QList<BookmarksItem*>();
}

QStringList BookmarksItem::getKeywords()
{
	return m_keywords.keys();
}

QStringList BookmarksItem::getUrls()
{
	return m_urls.keys();
}

BookmarksItem *BookmarksItem::getBookmark(const QString &keyword)
{
	if (m_keywords.contains(keyword))
	{
		return m_keywords[keyword];
	}

	return NULL;
}

QVariant BookmarksItem::data(int role) const
{
	if (role == Qt::DecorationRole && QStandardItem::data(Qt::DecorationRole).isNull() && static_cast<BookmarkType>(QStandardItem::data(BookmarksModel::TypeRole).toInt()) != SeparatorBookmark)
	{
		return WebBackendsManager::getBackend()->getIconForUrl(data(BookmarksModel::UrlRole).toUrl());
	}

	return QStandardItem::data(role);
}

bool BookmarksItem::hasBookmark(const QString &url)
{
	return m_urls.contains(QUrl(url).toString());
}

bool BookmarksItem::hasKeyword(const QString &keyword)
{
	return m_keywords.contains(keyword);
}

bool BookmarksItem::hasUrl(const QString &url)
{
	return m_urls.contains(url);
}

BookmarksModel::BookmarksModel(QObject *parent) : QStandardItemModel(parent)
{
	appendRow(new BookmarksItem(RootBookmark, QUrl(), tr("Bookmarks")));
	appendRow(new BookmarksItem(TrashBookmark, QUrl(), tr("Trash")));
}

BookmarksItem* BookmarksModel::getRootItem()
{
	return dynamic_cast<BookmarksItem*>(item(0, 0));
}

BookmarksItem* BookmarksModel::getTrashItem()
{
	return dynamic_cast<BookmarksItem*>(item(1, 0));
}

QList<QStandardItem*> BookmarksModel::findUrls(const QString &url, QStandardItem *branch)
{
	if (!branch)
	{
		branch = item(0, 0);
	}

	QList<QStandardItem*> items;

	for (int i = 0; i < branch->rowCount(); ++i)
	{
		QStandardItem *item = branch->child(i);

		if (item)
		{
			const BookmarkType type = static_cast<BookmarkType>(item->data(TypeRole).toInt());

			if (type == FolderBookmark)
			{
				items.append(findUrls(url, item));
			}
			else if (type == UrlBookmark && item->data(UrlRole).toUrl().toString() == url)
			{
				items.append(item);
			}
		}
	}

	return items;
}

}