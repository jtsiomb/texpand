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
#ifndef EXPAND_H_
#define EXPAND_H_

struct img_pixmap;

#ifdef __cplusplus
extern "C" {
#endif

int expand(struct img_pixmap *res, int max_dist, struct img_pixmap *img,
		struct img_pixmap *mask);

int expand_scanlines(struct img_pixmap *res, int y, int ycount, int max_dist,
		struct img_pixmap *img, struct img_pixmap *mask);

#ifdef __cplusplus
}
#endif

#endif	/* EXPAND_H_ */
