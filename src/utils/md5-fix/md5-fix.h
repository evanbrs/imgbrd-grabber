#ifndef MD5_FIX_H
#define MD5_FIX_H

#include <QDialog>
#include <QMap>



namespace Ui
{
	class md5Fix;
}



class md5Fix : public QDialog
{
	Q_OBJECT

	public:
		explicit md5Fix(QWidget *parent = 0);
		~md5Fix();

	private slots:
		void on_buttonCancel_clicked();
		void on_buttonStart_clicked();

	private:
		Ui::md5Fix *ui;
};

#endif // MD5_FIX_H
