#include "../../Include/Common.h"
#include "Teapot_data.h"

using namespace glm;
using namespace std;

GLuint program;
GLuint PLoc;  // Projection matrix
GLuint InnerLoc;  // Inner tessellation paramter
GLuint OuterLoc;  // Outer tessellation paramter

GLfloat Inner = 1.0;
GLfloat Outer = 1.0;
mat4 projection ;

const char *vert[] = 
{
	"#version 410 core                       \n"
	"                                        \n"
	"layout (location = 0) in vec4 position; \n"
	"                                        \n"
	"void main(void)                         \n"
	"{                                       \n"
	"    gl_Position =position;              \n"
    "}                                       \n"
};

const char *tcs[] =
{
    "#version 410 core                                                              \n"
	"                                                                               \n"
	"layout (vertices = 16) out;                                                    \n"
	"                                                                               \n"
	"uniform float Inner;                                                           \n"
	"uniform float Outer;                                                           \n"
	"                                                                               \n"
    "void main()                                                                    \n"
	"{                                                                              \n"
	"    gl_TessLevelInner[0] = Inner;                                              \n"
	"    gl_TessLevelInner[1] = Inner;                                              \n"
	"    gl_TessLevelOuter[0] = Outer;                                              \n"
	"    gl_TessLevelOuter[1] = Outer;                                              \n"
	"    gl_TessLevelOuter[2] = Outer;                                              \n"
	"    gl_TessLevelOuter[3] = Outer;                                              \n"
	"    gl_out[gl_InvocationID].gl_Position =	gl_in[gl_InvocationID].gl_Position; \n"
	"}                                                                              \n"
};

const char *tes[] = 
{
	"#version 410 core                                               \n"
	"	                                                             \n"
	"layout(quads, equal_spacing, ccw) in;	                         \n"
	"	                                                             \n"
	"uniform mat4 mvp;                                               \n"
	"	                                                             \n"
	"float B(int i, float u)	                                     \n"
	"{	                                                             \n"
	"    const vec4 bc = vec4(1, 3, 3, 1);	                         \n"
	"    	                                                         \n"
	"    return bc[i] * pow(u, i) * pow(1.0 - u, 3 - i); 	         \n"
	"}	                                                             \n"
	"	                                                             \n"
	"void main()                                                     \n"
	"{	                                                             \n"
	"    vec4 pos = vec4(0.0);                                       \n"
	"	                                                             \n"
	"    float u = gl_TessCoord.x;                                   \n"
	"    float v = gl_TessCoord.y;                                   \n"
	"    	                                                         \n"
	"    for(int j = 0; j < 4; ++j)                                  \n"
	"    {                                                           \n"
	"        for(int i = 0; i < 4; ++i)                              \n"
	"        {                                                       \n"
	"            pos += B(i, u) * B(j, v) * gl_in[4*j+i].gl_Position;\n"
	"        }	                                                     \n"
	"    }                                                           \n"
	"    gl_Position = mvp * pos;                                    \n"
	"}                                                               \n"
};

const char *frag[] = 
{
	"#version 410 core                    \n"
	"                                     \n"
	"out vec4 color;                      \n"
	"                                     \n"
	"void main(void)                      \n"
	"{                                    \n"
	"	color = vec4(1.0, 0.0, 1.0, 1.0); \n"
	"}	                                  \n"
};

void My_Init()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	GLuint vs;
    GLuint fs;
	GLuint tcs_shader;
	GLuint tes_shader;

	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vert, 0);
	glCompileShader(vs);
	printGLShaderLog(vs);
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, frag, 0);
    glCompileShader(fs);
    printGLShaderLog(fs);

	tcs_shader = glCreateShader(GL_TESS_CONTROL_SHADER);
	glShaderSource(tcs_shader, 1, tcs, 0);
	glCompileShader(tcs_shader);
	printGLShaderLog(tcs_shader);
	
	tes_shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
	glShaderSource(tes_shader, 1, tes, 0);
	glCompileShader(tes_shader);
	printGLShaderLog(tes_shader);
	
	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, tcs_shader);
	glAttachShader(program, tes_shader);
    glAttachShader(program, fs);

	glLinkProgram(program);
	glUseProgram(program);

	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// Create and initialize a buffer object
	enum { ArrayBuffer, ElementBuffer, NumVertexBuffers };
	GLuint buffers[NumVertexBuffers];
	glGenBuffers( NumVertexBuffers, buffers );
	glBindBuffer( GL_ARRAY_BUFFER, buffers[ArrayBuffer] );
	glBufferData( GL_ARRAY_BUFFER, sizeof(TeapotVertices),	TeapotVertices, GL_STATIC_DRAW );
	
	glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 0, 0);
	glEnableVertexAttribArray( 0 );

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[ElementBuffer]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(TeapotIndices),TeapotIndices, GL_STATIC_DRAW);

	PLoc = glGetUniformLocation(program, "mvp");
	InnerLoc = glGetUniformLocation(program, "Inner");
	OuterLoc = glGetUniformLocation(program, "Outer");

	glUniform1f(InnerLoc, Inner);
	glUniform1f(OuterLoc, Outer);

    glPatchParameteri(GL_PATCH_VERTICES, NumTeapotVerticesPerPatch);
	
    glClearColor( 0.0, 0.0, 0.0, 1.0 );
}

void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	mat4 modelview = translate(mat4(),vec3(-0.0, -1.5,-1.0f));
	mat4 mvp = projection * modelview;

	glUniformMatrix4fv(PLoc, 1, GL_FALSE, value_ptr(mvp));
	glUseProgram(program);
	glDrawElements(GL_PATCHES, NumTeapotVertices,GL_UNSIGNED_INT, 0);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	projection = perspective(deg2rad(60.0f), viewportAspect, 0.1f, 1000.0f);
	projection = projection * lookAt(vec3(0.0f, 0.0f, 10.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void My_Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'i': 
		Inner = std::max(Inner - 1.0f, 0.0f);
		break;
	case 'I':
		Inner = std::min(Inner + 1.0f, 64.0f);
		break;
	case 'o': 
		Outer = std::max(Outer - 1.0f, 1.0f);
		break;
	case 'O':
		Outer = std::min(Outer + 1.0f, 64.0f);
		break;
	case 'P':
		Outer = std::min(Outer + 1.0f, 64.0f);
		Inner = std::min(Inner + 1.0f, 64.0f);
		break;
	case 'p':
		Outer = std::max(Outer - 1.0f, 1.0f);
		Inner = std::max(Inner - 1.0f, 0.0f);
		break;
	}
	printf("gl_TessLeveInner = %.2f, gl_TessLevelOuter = %.2f\n", Inner, Outer);
	glUniform1f(InnerLoc, Inner);
	glUniform1f(OuterLoc, Outer);
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
	glutKeyboardFunc(My_Keyboard);
	glutTimerFunc(16, My_Timer, 0);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}
