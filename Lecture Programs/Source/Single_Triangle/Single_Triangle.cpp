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

int draw_faceidx[999] = {};
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
	"layout(triangles, invocations = 4) in;                                                 \n"
	"layout(triangle_strip, max_vertices = 3) out;                                          \n"
	"                                                                                       \n"
	"                                                                                       \n"
	"void main(void)                                                                        \n"
	"{                                                                                      \n"
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
	"uniform vec4 vs_color;											   \n"
	"                                                                  \n"
	"void main(void)                                                   \n"
	"{                                                                 \n"
	"    color = vs_color;											   \n"
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
	//glAttachShader(program, gs);

	glLinkProgram(program);
 	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, bunny->vertex);
	glEnableVertexAttribArray(0);
	glUseProgram(program);

	uniforms.render.matrix = glGetUniformLocation(program, "mvp_matrix");
	uniforms.render.color = glGetUniformLocation(program, "vs_color");
	uniforms.render.position = glGetUniformLocation(program, "Position");
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat grey[]  = { 0.5f, 0.5f, 0.5f, 0.0f };
	static const GLfloat green[] = { 0.0f, 1.0f, 0.0f, 0.0f };

	//float currentTime = glutGet(GLUT_ELAPSED_TIME);
	model_matrix = (rot) ? translate(mat4(), vec3(0.1f, -0.5f, 0.0f))*
							rotate(mat4(), radians((mousePos.dx + mousePos.rx)* 0.5f), vec3(0, 1, 0))*
							scale(mat4(), vec3(6.0)): model_matrix;

	view_matrix = lookAt(vec3(0.0f, 0.5f, 0.8f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	mvp_matrix = proj_matrix * view_matrix * model_matrix;

	glUniformMatrix4fv(uniforms.render.matrix, 1, GL_FALSE, value_ptr(mvp_matrix));

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(1.5f);
	glUniform4fv(uniforms.render.color, 1, black);
	glDrawElements(GL_PATCHES, 3, GL_UNSIGNED_INT, bunny->face);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform4fv(uniforms.render.color, 1, grey);
	glDrawElements(GL_PATCHES, 3, GL_UNSIGNED_INT, bunny->face);


	if (draw_count)
	{
		glUniform4fv(uniforms.render.color, 1, green);
		glDrawElements(GL_PATCHES, 3 * draw_count, GL_UNSIGNED_INT, draw_faceidx);
	}

	glutSwapBuffers();
}


void My_RayCasting() 
{
	vector<pair<double, int>> hit;
	for (int i = 0; i < 1; ++i)
	{

		vec4 a = mvp_matrix * vec4(	(float)bunny->vertex[3 * bunny->face[3*i  ]  ], 
									(float)bunny->vertex[3 * bunny->face[3*i  ]+1],
									(float)bunny->vertex[3 * bunny->face[3*i  ]+2], 1.0f );

		vec4 b = mvp_matrix * vec4(	(float)bunny->vertex[3 * bunny->face[3*i+1]  ],
									(float)bunny->vertex[3 * bunny->face[3*i+1]+1],
									(float)bunny->vertex[3 * bunny->face[3*i+1]+2], 1.0f);

		vec4 c = mvp_matrix * vec4(	(float)bunny->vertex[3 * bunny->face[3*i+2]  ], 
									(float)bunny->vertex[3 * bunny->face[3*i+2]+1],
									(float)bunny->vertex[3 * bunny->face[3*i+2]+2], 1.0f );

		float mx = (mousePos.px * 2.0f / (float)viewportSize.width) - 1;
		float my = 1 - (mousePos.py * 2.0f / (float)viewportSize.height);

		if (a[3] == 0 || b[3] == 0 || c[3] == 0)
			continue;

		a[0] /= a[3]; a[1] /= a[3]; a[2] /= a[3]; a[3] /= a[3];
		b[0] /= b[3]; b[1] /= b[3]; b[2] /= b[3]; b[3] /= b[3];
		c[0] /= c[3]; c[1] /= c[3]; c[3] /= c[3]; c[3] /= c[3];

		printf("m: %f %f\n", mx, my);
		printf("a: %f %f\n", a[0], a[1]);
		printf("b: %f %f\n", b[0], b[1]);
		printf("c: %f %f\n", c[0], c[1]);

		//printf("a : %f %f\n", (a[0] + 1) * viewportSize.width / 2.0f, (1 - a[1]) * viewportSize.height / 2.0f);
		//printf("b : %f %f\n", (b[0] + 1) * viewportSize.width / 2.0f, (1 - b[1]) * viewportSize.height / 2.0f);
		//printf("c : %f %f\n", (c[0] + 1) * viewportSize.width / 2.0f, (1 - c[1]) * viewportSize.height / 2.0f);

		vec4 p = { mx, my, 0, 1 };

		vec4 v0 = c - a;
		vec4 v1 = b - a;
		vec4 v2 = p - a;
		
		float dot00 = dot(v0, v0);
		float dot01 = dot(v0, v1);
		float dot02 = dot(v0, v2);
		float dot11 = dot(v1, v1);
		float dot12 = dot(v1, v2);

		float det = (dot00 * dot11 - dot01 * dot01);
		//if (det < 1e-6 && det > -1e-6)
		//	continue;
		
		vec2 a2 = { (a[0] + 1) * viewportSize.width / 2.0f , (1 - a[1]) * viewportSize.height / 2.0f };
		vec2 b2 = { (b[0] + 1) * viewportSize.width / 2.0f, (1 - b[1]) * viewportSize.height / 2.0f };
		vec2 c2 = { (c[0] + 1) * viewportSize.width / 2.0f, (1 - c[1]) * viewportSize.height / 2.0f };

		vec2 v20 = c2 - a2;
		vec2 v21 = b2 - a2;
		vec2 tmp = { mousePos.x, mousePos.y };
		vec2 v22 = tmp - a2;

		printf("a: %f %f\n", a2[0], a2[1]);
		printf("b: %f %f\n", b2[0], b2[1]);
		printf("c: %f %f\n", c2[0], c2[1]);

		printf("dot: %f %f\n", dot(v21, v22)/(v21[0]*v21[0] + v21[1]*v21[1]), dot(v20, v22));

		double invDenom = 1.0f / det;
		double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

		printf("u : %f, v = %f", u, v);

		if ((u >= 0) && (v >= 0) && (u + v < 1))
		{
			hit.push_back({ a[2] + u * (b[2] - a[2]) + v * (c[2] - a[2]), i });
			printf("hit!\n");
		}
			
	
	}
	if (hit.size())
	{
		sort(hit.begin(), hit.end());
		printf("hit face id: %d\n", hit[0].second);
		draw_faceidx[draw_count * 3]     = 3 * bunny->face[3 * hit[0].second];
		draw_faceidx[draw_count * 3 + 1] = 3 * bunny->face[3 * hit[0].second + 1];
		draw_faceidx[draw_count * 3 + 2] = 3 * bunny->face[3 * hit[0].second + 2];
		draw_count++;
	}

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
			printf("Mouse Pressed: x = %d, y = %d\n", x, y);
			My_RayCasting();
		}
		break;
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		{
			mousePos.px = x;
			mousePos.py = y;
			printf("Mouse Pressed: x = %d, y = %d\n", x, y);
			rot = true;
		}
		else
		{
			mousePos.rx += mousePos.dx;
			mousePos.ry += mousePos.dy;
			mousePos.dx = 0;
			mousePos.dy = 0;
			rot = false;
			printf("Mouse Released\n");
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
	//printf("Mouse Position: x = %d, y = %d\n", x, y);
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