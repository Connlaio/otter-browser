/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
* Copyright (C) 2014 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "HtmlBookmarksImporter.h"
#include "../../../core/BookmarksModel.h"

//TODO QtWebKit should not be used directly
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKit/QWebElementCollection>

namespace Otter
{

HtmlBookmarksImporter::HtmlBookmarksImporter(QObject *parent) : BookmarksImporter(parent),
	m_file(NULL),
	m_optionsWidget(new BookmarksImporterWidget())
{
}

HtmlBookmarksImporter::~HtmlBookmarksImporter()
{
	m_optionsWidget->deleteLater();
}

void HtmlBookmarksImporter::handleOptions()
{
	if (m_optionsWidget->removeExisting())
	{
		removeAllBookmarks();

		if (m_optionsWidget->importIntoSubfolder())
		{
			BookmarksItem *folder = new BookmarksItem(FolderBookmark, QUrl(), m_optionsWidget->getSubfolderName());

			BookmarksManager::getModel()->getRootItem()->appendRow(folder);

			setImportFolder(folder);
		}
		else
		{
			setImportFolder(BookmarksManager::getModel()->getRootItem());
		}
	}
	else
	{
		setAllowDuplicates(m_optionsWidget->allowDuplicates());
		setImportFolder(m_optionsWidget->targetFolder());
	}
}

void HtmlBookmarksImporter::processElement(const QWebElement &element)
{
	if (element.tagName().toLower() == QLatin1String("h3"))
	{
		BookmarksItem *bookmark = new BookmarksItem(FolderBookmark, QUrl(), element.toPlainText());
		const QString keyword = element.attribute(QLatin1String("SHORTCUTURL"));

		if (!BookmarksManager::hasKeyword(keyword))
		{
			bookmark->setData(keyword, BookmarksModel::KeywordRole);
		}

		if (!element.attribute(QLatin1String("ADD_DATE")).isEmpty())
		{
			const QDateTime time = QDateTime::fromTime_t(element.attribute(QLatin1String("ADD_DATE")).toUInt());

			bookmark->setData(time, BookmarksModel::TimeAddedRole);
			bookmark->setData(time, BookmarksModel::TimeModifiedRole);
		}

		getCurrentFolder()->appendRow(bookmark);;
		setCurrentFolder(bookmark);
	}
	else if (element.tagName().toLower() == QLatin1String("a"))
	{
		const QString url = element.attribute(QLatin1String("href"));

		if (!allowDuplicates() && BookmarksManager::hasBookmark(url))
		{
			return;
		}

		BookmarksItem *bookmark = new BookmarksItem(UrlBookmark, QUrl(url), element.toPlainText());
		const QString keyword = element.attribute(QLatin1String("SHORTCUTURL"));

		if (!BookmarksManager::hasKeyword(keyword))
		{
			bookmark->setData(keyword, BookmarksModel::KeywordRole);
		}

		if (element.parent().nextSibling().tagName().toLower() == QLatin1String("dd"))
		{
			bookmark->setData(element.parent().nextSibling().toPlainText(), BookmarksModel::DescriptionRole);
		}

		if (!element.attribute(QLatin1String("ADD_DATE")).isEmpty())
		{
			bookmark->setData(QDateTime::fromTime_t(element.attribute(QLatin1String("ADD_DATE")).toUInt()), BookmarksModel::TimeAddedRole);
		}

		if (!element.attribute(QLatin1String("LAST_MODIFIED")).isEmpty())
		{
			bookmark->setData(QDateTime::fromTime_t(element.attribute(QLatin1String("LAST_MODIFIED")).toUInt()), BookmarksModel::TimeModifiedRole);
		}

		if (!element.attribute(QLatin1String("LAST_VISITED")).isEmpty())
		{
			bookmark->setData(QDateTime::fromTime_t(element.attribute(QLatin1String("LAST_VISITED")).toUInt()), BookmarksModel::TimeVisitedRole);
		}

		getCurrentFolder()->appendRow(bookmark);
	}
	else if (element.tagName().toLower() == QLatin1String("hr"))
	{
		getCurrentFolder()->appendRow(new BookmarksItem(SeparatorBookmark));
	}

	const QWebElementCollection descendants = element.findAll(QLatin1String("*"));

	for (int i = 0; i < descendants.count(); ++i)
	{
		if (descendants.at(i).parent() == element)
		{
			processElement(descendants.at(i));
		}
	}

	if (element.tagName().toLower() == QLatin1String("dl"))
	{
		goToParent();
	}
}

QWidget* HtmlBookmarksImporter::getOptionsWidget()
{
	return m_optionsWidget;
}

QString HtmlBookmarksImporter::getTitle() const
{
	return QString(tr("HTML Bookmarks"));
}

QString HtmlBookmarksImporter::getDescription() const
{
	return QString(tr("Imports bookmarks from HTML file (Netscape format)."));
}

QString HtmlBookmarksImporter::getVersion() const
{
	return QLatin1String("1.0");
}

QString HtmlBookmarksImporter::getSuggestedPath() const
{
	return QString();
}

QString HtmlBookmarksImporter::getBrowser() const
{
	return QLatin1String("other");
}

bool HtmlBookmarksImporter::import()
{
	QWebPage page;

	handleOptions();

	page.mainFrame()->setHtml(m_file->readAll());

	processElement(page.mainFrame()->documentElement());

	return true;
}

bool HtmlBookmarksImporter::setPath(const QString &path, bool isPrefix)
{
	QUrl url(path);

	if (isPrefix)
	{
		url = url.resolved(QUrl(QLatin1String("bookmarks.html")));
	}

	if (m_file)
	{
		m_file->close();
		m_file->deleteLater();
	}

	m_file = new QFile(url.url(), this);

	return m_file->open(QIODevice::ReadOnly);
}

}