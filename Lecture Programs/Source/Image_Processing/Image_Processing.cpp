#include "../../Include/Common.h"
#define GLM_SWIZZLE

#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3

#define Shader_Blur 4
#define Shader_Quantization 5
#define Shader_DoG 6

int shader_now = 0;

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;

mat4 mvp;
GLint um4mvp;

GLuint hawk_texture;
GLint Shader_now_Loc;
int defalut_w = 800;
int defalut_h = 600;

static const char * vs_source[] =
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

static const char * fs_source[] =
{
	"#version 410\n"
	"uniform sampler2D tex; \n"
	"out vec4 color;\n"
	"uniform int shader_now ;\n"
	"in VS_OUT\n"
	"{\n"
	"   vec2 texcoord;\n"
	"} fs_in;\n"
	"float sigma_e = 2.0f;\n"
	"float sigma_r = 2.8f;\n"
	"float phi = 3.4f;\n"
	"float tau = 0.99f;\n"
	"float twoSigmaESquared = 2.0 * sigma_e * sigma_e;		\n"
	"float twoSigmaRSquared = 2.0 * sigma_r * sigma_r;		\n"
	"int halfWidth = int(ceil( 2.0 * sigma_r ));\n"
	"vec2 img_size = vec2(1024,768);\n"
	"int nbins = 8;\n"
	"void main(void)\n"
	"{\n"
	" \n"
	"	switch(shader_now)\n"
	"	{\n"
	"		case(2):\n"
	"			{\n"
	"				\n"
	"				vec2 sum = vec2(0.0);\n"
	"				vec2 norm = vec2(0.0);\n"
	"				int kernel_count = 0;\n"
	"			for ( int i = -halfWidth; i <= halfWidth; ++i ) {\n"
	"			for ( int j = -halfWidth; j <= halfWidth; ++j ) {\n"
	"					float d = length(vec2(i,j));\n"
	"					vec2 kernel = vec2( exp( -d * d / twoSigmaESquared ), \n"
	"										exp( -d * d / twoSigmaRSquared ));\n"
	"					vec4 c = texture(tex, fs_in.texcoord + vec2(i,j) / img_size);\n"
	"					vec2 L = vec2(0.299 * c.r + 0.587 * c.g + 0.114 * c.b);\n"
	"														\n"
	"					norm += 2.0 * kernel;\n"
	"					sum += kernel * L;\n"
	"				}\n"
	"			}\n"
	"			sum /= norm;\n"
	"			\n"
	"			float H = 100.0 * (sum.x - tau * sum.y);\n"
	"			float edge = ( H > 0.0 )? 1.0 : 2.0 * smoothstep(-2.0, 2.0, phi * H );\n"
	"				\n"
	"		   color = vec4(edge,edge,edge,1.0 );\n"
	"				break;\n"
	"			}\n"
	"		case(1):\n"
	"			{\n"
	"				vec4 texture_color = texture(tex,fs_in.texcoord);\n"
	"   \n"
	"			float r = floor(texture_color.r * float(nbins)) / float(nbins);\n"
	"			 float g = floor(texture_color.g * float(nbins)) / float(nbins);\n"
	"			float b = floor(texture_color.b * float(nbins)) / float(nbins); \n"
	"			color = vec4(r,g,b,texture_color.a);\n"
	"				break;\n"
	"			}\n"
	"		case(0):\n"
	"			{\n"
	"				color = vec4(0);	\n"
	"			int n = 0;\n"
	"			int half_size = 3;\n"
	"			for ( int i = -half_size; i <= half_size; ++i ) {        \n"
	"				for ( int j = -half_size; j <= half_size; ++j ) {\n"
	"					 vec4 c = texture(tex, fs_in.texcoord + vec2(i,j)/img_size); \n"
	"					 color+= c;\n"
	"				     n++;\n"
	"					}\n"
	"				}\n"
	"				color /=n;\n"
	"					break;\n"
	"			}\n"
	"			\n"
	"	\n"
	"	}\n"
	"}\n"

};


void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

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
	um4mvp = glGetUniformLocation(program, "um4mvp");
	glUseProgram(program);

	Shader_now_Loc = glGetUniformLocation(program, "shader_now");

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	float data[] = {
		1.0f,-1.0f,1.0f,0.0f,
		-1.0f,-1.0f,0.0f,0.0f,
		-1.0f,1.0f,0.0f,1.0f,
		1.0f,1.0f,1.0f,1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

	TextureData tdata = loadPNG("../../Media/Textures/hawk.png"); 

	glGenTextures( 1, &hawk_texture );
	glBindTexture( GL_TEXTURE_2D, hawk_texture);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	///////////////////////////////////////////////////////////////////////////
	printf("\nNote : Use Right Click Menu to switch Effect\n");
	//////////////////////////////////////////////////////////////////////////
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float f_timer_cnt = timer_cnt / 255.0f;

	
	glUniform1i(Shader_now_Loc,shader_now);
	glUniformMatrix4fv(um4mvp, 1, GL_FALSE, value_ptr(mvp));
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;
	mvp = ortho(-1 * viewportAspect, 1 * viewportAspect, -1.0f, 1.0f);
	mvp = mvp * lookAt(vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
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

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case Shader_Blur:
		shader_now = 0;
		break;
	case Shader_Quantization:
		shader_now = 1;
		break;
	case Shader_DoG:
		shader_now = 2;
		break;
	default:
		break;
	}
	glutPostRedisplay();
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
	glutInitWindowSize(defalut_w, defalut_h);
	glutCreateWindow(__FILENAME__); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	printGLContextInfo();
	My_Init();
	////////////////////

	// Create a menu and bind it to mouse right button.
	////////////////////////////
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);
	int shader = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddSubMenu("Shader", shader);

	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(shader);
	glutAddMenuEntry("Blur", Shader_Blur);
	glutAddMenuEntry("Quantization", Shader_Quantization);
	glutAddMenuEntry("DoG", Shader_DoG);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	////////////////////////////

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