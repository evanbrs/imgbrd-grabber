#include <QApplication>
#include <QCommonStyle>
#include <QStyleOptionFrameV2>
#include <QWheelEvent>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QMenu>
#include <QTextDocumentFragment>
#include <QFile>
#include "textedit.h"
#include "functions.h"



TextEdit::TextEdit(Profile *profile, QWidget *parent)
	: QTextEdit(parent), c(0), m_profile(profile), m_favorites(profile->getFavorites()), m_viewItLater(profile->getKeptForLater())
{
	setTabChangesFocus(true);
	setWordWrapMode(QTextOption::NoWrap);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setFixedHeight(sizeHint().height());
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QTextEdit::customContextMenuRequested, this, &TextEdit::customContextMenuRequested);
}

TextEdit::~TextEdit()
{ }

QSize TextEdit::sizeHint() const
{
	QFontMetrics fm(font());
	int h = qMax(fm.height(), 14) + 4;
	int w = fm.width(QLatin1Char('x')) * 17 + 4;
	QStyleOptionFrameV2 opt;
	opt.initFrom(this);
	return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).expandedTo(QApplication::globalStrut()), this));
}

/**
 * Ignore all wheel events on the field.
 * @param e The Qt event.
 */
void TextEdit::wheelEvent(QWheelEvent *e)
{
	e->ignore();
}

/**
 * Colorize the contents of the text field.
 */
void TextEdit::doColor()
{
	QString txt = " "+this->toPlainText().toHtmlEscaped()+" ";

	// Color favorited tags
	for (Favorite fav : m_favorites)
		txt.replace(" "+fav.getName()+" ", " <span style=\"color:#ffc0cb\">"+fav.getName()+"</span> ");

	// Color metatags
	QRegExp r1(" ~([^ ]+)"),
			r2(" -([^ ]+)"),
			r3(" (user|fav|md5|pool|rating|source|status|approver|unlocked|sub|id|width|height|score|mpixels|filesize|date|gentags|arttags|chartags|copytags|status|status|approver|order|parent):([^ ]*)");
	int pos = 0;
	while ((pos = r1.indexIn(txt, pos)) != -1)
	{
		QString rep = " <span style=\"color:green\">~"+r1.cap(1)+"</span>";
		txt.replace(r1.cap(0), rep);
		pos += rep.length();
	}
	pos = 0;
	while ((pos = r2.indexIn(txt, pos)) != -1)
	{
		QString rep = " <span style=\"color:red\">-"+r2.cap(1)+"</span>";
		txt.replace(r2.cap(0), rep);
		pos += rep.length();
	}
	pos = 0;
	while ((pos = r3.indexIn(txt, pos)) != -1)
	{
		QString rep = " <span style=\"color:brown\">"+r3.cap(1)+":"+r3.cap(2)+"</span>";
		txt.replace(r3.cap(0), rep);
		pos += rep.length();
	}

	// Setup cursor
	QTextCursor crsr = textCursor();
	pos = crsr.columnNumber();
	int start = crsr.selectionStart();
	int end = crsr.selectionEnd();
	setHtml(txt.mid(1, txt.length()-2));
	//If the cursor is at the right side of (if any) selected text
	if (pos == end)
	{
		crsr.setPosition(start, QTextCursor::MoveAnchor);
		crsr.setPosition(end, QTextCursor::KeepAnchor);
	}
	else
	{
		crsr.setPosition(end, QTextCursor::MoveAnchor);
		crsr.setPosition(start, QTextCursor::KeepAnchor);
	}
	setTextCursor(crsr);
}

/**
 * Set the text of the field and color it.
 * @param text The text the field should be set to.
 */
void TextEdit::setText(const QString &text)
{
	QTextEdit::setText(text);
	doColor();
}

