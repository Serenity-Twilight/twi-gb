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

// The number of elements rendered in order to draw a single quad
// (composed of two triangles).
static const uint_fast8_t QUAD_NUM_ELEMENTS = 6;

//=======================================================================
//-----------------------------------------------------------------------
// Private type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
//-----------------------------------------------------------------------
// Private function declarations
//-----------------------------------------------------------------------
//=======================================================================
static uint32_t gl_compile_program(size_t, const char* const[], const GLenum[]);
static uint32_t gl_compile_shader(const char*, GLenum);
static void gl_log_errors(GLenum);
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

	//printf("%s\n", glGetString(GL_VERSION));

	// TODO: Report specific GL errors.
	if (glewInit() != GLEW_OK) {
		LOGF("Failed to initialize OpenGL extensions via GLEW.");
		goto destroy_context;
	}

	{	// Compile main rendering program.
		uint32_t program;
		static const char* const src_path[] = {"gl/vs", "gl/dmgfs"};
		static const GLenum type[] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
		if (!(program = gl_compile_program(2, src_path, type)))
			goto destroy_context;
		glUseProgram(program);
	}

	{	// Vertex Array Object
		uint32_t vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
	}

	{	// Create buffers
		// [0]: Vertex Buffer Object
		// [1]: Element Buffer Object
		uint32_t gl_buf[2];
		glGenBuffers(2, gl_buf);
	
		{ // Initialize & configure Vertex Buffer Object
			static const float vbo_data[] = {
				-1.0f, 1.0f, // top-left vertex
				1.0f, 1.0f, // top-right vertex
				-1.0f, -1.0f, // bottom-left vertex
				1.0f, -1.0f // bottom-right vertex
			};
			glBindBuffer(GL_ARRAY_BUFFER, gl_buf[0]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vbo_data), vbo_data, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
		}
		{ // Initialize Element Buffer object
			// 0: top-left of quad
			// 1: top-right of quad
			// 2: bottom-left of quad
			// 3: bottom-right of quad
			// Indices 0-2 are for the top-left triangle of the quad.
			// Indices 3-5 are for the bottom-right triangle of the quad.
			// Indices are ordered such that triangles are drawn counter-clockwise,
			// which, by default in OpenGL, makes them front-facing (facing the screen).
			static const uint8_t ebo_data[] = {0, 2, 1, 1, 2, 3};
			twi_assertf(sizeof(ebo_data) == QUAD_NUM_ELEMENTS,
					"The size of the `ebo_data` (%zu) array used to intialize "
					"the EBO does not equal QUAD_NUM_ELEMENTS (%" PRIuFAST8 ").",
					sizeof(ebo_data), QUAD_NUM_ELEMENTS);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buf[1]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ebo_data), ebo_data, GL_STATIC_DRAW);
		}
	} // end stack allocation control block around buffer creation

	// Initialize framebuffer and its renderbuffer, which will be the target of all
	// rendering operations and will have its contents blitted to the default
	// framebuffer in order to render the emulated screen independent of window resolution.
	glGenFramebuffers(1, &(vid->fbo)); // Framebuffer Object
	glBindFramebuffer(GL_FRAMEBUFFER, vid->fbo);
	{	// Create, configure, and attach Renderbuffer Object
		uint32_t rbo;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, GB_LCD_WIDTH, GB_LCD_HEIGHT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black
	vid->flags = RESOLUTION_CHANGED;
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

	// Draw all pixels of the LCD.
	glBindFramebuffer(GL_FRAMEBUFFER, vid->fbo);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, QUAD_NUM_ELEMENTS, GL_UNSIGNED_BYTE, 0);

	// Blit renderbuffer output to the default framebuffer,
	// centering the image within the window and maintaining aspect ratio.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, vid->fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlitFramebuffer(
			0, 0, GB_LCD_WIDTH, GB_LCD_HEIGHT,
			vid->winx0, vid->winy0, vid->winx1, vid->winy1,
			GL_COLOR_BUFFER_BIT, GL_NEAREST
	);
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
// decl gl_compile_program()
// def gl_compile_program()
// TODO
//=======================================================================
static uint32_t
gl_compile_program(
		size_t num_shaders,
		const char* const src_path[num_shaders],
		const GLenum type[num_shaders]
) {
	// Assert argument correctness.
	twi_assert_gti(num_shaders, 0);
	twi_assert_notnull(src_path);
	twi_assert_notnull(type);
	for (size_t i = 0; i < num_shaders; ++i)
		twi_assert_notnull(src_path[i]);

	uint32_t program = glCreateProgram();
	if (!program) {
		LOGE("Failed to create OpenGL program. Error flags:");
		gl_log_errors(0);
		return 0;
	}

	uint32_t shaders[num_shaders];
	for (size_t i = 0; i < num_shaders; ++i) {
		// gl_compile_shader() handles error logging if necessary.
		shaders[i] = gl_compile_shader(src_path[i], type[i]);
		if (!shaders[i]) {
			// Delete previously-compiled shaders.
			while (i > 0)
				glDeleteShader(shaders[--i]);
			glDeleteProgram(program);
			return 0;
		} // end if shader compilation failed
		glAttachShader(program, shaders[i]);
	} // end shader compilation and attachment
	
	glLinkProgram(program);
	for (size_t i = 0; i < num_shaders; ++i)
		glDeleteShader(shaders[i]);

	int32_t success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infolog[GL_BUFSZ];
		glGetProgramInfoLog(program, GL_BUFSZ, NULL, infolog);
		LOGE("OpenGL program linker failure. Info Log:\n%s", infolog);
		while (glGetError()); // Clear any set error flags.
		glDeleteProgram(program);
		return 0;
	} // end if program linking failed

	return program;
} // end gl_compile_program()

