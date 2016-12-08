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
#include <imago2.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glut.h>
#include <GL/glext.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <assimp/vector3.h>

static int uses_texture(const struct aiScene *scn, const struct aiMesh *mesh, const char *texname);

static int init_gl(int xsz, int ysz);
static void destroy_gl(void);
static void draw_uvmesh(const struct aiMesh *mesh, int uvset);

int mask_from_scene(struct img_pixmap *mask, int xsz, int ysz, const char *fname,
		int uvset, const char *filter)
{
	int i;
	const struct aiScene *scn;
	unsigned int ppflags = aiProcess_Triangulate | aiProcess_SortByPType |
		aiProcess_GenUVCoords | aiProcess_TransformUVCoords | aiProcess_FlipUVs;

	if(!(scn = aiImportFile(fname, ppflags))) {
		fprintf(stderr, "failed to load scene file: %s\n", fname);
		return -1;
	}

	if(img_set_pixels(mask, xsz, ysz, IMG_FMT_GREY8, 0) == -1) {
		fprintf(stderr, "failed to allocate mask image\n");
		return -1;
	}

	if(init_gl(xsz, ysz) == -1) {
		fprintf(stderr, "failed to initialize OpenGL\n");
		return -1;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	for(i=0; i<(int)scn->mNumMeshes; i++) {
		const struct aiMesh *mesh = scn->mMeshes[i];
		if(filter && !uses_texture(scn, mesh, filter)) {
			continue;
		}
		draw_uvmesh(mesh, uvset);
	}

	glReadPixels(0, 0, xsz, ysz, GL_LUMINANCE, GL_UNSIGNED_BYTE, mask->pixels);

	destroy_gl();
	aiReleaseImport(scn);
	return 0;
}

static const enum aiTextureType types[] = {
	aiTextureType_NONE,
	aiTextureType_DIFFUSE,
	aiTextureType_SPECULAR,
	aiTextureType_AMBIENT,
	aiTextureType_EMISSIVE,
	aiTextureType_HEIGHT,
	aiTextureType_NORMALS,
	aiTextureType_SHININESS,
	aiTextureType_OPACITY,
	aiTextureType_DISPLACEMENT,
	aiTextureType_LIGHTMAP,
	aiTextureType_REFLECTION,
	aiTextureType_UNKNOWN
};

static int uses_texture(const struct aiScene *scn, const struct aiMesh *mesh, const char *texname)
{
	int i, j;
	const struct aiMaterial *mtl;
	struct aiString name;

	mtl = scn->mMaterials[mesh->mMaterialIndex];

	for(i=0; i<(int)(sizeof(types) / sizeof(types[0])); i++) {
		for(j=0; aiGetMaterialString(mtl, AI_MATKEY_TEXTURE(types[i], j), &name) == AI_SUCCESS; j++) {
			if(strstr(name.data, texname)) {
				return 1;
			}
		}
	}
	return 0;
}

static void draw_uvmesh(const struct aiMesh *mesh, int uvset)
{
	int i, j;
	const struct aiVector3D *uv = mesh->mTextureCoords[uvset];
	if(!uv) {
		fprintf(stderr, "warning: mesh %s doesn't have UV set %d. Falling back to 0\n",
				mesh->mName.data, uvset);
		uv = mesh->mTextureCoords[0];
		assert(uv);
	}

	glBegin(GL_TRIANGLES);
	glColor3f(1, 1, 1);

	for(i=0; i<(int)mesh->mNumFaces; i++) {
		for(j=0; j<3; j++) {
			int idx = mesh->mFaces[i].mIndices[j];
			glVertex2f(uv[idx].x, uv[idx].y);
		}
	}
	glEnd();
}

#ifdef __unix__
#include <GL/glx.h>

static Display *dpy;
static Window win;
static GLXContext ctx;

static int init_gl(int xsz, int ysz)
{
	XSetWindowAttributes xattr;
	unsigned int xattr_mask;
	XVisualInfo *vis_info;
	Window root;
	int scr;
	static int glxattr[] = {
		GLX_RGBA, GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		None
	};
	unsigned int fbo, rtex;

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "inig_gl: failed to connect to the X server\n");
		return -1;
	}
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);

	if(!(vis_info = glXChooseVisual(dpy, scr, glxattr))) {
		fprintf(stderr, "init_gl: no matching visual\n");
		XCloseDisplay(dpy);
		return -1;
	}

	xattr.colormap = XCreateColormap(dpy, root, vis_info->visual, AllocNone);
	xattr.background_pixel = xattr.border_pixel = BlackPixel(dpy, scr);
	xattr_mask = CWBackPixel | CWBorderPixel | CWColormap;

	if(!(win = XCreateWindow(dpy, root, 0, 0, xsz, ysz, 0, vis_info->depth, InputOutput,
					vis_info->visual, xattr_mask, &xattr))) {
		fprintf(stderr, "init_gl: failed to create window\n");
		XFree(vis_info);
		XCloseDisplay(dpy);
		return -1;
	}

	if(!(ctx = glXCreateContext(dpy, vis_info, 0, True))) {
		fprintf(stderr, "init_gl: failed to create OpenGL context\n");
		XDestroyWindow(dpy, win);
		XFree(vis_info);
		XCloseDisplay(dpy);
		return -1;
	}
	XFree(vis_info);

	glXMakeCurrent(dpy, win, ctx);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &rtex);
	glBindTexture(GL_TEXTURE_2D, rtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xsz, ysz, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rtex, 0);

	glViewport(0, 0, xsz, ysz);
	/*glClear(GL_COLOR_BUFFER_BIT);
	glXSwapBuffers(dpy, win);*/
	return 0;
}

static void destroy_gl(void)
{
	if(dpy) {
		glXMakeCurrent(dpy, 0, 0);
		glXDestroyContext(dpy, ctx);
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
		dpy = 0;
	}
}

#endif