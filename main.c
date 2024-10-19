// +----------------------------------------------------+
// |                                                    |
// |   _____                                            |
// |  |__  /   ___     ___    _ __ ___     ___   _ __   |
// |    / /   / _ \   / _ \  | '_ ` _ \   / _ \ | '__|  |
// |   / /_  | (_) | | (_) | | | | | | | |  __/ | |     |
// |  /____|  \___/   \___/  |_| |_| |_|  \___| |_|     |
// |                                                    |
// |                                                    |
// +----------------------------------------------------+
// |  Magnifier application for Linux and Window in C!  |
// +----------------------------------------------------+
// | Made by:                                           |
// |  https://github.com/itsYakub                       |
// +----------------------------------------------------+

// -----------------
// SECTION: Includes
// -----------------

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "SDL2/SDL.h"
#include "glad/glad.h"
#include "cglm/cglm.h"

// --------------------------
// SECTION: Macro Definitions
// --------------------------

#ifndef ZOOMER_ZOOM_MIN
    #define ZOOMER_ZOOM_MIN 0.5f // This value is for zooming out (range: 0.0f - 1.0f)
#endif // ZOOMER_ZOOM_MIN

#ifndef ZOOMER_ZOOM_MAX
    #define ZOOMER_ZOOM_MAX 64.0f // This value is for zooming in (range: 0.0f - n)
#endif // ZOOMER_ZOOM_MAX

#ifndef ZOOMER_DISPLAY_WIDTH
    #define ZOOMER_DISPLAY_WIDTH 1920
#endif // ZOOMER_DISPLAY_WIDTH

#ifndef ZOOME_DISPLAY_HEIGHT
    #define ZOOMER_DISPLAY_HEIGHT 1080
#endif // ZOOMER_DISPLAY_HEIGHT

// -------------------------
// SECTION: Global Variables
// -------------------------

const char* glsl_vert =
"#version 460 core\n"
"layout (location = 0) in vec3 a_Pos;\n"
"layout (location = 1) in vec4 a_Col;\n"
"layout (location = 2) in vec2 a_TexCoord;\n"
"layout (location = 3) in float a_TexId;\n"
"out vec4 v_Col;\n"
"out vec2 v_TexCoord;\n"
"out float v_TexId;\n"
"uniform mat4 u_proj;\n"
"uniform mat4 u_view;\n"
"void main() {\n"
" 	gl_Position = u_proj * u_view * vec4(a_Pos, 1.0f);\n"
"	v_Col = a_Col;\n"
"	v_TexCoord = a_TexCoord;\n"
"	v_TexId = a_TexId;\n"
"}";

const char* glsl_frag =
"#version 460 core\n"
"in vec4 v_Col;\n"
"in vec2 v_TexCoord;\n"
"in float v_TexId;\n"
"out vec4 f_Col;\n"
"uniform sampler2D u_Texture;\n"
"void main() {\n"
"   int f_TexId = int(v_TexId);"
"   if(f_TexId != 0)\n"
"	    f_Col = texture(u_Texture, v_TexCoord) * v_Col;\n"
"   else\n"
"       f_Col = v_Col;\n"
"}";

// -----------------
// SECTION: Typedefs
// -----------------

typedef struct s_tex2d {
    unsigned int w;
    unsigned int h;
    unsigned int ch;
    unsigned int id;
} t_tex2d;

typedef struct s_cam2d {
    vec2 target;
    vec2 offset;
    float scale;
} t_cam2d;

typedef struct s_core {
    void* window;
    SDL_GLContext context;
    int exit;

    unsigned int sh_prog;

    vec2 mouse_wheel;

    vec2 mouse_pos;
    vec2 mouse_pos_prev;

    int mouse_button[3];

    int key[SDL_NUM_SCANCODES];
    int key_prev[SDL_NUM_SCANCODES];
} t_core;

static t_core CORE;

// ------------------------------
// SECTION: Function declarations
// ------------------------------