//=======================================================================
// decl gl_compile_shader()
// def gl_compile_shader()
// TODO
//=======================================================================
static uint32_t
gl_compile_shader(const char* path, GLenum type) {
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
} // end gl_compile_shader()

//=======================================================================
// decl gl_log_errors()
// def gl_log_errors()
// TODO
//=======================================================================
void
gl_log_errors(GLenum first_error) {
	// If caller passed an initial pre-fetched error, log now.
	if (first_error)
		LOGE(gl_strerror(first_error));

	// If the caller did not pass an error, and no error flags are set,
	// log the lack of errors for log readability.
	GLenum error_code = glGetError();
	if (!error_code && !first_error) {
		LOGE("No GL error codes.");
		return;
	}

	// Log the remainder of the errors.
	while (error_code) {
		LOGE(gl_strerror(error_code));
		error_code = glGetError();
	}
} // end gl_log_errors()

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

	// Update default framebuffer rendering size to maintain aspect ratio.
	double width_ratio = GB_LCD_WIDTH / width;
	double height_ratio = GB_LCD_HEIGHT / height;
	if (height_ratio > width_ratio) {
		// Use full window height, clamp width to aspect ratio.
		vid->winy0 = 0;
		vid->winy1 = height;
		uint_fast16_t mid = width / 2;
		double reach = (GB_LCD_WIDTH / height_ratio) / 2;
		vid->winx0 = mid - reach;
		vid->winx1 = mid + reach;
		if (reach - (uint_fast16_t)reach > 0) // Round up
			++(vid->winx1);
	} else {
		// Use full window width, clamp height to aspect ratio.
		vid->winx0 = 0;
		vid->winx1 = width;
		uint_fast16_t mid = height / 2;
		double reach = (GB_LCD_HEIGHT / width_ratio) / 2;
		vid->winy0 = mid - reach;
		vid->winy1 = mid + reach;
		if (reach - (uint_fast16_t)reach > 0) // Round up
			++(vid->winy1);
	} // end determining what axis to clamp to
} // end gl_update_res()