void TextEdit::setCompleter(QCompleter *completer)
{
	if (!completer)
		return;

	// Disconnect the previous completer
	if (c)
		QObject::disconnect(c, 0, this, 0);

	// Set the new completer and connect it to the field
	c = completer;
	c->setWidget(this);
	c->setCompletionMode(QCompleter::PopupCompletion);
	c->setCaseSensitivity(Qt::CaseInsensitive);
	QObject::connect(c, SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
}

QCompleter *TextEdit::completer() const
{
	return c;
}

void TextEdit::insertCompletion(const QString& completion)
{
	if (c->widget() != this)
		return;

	QTextCursor tc = textCursor();
	int extra = completion.length() - c->completionPrefix().length();
	tc.movePosition(QTextCursor::Left);
	tc.movePosition(QTextCursor::EndOfWord);
	tc.insertText(completion.right(extra));
	setTextCursor(tc);
}

QString TextEdit::textUnderCursor() const
{
	QTextCursor tc = textCursor();
	QString txt = ' ' + toPlainText() + ' ';
	int pos = tc.position();
	int i2 = txt.indexOf(' ', pos);
	int i1 = txt.lastIndexOf(' ', i2 - 1) + 1;
	return txt.mid(i1, i2 - i1);
}

void TextEdit::focusInEvent(QFocusEvent *e)
{
	if (c)
		c->setWidget(this);

	QTextEdit::focusInEvent(e);
}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
	if (c && c->popup()->isVisible())
	{
		// The following keys are forwarded by the completer to the widget
		QString curr = c->popup()->currentIndex().data().toString(), under = textUnderCursor();
		switch (e->key())
		{
			case Qt::Key_Enter:
			case Qt::Key_Return:
				c->popup()->hide();
				if (curr == "" || under == curr)
				{ emit returnPressed(); }
				else
				{
					insertCompletion(curr);
					doColor();
				}
				return;

			case Qt::Key_Escape:
			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				e->ignore();
				return;
		}
	}

	bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space); // CTRL+Space
	if (!c || !isShortcut) // do not process the shortcut when we have a completer
	{
		if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
		{
			emit returnPressed();
			return;
		}
		QTextEdit::keyPressEvent(e);
	}
	doColor();
	
	const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
	if (!c || (ctrlOrShift && e->text().isEmpty()))
		return;
	
	static QString eow(" ");
	bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
	QString completionPrefix = textUnderCursor();
	
	if (!isShortcut && (hasModifier || e->text().isEmpty()|| completionPrefix.length() < 3 || eow.contains(e->text().right(1))))
	{
		c->popup()->hide();
		return;
	}
	
	if (completionPrefix != c->completionPrefix())
		c->setCompletionPrefix(completionPrefix);

	QRect cr = cursorRect();
	cr.setWidth(c->popup()->sizeHintForColumn(0) + c->popup()->verticalScrollBar()->sizeHint().width());
	c->complete(cr);
}