// ------------------------------
// SECTION: Functions - Windowing
// ------------------------------

int ft_init(unsigned int w, unsigned int h, const char* title);
int ft_init_window(unsigned int w, unsigned int h, const char* title);
int ft_init_opengl(void);

int ft_poll_events(void);
int ft_should_quit(void);
int ft_display(void);
int ft_quit(void);

// -----------------------------------
// SECTION: Functions - Screen Capture
// -----------------------------------

char* ft_screen_capture(int w, int h);

// ------------------------------
// SECTION: Functions - Texturing
// ------------------------------
t_tex2d ft_tex2d(int w, int h, char* data);
int ft_draw_tex2d(t_tex2d tex, vec2 position, vec2 size);

// -----------------------------
// SECTION: Functions - Inputing
// -----------------------------
int ft_mousedown(int button);
int ft_mouseup(int button);
float ft_mousewheel(void);

int ft_keydown(SDL_Scancode code);
int ft_keyup(SDL_Scancode code);
int ft_keypress(SDL_Scancode code);
int ft_keyrelease(SDL_Scancode code);

// ---------------------------
// SECTION: Functions - Camera
// ---------------------------

int ft_cam2d_display(t_cam2d cam);
int ft_screen_to_world(t_cam2d cam, vec2 src, vec2 dest);
int ft_cam2d_matrix(t_cam2d cam, mat4 dest);

int ft_cam2d_pan(t_cam2d* cam);
int ft_cam2d_zoom(t_cam2d* cam);

// ----------------
// SECTION: Program
// ----------------

int main(int argc, const char* argv[]) {

    // -----------------------
    // SECTION: Program - Load
    // -----------------------

    char* capture_data = ft_screen_capture(ZOOMER_DISPLAY_WIDTH, ZOOMER_DISPLAY_HEIGHT);

    ft_init(ZOOMER_DISPLAY_WIDTH, ZOOMER_DISPLAY_HEIGHT, "Zoomer | 1.0.0"); 

    t_tex2d capture_texture = ft_tex2d(ZOOMER_DISPLAY_WIDTH, ZOOMER_DISPLAY_HEIGHT, capture_data);
    t_cam2d cam = { .scale = 1.0f };
    int cam_reset = 0;

	while(!ft_should_quit()) {

        // -------------------------
        // SECTION: Program - Update
        // -------------------------

        // Camera panning
        if(ft_mousedown(SDL_BUTTON_LEFT) || ft_mousedown(SDL_BUTTON_RIGHT))
            ft_cam2d_pan(&cam);    
        
        // Camera zooming
        if(ft_mousewheel() != 0.0f)
            ft_cam2d_zoom(&cam);
        
        // Camera reseting
        if(ft_keypress(SDL_SCANCODE_R) && !cam_reset) cam_reset = 1;
        if(cam_reset) {
            if(
                    round(cam.target[0]) == 0 &&
                    round(cam.target[1]) == 0 &&
                    round(cam.offset[0]) == 0 &&
                    round(cam.offset[1]) == 0 &&
                    round(cam.scale) == 1.0
            ) {
                cam.target[0] = 0.0f;
                cam.target[1] = 0.0f;
                cam.offset[0] = 0.0f;
                cam.offset[1] = 0.0f;
                cam.scale = 1.0f;

                cam_reset = 0;
            } else {
                glm_vec2_lerp(cam.target, GLM_VEC2_ZERO, 0.5f, cam.target);
                glm_vec2_lerp(cam.offset, GLM_VEC2_ZERO, 0.5f, cam.offset);
                cam.scale = glm_lerp(cam.scale, 1.0f, 0.5f);
            }
        }
        
        // -------------------------
        // SECTION: Program - Render
        // -------------------------

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

        ft_cam2d_display(cam);
        ft_draw_tex2d(capture_texture, (vec2) { 0.0f, 0.0f }, (vec2) { ZOOMER_DISPLAY_WIDTH, ZOOMER_DISPLAY_HEIGHT });

        ft_display();
	    ft_poll_events();	
	}
    
    // ------------------------
    // SECTION: Program - Close
    // ------------------------

    free(capture_data);
    glDeleteTextures(1, &capture_texture.id);

    ft_quit();

	return 0;
}

