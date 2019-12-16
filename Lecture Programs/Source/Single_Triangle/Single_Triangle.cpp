#include "../../Include/Common.h"
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <vector>
#define Scale 5

using namespace glm;
using namespace std;

struct
{
	int width;
	int height;
} viewportSize;


struct
{
	int x;
	int y;
	int px; //position uppon left button pressed.
	int py; //position uppon right button pressed.
	int rx;
	int ry;
	int dx;
	int dy;
} mousePos;

struct
{
	struct
	{
		GLint matrix;
		GLint color;
		GLint position;
		GLint mouse;
	} render;
} uniforms;

mat4 proj_matrix;
mat4 model_matrix = translate(mat4(), vec3(0.1f, -0.5f, 0.0f))*scale(mat4(), vec3(5.0));
mat4 view_matrix;
mat4 mvp_matrix;
bool rot = false;
bool draw = false;

typedef struct object {
	float *vertex;
	int *face;
	int faceCount, vertexCount;
}object;

int *draw_faceidx;
int draw_count = 0;

object* parser(char* filename) {

	FILE *fp = fopen(filename, "r");
	object *obj = (object*)malloc(sizeof(object));

	while (!feof(fp)) {
		int vertexCount, faceCount;
		fscanf(fp, "%*c %*s %*s %*s %*s %*s %*s %*c %*s %*s %*c %d %*c %*s %*s %*c %d %*c", &vertexCount, &faceCount);

		obj->vertex = (float*)malloc(3 * vertexCount * sizeof(float));
		obj->face = (int*)malloc(3 * faceCount * sizeof(int));
		obj->vertexCount = vertexCount;
		obj->faceCount = faceCount;
		printf("vertexCount = %d, faceCount = %d\n", vertexCount, faceCount);
		
		for (int i = 0; i < vertexCount; ++i)
			fscanf(fp, "%*c %e %e %e %*c", &obj->vertex[3 * i], &obj->vertex[3 * i + 1], &obj->vertex[3 * i + 2]);


		for (int i = 0; i < faceCount; ++i)
		{
			fscanf(fp, "%*c %d %d %d %*c", &obj->face[3 * i], &obj->face[3 * i + 1], &obj->face[3 * i + 2]);
			--obj->face[3 * i];
			--obj->face[3 * i + 1];
			--obj->face[3 * i + 2];
		}
	}

	return obj;
}

object *bunny;

