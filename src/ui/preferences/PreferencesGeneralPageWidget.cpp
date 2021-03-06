/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2016 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Jan Bajer aka bajasoft <jbajer@gmail.com>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "PreferencesGeneralPageWidget.h"
#include "AcceptLanguageDialog.h"
#include "../Menu.h"
#include "../../core/Application.h"
#include "../../core/BookmarksManager.h"
#include "../../core/BookmarksModel.h"
#include "../../core/PlatformIntegration.h"
#include "../../core/SettingsManager.h"
#include "../../core/WindowsManager.h"

#include "ui_PreferencesGeneralPageWidget.h"

namespace Otter
{

PreferencesGeneralPageWidget::PreferencesGeneralPageWidget(QWidget *parent) : QWidget(parent),
	m_acceptLanguage(SettingsManager::getValue(QLatin1String("Network/AcceptLanguage")).toString()),
	m_ui(new Ui::PreferencesGeneralPageWidget)
{
	m_ui->setupUi(this);
	m_ui->startupBehaviorComboBox->addItem(tr("Continue previous session"), QLatin1String("continuePrevious"));
	m_ui->startupBehaviorComboBox->addItem(tr("Show startup dialog"), QLatin1String("showDialog"));
	m_ui->startupBehaviorComboBox->addItem(tr("Show home page"), QLatin1String("startHomePage"));
	m_ui->startupBehaviorComboBox->addItem(tr("Show start page"), QLatin1String("startStartPage"));
	m_ui->startupBehaviorComboBox->addItem(tr("Show empty page"), QLatin1String("startEmpty"));

	const int startupBehaviorIndex(m_ui->startupBehaviorComboBox->findData(SettingsManager::getValue(QLatin1String("Browser/StartupBehavior")).toString()));

	m_ui->startupBehaviorComboBox->setCurrentIndex((startupBehaviorIndex < 0) ? 0 : startupBehaviorIndex);
	m_ui->homePageLineEdit->setText(SettingsManager::getValue(QLatin1String("Browser/HomePage")).toString());

	Menu *bookmarksMenu(new Menu(Menu::BookmarkSelectorMenuRole, m_ui->useBookmarkAsHomePageButton));

	m_ui->useBookmarkAsHomePageButton->setMenu(bookmarksMenu);
	m_ui->useBookmarkAsHomePageButton->setEnabled(BookmarksManager::getModel()->getRootItem()->rowCount() > 0);
	m_ui->downloadsFilePathWidget->setSelectFile(false);
	m_ui->downloadsFilePathWidget->setPath(SettingsManager::getValue(QLatin1String("Paths/Downloads")).toString());
	m_ui->alwaysAskCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/AlwaysAskWhereToSaveDownload")).toBool());
	m_ui->tabsInsteadOfWindowsCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/OpenLinksInNewTab")).toBool());
	m_ui->delayTabsLoadingCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/DelayRestoringOfBackgroundTabs")).toBool());
	m_ui->reuseCurrentTabCheckBox->setChecked(SettingsManager::getValue(QLatin1String("Browser/ReuseCurrentTab")).toBool());
	m_ui->openNextToActiveheckBox->setChecked(SettingsManager::getValue(QLatin1String("TabBar/OpenNextToActive")).toBool());

	PlatformIntegration *integration(Application::getInstance()->getPlatformIntegration());

	if (integration == NULL || integration->isDefaultBrowser())
	{
		m_ui->setDefaultButton->setEnabled(false);
	}
	else if (!integration->canSetAsDefaultBrowser())
	{
		m_ui->setDefaultButton->setVisible(false);
		m_ui->systemDefaultLabel->setText(tr("Run Otter Browser with administrator rights to set it as a default browser."));
	}
	else
	{
		connect(m_ui->setDefaultButton, SIGNAL(clicked()), integration, SLOT(setAsDefaultBrowser()));
	}

	connect(bookmarksMenu, SIGNAL(triggered(QAction*)), this, SLOT(useBookmarkAsHomePage(QAction*)));
	connect(m_ui->useCurrentAsHomePageButton, SIGNAL(clicked()), this, SLOT(useCurrentAsHomePage()));
	connect(m_ui->restoreHomePageButton, SIGNAL(clicked()), this, SLOT(restoreHomePage()));
	connect(m_ui->acceptLanguageButton, SIGNAL(clicked()), this, SLOT(setupAcceptLanguage()));
}

PreferencesGeneralPageWidget::~PreferencesGeneralPageWidget()
{
	delete m_ui;
}

void PreferencesGeneralPageWidget::changeEvent(QEvent *event)
{
	QWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void PreferencesGeneralPageWidget::useCurrentAsHomePage()
{
	WindowsManager *manager(SessionsManager::getWindowsManager());

	if (manager)
	{
		m_ui->homePageLineEdit->setText(manager->getUrl().toString(QUrl::RemovePassword));
	}
}

void PreferencesGeneralPageWidget::useBookmarkAsHomePage(QAction *action)
{
	if (action)
	{
		const QString url(action->data().toModelIndex().data(BookmarksModel::UrlRole).toString());

		if (!url.isEmpty())
		{
			m_ui->homePageLineEdit->setText(url);
		}
		else
		{
			m_ui->homePageLineEdit->setText(QLatin1String("bookmarks:") + QString::number(action->data().toModelIndex().data(BookmarksModel::IdentifierRole).toULongLong()));
		}
	}
}

void PreferencesGeneralPageWidget::restoreHomePage()
{
	m_ui->homePageLineEdit->setText(SettingsManager::getDefinition(QLatin1String("Browser/HomePage")).defaultValue.toString());
}

void PreferencesGeneralPageWidget::setupAcceptLanguage()
{
	AcceptLanguageDialog dialog(m_acceptLanguage, this);

	if (dialog.exec() == QDialog::Accepted)
	{
		m_acceptLanguage = dialog.getLanguages();

		emit settingsModified();
	}
}

void PreferencesGeneralPageWidget::save()
{
	SettingsManager::setValue(QLatin1String("Browser/StartupBehavior"), m_ui->startupBehaviorComboBox->currentData().toString());
	SettingsManager::setValue(QLatin1String("Browser/HomePage"), m_ui->homePageLineEdit->text());
	SettingsManager::setValue(QLatin1String("Paths/Downloads"), m_ui->downloadsFilePathWidget->getPath());
	SettingsManager::setValue(QLatin1String("Browser/AlwaysAskWhereToSaveDownload"), m_ui->alwaysAskCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/OpenLinksInNewTab"), m_ui->tabsInsteadOfWindowsCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/DelayRestoringOfBackgroundTabs"), m_ui->delayTabsLoadingCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Browser/ReuseCurrentTab"), m_ui->reuseCurrentTabCheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("TabBar/OpenNextToActive"), m_ui->openNextToActiveheckBox->isChecked());
	SettingsManager::setValue(QLatin1String("Network/AcceptLanguage"), m_acceptLanguage);
}

}
