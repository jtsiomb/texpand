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
#ifndef GENMASK_H_
#define GENMASK_H_

struct aiScene;
struct img_pixmap;

#ifdef __cplusplus
extern "C" {
#endif

/* all-in-one operation */
int mask_from_scene(struct img_pixmap *mask, int xsz, int ysz, const char *fname,
		int uvset, const char *filter);

/* sub-parts of the mask generation routine */
struct aiScene *load_scene(const char *fname);
void free_scene(struct aiScene *scn);
int gen_mask(struct img_pixmap *mask, int xsz, int ysz, struct aiScene *scn,
		int uvset, const char *filter);

#ifdef __cplusplus
}
#endif

#endif	/* GENMASK_H_ */
