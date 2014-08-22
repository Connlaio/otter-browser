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

#ifndef OTTER_BOOKMARKSMODEL_H
#define OTTER_BOOKMARKSMODEL_H

#include "BookmarksManager.h"

#include <QtCore/QUrl>
#include <QtGui/QStandardItemModel>

namespace Otter
{

class BookmarksItem : public QStandardItem
{
public:
	explicit BookmarksItem(BookmarkType type, const QUrl &url = QUrl(), const QString &title = QString());
	~BookmarksItem();

	void setData(const QVariant &value, int role);
	QVariant data(int role) const;

protected:
	static QList<BookmarksItem*> getBookmarks(const QString &url);
	static QStringList getKeywords();
	static QStringList getUrls();
	static BookmarksItem* getBookmark(const QString &keyword);
	static bool hasBookmark(const QString &url);
	static bool hasKeyword(const QString &keyword);
	static bool hasUrl(const QString &url);

private:
	static QHash<QString, QList<BookmarksItem*> > m_urls;
	static QHash<QString, BookmarksItem*> m_keywords;

	friend class BookmarksManager;
	friend class BookmarkPropertiesDialog;
};

class BookmarksModel : public QStandardItemModel
{
	Q_OBJECT

public:
	enum BookmarksRole
	{
		TitleRole = Qt::DisplayRole,
		DescriptionRole = Qt::ToolTipRole,
		TypeRole = Qt::UserRole,
		UrlRole = (Qt::UserRole + 1),
		KeywordRole = (Qt::UserRole + 2),
		TimeAddedRole = (Qt::UserRole + 3),
		TimeModifiedRole = (Qt::UserRole + 4),
		TimeVisitedRole = (Qt::UserRole + 5),
		VisitsRole = (Qt::UserRole + 6)
	};

	explicit BookmarksModel(QObject *parent = NULL);

	BookmarksItem* getRootItem();
	BookmarksItem* getTrashItem();
	QList<QStandardItem*> findUrls(const QString &url, QStandardItem *branch = NULL);
};

}

#endif