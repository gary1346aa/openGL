#include "../../Include/Common.h"

#include <cstdlib>
#include <ctime>

using namespace glm;
using namespace std;

GLuint render_prog;
GLuint *vao;
size_t index_count;

mat4 proj_matrix;
mat4 model_matrix;
mat4 view_matrix;
mat4 mvp_matrix;
vector<tinyobj::shape_t> shapes;
vector<tinyobj::material_t> materials;
GLfloat Scale = 0.001f;

GLuint position_buffer;
GLuint normal_buffer;
GLuint index_buffer;

struct
{
	struct
	{
		GLint color;
		GLint mvp_matrix;
	} render;
} uniforms;

const char *render_fs[] =
{
	"#version 410 core                \n"
	"                                 \n"
	"uniform vec4 color;              \n"
	"out vec4 frag_color;             \n"
	"                                 \n"
	"void main(void)                  \n"
	"{                                \n"
	"    frag_color = color;          \n"
	"}                                \n"
};

const char *render_gs[] =
{
	"#version 410 core                                                                      \n"
	"                                                                                       \n"
	"layout(triangles, invocations = 1) in;                                                 \n"
	"layout(line_strip, max_vertices = 2) out;                                              \n"
	"                                                                                       \n"
	"uniform mat4 mvp_matrix;                                                               \n"
	"                                                                                       \n"
	"void main()                                                                            \n"
	"{                                                                                      \n"
	"    vec4 v10 = gl_in[1].gl_Position - gl_in[0].gl_Position;                            \n"
	"    vec4 v20 = gl_in[2].gl_Position - gl_in[0].gl_Position;                            \n"
	"    vec3 normal = normalize(cross(v10.xyz, v20.xyz));                                  \n"
	"    vec4 center = gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position;  \n"
	"    center /= 3.0;                                                                     \n"
	"    gl_Position = mvp_matrix * center;                                                 \n"
	"    EmitVertex();                                                                      \n"
	"    center.xyz += normal * 0.2;                                                        \n"
	"    gl_Position = mvp_matrix * center;                                                 \n"
	"    EmitVertex();                                                                      \n"
	"    EndPrimitive();                                                                    \n"
	"}                                                                                      \n"
};

const char *render_vs[] =
{
	"#version 410 core                                 \n"
	"                                                  \n"
	"uniform mat4 mvp_matrix;                          \n"
	"layout (location = 0) in vec3 position;           \n"
	"                                                  \n"
	"void main(void)                                   \n"
	"{                                                 \n"
	"    gl_Position = mvp_matrix*vec4(position, 1.0); \n"
	"}                                                 \n"
};

void My_Init()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// ----- Begin Initialize Depth-Normal Only Program -----
	GLuint fs;
	GLuint vs;
	GLuint gs;
	render_prog = glCreateProgram();
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, render_fs, NULL);
	glCompileShader(fs);
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, render_vs, NULL);
	glCompileShader(vs);
	gs = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(gs, 1, render_gs, NULL);
	glCompileShader(gs);
	printGLShaderLog(vs);
	printGLShaderLog(gs);
	printGLShaderLog(fs);
	glAttachShader(render_prog, vs);
	//glAttachShader(render_prog, gs);
	glAttachShader(render_prog, fs);

	glLinkProgram(render_prog);
	glUseProgram(render_prog);
	uniforms.render.mvp_matrix = glGetUniformLocation(render_prog, "mvp_matrix");
	uniforms.render.color = glGetUniformLocation(render_prog, "color");
	// ----- End Initialize Depth-Normal Only Program -----

	// ----- Begin Initialize Input Mesh -----

	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/sponza.obj");

	vao = (GLuint*)malloc(shapes.size() * sizeof(GLuint));

	for (int i = 0; i < shapes.size(); i++)
	{
		glGenVertexArrays(1, &vao[i]);
		glBindVertexArray(vao[i]);
		glGenBuffers(1, &position_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float), shapes[i].mesh.positions.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
	}


	/*
	glGenBuffers(1, &normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.normals.size() * sizeof(float), shapes[0].mesh.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	*/



	// ----- End Initialize Input Mesh -----
}

void My_Display()
{
	static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat grey[] =  { 0.5f, 0.5f, 0.5f, 1.0f };
	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat ones[] = { 1.0f };
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.0005f;

	// ----- Begin Render Pass -----
	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, ones);
	glUseProgram(render_prog);
	glEnable(GL_DEPTH_TEST);

	model_matrix = translate(mat4(), vec3(-1.0f, 0.0f, 0.0f))* scale(mat4(), vec3(Scale));

	view_matrix = lookAt(vec3(1.0f, 0.9f, 0.8f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	mvp_matrix = proj_matrix * view_matrix * model_matrix;
	//vec4 testpos = mvp_matrix * vec4(shapes[0].mesh.positions[0], shapes[0].mesh.positions[1], shapes[0].mesh.positions[2],1);
	//printf("%f %f %f \n", testpos[0], testpos[1], testpos[2]);

	glUniformMatrix4fv(uniforms.render.mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
	
	
	glLineWidth(2.0f);
	for (int i = 0; i < shapes.size(); i++)
	{

		glGenBuffers(1, &index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);
		index_count = shapes[i].mesh.indices.size();

		glUniform4fv(uniforms.render.color, 1, black);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBindVertexArray(vao[i]);
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
		/*
		glUniform4fv(uniforms.render.color, 1, grey);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBindVertexArray(vao[i]);
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
		*/
	}

	// ----- End Render Pass -----

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(deg2rad(60.0f), viewportAspect, 0.1f, 1000.0f);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	return;
}

void My_Wheel(int button, int dir, int x, int y)
{
	if (dir > 0)
	{
		Scale += 0.0001;
	}
	else
	{
		Scale -= 0.0001;
	}
}

void My_Motion(int x, int y)
{
	return;
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
	glutMouseFunc(My_Mouse);
	glutMouseWheelFunc(My_Wheel);
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