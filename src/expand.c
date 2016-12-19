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
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <imago2.h>
#include "expand.h"

static int find_nearest(int x, int y, struct img_pixmap *mask, int max_dist, int *resx, int *resy);

int expand(struct img_pixmap *res, int max_dist, struct img_pixmap *img, struct img_pixmap *mask)
{
	int i, width = res->width;

	assert(img->fmt == IMG_FMT_RGBAF);
	assert(res->fmt == IMG_FMT_RGBAF);
	assert(mask->fmt == IMG_FMT_GREY8);

#pragma omp parallel for schedule(dynamic)
	for(i=0; i<res->height; i++) {
		int j;
		unsigned char *maskptr = (unsigned char*)mask->pixels + i * width;
		float *dest = (float*)res->pixels + i * width * 4;

		for(j=0; j<res->width; j++) {
			if(*maskptr != 0xff) {
				int nx, ny;
				float *src;

				if(!find_nearest(j, i, mask, max_dist, &nx, &ny)) {
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

/* calculates the offset (dx, dy) of the idx'th point around the origin on
 * the dist'th ring, and returns the coordinates and its manhattan distance
 */
static int pixoffs(int dist, int idx, int *resx, int *resy)
{
	int dx, dy;
	int side_len = dist * 2;
	int side = idx / side_len;
	int sidx = idx % side_len;

	if(side & 1) { /* odd sides are vertical, swap dx<->dy */
		dy = sidx - dist;
		dx = dist;
	} else {
		dx = sidx - dist;
		dy = dist;
	}

	*resx = side >= 2 ? -dx : dx;	/* last two sides are increasing in reverse ... I guess */
	*resy = side == 1 || side == 2 ? -dy : dy;	/* fuck it... just draw the diagram ... */
	return abs(dx) + abs(dy);	/* return manhattan distance */
}

static int find_nearest(int x, int y, struct img_pixmap *mask, int max_dist, int *resx, int *resy)
{
	int i, j, max_xoffs, max_yoffs;
	int mindist = INT_MAX;
	int minx = -1, miny = -1;
	unsigned char *maskpix = mask->pixels;

	if(max_dist <= 0) {
		max_xoffs = x > mask->width / 2 ? x : mask->width - x;
		max_yoffs = y > mask->height / 2 ? y : mask->height - y;
		max_dist = max_xoffs > max_yoffs ? max_xoffs : max_yoffs;
	}

	for(i=1; i<max_dist; i++) {
		int npix = i * 8;

		for(j=0; j<npix; j++) {
			int xoffs, yoffs;
			int dist = pixoffs(i, j, &xoffs, &yoffs);
			int px = x + xoffs;
			int py = y + yoffs;

			if(px < 0 || px >= mask->width || py < 0 || py >= mask->height) {
				continue;
			}

			if(maskpix[py * mask->width + px] == 0xff && dist < mindist) {
				minx = px;
				miny = py;
			}
		}

		if(minx != -1) {
			*resx = minx;
			*resy = miny;
			return 1;
		}
	}
	return 0;
}
