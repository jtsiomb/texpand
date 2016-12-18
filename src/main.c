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
#include <string.h>
#include <imago2.h>

/* in genmask.c */
int mask_from_scene(struct img_pixmap *mask, int xsz, int ysz, const char *fname,
		int uvset, const char *filter);

/* in expand.c */
int expand(struct img_pixmap *res, int max_dist, struct img_pixmap *img, struct img_pixmap *mask);

static int mask_from_alpha(struct img_pixmap *mask, struct img_pixmap *img);
static float calc_usage(struct img_pixmap *mask);
static int parse_args(int argc, char **argv);

const char *opt_out_fname = "out.png";
const char *opt_tex_fname;
const char *opt_scene_fname;
const char *opt_mask_fname;
int opt_uvset;		/* texture coordinate set to use */
int opt_force;		/* force using all meshes regardless of texture filename matching in materials */
int opt_genmask;	/* just generate usage mask */
int opt_maskalpha;	/* use alpha channel as usage mask */
int opt_usage;		/* just print usage percentage */
int opt_radius = -1;/* how much to expand (negative values signify infinite expansion) */

static struct img_pixmap img;

int main(int argc, char **argv)
{
	struct img_pixmap mask, res;

	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	img_init(&img);
	img_init(&mask);

	if(img_load(&img, opt_tex_fname) == -1 || img_convert(&img, IMG_FMT_RGBAF) == -1) {
		fprintf(stderr, "failed to load image: %s\n", opt_tex_fname);
		return 1;
	}

	if(opt_mask_fname) {
		if(img_load(&mask, opt_mask_fname) == -1 || img_convert(&mask, IMG_FMT_GREY8)) {
			fprintf(stderr, "failed to load mask file: %s\n", opt_mask_fname);
			return 1;
		}
		if(img.width != mask.width || img.height != mask.height) {
			fprintf(stderr, "texture (%s) and mask (%s) dimensions differ\n", opt_tex_fname, opt_mask_fname);
			return 1;
		}

	} else if(opt_maskalpha) {
		if(!img_has_alpha(&img)) {
			fprintf(stderr, "maskalpha requested, but %s doesn't have an alpha channel\n", opt_tex_fname);
			return 1;
		}
		if(mask_from_alpha(&mask, &img) == -1) {
			return 1;
		}

	} else {
		const char *filter = 0;
		/* generate the mask from a mesh/scene file */
		if(!opt_scene_fname) {
			fprintf(stderr, "a mesh/scene file is required to generate the usage mask\n");
			return 1;
		}
		if(!opt_force) {
			char *ptr = strrchr(opt_tex_fname, '/');
			filter = ptr ? ptr + 1 : opt_tex_fname;
		}
		if(mask_from_scene(&mask, img.width, img.height, opt_scene_fname, opt_uvset, filter) == -1) {
			return 1;
		}
	}

	if(opt_usage) {
		/* calculate and print utilization */
		float usage = calc_usage(&mask);
		printf("%f\n", usage);
		return 0;
	}

	if(opt_genmask) {
		/* output the mask and exit */
		if(img_save(&mask, opt_out_fname) == -1) {
			fprintf(stderr, "failed to save mask file: %s\n", opt_out_fname);
			return 1;
		}
		return 0;
	}

	img_init(&res);
	if(img_copy(&res, &img) == -1) {
		fprintf(stderr, "failed to allocate result image\n");
		return 1;
	}
	expand(&res, opt_radius, &img, &mask);
	if(img_save(&res, opt_out_fname) == -1) {
		fprintf(stderr, "failed to write output file: %s\n", opt_out_fname);
		return 1;
	}
	return 0;
}

static int mask_from_alpha(struct img_pixmap *mask, struct img_pixmap *img)
{
	return -1;	/* TODO */
}

static float calc_usage(struct img_pixmap *mask)
{
	int i, area = mask->width * mask->height;
	int count = 0;
	unsigned char *pptr = mask->pixels;

	if(!area) return 0.0f;

	for(i=0; i<area; i++) {
		if(*pptr++ > 128) ++count;
	}
	return (float)count / (float)area;
}

static void print_usage(const char *progname, FILE *fp)
{
	fprintf(fp, "Usage: %s [options] <texture file>\n", progname);
	fprintf(fp, "Options:\n");
	fprintf(fp, "   -o <fname>: output filename\n");
	fprintf(fp, "   -uvset <n>: which UV set to use for mask generation (default: 0)\n");
	fprintf(fp, "   -radius <n>: maximum expansion radius in pixels\n");
	fprintf(fp, "   -force, -f: use all meshes in mask gen. without matching the texture filename\n");
	fprintf(fp, "   -genmask: output the texture usage mask\n");
	fprintf(fp, "   -mesh <fname>: use mesh/scene file for generating the texture usage mask\n");
	fprintf(fp, "   -mask <fname>: use a mask file instead of generating it from geometry mesh\n");
	fprintf(fp, "   -maskalpha: use alpha channel as the usage mask\n");
	fprintf(fp, "   -usage, -u: calculate and print texture space utilization [0, 1]\n");
	fprintf(fp, "   -help, -h: print usage information and exit\n");
	fprintf(fp, " (exactly one of -mesh, -mask, or -maskalpha must be specified).\n");
}

static int parse_args(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-o") == 0) {
				if(!argv[++i]) {
					fprintf(stderr, "-o must be followed by the output filename\n");
					return -1;
				}
				opt_out_fname = argv[i];

			} else if(strcmp(argv[i], "-uvset") == 0) {
				char *endp;
				opt_uvset = strtol(argv[++i], &endp, 10);
				if(*endp) {
					fprintf(stderr, "-uvset must be followed by a number\n");
					return -1;
				}

			} else if(strcmp(argv[i], "-radius") == 0) {
				char *endp;
				opt_radius = strtol(argv[++i], &endp, 10);
				if(*endp) {
					fprintf(stderr, "-radius must be followed by a number\n");
					return -1;
				}

			} else if(strcmp(argv[i], "-force") == 0 || strcmp(argv[i], "-f") == 0) {
				opt_force = 1;

			} else if(strcmp(argv[i], "-genmask") == 0) {
				opt_genmask = 1;

			} else if(strcmp(argv[i], "-mesh") == 0) {
				if(!argv[++i]) {
					fprintf(stderr, "-mesh must be followed by a filename\n");
					return -1;
				}
				opt_scene_fname = argv[i];

			} else if(strcmp(argv[i], "-mask") == 0) {
				if(!argv[++i]) {
					fprintf(stderr, "-mask must be followed by a filename\n");
					return -1;
				}
				opt_mask_fname = argv[i];

			} else if(strcmp(argv[i], "-maskalpha") == 0) {
				opt_maskalpha = 1;

			} else if(strcmp(argv[i], "-usage") == 0 || strcmp(argv[i], "-u") == 0) {
				opt_usage = 1;

			} else if(strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
				print_usage(argv[0], stdout);
				exit(0);

			} else {
				fprintf(stderr, "invalid option: %s\n\n", argv[i]);
				print_usage(argv[0], stderr);
				return -1;
			}

		} else {
			if(!opt_tex_fname) {
				opt_tex_fname = argv[i];

			} else {
				fprintf(stderr, "unexpected argument: %s\n\n", argv[i]);
				print_usage(argv[0], stderr);
				return -1;
			}
		}
	}

	if(!opt_scene_fname && !opt_mask_fname && !opt_maskalpha) {
		fprintf(stderr, "exactly one of -mesh, -mask, or -maskalpha must be specified\n");
		return -1;
	}

	return 0;
}
