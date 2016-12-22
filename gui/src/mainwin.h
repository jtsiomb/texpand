/*
texpand - Texture pre-processing tool for expanding texels, to avoid filtering artifacts.
Copyright (C) 2016  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
