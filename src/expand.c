/*
texpand - Texture pre-processing tool for expanding texels, to avoid filtering artifacts.
Copyright (C) 2016-2017  John Tsiombikas <nuclear@member.fsf.org>

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
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <imago2.h>
#include "expand.h"

static int find_nearest(int x, int y, struct img_pixmap *mask, int max_dist, int *resx, int *resy);

int expand(struct img_pixmap *res, int max_dist, struct img_pixmap *img, struct img_pixmap *mask)
{
	return expand_scanlines(res, 0, img->height, max_dist, img, mask);
}


int expand_scanlines(struct img_pixmap *res, int ystart, int ycount, int max_dist, struct img_pixmap *img, struct img_pixmap *mask)
{
	int i, width = res->width;

	assert(img->fmt == IMG_FMT_RGBAF);
	assert(res->fmt == IMG_FMT_RGBAF);
	assert(mask->fmt == IMG_FMT_GREY8);

#pragma omp parallel for schedule(dynamic)
	for(i=0; i<ycount; i++) {
		int j, y = i + ystart;
		unsigned char *maskptr = (unsigned char*)mask->pixels + y * width;
		float *dest = (float*)res->pixels + y * width * 4;

		for(j=0; j<res->width; j++) {
			if(*maskptr != 0xff) {
				int nx, ny;
				float *src;

				if(!find_nearest(j, y, mask, max_dist, &nx, &ny)) {
					break;
				}

				src = (float*)img->pixels + (ny * width + nx) * 4;
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
			}
			dest += 4;
			++maskptr;
		}
	}
	return 0;
}

#define GET_PIXEL(mask, x, y) (((unsigned char*)(mask)->pixels)[(y) * (mask)->width + (x)])

static int find_nearest(int x, int y, struct img_pixmap *mask, int max_dist, int *resx, int *resy)
{
	int i, j, startx, starty, endx, endy, px, py, min_px = -1, min_py;
	int min_distsq = INT_MAX;
	int bwidth, bheight;

	startx = x >= max_dist ? x - max_dist : 0;
	starty = y >= max_dist ? y - max_dist : 0;
	endx = x + max_dist < mask->width ? x + max_dist : mask->width - 1;
	endy = y + max_dist < mask->height ? y + max_dist : mask->height - 1;

	/* try the cardinal directions first to find the search bounding box */
	for(i=0; i<4; i++) {
		int max_dist = x - startx;
		for(j=0; j<max_dist; j++) {
			if(GET_PIXEL(mask, x - j, y) == 0xff) {
				startx = x - j;
				break;
			}
		}
		max_dist = endx + 1 - x;
		for(j=0; j<max_dist; j++) {
			if(GET_PIXEL(mask, x + j, y) == 0xff) {
				endx = x + j;
				break;
			}
		}
		max_dist = y - starty;
		for(j=0; j<max_dist; j++) {
			if(GET_PIXEL(mask, x, y - j) == 0xff) {
				starty = y - j;
				break;
			}
		}
		max_dist = endy + 1 - y;
		for(j=0; j<max_dist; j++) {
			if(GET_PIXEL(mask, x, y + j) == 0xff) {
				endy = y + j;
				break;
			}
		}
	}

	/* find the nearest */
	bwidth = endx + 1 - startx;
	bheight = endy + 1 - starty;

	py = starty;
	for(i=0; i<bheight; i++) {
		px = startx;
		for(j=0; j<bwidth; j++) {
			if(GET_PIXEL(mask, px, py) == 0xff) {
				int dx = px - x;
				int dy = py - y;
				int distsq = dx * dx + dy * dy;

				if(distsq < min_distsq) {
					min_distsq = distsq;
					min_px = px;
					min_py = py;
				}
			}
			++px;
		}
		++py;
	}

	if(min_px != -1) {
		*resx = min_px;
		*resy = min_py;
		return 1;
	}
	return 0;
}
