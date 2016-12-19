#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <imago2.h>
#include <QFileDialog>
#include <QMessageBox>
#include "mainwin.h"
#include "ui_mainwin.h"
#include "genmask.h"

#define IMAGE_VALID(img) (img && img->pixels && img->width > 0 && img->height > 0)

static void update_image_widget(QLabel *w, struct img_pixmap *img);

static int pfd[2];

MainWin::MainWin(QWidget *parent)
	: QMainWindow(parent)
{
	ui = new Ui::MainWin;
	ui->setupUi(this);

	scn = 0;
	in_tex = img_create();
	out_tex = img_create();
	mask = img_create();

	// redirect stdout/stderr to the log output widget
	pipe(pfd);
	::close(1);
	::close(2);
	dup2(pfd[1], 1);
	dup2(pfd[1], 2);

	fcntl(pfd[0], F_SETFL, fcntl(pfd[0], F_GETFL) | O_NONBLOCK);
	sock_notifier = new QSocketNotifier(pfd[0], QSocketNotifier::Read);
	connect(sock_notifier, &QSocketNotifier::activated, this, &MainWin::socket_readable);
}

MainWin::~MainWin()
{
	delete ui;
	delete sock_notifier;

	if(scn) free_scene(scn);
	if(in_tex) img_free(in_tex);
	if(out_tex) img_free(out_tex);
	if(mask) img_free(mask);
}

void MainWin::socket_readable(int s)
{
	int sz;
	char buf[256];
	while((sz = read(s, buf, sizeof buf - 1)) > 0) {
		buf[sz] = 0;
		ui->ed_log->insertPlainText(QString(buf));
	}
}

void MainWin::on_bn_selmesh_clicked()
{
	aiScene *new_scn;

	QString fname = QFileDialog::getOpenFileName(this, "Open mesh/scene file");
	if(!fname.isEmpty() && (new_scn = load_scene(fname.toUtf8().data()))) {
		free_scene(scn);
		scn = new_scn;

		ui->tx_meshfile->setText(fname);
		ui->bn_gen_mask->setEnabled(true);
		ui->spin_uvset->setEnabled(true);
	}
}

void MainWin::on_bn_gen_mask_clicked()
{
	if(!IMAGE_VALID(in_tex)) {
		QMessageBox::critical(this, "Mask generation error", "You need to open an input texture before generating the mask");
		return;
	}
	if(!mask) {
		mask = img_create();
		assert(mask);
	}
	int uvset = ui->spin_uvset->value();
	if(gen_mask(mask, in_tex->width, in_tex->height, scn, uvset, 0) == -1) {	// TODO filter
		QMessageBox::critical(this, "Mask generation error", "Failed to generate mask. See output log for details.");
		return;
	}

	update_image_widget(ui->img_mask, mask);
	ui->bn_expand->setEnabled(true);
}

void MainWin::on_bn_seltex_clicked()
{
	QString fname = QFileDialog::getOpenFileName(this, "Open input texture", "Images (*.png *.jpg *.jpeg *.tga *.ppm)");
	if(!fname.isEmpty()) {
		const char *cfname = fname.toUtf8().data();
		if(img_load(in_tex, cfname) == -1 || img_convert(in_tex, IMG_FMT_RGBAF) == -1) {
			fprintf(stderr, "Failed to load image: %s\n", cfname);
			QMessageBox::critical(this, "Image loading error", "Failed to load image: " + fname);
			return;
		}

		update_image_widget(ui->img_input, in_tex);

		if(IMAGE_VALID(mask)) {
			if(mask->width == in_tex->width && mask->height == in_tex->height) {
				ui->bn_expand->setEnabled(true);
			} else {
				img_destroy(mask);
				img_init(mask);
				update_image_widget(ui->img_mask, mask);
			}
		}
	}
}

void MainWin::on_bn_save_mask_clicked()
{
	assert(IMAGE_VALID(mask));
}


static void update_image_widget(QLabel *w, struct img_pixmap *img)
{
	struct img_pixmap tmp;

	if(!IMAGE_VALID(img)) {
		w->setPixmap(QPixmap());
		return;
	}

	img_init(&tmp);
	if(img_is_float(img)) {
		img_copy(&tmp, img);
		img_to_integer(&tmp);
		img = &tmp;
	}

	QImage::Format qfmt;
	switch(img->fmt) {
	case IMG_FMT_GREY8:
		qfmt = QImage::Format_Grayscale8;
		break;
	case IMG_FMT_RGB24:
		qfmt = QImage::Format_RGB888;
		break;
	case IMG_FMT_RGBA32:
		qfmt = QImage::Format_RGBA8888;
		break;
	default:
		fprintf(stderr, "update_image_widget: unsupported pixmap format!\n");
		return;
	}

	QImage qimg((unsigned char*)img->pixels, img->width, img->height, qfmt);
	w->setPixmap(QPixmap::fromImage(qimg));
	img_destroy(&tmp);
}
