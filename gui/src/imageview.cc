#include <stdio.h>
#include <QResizeEvent>
#include "imageview.h"

ImageView::ImageView(QWidget *parent)
	: QGraphicsView(parent)
{
}

void ImageView::resizeEvent(QResizeEvent *ev)
{
	QGraphicsView::resizeEvent(ev);

	float sx = (float)rect().width() / scene()->sceneRect().width();
	float sy = (float)rect().height() / scene()->sceneRect().height();
	float s = sx < sy ? sx : sy;
	resetTransform();
	scale(s, s);
}
