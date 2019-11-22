#include "../../Include/Common.h"

static const char * vs_source[] =
{
	"#version 410 core                                                              \n"
	"                                                                               \n"
	"uniform vec2 offset;                                                           \n"
	"                                                                               \n"
	"out vec2 tex_coord;                                                            \n"
	"                                                                               \n"
	"void main(void)                                                                \n"
	"{                                                                              \n"
	"    const vec4 vertices[] = vec4[](vec4(-0.45, -0.45, 0.0, 1.0),               \n"
	"                                   vec4( 0.45, -0.45, 0.0, 1.0),               \n"
	"                                   vec4(-0.45,  0.45, 0.0, 1.0),               \n"
	"                                   vec4( 0.45,  0.45, 0.0, 1.0));              \n"
	"                                                                               \n"
	"    gl_Position = vertices[gl_VertexID] + vec4(offset, 0.0, 0.0);              \n"
	"    tex_coord = vertices[gl_VertexID].xy * 3.0 + vec2(0.45 * 3);               \n"
	"}                                                                              \n"
};

static const char * fs_source[] =
{
	"#version 410 core                                                              \n"
	"                                                                               \n"
	"uniform sampler2D s;                                                           \n"
	"                                                                               \n"
	"out vec4 color;                                                                \n"
	"                                                                               \n"
	"in vec2 tex_coord;                                                             \n"
	"                                                                               \n"
	"void main(void)                                                                \n"
	"{                                                                              \n"
	"    color = texture(s, tex_coord);                                             \n"
	"}                                                                              \n"
};

GLuint          program;
GLuint          vao;
GLuint          texture;

struct
{
	GLint       mvp;
	GLint       offset;
} uniforms;

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fs_source, NULL);
	glCompileShader(fs);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenTextures(1, &texture);

	// Load texture from file
	TextureData tex = loadPNG("../../Media/Textures/rightarrows.png");
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	delete[] tex.data;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static const GLenum wrapmodes[] = { GL_CLAMP_TO_EDGE, GL_REPEAT, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT };
	static const GLfloat yellow[] = { 0.4f, 0.4f, 0.0f, 1.0f };

	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

	glUseProgram(program);
	static const float offsets[] =
	{
		-0.5f, -0.5f,
		 0.5f, -0.5f,
		-0.5f,  0.5f,
		 0.5f,  0.5f
	};

	GLint loc_offset;
	loc_offset = glGetUniformLocation(program, "offset");

	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, yellow);

	for (int i = 0; i < 4; i++)
	{
		glUniform2fv(loc_offset, 1, &offsets[i * 2]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapmodes[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapmodes[i]);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

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