#ifndef GENMASK_H_
#define GENMASK_H_

struct img_pixmap;

int mask_from_scene(struct img_pixmap *mask, int xsz, int ysz, const char *fname, int uvset, const char *filter);

#endif	/* GENMASK_H_ */
