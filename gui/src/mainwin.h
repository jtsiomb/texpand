#ifndef MAINWIN_H
#define MAINWIN_H

#include <QMainWindow>
#include <QSocketNotifier>
#include "genmask.h"

namespace Ui {
	class MainWin;
}

class MainWin : public QMainWindow
{
	Q_OBJECT

private:
	Ui::MainWin *ui;
	QSocketNotifier *sock_notifier;
	aiScene *scn;
	struct img_pixmap *mask;
	struct img_pixmap *in_tex;
	struct img_pixmap *out_tex;

	void precond_genmask();
	void precond_savemask();
	void precond_expand();
	void precond_save_expanded();

public:
	explicit MainWin(QWidget *parent = 0);
	~MainWin();

private slots:
	void socket_readable(int s);
	void on_bn_selmesh_clicked();
	void on_bn_gen_mask_clicked();
	void on_bn_seltex_clicked();
	void on_bn_save_mask_clicked();
	void on_bn_expand_clicked();
};

#endif // MAINWIN_H
