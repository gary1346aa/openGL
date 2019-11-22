#include "../../Include/Common.h"
#define GLM_SWIZZLE

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;

struct
{
	GLuint colormap;
} textures;

struct
{
	GLint c;
	GLint colormap;
} uniforms;

static const char * vs_source[] =
{
	"#version 410 core                                                  \n"
	"                                                                   \n"
	"layout (location = 0) in vec2 position;                            \n"
	"layout (location = 1) in vec2 texcoord;                            \n"
	"out vec2 m_texcoord;                                               \n"
	"out vec2 initial_z;                                                \n"
	"                                                                   \n"
	"void main(void)                                                    \n"
	"{                                                                  \n"
	"    gl_Position = vec4(position,0.0,1.0);							\n"
	"    m_texcoord = texcoord;                                         \n"
	"	 initial_z = position;                                          \n"
	"}																	\n"
};

static const char * fs_source[] =
{
	"#version 410 core                                                                      \n"
	"                                                                                       \n"
	"in vec2 m_texcoord;																	\n"
	"in vec2 initial_z;                                                                     \n"
	"                                                                                       \n"
	"out vec4 color;                                                                        \n"
	"                                                                                       \n"
	"uniform sampler1D colormap;                                                            \n"
	"uniform vec2 c;                                                                        \n"
	"                                                                                       \n"
	"void main(void)                                                                        \n"
	"{                                                                                      \n"
	"    vec2 z = initial_z;                                                                \n"
	"    int iterations = 0;                                                                \n"
	"    const float threshold_squared = 16.0;                                              \n"
	"    const int max_iterations = 256;                                                    \n"
	"	 int i = 0;																	    	\n"
	"	 for(i = 0; i < max_iterations; ++i)                                                \n"
	"    {												                                    \n"
	"	     float x = (z.x * z.x - z.y * z.y) + c.x;									    \n"
	"	     float y = (z.y * z.x + z.x * z.y) + c.y;									    \n"
	"	     if((x * x + y * y) > 4.0) break;                                               \n"
	"	     z.x = x;                                                                       \n"
	"	     z.y = y;                                                                       \n"
    "    }                                                                                  \n"
	"	 color = texture(colormap, float(i) / 100.0);                                       \n"
	"}                                                                                      \n"

};


void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// ----- Begin Initialize Shader Program -----
	GLuint program = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertexShader, 1, vs_source, NULL);
	glShaderSource(fragmentShader, 1, fs_source, NULL);
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	printGLShaderLog(vertexShader);
	printGLShaderLog(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glUseProgram(program);
	uniforms.c = glGetUniformLocation(program, "c");
	uniforms.colormap = glGetUniformLocation(program, "colormap");
	// ----- End Initialize Shader Program -----

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// ----- Begin Initialize Buffer & Vertex Array Attribute -----
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	// Interleaved vec2 position / vec2 texcoord data
	float data[] = {
		 1.0f, -1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 1.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	// ----- End Initialize Buffer & Vertex Array Attribute -----

	// ----- Begin Initialize Colormap Texture -----
	TextureData tdata = loadPNG("../../Media/Textures/pal.png"); 
	glGenTextures(1, &textures.colormap);
	glBindTexture(GL_TEXTURE_1D, textures.colormap);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, tdata.width, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	delete[] tdata.data;
	// ----- End Initialize Colormap Texture -----
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float f_timer_cnt = glutGet(GLUT_ELAPSED_TIME);
	float t = f_timer_cnt* 0.001f;
	float cx = (sin(cos(t / 10) * 10) + cos(t * 2.0) / 4.0 + sin(t * 3.0) / 6.0) * 0.8;
	float cy = (cos(sin(t / 10) * 10) + sin(t * 2.0) / 4.0 + cos(t * 3.0) / 6.0) * 0.8;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, textures.colormap);
	glUniform1i(uniforms.colormap, 0);
	glUniform2f(uniforms.c, cx, cy);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
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