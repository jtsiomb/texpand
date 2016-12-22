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

signals:
	void sig_expand_progress(float p);
	void sig_expand_done();

private slots:
	void expand_progress(float p);
	void expand_done();

	void socket_readable(int s);
	void on_bn_selmesh_clicked();
	void on_bn_gen_mask_clicked();
	void on_bn_seltex_clicked();
	void on_bn_save_mask_clicked();
	void on_bn_expand_clicked();
	void on_bn_save_exp_clicked();
	void on_bn_selmask_clicked();
};

#endif // MAINWIN_H
