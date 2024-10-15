#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "SDL2/SDL.h"
#include "glad/glad.h"

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
"out vec4 v_Col;\n"
"out vec2 v_TexCoord;\n"
"void main() {\n"
" 	gl_Position = vec4(a_Pos, 1.0f);\n"
"	v_Col = a_Col;\n"
"	v_TexCoord = a_TexCoord;\n"
"}";

const char* glsl_frag =
"#version 460 core\n"
"in vec4 v_Col;\n"
"in vec2 v_TexCoord;\n"
"out vec4 f_Col;\n"
"uniform sampler2D u_Texture;\n"
"void main() {\n"
"	f_Col = texture(u_Texture, v_TexCoord) * v_Col;\n"
"}";

typedef struct s_tex2d {
    unsigned int w;
    unsigned int h;
    unsigned int ch;
    unsigned int id;
} t_tex2d;

char* ft_screen_capture(int* w, int* h);
t_tex2d ft_tex2d_screen_capture(int w, int h, char* data);

int ft_draw_tex2d(t_tex2d tex, float position[2], float size[2]);

int main(int argc, const char* argv[]) {
    unsigned int capture_width = 0;
    unsigned int capture_height = 0;
    char* capture_data = ft_screen_capture(&capture_width, &capture_height);

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_Window* window = SDL_CreateWindow(
		"Hello, OpenGL!",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		1920,
		1080,
		SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
	);

	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, context);
	SDL_GL_SetSwapInterval(1);

	gladLoadGL();

    // SECTION: Buffer objects

    GLuint vert_arr;
    glGenVertexArrays(1, &vert_arr);
    glBindVertexArray(vert_arr);

    GLuint vert_buf;
    glGenBuffers(1, &vert_buf);
    glBindBuffer(GL_ARRAY_BUFFER, vert_buf);

    GLuint elem_buf;
    glGenBuffers(1, &elem_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elem_buf);

    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    };

    GLuint indices[] = {
        0, 1, 2,
        1, 2, 3
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*) (0 * sizeof(GLfloat)));
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*) (3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*) (7 * sizeof(GLfloat)));

    // SECTION: Shaders

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

    // SECTION: Textures

    t_tex2d capture_texture = ft_tex2d_screen_capture(capture_width, capture_height, capture_data); 

	int exit = 0;
	while(!exit) {
		SDL_Event event = { 0 };
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT: {
					exit = 1;
				} break;

                case SDL_KEYDOWN: {
                    if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        exit = 1;
                    }
                }
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        
        glUseProgram(sh_prog);
        glBindVertexArray(vert_arr);
        glBindTextureUnit(0, capture_texture.id);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(window);
	}

    free(capture_data);

    glDeleteVertexArrays(1, &vert_arr);
    glDeleteBuffers(1, &vert_buf);
    glDeleteBuffers(1, &elem_buf);
    glDeleteTextures(1, &capture_texture.id);
    glDeleteProgram(sh_prog);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}


char* ft_screen_capture(int* w, int* h) {
    Display* x_display = XOpenDisplay(NULL);
    Window x_root = DefaultRootWindow(x_display);

    XWindowAttributes x_window_attrib = { 0 };
    XGetWindowAttributes(x_display, x_root, &x_window_attrib);

    *w = x_window_attrib.width;
    *h = x_window_attrib.height;

    XImage* x_image = XGetImage(
        x_display, x_root,
        0, 0,
        *w, *h,
        AllPlanes,
        ZPixmap
    );

    char* data = (unsigned char*) calloc(*w * *h * 4, sizeof(char));
    if(!data) {
        fprintf(stderr, "[ ERR ] X11: %s\n", strerror(errno));

        XDestroyImage(x_image);
        XCloseDisplay(x_display);

        return NULL;
    }

    for(int i = 0; i < *w * *h * 4; i += 4) {
        data[i + 0] = x_image->data[i + 2];
        data[i + 1] = x_image->data[i + 1];
        data[i + 2] = x_image->data[i + 0];
        data[i + 3] = x_image->data[i + 3];
    }

    fprintf(stdout, "[ INFO ] X11: Captured the screen\n");
    fprintf(stdout, "[ INFO ] X11: Width: %i\n", *w);
    fprintf(stdout, "[ INFO ] X11: Height: %i\n", *h);

    XDestroyImage(x_image);
    XCloseDisplay(x_display);

    return data;
}

t_tex2d ft_tex2d_screen_capture(int w, int h, char* data) {
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

int ft_draw_tex2d(t_tex2d tex, float position[2], float size[2]) {
    // TODO: Implement a texture drawing function
    // I wonder if I should use a global OpenGL objects or local
    // Will think about that tomorrow, gn o/

    return 1;
}
