//=======================================================================
//-----------------------------------------------------------------------
// Copyright 2022 Serenity Twilight
//
// This file is part of twi-gb.
//
// twi-gb is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// twi-gb is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with twi-gb. If not, see <https://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------
//=======================================================================
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <twi/gb/log.h>
#include <twi/gb/vid.h>
#include <twi/std/utils.h>

//=======================================================================
//-----------------------------------------------------------------------
// Private constant definitions
//-----------------------------------------------------------------------
//=======================================================================

// Video backend flags (for use with twi_gb_vid.flags)
static const uint_fast8_t RESOLUTION_CHANGED = 0x01;

// Width and height of a Game Boy LCD screen.
// Used in ratio calculations, so stored as doubles to prevent repeated
// type conversion.
static const double GB_LCD_WIDTH = 160.0; // px
static const double GB_LCD_HEIGHT = 144.0; // px

// Maximum number of bytes of shader source that are allowed to
// be allocated to the stack. If a shader's source is larger than this
// value, it received dynamically-allocated storage.
static const size_t MAX_STACK_SHADER_SIZE = 262144; // 256 KiB

//=======================================================================
//-----------------------------------------------------------------------
// Public function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def twi_gb_vid_init()
//=======================================================================
uint_fast8_t
twi_gb_vid_init(struct twi_gb_vid* vid) {

	// TODO: Make these user configurable.
	static const int width = 800;
	static const int height = 600;

	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		if (SDL_Init(SDL_INIT_VIDEO)) {
			LOGF("Failed to initialize SDL video subsystem with the following error: %s", SDL_GetError());
			return 1;
		}
	}

	if (!(vid->window = SDL_CreateWindow(
					"twi-gb",
					SDL_WINDOWPOS_UNDEFINED,
					SDL_WINDOWPOS_UNDEFINED,
					width, height,
					SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
	))) {
		LOGF("Failed to create an SDL-managed window with the following error: %s", SDL_GetError());
		return 1;
	} // end if SDL failed to create window

	if (!(vid->gl = SDL_GL_CreateContext(vid->window))) {
		LOGF("Failed to create OpenGL context for SDL-managed window with the following error: %s", SDL_GetError());
		goto destroy_window;
	}

	// TODO: Report specific GL errors.
	if (glewInit() != GLEW_OK) {
		LOGF("Failed to initialize OpenGL extensions via GLEW.");
		goto destroy_context;
	}

	// Compile vertex and fragment shaders.
	// compile_shader() writes detailed error messages on failure.
	if (!(vid->vs = compile_shader("gl/dmg.vs", GL_VERTEX_SHADER)))
		goto destroy_context;
	if (!(vid->fs = compile_shader("gl/dmg.fs", GL_FRAGMENT_SHADER)))
		goto destroy_vs;

	glGenBuffers(1, &(vid->vbo)); // Vertex Attribute Object
	set_resolution(vid, width, height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(vid->window);
	//int block_size;
	//glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &block_size);
	//printf("GL_MAX_UNIFORM_BLOCK_SIZE: %d\n", block_size);

	return 0;

	// Cleanup, in the case of failure.
destroy_fs:
	glDeleteShader(vid->fs);
destroy_vs:
	glDeleteShader(vid->vs);
destroy_context:
	SDL_GL_DeleteContext(vid->gl);
destroy_window:
	SDL_DestroyWindow(vid->window);
	return 1;
} // end twi_gb_vid_init()

//=======================================================================
// def twi_gb_vid_destroy()
//=======================================================================
void
twi_gb_vid_destroy(struct twi_gb_vid* vid) {
	SDL_GL_DeleteContext(vid->gl);
	SDL_DestroyWindow(vid->window);
} // end twi_gb_vid_destroy()

//=======================================================================
// def twi_gb_vid_onchange_resolution
//=======================================================================
void
twi_gb_vid_onchange_resolution(struct twi_gb_vid* vid) {
	//int width, height;
	//SDL_GetWindowSize(vid->window, &width, &height);
	//set_resolution(vid, width, height);
} // end twi_gb_vid_onchange_resolution()

//=======================================================================
//-----------------------------------------------------------------------
// Private function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl compile_shader()
// def compile_shader()
// TODO
//=======================================================================
static uint32_t
compile_shader(const char* path, GLenum type) {
	// Determine shader source size.
	size_t src_size = twi_fsize(path) + 1; // +1 for NULL terminator.
	if (src_size == ((size_t)-1)) {
		LOGE("Unable to retrieve size of \"%s\" (%s).", path, strerror(errno));
		return 0;
	}

	// Allocate memory for shader source.
	unsigned char src_stack[MAX_STACK_SHADER_SIZE];
	unsigned char* src;
	if (src_size > MAX_STACK_SHADER_SIZE) {
		errno = 0;
		src = malloc(src_size);
		if (src == NULL) {
			LOGCE("Unable to allocate memory for shader source \"%s\":%zu bytes.", path, src_size);
			return 0;
		}
	} else {
		src = src_stack;
	} // end if dynamic/stack allocation
	
	// Copy shader source from file to memory.
	errno = 0;
	FILE* fsrc = fopen(path, "r");
	if (fsrc == NULL) {
		LOGCE("Unable to open \"%s\".", path);
		goto failure_dealloc;
	}
	errno = 0;
	fread(src, 1, src_size-1, fsrc);
	int error = ferror(fsrc);
	fclose(fsrc);
	if (error) {
		LOGCE("Failed to copy shader source \"%s\" into memory.", path);
		goto failure_dealloc;
	}
	src[src_size-1] = 0;

	// Create shader and input source.
	uint32_t shader_handle = glCreateShader(type);
	if (shader_handle == 0) {
		LOGF("Unable to allocate OpenGL shader object.");
		goto failure_dealloc;
	}
	glShaderSource(shader_handle, 1, &src, NULL);
	// Dynamically-allocated memory no longer required.
	if (src != src_stack)
		free(src);

	// Compile.
	glCompileShader(shader_handle);
	int32_t gl_success;
	glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &gl_success);
	if (!gl_success) {
		// Compilation failed. Output InfoLog to process log.
		// The shader source is no longer required, so that memory will be reused.
		glGetShaderInfoLog(shader_handle, MAX_STACK_SHADER_SIZE, NULL, src_stack);
		glDeleteShader(shader_handle);
		LOGE("Open GL shader compilation failure. Info Log:\n%s", src_stack);
		return 0;
	}
	return shader_handle;

failure_dealloc:
	if (src != src_stack)
		free(src);
	return 0;
} // end compile_shader()

//=======================================================================
// decl set_resolution()
// def set_resolution()
// TODO
//=======================================================================
static void
set_resolution(struct twi_gb_vid* vid, int width, int height) {
	glViewport(0, 0, width, height);

	// Update internal viewport size of emulated LCD screen.
	double width_ratio = GB_LCD_WIDTH / width;
	double height_ratio = GB_LCD_HEIGHT / height;
	if (height_ratio > width_ratio) {
		// Clamp by height
	} else {
		// Clamp by width
	}
	glBindBuffer(GL_ARRAY_BUFFER, vid->vbo);
	//glClear(GL_COLOR_BUFFER_BIT);
	//SDL_GL_SwapWindow(vid->window);
} // end set_resolution()

