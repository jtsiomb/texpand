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