// -----------------------------
// SECTION: Function definitions
// -----------------------------

// ------------------------------
// SECTION: Functions - Windowing
// ------------------------------

int ft_init(unsigned int w, unsigned int h, const char* title) {
    ft_init_window(w, h, title);
    ft_init_opengl();

    return 1;
}


int ft_init_window(unsigned int w, unsigned int h, const char* title) {
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		return 0;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	CORE.window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		w,
	    h,
		SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS
	);

	CORE.context = SDL_GL_CreateContext(CORE.window);
	SDL_GL_MakeCurrent(CORE.window, CORE.context);
	SDL_GL_SetSwapInterval(1);

	gladLoadGL();

    glViewport(0, 0, w, h);

    return 1;
}

int ft_init_opengl(void) {
    GLuint sh_vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint sh_frag = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(sh_vert, 1, &glsl_vert, NULL);
    glShaderSource(sh_frag, 1, &glsl_frag, NULL);

    glCompileShader(sh_vert);
    glCompileShader(sh_frag);

    GLuint sh_prog = glCreateProgram();
    glAttachShader(sh_prog, sh_vert);
    glAttachShader(sh_prog, sh_frag);
    glLinkProgram(sh_prog);

    glDeleteShader(sh_vert);
    glDeleteShader(sh_frag);
    glUseProgram(sh_prog);

    CORE.sh_prog = sh_prog;

    return 1;
}

int ft_poll_events(void) {
    CORE.mouse_pos_prev[0] = CORE.mouse_pos[0];
    CORE.mouse_pos_prev[1] = CORE.mouse_pos[1];
    CORE.mouse_pos_prev[2] = CORE.mouse_pos[2];

    for(int i = 0; i < SDL_NUM_SCANCODES; i++)
        CORE.key_prev[i] = CORE.key[i];

    CORE.mouse_wheel[0] = 0.0f;
    CORE.mouse_wheel[1] = 0.0f;

    SDL_Event event = { 0 };
    while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: {
				CORE.exit = 1;
			} break;

            case SDL_MOUSEMOTION: {
                CORE.mouse_pos[0] = event.motion.x;
                CORE.mouse_pos[1] = event.motion.y;
            } break;

            case SDL_MOUSEBUTTONDOWN: {
                CORE.mouse_button[event.button.button] = 1;
            } break;

            case SDL_MOUSEBUTTONUP: {
                CORE.mouse_button[event.button.button] = 0;
            } break;

            case SDL_MOUSEWHEEL: {
                CORE.mouse_wheel[0] = event.wheel.x;
                CORE.mouse_wheel[1] = event.wheel.y;
            } break;

            case SDL_KEYDOWN: {
                CORE.key[event.key.keysym.scancode] = 1;

                if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    CORE.exit = 1;
            } break;
            
            case SDL_KEYUP: {
                CORE.key[event.key.keysym.scancode] = 0;
            } break;
		}
	}

    return 1;
}

int ft_should_quit(void) {
    return CORE.exit;
}

int ft_display(void) {
	SDL_GL_SwapWindow(CORE.window);

    return 1;
}

int ft_quit(void) {
    glDeleteProgram(CORE.sh_prog);

    SDL_GL_DeleteContext(CORE.context);
	SDL_DestroyWindow(CORE.window);

	SDL_Quit();
    return 1;
}

// -----------------------------------
// SECTION: Functions - Screen Capture
// -----------------------------------

