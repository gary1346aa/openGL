#include "../../Include/Common.h"

static const char * square_vs_source[] =
{
    "#version 410 core                                                               \n"
    "                                                                                \n"
    "layout (location = 0) in vec4 position;                                         \n"
    "layout (location = 1) in vec4 instance_color;                                   \n"
    "layout (location = 2) in vec4 instance_position;                                \n"
    "                                                                                \n"
    "out Fragment                                                                    \n"
    "{                                                                               \n"
    "    vec4 color;                                                                 \n"
    "} fragment;                                                                     \n"
    "                                                                                \n"
    "void main(void)                                                                 \n"
    "{                                                                               \n"
    "    gl_Position = (position + instance_position) * vec4(0.25, 0.25, 1.0, 1.0);  \n"
    "    fragment.color = instance_color;                                            \n"
    "}                                                                               \n"
};

static const char * square_fs_source[] =
{
    "#version 410 core                                                                \n"
    "precision highp float;                                                           \n"
    "                                                                                 \n"
    "in Fragment                                                                      \n"
    "{                                                                                \n"
    "    vec4 color;                                                                  \n"
    "} fragment;                                                                      \n"
    "                                                                                 \n"
    "out vec4 color;                                                                  \n"
    "                                                                                 \n"
    "void main(void)                                                                  \n"
    "{                                                                                \n"
    "    color = fragment.color;                                                      \n"
    "}                                                                                \n"
};

GLuint      square_buffer;
GLuint      square_vao;
GLuint      square_program;


void My_Init()
{
    static const GLfloat square_vertices[] =
    {
        -1.0f, -1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f,
            1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    static const GLfloat instance_colors[] =
    {
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f
    };

    static const GLfloat instance_positions[] =
    {
        -2.0f, -2.0f, 0.0f, 0.0f,
            2.0f, -2.0f, 0.0f, 0.0f,
            2.0f,  2.0f, 0.0f, 0.0f,
        -2.0f,  2.0f, 0.0f, 0.0f
    };

    GLuint offset = 0;

    glGenVertexArrays(1, &square_vao);
    glGenBuffers(1, &square_buffer);
    glBindVertexArray(square_vao);
    glBindBuffer(GL_ARRAY_BUFFER, square_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices) + sizeof(instance_colors) + sizeof(instance_positions), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(square_vertices), square_vertices);
    offset += sizeof(square_vertices);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(instance_colors), instance_colors);
    offset += sizeof(instance_colors);
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(instance_positions), instance_positions);
    offset += sizeof(instance_positions);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)sizeof(square_vertices));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)(sizeof(square_vertices) + sizeof(instance_colors)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    square_program = glCreateProgram();

    GLuint square_vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(square_vs, 1, square_vs_source, NULL);
    glCompileShader(square_vs);
    glAttachShader(square_program, square_vs);
    GLuint square_fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(square_fs, 1, square_fs_source, NULL);
    glCompileShader(square_fs);
    glAttachShader(square_program, square_fs);

    glLinkProgram(square_program);
    glDeleteShader(square_vs);
    glDeleteShader(square_fs);
}

void My_Display()
{
    static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, black);

    glUseProgram(square_program);
    glBindVertexArray(square_vao);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, 4);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
}

int main(int argc, char *argv[])
{
    // Change working directory to source code path
    chdir(__FILEPATH__);
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow(__FILENAME__); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	printGLContextInfo();
	My_Init();
	////////////////////

	// Register GLUT callback functions.
	///////////////////////////////
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}