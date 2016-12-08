#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <imago2.h>
#include "genmask.h"

static int mask_from_alpha(struct img_pixmap *mask, struct img_pixmap *img);
static int parse_args(int argc, char **argv);

const char *opt_out_fname = "out.png";
const char *opt_tex_fname;
const char *opt_scene_fname;
const char *opt_mask_fname;
int opt_uvset;		/* texture coordinate set to use */
int opt_force;		/* force using all meshes regardless of texture filename matching in materials */
int opt_genmask;	/* just generate usage mask */
int opt_maskalpha;	/* use alpha channel as usage mask */

static struct img_pixmap img;

int main(int argc, char **argv)
{
	struct img_pixmap mask;

	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	img_init(&img);
	img_init(&mask);

	if(img_load(&img, opt_tex_fname) == -1) {
		fprintf(stderr, "failed to load image: %s\n", opt_tex_fname);
		return 1;
	}
	if(opt_mask_fname) {
		if(img_load(&mask, opt_mask_fname) == -1) {
			fprintf(stderr, "failed to load mask file: %s\n", opt_mask_fname);
			return 1;
		}
		if(img.width != mask.width || img.height != mask.height) {
			fprintf(stderr, "texture (%s) and mask (%s) dimensions differ\n", opt_tex_fname, opt_mask_fname);
			return 1;
		}
		img_convert(&mask, IMG_FMT_GREY8);

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

	if(opt_genmask) {
		/* output the mask and exit */
		if(img_save(&mask, opt_out_fname) == -1) {
			fprintf(stderr, "failed to save mask file: %s\n", opt_out_fname);
			return 1;
		}
		return 0;
	}

	/* TODO continue */

	return 0;
}

static int mask_from_alpha(struct img_pixmap *mask, struct img_pixmap *img)
{
	return -1;	/* TODO */
}

static void print_usage(const char *progname, FILE *fp)
{
	fprintf(fp, "Usage: %s [options] <texture file>\n", progname);
	fprintf(fp, "Options:\n");
	fprintf(fp, "   -o <fname>: specify output filename\n");
	fprintf(fp, "   -uvset <n>: specify which UV set to use (default: 0)\n");
	fprintf(fp, "   -force, -f: use of all meshes specified without matching the texture filename\n");
	fprintf(fp, "   -genmask: output the texture usage mask\n");
	fprintf(fp, "   -mesh <fname>: use mesh/scene file for generating the texture usage mask\n");
	fprintf(fp, "   -mask <fname>: use a mask file instead of generating it from geometry mesh\n");
	fprintf(fp, "   -maskalpha: use alpha channel as the usage mask\n");
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