char* ft_screen_capture(int w, int h) {
    Display* x_display = XOpenDisplay(NULL);
    Window x_root = DefaultRootWindow(x_display);

    XWindowAttributes x_window_attrib = { 0 };
    XGetWindowAttributes(x_display, x_root, &x_window_attrib);

    XImage* x_image = XGetImage(
        x_display, x_root,
        0, 0,
        w, h,
        AllPlanes,
        ZPixmap
    );

    char* data = (unsigned char*) calloc(w * h * 4, sizeof(char));
    if(!data) {
        fprintf(stderr, "[ ERR ] X11: %s\n", strerror(errno));

        XDestroyImage(x_image);
        XCloseDisplay(x_display);

        return NULL;
    }

    for(int i = 0; i < w * h * 4; i += 4) {
        data[i + 0] = x_image->data[i + 2];
        data[i + 1] = x_image->data[i + 1];
        data[i + 2] = x_image->data[i + 0];
        data[i + 3] = x_image->data[i + 3];
    }

    XDestroyImage(x_image);
    XCloseDisplay(x_display);

    return data;
}

// ------------------------------
// SECTION: Functions - Texturing
// ------------------------------

t_tex2d ft_tex2d(int w, int h, char* data) {
    t_tex2d tex = { 
        .w = w,
        .h = h,
        .ch = 4
    };

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        w,
        h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

int ft_draw_tex2d(t_tex2d tex, vec2 position, vec2 size) {
    GLfloat vertices[] = {
        position[0], position[1], 0.0f,     1.0f, 1.0f, 1.0f, 1.0f,     0.0f, 0.0f,     tex.id,
        position[0] + size[0], position[1], 0.0f,     1.0f, 1.0f, 1.0f, 1.0f,     1.0f, 0.0f,   tex.id,
        position[0], position[1] + size[1], 0.0f,     1.0f, 1.0f, 1.0f, 1.0f,     0.0f, 1.0f,   tex.id,
        position[0] + size[0], position[1] + size[1], 0.0f,     1.0f, 1.0f, 1.0f, 1.0f,     1.0f, 1.0f,     tex.id,
    };

    GLuint indices[] = {
        0, 1, 2,
        1, 2, 3
    };

    GLuint vert_arr;
    glGenVertexArrays(1, &vert_arr);
    glBindVertexArray(vert_arr);

    GLuint vert_buf;
    glGenBuffers(1, &vert_buf);
    glBindBuffer(GL_ARRAY_BUFFER, vert_buf);

    GLuint elem_buf;
    glGenBuffers(1, &elem_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*) (0 * sizeof(GLfloat)));
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*) (7 * sizeof(GLfloat)));
 
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (void*) (9 * sizeof(GLfloat)));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDeleteBuffers(1, &vert_buf);
    glDeleteBuffers(1, &elem_buf);
    glDeleteVertexArrays(1, &vert_arr);

    return 1;
}

// -----------------------------
// SECTION: Functions - Inputing
// -----------------------------

int ft_mousedown(int button) {
    return CORE.mouse_button[button];
}

int ft_mouseup(int button) {
    return !CORE.mouse_button[button];
}

float ft_mousewheel(void) {
    float wheel = 0.0f;

    if(fabsf(CORE.mouse_wheel[0]) > fabsf(CORE.mouse_wheel[1]))
        wheel = CORE.mouse_wheel[0];
    else
        wheel = CORE.mouse_wheel[1];

    return wheel;
}

int ft_keydown(SDL_Scancode code) {
    return CORE.key[code];
}

int ft_keyup(SDL_Scancode code) {
    return !CORE.key[code];
}

int ft_keypress(SDL_Scancode code) {
    return CORE.key[code] && !CORE.key_prev[code];
}

int ft_keyrelease(SDL_Scancode code) {
    return !CORE.key[code] && CORE.key_prev[code];
}

// ---------------------------
// SECTION: Functions - Camera
// ---------------------------

