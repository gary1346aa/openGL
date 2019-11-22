#include "../../Include/Common.h"
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define M_PI = 3.14;

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;
#include<iostream>
using namespace std;
using namespace glm;

mat4 mvp;
GLint um4mvp;
GLuint m_texture;
static unsigned int seed = 0x13371337;

static inline float random_float()
{
	float res;
	unsigned int tmp;

	seed *= 16807;

	tmp = seed ^ (seed >> 4) ^ (seed << 15);

	*((unsigned int *) &res) = (tmp >> 9) | 0x3F800000;

	return (res - 1.0f);
}

enum
{
	NUM_STARS           = 1000
};


static const char * vs_source[] =
{
	"#version 410 core                                              \n"
	"                                                               \n"
	"layout (location = 0) in vec4 position;                        \n"
	"layout (location = 1) in vec4 color;                           \n"
	"                                                               \n"
	"uniform float time;                                            \n"
	"uniform mat4 proj_matrix;                                      \n"
	"                                                               \n"
	"flat out vec4 starColor;                                       \n"
	"                                                               \n"
	"void main(void)                                                \n"
	"{                                                              \n"
	"    vec4 newVertex = position;                                 \n"
	"                                                               \n"
	"    newVertex.z += time;                                       \n"
	"    newVertex.z = fract(newVertex.z);                          \n"
	"                                                               \n"
	"    float size = (20.0 * newVertex.z * newVertex.z);           \n"
	"                                                               \n"
	"    starColor = smoothstep(1.0, 7.0, size) * color;            \n"
	"                                                               \n"
	"    newVertex.z = (999.9 * newVertex.z) - 1000.0;              \n"
	"    gl_Position = proj_matrix * newVertex;                     \n"
	"    gl_PointSize = size;                                       \n"
	"}                                                              \n"
};

static const char * fs_source[] =
{ 
	"#version 410 core                                              \n"
	"                                                               \n"
	"out vec4 color;										        \n"
	"                                                               \n"
	"uniform sampler2D tex_star;									\n"
	"flat in vec4 starColor;                                        \n"
	"                                                               \n"
	"void main(void)                                                \n"
	"{                                                              \n"
	"    color = texture(tex_star,gl_PointCoord);				    \n"
	"}                                                              \n"
};


GLuint          program;
GLuint          vao;
GLuint			vertex_shader;
GLuint			fragment_shader;
GLuint          buffer;
GLint           mv_location;
GLint           proj_location;
GLint			time_Loc;

glm::mat4 proj_matrix;

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fs_source, NULL);
	glCompileShader(fs);
	printGLShaderLog(fs);
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	proj_location = glGetUniformLocation(program, "proj_matrix");
	time_Loc = glGetUniformLocation(program, "time");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	struct star_t
	{
		glm::vec3     position;
		glm::vec3     color;
	};

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, NUM_STARS * sizeof(star_t), NULL, GL_STATIC_DRAW);

	star_t * star = (star_t *)glMapBufferRange(GL_ARRAY_BUFFER, 0, NUM_STARS * sizeof(star_t), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	int i;

	for (i = 0; i < 1000; i++)
	{
		star[i].position[0] = (random_float() * 2.0f - 1.0f) * 100.0f;
		star[i].position[1] = (random_float() * 2.0f - 1.0f) * 100.0f;
		star[i].position[2] = random_float();
		star[i].color[0] = 0.8f + random_float() * 0.2f;
		star[i].color[1] = 0.8f + random_float() * 0.2f;
		star[i].color[2] = 0.8f + random_float() * 0.2f;
	}



	glUnmapBuffer(GL_ARRAY_BUFFER);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(star_t), NULL);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(star_t), (void *)sizeof(glm::vec3));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture( GL_TEXTURE0 );

	glEnable(GL_POINT_SPRITE);
	TextureData tdata = loadPNG("../../Media/Textures/star.png");

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glGenTextures( 1, &m_texture );
	glBindTexture( GL_TEXTURE_2D, m_texture);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );;

}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static const GLfloat black[] = { 0.0f, 0, 0.0f, 1.0f };
	static const GLfloat one = 1.0f;

	glClearBufferfv(GL_COLOR, 0, black);
	glClearBufferfv(GL_DEPTH, 0, &one);

	glUseProgram(program);

	float f_timer_cnt = glutGet(GLUT_ELAPSED_TIME);
	float currentTime = f_timer_cnt* 0.001f;

	currentTime *= 0.1f;
	currentTime -= floor(currentTime);

	glUniform1f(time_Loc, currentTime);
	glUniformMatrix4fv(proj_location, 1, GL_FALSE, &proj_matrix[0][0]);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_texture);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glDrawArrays(GL_POINTS, 0, NUM_STARS);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;

	proj_matrix = glm::perspective(deg2rad(50.0f), viewportAspect, 0.1f, 1000.0f);

}

void My_Timer(int val)
{
	timer_cnt++;
	glutPostRedisplay();
	if(timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
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
	glutTimerFunc(timer_speed, My_Timer, 0); 
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}