static const char * vs_source[] =
{
	"#version 410 core						  \n"		
	"layout(location = 0) in vec4 vPosition;  \n"
	"uniform mat4 mvp_matrix;   			  \n"
	"void main()                              \n"
	"{                                        \n"
	"   gl_Position = mvp_matrix * vPosition; \n"
	"}                                        \n"
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
	"        gl_TessLevelInner[0] = 2.0;                                               \n"
	"        gl_TessLevelOuter[0] = 2.0;                                               \n"
	"        gl_TessLevelOuter[1] = 2.0;                                               \n"
	"        gl_TessLevelOuter[2] = 2.0;                                               \n"
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


const char *gs_source[] =
{
	"#version 410 core                                                                      \n"
	"                                                                                       \n"
	"layout(triangles, invocations = 16) in;                                                \n"
	"layout(triangle_strip, max_vertices = 3) out;                                          \n"
	"                                                                                       \n"
	"out vec4 gs_color;                                                                     \n"
	"uniform vec4 mouse;                                                                    \n"
	"                                                                                       \n"
	"void main(void)                                                                        \n"
	"{                                                                                      \n"
	"    if(mouse.w == -1)                                                                  \n"
	"        gs_color = vec4(0.0, 0.0, 0.0, 0.0);                                           \n"
	"    else                                                                               \n"
	"   {                                                                                   \n"
	"    gs_color = vec4(0.5, 0.5, 0.5, 0.0);                                               \n"
	"    vec4 a = gl_in[0].gl_Position / gl_in[0].gl_Position.w;                            \n"
	"    vec4 b = gl_in[1].gl_Position / gl_in[1].gl_Position.w;                            \n"
	"    vec4 c = gl_in[2].gl_Position / gl_in[2].gl_Position.w;                            \n"
	"	 vec2 a2 = { (a.x + 1) * mouse.z / 2.0f, (1 - a.y) * mouse.w / 2.0f };              \n"
	"	 vec2 b2 = { (b.x + 1) * mouse.z / 2.0f, (1 - b.y) * mouse.w / 2.0f };              \n"
	"	 vec2 c2 = { (c.x + 1) * mouse.z / 2.0f, (1 - c.y) * mouse.w / 2.0f };              \n"
	"	 vec2 p2 = vec2(mouse.x, mouse.y);                                                  \n"
	"	 vec2 v0 = c2 - a2;                                                                 \n"
	"	 vec2 v1 = b2 - a2;                                                                 \n"
	"	 vec2 v2 = p2 - a2;                                                                 \n"
	"	 float dot01 = dot(v0, v1);															\n"
	"	 float dot00 = dot(v0, v0);															\n"
	"	 float dot02 = dot(v0, v2);															\n"
	"	 float dot11 = dot(v1, v1);															\n"
	"	 float dot12 = dot(v1, v2);															\n"
	"	 float det = (dot00 * dot11 - dot01 * dot01);										\n"
	"	 double invDenom = 1.0f / det;														\n"
	"	 double u = (dot11 * dot02 - dot01 * dot12) * invDenom;								\n"
	"	 double v = (dot00 * dot12 - dot01 * dot02) * invDenom;								\n"
	"    if ((u >= 0) && (v >= 0) && (u + v < 1))                                           \n"
	"    {                                                                                  \n"
	"		gs_color = vec4(0.0, 1.0, 0.0, 0.0);                                            \n"
	"    }                                                                                  \n"
	"    }                                                                                  \n"
	"    for(int i = 0; i < 3; ++i)                                                         \n"
	"    {                                                                                  \n"
	"        gl_Position = gl_in[i].gl_Position;								            \n"
	"        gl_ViewportIndex = gl_InvocationID;                                            \n"
	"        EmitVertex();                                                                  \n"
	"    }                                                                                  \n"
	"    EndPrimitive();                                                                    \n"
	"}                                                                                      \n"
};

static const char * fs_source[] =
{
	"#version 410 core                                                 \n"
	"                                                                  \n"
	"out vec4 color;                                                   \n"
	"in vec4 gs_color;											       \n"
	"                                                                  \n"
	"void main(void)                                                   \n"
	"{                                                                 \n"
	"    color = gs_color;											   \n"
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
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fs_source, NULL);
	glCompileShader(fs);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);

	GLuint tcs = glCreateShader(GL_TESS_CONTROL_SHADER);
	glShaderSource(tcs, 1, tcs_source, NULL);
	glCompileShader(tcs);

	GLuint tes = glCreateShader(GL_TESS_EVALUATION_SHADER);
	glShaderSource(tes, 1, tes_source, NULL);
	glCompileShader(tes);

	GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(gs, 1, gs_source, NULL);
	glCompileShader(gs);

	glAttachShader(program, vs);
	glAttachShader(program, tcs);
	glAttachShader(program, tes);
	glAttachShader(program, fs);
	glAttachShader(program, gs);

	glLinkProgram(program);
 	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, bunny->vertex);
	glEnableVertexAttribArray(0);
	glUseProgram(program);

	uniforms.render.matrix = glGetUniformLocation(program, "mvp_matrix");
	uniforms.render.mouse = glGetUniformLocation(program, "mouse");
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//float currentTime = glutGet(GLUT_ELAPSED_TIME);
	model_matrix = (rot) ? translate(mat4(), vec3(0.1f, -0.5f, 0.0f))*
							rotate(mat4(), radians((mousePos.dx + mousePos.rx)* 0.5f), vec3(0, 1, 0))*
							scale(mat4(), vec3(5.0)): model_matrix;

	view_matrix = lookAt(vec3(0.0f, 0.5f, 0.8f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	mvp_matrix = proj_matrix * view_matrix * model_matrix;

	glUniformMatrix4fv(uniforms.render.matrix, 1, GL_FALSE, value_ptr(mvp_matrix));

	GLfloat grey[] = { 0, 0, 0, -1 };
	glUniform4fv(uniforms.render.mouse, 1, grey);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.5f);
	glDrawElements(GL_PATCHES, 3 * bunny->faceCount, GL_UNSIGNED_INT, bunny->face);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GLfloat m2[] = { float(mousePos.x), float(mousePos.y), viewportSize.width, viewportSize.height };
	glUniform4fv(uniforms.render.mouse, 1, m2);
	glDrawElements(GL_PATCHES, 3 * bunny->faceCount, GL_UNSIGNED_INT, bunny->face);
	

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	viewportSize.width = width;
	viewportSize.height = height;
	proj_matrix = perspective(deg2rad(75.0f), (float)viewportSize.width / (float)viewportSize.height, 0.1f, 1000.0f);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)
		{
			mousePos.px = x;
			mousePos.py = y;
		}
		break;
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		{
			mousePos.px = x;
			mousePos.py = y;
			rot = true;
		}
		else
		{
			mousePos.rx += mousePos.dx;
			mousePos.ry += mousePos.dy;
			mousePos.dx = 0;
			mousePos.dy = 0;
			rot = false;
		}
		break;
	}
}

void My_Motion(int x, int y)
{
	mousePos.x = x;
	mousePos.y = y;
	if(rot)
	{
		mousePos.dx = x - mousePos.px;
		mousePos.dy = y - mousePos.py; 
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
	bunny = parser("../../bunny.obj");
	My_Init();

	////////////////////
	// Register GLUT callback functions.
	///////////////////////////////
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_Motion);
	glutPassiveMotionFunc(My_Motion);
	glutTimerFunc(16, My_Timer, 0);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}