int ft_cam2d_display(t_cam2d cam) {
    mat4 mat_proj = GLM_MAT4_IDENTITY_INIT;
    mat4 mat_view = GLM_MAT4_IDENTITY_INIT;

    glm_ortho(0.0f, ZOOMER_DISPLAY_WIDTH, ZOOMER_DISPLAY_HEIGHT, 0.0f, -1.0f, 1.0f, mat_proj);
    glUniformMatrix4fv(glGetUniformLocation(CORE.sh_prog, "u_proj"), 1, GL_FALSE, &mat_proj[0][0]);

    ft_cam2d_matrix(cam, mat_view);
    glUniformMatrix4fv(glGetUniformLocation(CORE.sh_prog, "u_view"), 1, GL_FALSE, &mat_view[0][0]);

    return 1;
}

int ft_screen_to_world(t_cam2d cam, vec2 src, vec2 dest) {
    mat4 mat_cam;
    mat4 mat_cam_inv;
    ft_cam2d_matrix(cam, mat_cam);
    glm_mat4_inv(mat_cam, mat_cam_inv);

    vec4 trans_mulv;
    vec4 trans = {
        src[0],
        src[1],
        0.0f,
        1.0f
    };
    
    glm_mat4_mulv(mat_cam_inv, trans, trans_mulv); 
    
    dest[0] = trans_mulv[0];
    dest[1] = trans_mulv[1];

    return 1;
}

int ft_cam2d_matrix(t_cam2d cam, mat4 dest) {
    glm_mat4_identity(dest);

    glm_translate(dest, (vec3) { cam.offset[0], cam.offset[1], 0.0f });
    glm_scale(dest, (vec3) { cam.scale, cam.scale, 1.0f });
    glm_translate(dest, (vec3) { -cam.target[0], -cam.target[1], 0.0f });
    
    return 1;    
}

int ft_cam2d_pan(t_cam2d* cam) {
    vec2 delta_scaled;
    vec2 delta = {
        (CORE.mouse_pos[0] - CORE.mouse_pos_prev[0]),
        (CORE.mouse_pos[1] - CORE.mouse_pos_prev[1])
    };

    glm_vec2_scale(delta, -1.0f / cam->scale, delta_scaled);

    cam->target[0] += delta_scaled[0];
    cam->target[1] += delta_scaled[1];

    return 1;
}

int ft_cam2d_zoom(t_cam2d* cam) {
    vec2 mouse_pos_world;

    ft_screen_to_world(*cam, CORE.mouse_pos, mouse_pos_world);
            
    cam->target[0] = mouse_pos_world[0];
    cam->target[1] = mouse_pos_world[1];
    
    cam->offset[0] = CORE.mouse_pos[0];
    cam->offset[1] = CORE.mouse_pos[1];

    float mouse_wheel = ft_mousewheel();
    float scale_factor = 1.0f + (0.25f * fabsf(mouse_wheel));  
    if(mouse_wheel < 0.0f)
    
    scale_factor = 1.0f / scale_factor;
    cam->scale = glm_clamp(cam->scale * scale_factor, ZOOMER_ZOOM_MIN, ZOOMER_ZOOM_MAX);   

    return 1;
}

// +--------------------------------------------------------------------------------+
// |                                     LICENCE                                    |
// +--------------------------------------------------------------------------------+
// | Copyright (c) 2024 Jakub Oleksiak <yakubofficialmail@gmail.com>                |
// |                                                                                |
// | Permission is hereby granted, free of charge, to any person obtaining a copy   |
// | of this software and associated documentation files (the "Software"), to deal  |
// | in the Software without restriction, including without limitation the rights   |
// | to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      |
// | copies of the Software, and to permit persons to whom the Software is          |
// | furnished to do so, subject to the following conditions:                       |
// |                                                                                |
// | The above copyright notice and this permission notice shall be included in all |
// | copies or substantial portions of the Software.                                |
// |                                                                                |
// | THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,                |
// | EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF             |
// | MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.         |
// | IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    |
// | DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR          |
// | OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE  |
// | OR OTHER DEALINGS IN THE SOFTWARE.                                             |
// +--------------------------------------------------------------------------------+
