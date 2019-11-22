#include "../../Include/Common.h"
#define GLM_SWIZZLE

#include <cstdio>
#include <cstdlib>

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


static const char * vs_source[] =
{
	"#version 410 core                                                  \n"
	"                                                                   \n"
	"in vec4 position;                                                  \n"
	"                                                                   \n"
	"out VS_OUT                                                         \n"
	"{                                                                  \n"
	"    vec4 color;                                                    \n"
	"} vs_out;                                                          \n"
	"                                                                   \n"
	"uniform mat4 mv_matrix;                                            \n"
	"uniform mat4 proj_matrix;                                          \n"
	"                                                                   \n"
	"void main(void)                                                    \n"
	"{                                                                  \n"
	"    gl_Position = proj_matrix * mv_matrix * position;              \n"
	"    vs_out.color = position * 2.0 + vec4(0.5, 0.5, 0.5, 0.0);      \n"
	"}                                                                  \n"
};

static const char * fs_source[] =
{
	"#version 410 core                                                  \n"
	"                                                                   \n"
	"out vec4 color;                                                    \n"
	"                                                                   \n"
	"in VS_OUT                                                          \n"
	"{                                                                  \n"
	"    vec4 color;                                                    \n"
	"} fs_in;                                                           \n"
	"                                                                   \n"
	"void main(void)                                                    \n"
	"{                                                                  \n"
	"    color = fs_in.color;                                           \n"
	"}                                                                  \n"
};


static const char * vs_source2[] =
{
	"#version 410 core                                                  \n"
	"                                                                   \n"
	"layout (location = 0) in vec2 position;                            \n"
	"layout (location = 1) in vec2 texcoord;                            \n"
	"out VS_OUT                                                         \n"
	"{                                                                  \n"
	"    vec2 texcoord;                                                 \n"
	"} vs_out;                                                          \n"
	"                                                                   \n"
	"                                                                   \n"
	"void main(void)                                                    \n"
	"{                                                                  \n"
	"    gl_Position = vec4(position,0.0,1.0);							\n"
	"    vs_out.texcoord = texcoord;                                    \n"
	"}																	\n"
};


static const char * fs_source2[] =
{
	"#version 410 core                                                              \n"
	"                                                                               \n"
	"uniform sampler2D tex;                                                         \n"
	"                                                                               \n"
	"out vec4 color;                                                                \n"
	"                                                                               \n"
	"in VS_OUT                                                                      \n"
	"{                                                                              \n"
	"    vec2 texcoord;                                                             \n"
	"} fs_in;                                                                       \n"
	"                                                                               \n"
	"void main(void)                                                                \n"
	"{                                                                              \n"
	"    vec4 texture_color = texture(tex,fs_in.texcoord);							\n"
	"	 float grayscale_color = 0.2126*texture_color.r+0.7152*texture_color.g+0.0722*texture_color.b; \n"
	"    color = vec4(grayscale_color,grayscale_color,grayscale_color,1.0);			\n"
	"}                                                                              \n"
};
static const GLfloat vertex_positions[] =
{
	-0.25f,  0.25f, -0.25f,
	-0.25f, -0.25f, -0.25f,
	0.25f, -0.25f, -0.25f,

	0.25f, -0.25f, -0.25f,
	0.25f,  0.25f, -0.25f,
	-0.25f,  0.25f, -0.25f,

	0.25f, -0.25f, -0.25f,
	0.25f, -0.25f,  0.25f,
	0.25f,  0.25f, -0.25f,

	0.25f, -0.25f,  0.25f,
	0.25f,  0.25f,  0.25f,
	0.25f,  0.25f, -0.25f,

	0.25f, -0.25f,  0.25f,
	-0.25f, -0.25f,  0.25f,
	0.25f,  0.25f,  0.25f,

	-0.25f, -0.25f,  0.25f,
	-0.25f,  0.25f,  0.25f,
	0.25f,  0.25f,  0.25f,

	-0.25f, -0.25f,  0.25f,
	-0.25f, -0.25f, -0.25f,
	-0.25f,  0.25f,  0.25f,

	-0.25f, -0.25f, -0.25f,
	-0.25f,  0.25f, -0.25f,
	-0.25f,  0.25f,  0.25f,

	-0.25f, -0.25f,  0.25f,
	0.25f, -0.25f,  0.25f,
	0.25f, -0.25f, -0.25f,

	0.25f, -0.25f, -0.25f,
	-0.25f, -0.25f, -0.25f,
	-0.25f, -0.25f,  0.25f,

	-0.25f,  0.25f, -0.25f,
	0.25f,  0.25f, -0.25f,
	0.25f,  0.25f,  0.25f,

	0.25f,  0.25f,  0.25f,
	-0.25f,  0.25f,  0.25f,
	-0.25f,  0.25f, -0.25f
};

