#include "../../Include/Common.h"

static const char * vs_source[] =
{
	"#version 410 core                                                 \n"
	"                                                                  \n"
	"void main(void)                                                   \n"
	"{                                                                 \n"
	"    const vec4 vertices[] = vec4[](vec4( 0.25, -0.25, 0.5, 1.0),  \n"
	"                                   vec4(-0.25, -0.25, 0.5, 1.0),  \n"
	"                                   vec4( 0.25,  0.25, 0.5, 1.0)); \n"
	"                                                                  \n"
	"    gl_Position = vertices[gl_VertexID];                          \n"
	"}                                                                 \n"
};

static const char * tcs_source[] =
{
	"#version 410 core                                                                 \n"
	"                                                                                  \n"
	"layout (vertices = 3) out;                                                        \n"
	"                                                                                  \n"
	"void main(void)                                                                   \n"
	"{                                                                                 \n"
	"    if (gl_InvocationID == 0)                                                     \n"
	"    {                                                                             \n"
	"        gl_TessLevelInner[0] = 5.0;                                               \n"
	"        gl_TessLevelOuter[0] = 5.0;                                               \n"
	"        gl_TessLevelOuter[1] = 5.0;                                               \n"
	"        gl_TessLevelOuter[2] = 5.0;                                               \n"
	"    }                                                                             \n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;     \n"
	"}                                                                                 \n"
};

static const char * tes_source[] =
{
	"#version 410 core                                                                 \n"
	"                                                                                  \n"
	"layout (triangles, equal_spacing, cw) in;                                         \n"
	"                                                                                  \n"
	"void main(void)                                                                   \n"
	"{                                                                                 \n"
	"    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +                       \n"
	"                  (gl_TessCoord.y * gl_in[1].gl_Position) +                       \n"
	"                  (gl_TessCoord.z * gl_in[2].gl_Position);                        \n"
	"}                                                                                 \n"
};

static const char * fs_source[] =
{
	"#version 410 core                                                 \n"
	"                                                                  \n"
	"out vec4 color;                                                   \n"
	"                                                                  \n"
	"void main(void)                                                   \n"
	"{                                                                 \n"
	"    color = vec4(1.0, 0.0, 0.0, 1.0);                             \n"
	"}                                                                 \n"
};

GLuint          program;
GLuint          vao;

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);

	GLuint tcs = glCreateShader(GL_TESS_CONTROL_SHADER);
	glShaderSource(tcs, 1, tcs_source, NULL);
	glCompileShader(tcs);

	GLuint tes = glCreateShader(GL_TESS_EVALUATION_SHADER);
	glShaderSource(tes, 1, tes_source, NULL);
	glCompileShader(tes);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fs_source, NULL);
	glCompileShader(fs);

	glAttachShader(program, vs);
	glAttachShader(program, tcs);
	glAttachShader(program, tes);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);
	glDrawArrays(GL_PATCHES, 0, 3);
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