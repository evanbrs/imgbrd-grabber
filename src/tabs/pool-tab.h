#ifndef POOL_TAB_H
#define POOL_TAB_H

#include <QWidget>
#include <QMap>
#include <QCalendarWidget>
#include "ui/textedit.h"
#include "models/page.h"
#include "search-tab.h"



namespace Ui
{
	class poolTab;
}


class mainWindow;

class poolTab : public searchTab
{
	Q_OBJECT

	public:
		explicit poolTab(int id, QMap<QString,Site*> *sites, Profile *profile, mainWindow *parent);
		~poolTab();
		Ui::poolTab *ui;
		QString tags();
		QString wiki();
		QString site();
		int imagesPerPage();
		int columns();
		QString postFilter();

	public slots:
		// Search
		void firstPage();
		void previousPage();
		void nextPage();
		void lastPage();
		// Zooms
		void setTags(QString);
		void setPool(int id, QString site);
		// Loading
		void load();
		void finishedLoading(Page*);
		void finishedLoadingTags(Page*);
		// Batch
		void getPage();
		void getAll();
		// Others
		void optionsChanged();
		void closeEvent(QCloseEvent*);
		void on_buttonSearch_clicked();
		void setSite(QString);
		void setImagesPerPage(int ipp);
		void setColumns(int columns);
		void setPostFilter(QString postfilter);
		void focusSearch();

	private:
		int				m_id;
		TextEdit		*m_search, *m_postFiltering;
		QCalendarWidget	*m_calendar;
		QString			m_lastTags, m_wiki;
		bool			m_sized;
};

#endif // POOL_TAB_H
