#include "../../Include/Common.h"

using namespace glm;

static const char * grass_vs_source[] =
{
    "#version 410 core                                                                                           \n"
    "                                                                                                            \n"
    "in vec4 vVertex;                                                                                            \n"
    "                                                                                                            \n"
    "out vec4 color;                                                                                             \n"
    "                                                                                                            \n"
    "uniform mat4 mvpMatrix;                                                                                     \n"
    "                                                                                                            \n"
    "int random(int seed, int iterations)                                                                        \n"
    "{                                                                                                           \n"
    "    int value = seed;                                                                                       \n"
    "    int n;                                                                                                  \n"
    "                                                                                                            \n"
    "    for (n = 0; n < iterations; n++) {                                                                      \n"
    "        value = ((value >> 7) ^ (value << 9)) * 15485863;                                                   \n"
    "    }                                                                                                       \n"
    "                                                                                                            \n"
    "    return value;                                                                                           \n"
    "}                                                                                                           \n"
    "                                                                                                            \n"
    "vec4 random_vector(int seed)                                                                                \n"
    "{                                                                                                           \n"
    "    int r = random(gl_InstanceID, 4);                                                                       \n"
    "    int g = random(r, 2);                                                                                   \n"
    "    int b = random(g, 2);                                                                                   \n"
    "    int a = random(b, 2);                                                                                   \n"
    "                                                                                                            \n"
    "    return vec4(float(r & 0x3FF) / 1024.0,                                                                  \n"
    "                float(g & 0x3FF) / 1024.0,                                                                  \n"
    "                float(b & 0x3FF) / 1024.0,                                                                  \n"
    "                float(a & 0x3FF) / 1024.0);                                                                 \n"
    "}                                                                                                           \n"
    "                                                                                                            \n"
    "mat4 construct_rotation_matrix(float angle)                                                                 \n"
    "{                                                                                                           \n"
    "    float st = sin(angle);                                                                                  \n"
    "    float ct = cos(angle);                                                                                  \n"
    "                                                                                                            \n"
    "    return mat4(vec4(ct, 0.0, st, 0.0),                                                                     \n"
    "                vec4(0.0, 1.0, 0.0, 0.0),                                                                   \n"
    "                vec4(-st, 0.0, ct, 0.0),                                                                    \n"
    "                vec4(0.0, 0.0, 0.0, 1.0));                                                                  \n"
    "}                                                                                                           \n"
    "                                                                                                            \n"
    "void main(void)                                                                                             \n"
    "{                                                                                                           \n"
    "    vec4 offset = vec4(float(gl_InstanceID >> 10) - 512.0,                                                  \n"
    "                       0.0f,                                                                                \n"
    "                       float(gl_InstanceID & 0x3FF) - 512.0,                                                \n"
    "                       0.0f);                                                                               \n"
    "    int number1 = random(gl_InstanceID, 3);                                                                 \n"
    "    int number2 = random(number1, 2);                                                                       \n"
    "    offset += vec4(float(number1 & 0xFF) / 256.0,                                                           \n"
    "                   0.0f,                                                                                    \n"
    "                   float(number2 & 0xFF) / 256.0,                                                           \n"
    "                   0.0f);                                                                                   \n"
    "    float angle = float(random(number2, 2) & 0x3FF) / 1024.0;                                               \n"
    "                                                                                                            \n"
    "    vec2 texcoord = offset.xz / 1024.0 + vec2(0.5);                                                         \n"
    "                                                                                                            \n"
    "    float bend_factor = float(random(number2, 7) & 0x3FF) / 1024.0;                                         \n"
    "    float bend_amount = cos(vVertex.y);                                                                     \n"
    "                                                                                                            \n"
    "    mat4 rot = construct_rotation_matrix(angle);                                                            \n"
    "    vec4 position = (rot * (vVertex + vec4(0.0, 0.0, bend_amount * bend_factor, 0.0))) + offset;            \n"
    "                                                                                                            \n"
    "    position *= vec4(1.0, float(number1 & 0xFF) / 256.0 * 0.9 + 0.3, 1.0, 1.0);                             \n"
    "                                                                                                            \n"
    "    gl_Position = mvpMatrix * position;                                                                     \n"
    "    color = vec4(random_vector(gl_InstanceID).xyz * vec3(0.1, 0.5, 0.1) + vec3(0.1, 0.4, 0.1), 1.0);        \n"
    "}                                                                                                           \n"
};

static const char * grass_fs_source[] =
{
    "#version 410 core                \n"
    "                                 \n"
    "in vec4 color;                   \n"
    "                                 \n"
    "out vec4 output_color;           \n"
    "                                 \n"
    "void main(void)                  \n"
    "{                                \n"
    "    output_color = color;        \n"
    "}                                \n"
};

GLuint grass_buffer;
GLuint grass_vao;
GLuint grass_program;
mat4 proj_matrix;

struct
{
	GLint mvpMatrix;
} uniforms;

void My_Init()
{
    static const GLfloat grass_blade[] =
    {
        -0.3f, 0.0f,
         0.3f, 0.0f,
        -0.20f, 1.0f,
         0.1f, 1.3f,
        -0.05f, 2.3f,
         0.0f, 3.3f
    };

    glGenBuffers(1, &grass_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, grass_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grass_blade), grass_blade, GL_STATIC_DRAW);

    glGenVertexArrays(1, &grass_vao);
    glBindVertexArray(grass_vao);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    grass_program = glCreateProgram();
    GLuint grass_vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint grass_fs = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(grass_vs, 1, grass_vs_source, NULL);
    glShaderSource(grass_fs, 1, grass_fs_source, NULL);

    glCompileShader(grass_vs);
    glCompileShader(grass_fs);

    glAttachShader(grass_program, grass_vs);
    glAttachShader(grass_program, grass_fs);

    glLinkProgram(grass_program);
    glDeleteShader(grass_fs);
    glDeleteShader(grass_vs);

    uniforms.mvpMatrix = glGetUniformLocation(grass_program, "mvpMatrix");
}

void My_Display()
{
    float t = glutGet(GLUT_ELAPSED_TIME) * 0.00002f;
    float r = 550.0f;

    static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    static const GLfloat one = 1.0f;
    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_DEPTH, 0, &one);

    mat4 mv_matrix = lookAt(vec3(sinf(t) * r, 25.0f, cosf(t) * r),
                                            vec3(0.0f, -50.0f, 0.0f),
                                            vec3(0.0, 1.0, 0.0));

    glUseProgram(grass_program);
    glUniformMatrix4fv(uniforms.mvpMatrix, 1, GL_FALSE, &(proj_matrix * mv_matrix)[0][0]);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glBindVertexArray(grass_vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, 1024 * 1024);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(deg2rad(45.0f), viewportAspect, 0.1f, 1000.0f);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
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
	glutTimerFunc(16, My_Timer, 0);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}
