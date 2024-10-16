#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "SDL2/SDL.h"
#include "glad/glad.h"
#include "cglm/types.h"
#include "cglm/cam.h"
#include "cglm/affine.h"
#include "cglm/affine-pre.h"

// TODO(yakub):
// 1. Create a functionality for storing the display capture in the char* array
// 2. Create a borderless, fullscreen window 
// 3. Create a 2D Texture that will be the size of the screen
// 4. Render the texture on the screen
// 5. Pan and zoom the texture using the input events callbacks
// 6. Exit the application with a simple input (maybe a classic <ESC> key)

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

typedef struct s_tex2d {
    unsigned int w;
    unsigned int h;
    unsigned int ch;
    unsigned int id;
} t_tex2d;

typedef struct s_core {
    void* window;
    SDL_GLContext context;
    int exit;

    unsigned int sh_prog;

    vec2 mouse_wheel;

    ivec2 mouse_pos;
    ivec2 mouse_pos_prev;

    int mouse_button[3];
} t_core;

static t_core CORE;

int ft_init(unsigned int w, unsigned int h, const char* title);
int ft_init_window(unsigned int w, unsigned int h, const char* title);
int ft_init_opengl(void);

int ft_poll_events(void);
int ft_should_quit(void);
int ft_display(void);
int ft_quit(void);

char* ft_screen_capture(int w, int h);
t_tex2d ft_tex2d(int w, int h, char* data);

int ft_draw_rect(vec2 position, vec2 size, vec4 color);
int ft_draw_tex2d(t_tex2d tex, vec2 position, vec2 size);

int ft_mousedown(int button);
int ft_mouseup(int button);
float ft_mousewheel(void);

int main(int argc, const char* argv[]) {
    unsigned int capture_width = 1920;
    unsigned int capture_height = 1080;
    char* capture_data = ft_screen_capture(capture_width, capture_height);

    ft_init(capture_width, capture_height, "Okay, Zoomer | 1.0.0"); 

    t_tex2d capture_texture = ft_tex2d(capture_width, capture_height, capture_data);

    mat4 mat_proj = GLM_MAT4_IDENTITY_INIT;
    mat4 mat_view = GLM_MAT4_IDENTITY_INIT;

    vec2 cam_pan = { 0 };
    float cam_zoom = 0.0f;

    glm_ortho(0.0f, capture_width, capture_height, 0.0f, -1.0f, 1.0f, mat_proj);
    glUniformMatrix4fv(glGetUniformLocation(CORE.sh_prog, "u_proj"), 1, GL_FALSE, &mat_proj[0][0]);

	while(!ft_should_quit()) {
        // Camera panning
        if(ft_mousedown(SDL_BUTTON_LEFT)) {
            cam_pan[0] = (CORE.mouse_pos_prev[0] - CORE.mouse_pos[0]);
            cam_pan[1] = (CORE.mouse_pos_prev[1] - CORE.mouse_pos[1]);

            glm_translate(mat_view, (vec3) { -cam_pan[0], -cam_pan[1], 0.0f });
        }
        
        // Camera zooming
        if(ft_mousewheel() != 0.0f) {
            // TODO: Implement zooming
        }

	    ft_poll_events();	

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

        glUniformMatrix4fv(glGetUniformLocation(CORE.sh_prog, "u_view"), 1, GL_FALSE, &mat_view[0][0]);

        ft_draw_tex2d(capture_texture, (vec2) { 0.0f, 0.0f }, (vec2) { capture_width, capture_height });

        ft_display();
	}

    free(capture_data);
    glDeleteTextures(1, &capture_texture.id);

    ft_quit();

	return 0;
}

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

    SDL_Event event = { 0 };
    while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: {
				CORE.exit = 1;
			} break;

            case SDL_KEYDOWN: {
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

t_tex2d ft_tex2d(int w, int h, char* data) {
    t_tex2d tex = { 
        .w = w,
        .h = h,
        .ch = 4
    };

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

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

int ft_draw_rect(vec2 position, vec2 size, vec4 color) {
    GLfloat vertices[] = {
        position[0], position[1], 0.0f,     color[0], color[1], color[2], color[3],     0.0f, 0.0f,     0.0f,
        position[0] + size[1], position[1], 0.0f,     color[0], color[1], color[2], color[3],     1.0f, 0.0f,   0.0f,
        position[0], position[1] + size[1], 0.0f,     color[0], color[1], color[2], color[3],     0.0f, 1.0f,   0.0f,
        position[0] + size[0], position[1] + size[1], 0.0f,     color[0], color[1], color[2], color[3],     1.0f, 1.0f,     0.0f,
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
 
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDeleteBuffers(1, &vert_buf);
    glDeleteBuffers(1, &elem_buf);
    glDeleteVertexArrays(1, &vert_arr);

    return 1;
}

int ft_draw_tex2d(t_tex2d tex, vec2 position, vec2 size) {
    // TODO: Implement a texture drawing function
    // I wonder if I should use a global OpenGL objects or local
    // Will think about that tomorrow, gn o/
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

int ft_mousedown(int button) {
    return CORE.mouse_button[button];
}

int ft_mouseup(int button) {
    return !CORE.mouse_button[button];
}

float ft_mousewheel(void) {
    return CORE.mouse_wheel[1];
}
