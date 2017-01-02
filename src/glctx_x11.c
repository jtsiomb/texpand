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
#ifdef USE_GLX
#include <stdio.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>

static Display *dpy;
static Window win;
static GLXContext ctx;

int init_gl(int xsz, int ysz)
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
		fprintf(stderr, "init_gl: failed to connect to the X server\n");
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

void destroy_gl(void)
{
	if(dpy) {
		glXMakeCurrent(dpy, 0, 0);
		glXDestroyContext(dpy, ctx);
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
		dpy = 0;
	}
}
#endif	/* USE_GLX */
