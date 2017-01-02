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
#include <imago2.h>
#include <GL/gl.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <assimp/vector3.h>
#include "genmask.h"
#include "glctx.h"

static int uses_texture(const struct aiScene *scn, const struct aiMesh *mesh, const char *texname);
static void draw_uvmesh(const struct aiMesh *mesh, int uvset);

int mask_from_scene(struct img_pixmap *mask, int xsz, int ysz, const char *fname,
		int uvset, const char *filter)
{
	int res;
	struct aiScene *scn = load_scene(fname);
	if(!scn) {
		return -1;
	}

	res = gen_mask(mask, xsz, ysz, scn, uvset, filter);
	free_scene(scn);
	return res;
}

struct aiScene *load_scene(const char *fname)
{
	static const unsigned int ppflags = aiProcess_Triangulate | aiProcess_SortByPType |
		aiProcess_GenUVCoords | aiProcess_TransformUVCoords | aiProcess_FlipUVs;
	struct aiScene *scn;

	if(!(scn = (struct aiScene*)aiImportFile(fname, ppflags))) {
		fprintf(stderr, "failed to load scene file: %s\n", fname);
		return 0;
	}
	return scn;
}

void free_scene(struct aiScene *scn)
{
	aiReleaseImport(scn);
}

int gen_mask(struct img_pixmap *mask, int xsz, int ysz, struct aiScene *scn,
		int uvset, const char *filter)
{
	int i;

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