void TextEdit::customContextMenuRequested(QPoint)
{
	QMenu *menu = new QMenu(this);
		QMenu *favs = new QMenu(tr("Favoris"), menu);
			QActionGroup* favsGroup = new QActionGroup(favs);
				favsGroup->setExclusive(true);
				connect(favsGroup, SIGNAL(triggered(QAction *)), this, SLOT(insertFav(QAction *)));
				for (Favorite fav : m_favorites)
				{ favsGroup->addAction(fav.getName()); }
				if (!toPlainText().isEmpty())
				{
					if (m_favorites.contains(toPlainText()))
					{ favs->addAction(QIcon(":/images/icons/remove.png"), tr("Retirer"), this, SLOT(unsetFavorite())); }
					else
					{ favs->addAction(QIcon(":/images/icons/add.png"), tr("Ajouter"), this, SLOT(setFavorite())); }
					favs->addSeparator();
				}
				favs->addActions(favsGroup->actions());
				favs->setIcon(QIcon(":/images/icons/favorite.png"));
				favs->setStyleSheet("* { menu-scrollable: 1 }");
			menu->addMenu(favs);
		QMenu *vils = new QMenu(tr("Gardés pour plus tard"), menu);
			QActionGroup* vilsGroup = new QActionGroup(vils);
				vilsGroup->setExclusive(true);
				connect(vilsGroup, SIGNAL(triggered(QAction *)), this, SLOT(insertFav(QAction *)));
				for (QString viewItLater : m_viewItLater)
				{ vilsGroup->addAction(viewItLater); }
				if (!toPlainText().isEmpty())
				{
					if (m_viewItLater.contains(toPlainText()))
					{ vils->addAction(QIcon(":/images/icons/remove.png"), tr("Retirer"), this, SLOT(unsetKfl())); }
					else
					{ vils->addAction(QIcon(":/images/icons/add.png"), tr("Ajouter"), this, SLOT(setKfl())); }
					vils->addSeparator();
				}
				vils->addActions(vilsGroup->actions());
				vils->setIcon(QIcon(":/images/icons/book.png"));
			menu->addMenu(vils);
		QMenu *ratings = new QMenu(tr("Classes"), menu);
			QActionGroup* ratingsGroup = new QActionGroup(favs);
				ratingsGroup->setExclusive(true);
				connect(ratingsGroup, SIGNAL(triggered(QAction *)), this, SLOT(insertFav(QAction *)));
					ratingsGroup->addAction(QIcon(":/images/ratings/safe.png"), "rating:safe");
					ratingsGroup->addAction(QIcon(":/images/ratings/questionable.png"), "rating:questionable");
					ratingsGroup->addAction(QIcon(":/images/ratings/explicit.png"), "rating:explicit");
				ratings->addActions(ratingsGroup->actions());
				ratings->setIcon(QIcon(":/images/ratings/none.png"));
			menu->addMenu(ratings);
		QMenu *sortings = new QMenu(tr("Tris"), menu);
			QActionGroup* sortingsGroup = new QActionGroup(favs);
				sortingsGroup->setExclusive(true);
				connect(sortingsGroup, SIGNAL(triggered(QAction *)), this, SLOT(insertFav(QAction *)));
					sortingsGroup->addAction(QIcon(":/images/sortings/change.png"), "order:change");
					sortingsGroup->addAction(QIcon(":/images/sortings/change.png"), "order:change_desc");
					sortingsGroup->addAction(QIcon(":/images/icons/favorite.png"), "order:favcount");
					sortingsGroup->addAction(QIcon(":/images/sortings/size.png"), "order:filesize");
					sortingsGroup->addAction(QIcon(":/images/sortings/id.png"), "order:id");
					sortingsGroup->addAction(QIcon(":/images/sortings/id.png"), "order:id_desc");
					sortingsGroup->addAction(QIcon(":/images/sortings/landscape.png"), "order:landscape");
					sortingsGroup->addAction(QIcon(":/images/sortings/pixels.png"), "order:mpixels");
					sortingsGroup->addAction(QIcon(":/images/sortings/pixels.png"), "order:mpixels_asc");
					sortingsGroup->addAction(QIcon(":/images/sortings/portrait.png"), "order:portrait");
					sortingsGroup->addAction(QIcon(":/images/icons/favorite.png"), "order:rank");
					sortingsGroup->addAction(QIcon(":/images/sortings/score.png"), "order:score");
					sortingsGroup->addAction(QIcon(":/images/sortings/score.png"), "order:score_asc");
				sortings->addActions(sortingsGroup->actions());
				sortings->setIcon(QIcon(":/images/sortings/sort.png"));
			menu->addMenu(sortings);
		menu->addSeparator();
			if (!textCursor().selection().isEmpty())
			{
				menu->addAction(tr("Copier"), this, SLOT(copy()), QKeySequence::Copy);
				menu->addAction(tr("Couper"), this, SLOT(cut()), QKeySequence::Cut);
			}
			menu->addAction(tr("Coller"), this, SLOT(paste()), QKeySequence::Paste);
	menu->exec(QCursor::pos());
}

void TextEdit::setFavorite()
{
	m_profile->addFavorite(Favorite(toPlainText()));
}
void TextEdit::unsetFavorite()
{
	m_profile->removeFavorite(Favorite(toPlainText()));
}
void TextEdit::setKfl()
{
	m_profile->addKeptForLater(toPlainText());
}
void TextEdit::unsetKfl()
{
	m_profile->removeKeptForLater(toPlainText());
}

void TextEdit::insertFav(QAction *act)
{
	QString text = act->text();

	QTextCursor cursor = this->textCursor();
	int pos = cursor.columnNumber();

	QString txt = this->toPlainText();
	if (!cursor.hasSelection())
	{
		if (pos == 0 && (txt.count() == 0 || txt[0] != ' '))
			text.append(' ');
		if (pos == txt.count() && txt[txt.count() - 1] != ' ')
			text.prepend(' ');
		this->setPlainText(txt.mid(0, pos) + text + txt.mid(pos));
	}
	else
	{ this->setPlainText(txt.mid(0, cursor.selectionStart()) + text + txt.mid(cursor.selectionEnd())); }

	cursor.clearSelection();
	cursor.setPosition(pos + text.length(), QTextCursor::KeepAnchor);

	this->setTextCursor(cursor);
	this->doColor();
}