static const GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

GLuint          program;
GLuint			program2;
GLuint          vao;
GLuint          window_vao;
GLuint			vertex_shader;
GLuint			fragment_shader;
GLuint          buffer;
GLuint			window_buffer;
GLint           mv_location;
GLint           proj_location;

GLuint			FBO;
GLuint			depthRBO;
GLuint	FBODataTexture;

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

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	program2 = glCreateProgram();

	GLuint vs2 = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs2, 1, vs_source2, NULL);
	glCompileShader(vs2);
	printGLShaderLog(vs2);

	GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs2, 1, fs_source2, NULL);
	glCompileShader(fs2);
	printGLShaderLog(fs2);

	glAttachShader(program2, vs2);
	glAttachShader(program2, fs2);

	glLinkProgram(program2);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER,sizeof(vertex_positions),vertex_positions,	GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	mv_location = glGetUniformLocation(program, "mv_matrix");
	proj_location = glGetUniformLocation(program, "proj_matrix");

	glGenVertexArrays(1, &window_vao);
	glBindVertexArray(window_vao);

	glGenBuffers(1, &window_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, window_buffer);
	glBufferData(GL_ARRAY_BUFFER,sizeof(window_positions),window_positions,	GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*4, 0);
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*4, (const GLvoid*)(sizeof(GL_FLOAT)*2));

	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );

	glGenFramebuffers( 1, &FBO );

	//////////////////////////////////////////////////////////////////////////
	/////////Create RBO and Render Texture in Reshape Function////////////////
	//////////////////////////////////////////////////////////////////////////
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, FBO );
	glDrawBuffer( GL_COLOR_ATTACHMENT0 );

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static const GLfloat green[] = { 0.0f, 0.25f, 0.0f, 1.0f };
	static const GLfloat one = 1.0f;

	glClearBufferfv(GL_COLOR, 0, green);
	glClearBufferfv(GL_DEPTH, 0, &one);

	glUseProgram(program);

	glBindVertexArray(vao);
	glUniformMatrix4fv(proj_location, 1, GL_FALSE, &proj_matrix[0][0]);

	glm::mat4 Identy_Init(1.0);

	float f_timer_cnt = glutGet(GLUT_ELAPSED_TIME);
	float currentTime = f_timer_cnt* 0.001f;
	float f = (float)currentTime * 0.3f;

	glm::mat4 mv_matrix = glm::translate(Identy_Init,glm::vec3(0.0f, 0.0f, -4.0f));

	mv_matrix = glm::translate(mv_matrix,glm::vec3(sinf(2.1f * f) * 0.5f,cosf(1.7f * f) * 0.5f,	sinf(1.3f * f) * cosf(1.5f * f) * 2.0f));

	mv_matrix = glm::rotate(mv_matrix,deg2rad(currentTime*45.0f),glm::vec3(0.0f, 1.0f, 0.0f));
	mv_matrix = glm::rotate(mv_matrix,deg2rad(currentTime*81.0f),glm::vec3(1.0f, 0.0f, 0.0f));

	glUniformMatrix4fv(mv_location, 1, GL_FALSE, &mv_matrix[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
	
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glClearColor( 1.0f, 0.0f, 0.0f, 1.0f );
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture( GL_TEXTURE_2D, FBODataTexture );
	glBindVertexArray(window_vao);
	glUseProgram(program2);
	glDrawArrays(GL_TRIANGLE_FAN,0,4 );
	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;
	proj_matrix = glm::perspective(deg2rad(60.0f), viewportAspect, 0.1f, 1000.0f);

	glDeleteRenderbuffers(1,&depthRBO);
	glDeleteTextures(1,&FBODataTexture);
	glGenRenderbuffers( 1, &depthRBO );
	glBindRenderbuffer( GL_RENDERBUFFER, depthRBO );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height );

	glGenTextures( 1, &FBODataTexture );
	glBindTexture( GL_TEXTURE_2D, FBODataTexture);

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, FBO );
	glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO );
	glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBODataTexture, 0 );

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