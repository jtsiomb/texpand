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
#ifdef USE_WGL
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "glctx.h"

static LRESULT CALLBACK handle_event(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam);

static ATOM wclass;
static HWND win;
static HGLRC ctx;
static HDC dc;
static PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
static PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;

int init_gl(int xsz, int ysz)
{
	int pixfmt;
	PIXELFORMATDESCRIPTOR pfd;
	unsigned int fbo, rtex;

	if(!wclass) {
		WNDCLASS wc;
		memset(&wc, 0, sizeof wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hInstance = GetModuleHandle(0);
		wc.lpszClassName = "texpand";
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.lpfnWndProc = handle_event;
		wclass = RegisterClass(&wc);
	}

	if(!(win = CreateWindow("texpand", "Texpand", WS_OVERLAPPEDWINDOW, 0, 0, xsz, ysz, 0, 0,
					GetModuleHandle(0), 0))) {
		fprintf(stderr, "init_gl: failed to create window\n");
		return -1;
	}
	dc = GetDC(win);

	memset(&pfd, 0, sizeof pfd);
	pfd.nSize = sizeof pfd;
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cRedBits = pfd.cGreenBits = pfd.cBlueBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	if(!(pixfmt = ChoosePixelFormat(dc, &pfd))) {
		fprintf(stderr, "init_gl: failed to find suitable pixel format\n");
		ReleaseDC(win, dc);
		DestroyWindow(win);
		win = 0;
		return -1;
	}
	SetPixelFormat(dc, pixfmt, &pfd);

	if(!(ctx = wglCreateContext(dc))) {
		fprintf(stderr, "init_gl: failed to create OpenGL context\n");
		ReleaseDC(win, dc);
		DestroyWindow(win);
		win = 0;
		return -1;
	}
	wglMakeCurrent(dc, ctx);

	glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
	glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
	glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");

	if(!glGenFramebuffersEXT || !glBindFramebufferEXT || !glFramebufferTexture2DEXT) {
		fprintf(stderr, "failed to retrieve FBO entry points\n");
		destroy_gl();
		return -1;
	}

	glGenFramebuffersEXT(1, &fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

	glGenTextures(1, &rtex);
	glBindTexture(GL_TEXTURE_2D, rtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xsz, ysz, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, rtex, 0);

	glViewport(0, 0, xsz, ysz);
	/*glClear(GL_COLOR_BUFFER_BIT);
	glXSwapBuffers(dpy, win);*/
	return 0;
}

void destroy_gl(void)
{
	if(win) {
		wglMakeCurrent(0, 0);
		wglDeleteContext(ctx);
		ReleaseDC(win, dc);
		DestroyWindow(win);
		win = 0;
	}
}

static LRESULT CALLBACK handle_event(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(win, msg, wparam, lparam);
}

#endif	/* USE_WGL */
