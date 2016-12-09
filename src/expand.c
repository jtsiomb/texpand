#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <imago2.h>

static int find_nearest(int x, int y, struct img_pixmap *mask, int *resx, int *resy);

int expand(struct img_pixmap *res, struct img_pixmap *img, struct img_pixmap *mask)
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

				if(!find_nearest(j, i, mask, &nx, &ny)) {
					if(i == 0) {
						fprintf(stderr, "expand failed, empty mask?\n");
					}
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

static int find_nearest(int x, int y, struct img_pixmap *mask, int *resx, int *resy)
{
	int i, j, max_xoffs, max_yoffs, max_offs;
	int mindist = INT_MAX;
	int minx = -1, miny = -1;
	unsigned char *maskpix = mask->pixels;

	max_xoffs = x > mask->width / 2 ? x : mask->width - x;
	max_yoffs = y > mask->height / 2 ? y : mask->height - y;
	max_offs = max_xoffs > max_yoffs ? max_xoffs : max_yoffs;

	for(i=1; i<max_offs; i++) {
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
