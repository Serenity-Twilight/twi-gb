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
#include <twi/std/assert.h>
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

// Maximum number of bytes allocated for the following OpenGL operations:
// * The maximum number of bytes of shader source code that will be
//   stored on the stack. If shader source exceeds this size, then space
//   will be dynamically allocated for it.
// * The maximum number of bytes for an OpenGL Info Log dump.
static const size_t GL_BUFSZ = 65536; // 64 KiB

// Element buffer object that defines what vertices to use when drawing
// the triangles that the main screen is composed of.
// vertex 0: top left
// vertex 1: top right
// vertex 2: bottom left
// vertex 3: bottom right
// The first three values are the offsets of the vertices for the first
// triangle, and the second set of three values are the offsets of the
// vertices for the second triangle.
//
// Order matters! By default, OpenGL considers triangles drawn
// counter-clockwise to be facing the screen, and clockwise to be facing
// away from it. Therefore, triangles are drawn in a counter-clockwise
// motion.
static const uint8_t EBO_DATA[] = {0, 2, 1, 1, 2, 3};

//=======================================================================
//-----------------------------------------------------------------------
// Private function declarations
//-----------------------------------------------------------------------
//=======================================================================
static uint32_t compile_shader(const char*, GLenum);
static const char* gl_strerror(GLenum);
static void gl_update_res(struct twi_gb_vid*, int, int);

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

	// Compile shaders and link program.
	// compile_shader() writes detailed error messages on failure.
	if (!(vid->window_program = glCreateProgram())) {
		LOGE("Failed to create OpenGL program for rendering to window. Error flags:");
		GLenum error_code;
		while (error_code = glGetError())
			LOGE(gl_strerror(error_code));
		goto destroy_context;
	} // end creation of render-to-window program
	uint32_t vs, fs;
	if (!(vs = compile_shader("gl/dmg.vs", GL_VERTEX_SHADER)))
		goto destroy_context;
	if (!(fs = compile_shader("gl/dmg.fs", GL_FRAGMENT_SHADER))) {
		glDeleteShader(vs);
		goto destroy_context;
	}
	glAttachShader(vid->window_program, vs);
	glAttachShader(vid->window_program, fs);
	glLinkProgram(vid->window_program);
	glDeleteShader(fs);
	glDeleteShader(vs);
	{
		// Check if successful.
		int32_t success;
		glGetProgramiv(vid->window_program, GL_LINK_STATUS, &success);
		if (!success) {
			char infolog[GL_BUFSZ];
			glGetProgramInfoLog(vid->window_program, GL_BUFSZ, NULL, infolog);
			LOGE("OpenGL program linker failure. Info Log:\n%s", infolog);
			while (glGetError()); // Clear error flags, if set.
			goto destroy_context;
		} // end if linker failed
	}
	glUseProgram(vid->window_program);

	// Create buffers
	glGenVertexArrays(1, &(vid->vao)); // Vertex Array Object
	{
		uint32_t buffers[2];
		glGenBuffers(2, buffers);
		vid->vbo = buffers[0]; // Vertex Buffer Object
		vid->ebo = buffers[1]; // Element Buffer Object
	}
	
	// Bind and define buffers
	glBindVertexArray(vid->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vid->ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(EBO_DATA), EBO_DATA, GL_STATIC_DRAW);
	gl_update_res(vid, width, height);
	// Above function binds the vbo and sets its data.
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black
	glClear(GL_COLOR_BUFFER_BIT);

	// First draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
	SDL_GL_SwapWindow(vid->window);

	vid->flags = 0;
	return 0;

	// Cleanup, in the case of failure.
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
// def twi_gb_vid_draw()
//=======================================================================
void
twi_gb_vid_draw(struct twi_gb_vid* vid) {
	twi_assert_notnull(vid);

	SDL_GL_MakeCurrent(vid->window, vid->gl);
	// Update internal GL resolution if it has changed since the last draw.
	if (vid->flags & RESOLUTION_CHANGED) {
		int width, height;
		SDL_GetWindowSize(vid->window, &width, &height);
		gl_update_res(vid, width, height);
		vid->flags &= ~RESOLUTION_CHANGED;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(vid->window_program);
	glBindVertexArray(vid->vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
	SDL_GL_SwapWindow(vid->window);
} // end twi_gb_vid_draw()

//=======================================================================
// def twi_gb_vid_onchange_resolution()
//=======================================================================
void
twi_gb_vid_onchange_resolution(struct twi_gb_vid* vid) {
	vid->flags |= RESOLUTION_CHANGED;
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
	twi_assert_notnull(path);

	// Determine shader source size.
	size_t src_size = twi_fsize(path) + 1; // +1 for NULL terminator.
	if (src_size == ((size_t)-1)) {
		LOGE("Unable to retrieve size of \"%s\" (%s).", path, strerror(errno));
		return 0;
	}

	// Allocate memory for shader source.
	char src_stack[GL_BUFSZ];
	char* src;
	if (src_size > GL_BUFSZ) {
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
		LOGE("Unable to allocate OpenGL shader object. Error flags:");
		GLenum error_code;
		while (error_code = glGetError())
			LOGE(gl_strerror(error_code));
		goto failure_dealloc;
	}
	glShaderSource(shader_handle, 1, (const char* const*)&src, NULL);
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
		glGetShaderInfoLog(shader_handle, GL_BUFSZ, NULL, src_stack);
		glDeleteShader(shader_handle);
		LOGE("Open GL shader compilation failure. Info Log:\n%s", src_stack);
		while (glGetError()); // Clear error flags, in case they are set.
		return 0;
	}
	return shader_handle;

failure_dealloc:
	if (src != src_stack)
		free(src);
	return 0;
} // end compile_shader()

//=======================================================================
// decl gl_strerror()
// def gl_strerror()
// TODO
//=======================================================================
static const char*
gl_strerror(GLenum error_code) {
	switch (error_code) {
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
		default: return "Not a known OpenGL error.";
	} // end switch statement over error codes
} // end gl_strerror()

//=======================================================================
// decl gl_update_res()
// def gl_update_res()
// TODO
//=======================================================================
static void
gl_update_res(struct twi_gb_vid* vid, int width, int height) {
	glViewport(0, 0, width, height);
	glBindBuffer(GL_ARRAY_BUFFER, vid->vbo);

	// Update internal viewport size of emulated LCD screen.
	double width_ratio = GB_LCD_WIDTH / width;
	double height_ratio = GB_LCD_HEIGHT / height;
	if (height_ratio > width_ratio) {
		// Clamp by height
		double width_scale = width_ratio / height_ratio;
		float vertices[] = {
			-width_scale, 1.0,
			width_scale, 1.0,
			-width_scale, -1.0,
			width_scale, -1.0
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	} else {
		// Clamp by width
		double height_scale = height_ratio / width_ratio;
		float vertices[] = {
			-1.0, height_scale,
			1.0, height_scale,
			-1.0, -height_scale,
			1.0, -height_scale
		};
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	} // end determining what axis to clamp to
} // end gl_update_res()

