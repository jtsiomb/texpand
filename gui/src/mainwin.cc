#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <imago2.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include "mainwin.h"
#include "ui_mainwin.h"
#include "genmask.h"
#include "expand.h"

#define IMAGES_SUFFIX_FILTER "Images (*.png *.jpg *.jpeg *.tga *.ppm)"
#define IMAGE_VALID(img) (img && img->pixels && img->width > 0 && img->height > 0)

static bool update_image_widget(QGraphicsView *gview, struct img_pixmap *img);

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

	ui->gview_input->setScene(new QGraphicsScene);
	ui->gview_output->setScene(new QGraphicsScene);
	ui->gview_mask->setScene(new QGraphicsScene);

	// redirect stdout/stderr to the log output widget
	pipe(pfd);
	::close(1);
	::close(2);
	dup2(pfd[1], 1);
	dup2(pfd[1], 2);

	setvbuf(stdout, 0, _IONBF, 0);

	fcntl(pfd[0], F_SETFL, fcntl(pfd[0], F_GETFL) | O_NONBLOCK);
	sock_notifier = new QSocketNotifier(pfd[0], QSocketNotifier::Read);
	connect(sock_notifier, &QSocketNotifier::activated, this, &MainWin::socket_readable);

	connect(this, &MainWin::sig_expand_progress, this, &MainWin::expand_progress);
	connect(this, &MainWin::sig_expand_done, this, &MainWin::expand_done);
}

MainWin::~MainWin()
{
	delete ui->gview_input->scene();
	delete ui->gview_output->scene();
	delete ui->gview_mask->scene();

	delete ui;
	delete sock_notifier;

	if(scn) free_scene(scn);
	if(in_tex) img_free(in_tex);
	if(out_tex) img_free(out_tex);
	if(mask) img_free(mask);
}

// --- slots ---
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
		ui->spin_uvset->setEnabled(true);
		precond_genmask();
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

	update_image_widget(ui->gview_mask, mask);
	precond_expand();
	precond_savemask();
}

// select input texture button
void MainWin::on_bn_seltex_clicked()
{
	QString fname = QFileDialog::getOpenFileName(this, "Open input texture", QString(), IMAGES_SUFFIX_FILTER);
	if(!fname.isEmpty()) {
		const char *cfname = fname.toUtf8().data();
		if(img_load(in_tex, cfname) == -1 || img_convert(in_tex, IMG_FMT_RGBAF) == -1) {
			fprintf(stderr, "Failed to load image: %s\n", cfname);
			QMessageBox::critical(this, "Image loading error", "Failed to load image: " + fname);
			return;
		}

		update_image_widget(ui->gview_input, in_tex);

		if(IMAGE_VALID(mask)) {
			if(mask->width != in_tex->width && mask->height != in_tex->height) {
				img_destroy(mask);
				img_init(mask);
				update_image_widget(ui->gview_mask, mask);
			}
		}
		precond_genmask();
		precond_expand();
	}
}

void MainWin::on_bn_save_mask_clicked()
{
	assert(IMAGE_VALID(mask));

	QString fname = QFileDialog::getSaveFileName(this, "Save mask image", QString(), IMAGES_SUFFIX_FILTER);
	if(!fname.isEmpty()) {
		const char *cfname = fname.toUtf8().data();
		if(img_save(mask, cfname) == -1) {
			fprintf(stderr, "Failed to save mask image: %s\n", cfname);
			QMessageBox::critical(this, "Image save error", "Failed to save mask: " + fname);
			return;
		} else {
			printf("Mask saved to: %s\n", cfname);
		}
	}
}

void MainWin::on_bn_save_exp_clicked()
{
	assert(IMAGE_VALID(out_tex));

	QString fname = QFileDialog::getSaveFileName(this, "Save expanded image", QString(), IMAGES_SUFFIX_FILTER);
	if(!fname.isEmpty()) {
		const char *cfname = fname.toUtf8().data();
		if(img_save(out_tex, cfname) == -1) {
			fprintf(stderr, "Failed to save expanded image: %s\n", cfname);
			QMessageBox::critical(this, "Image save error", "Failed to save expanded image: " + fname);
			return;
		} else {
			printf("Expanded image saved to: %s\n", cfname);
		}
	}
}

struct ExpandData {
	img_pixmap *input, *output, *mask;
	int radius;
	MainWin *win;
	bool cancel;
} expand_data;

#define BLOCKSZ	32

static void thread_func()
{
	int height = expand_data.input->height;
	int idx = 0;
	while(idx < height) {
		if(expand_data.cancel) {
			break;
		}
		emit expand_data.win->sig_expand_progress((float)idx / (float)height);
		int ysz = std::min(height - idx, BLOCKSZ);
		expand_scanlines(expand_data.output, idx, ysz, expand_data.radius,
				expand_data.input, expand_data.mask);
		idx += ysz;
	}
	emit expand_data.win->sig_expand_done();
}

void MainWin::expand_progress(float p)
{
	ui->progr_expand->setValue((int)(p * 100.0f));
}

void MainWin::expand_done()
{
	ui->progr_expand->setValue(100);
	ui->progr_expand->setEnabled(false);
	ui->bn_expand->setText("Expand");

	expand_data.input = 0;
	if(!expand_data.cancel) {
		update_image_widget(ui->gview_output, out_tex);
		precond_save_expanded();
	}
}

void MainWin::on_bn_expand_clicked()
{
	if(!expand_data.input) {
		ui->bn_expand->setText("Stop");
		ui->progr_expand->setValue(0);
		ui->progr_expand->setEnabled(true);

		expand_data.input = in_tex;
		expand_data.output = out_tex;
		expand_data.mask = mask;
		expand_data.radius = ui->chk_rad_inf->isChecked() ? -1 : ui->spin_radius->value();
		expand_data.win = this;
		expand_data.cancel = false;

		printf("expanding image %dx%d\n", in_tex->width, in_tex->height);

		img_copy(out_tex, in_tex);
		std::thread thr{thread_func};
		thr.detach();
	} else {
		expand_data.cancel = true;
	}
}

// --- private ---
void MainWin::precond_genmask()
{
	ui->bn_gen_mask->setEnabled(IMAGE_VALID(in_tex) && scn);
}

void MainWin::precond_savemask()
{
	ui->bn_save_mask->setEnabled(IMAGE_VALID(mask));
}

void MainWin::precond_expand()
{
	ui->bn_expand->setEnabled(IMAGE_VALID(mask) && IMAGE_VALID(in_tex));
}

void MainWin::precond_save_expanded()
{
	ui->bn_save_exp->setEnabled(IMAGE_VALID(out_tex));
}

// --- static ---
static bool update_image_widget(QGraphicsView *gview, struct img_pixmap *img)
{
	struct img_pixmap tmp;

	if(!IMAGE_VALID(img)) {
		gview->scene()->clear();
		return false;
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
		return false;
	}

	QImage qimg((unsigned char*)img->pixels, img->width, img->height, qfmt);

	QGraphicsScene *gs = gview->scene();
	gs->clear();
	gs->addPixmap(QPixmap::fromImage(qimg));

	img_destroy(&tmp);

	// calculate a scaling factor to fit the image in the view
	float sx = (float)gview->rect().width() / gs->sceneRect().width();
	float sy = (float)gview->rect().height() / gs->sceneRect().height();
	float s = sx < sy ? sx : sy;
	gview->resetTransform();
	gview->scale(s, s);
	return true;
}
