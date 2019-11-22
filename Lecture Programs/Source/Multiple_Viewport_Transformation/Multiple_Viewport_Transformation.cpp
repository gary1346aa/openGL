#include "../../Include/Common.h"

#include <cstdlib>
#include <ctime>

using namespace glm;
using namespace std;

GLuint render_prog;
GLuint bunny_vao;
size_t index_count;
mat4 proj_matrix;

struct
{
	int width;
	int height;
} viewportSize;

struct
{
    struct
    {
        GLint model_matrix;
		GLint view_matrix[4];
		GLint proj_matrix;
		GLint color;
    } render;
} uniforms;

const char *render_fs[] = 
{
    "#version 410 core                \n"
	"                                 \n"
	"uniform vec4 color;              \n"
    "                                 \n"
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
	"layout(triangles, invocations = 4) in;                                                 \n"
	"layout(triangle_strip, max_vertices = 3) out;                                          \n"
	"                                                                                       \n"
	"uniform mat4 model_matrix;                                                             \n"
	"uniform mat4 view_matrix[4];                                                           \n"
	"uniform mat4 proj_matrix;                                                              \n"
	"                                                                                       \n"
	"void main(void)                                                                        \n"
	"{                                                                                      \n"
	"    mat4 mvp_matrix = proj_matrix * view_matrix[gl_InvocationID] * model_matrix;       \n"
	"    for(int i = 0; i < 3; ++i)                                                         \n"
	"    {                                                                                  \n"
	"        gl_Position = mvp_matrix * gl_in[i].gl_Position;                               \n"
    "        gl_ViewportIndex = gl_InvocationID;                                            \n"
	"        EmitVertex();                                                                  \n"
	"    }                                                                                  \n"
	"    EndPrimitive();                                                                    \n"
	"}                                                                                      \n"
};

const char *render_vs[] = 
{
    "#version 410 core                                 \n"
    "                                                  \n"
    "layout (location = 0) in vec3 position;           \n"
    "                                                  \n"
    "void main(void)                                   \n"
    "{                                                 \n"
	"    gl_Position = vec4(position, 1.0);            \n"
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
	glAttachShader(render_prog, gs);
	glAttachShader(render_prog, fs);

	glLinkProgram(render_prog);
	glUseProgram(render_prog);
    uniforms.render.model_matrix = glGetUniformLocation(render_prog, "model_matrix");
	uniforms.render.view_matrix[0] = glGetUniformLocation(render_prog, "view_matrix[0]");
	uniforms.render.view_matrix[1] = glGetUniformLocation(render_prog, "view_matrix[1]");
	uniforms.render.view_matrix[2] = glGetUniformLocation(render_prog, "view_matrix[2]");
	uniforms.render.view_matrix[3] = glGetUniformLocation(render_prog, "view_matrix[3]");
	uniforms.render.proj_matrix = glGetUniformLocation(render_prog, "proj_matrix");
	uniforms.render.color = glGetUniformLocation(render_prog, "color");
	// ----- End Initialize Depth-Normal Only Program -----

	// ----- Begin Initialize Input Mesh -----
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/bunny.obj");

	glGenVertexArrays(1, &bunny_vao);
	glBindVertexArray(bunny_vao);

	GLuint position_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;

	glGenBuffers(1, &position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.positions.size() * sizeof(float), shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.normals.size() * sizeof(float), shapes[0].mesh.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[0].mesh.indices.size() * sizeof(unsigned int), shapes[0].mesh.indices.data(), GL_STATIC_DRAW);
	index_count = shapes[0].mesh.indices.size();
	// ----- End Initialize Input Mesh -----
}

void My_Display()
{
	static const GLfloat black[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	static const GLfloat ones[] = { 1.0f };
	float currentTime = glutGet(GLUT_ELAPSED_TIME);

	// ----- Begin Render Pass -----
	glClearBufferfv(GL_COLOR, 0, black);
	glClearBufferfv(GL_DEPTH, 0, ones);
	glUseProgram(render_prog);

	mat4 model_matrix = rotate(mat4(), radians(currentTime * 0.05f), vec3(0, 1, 0)) * translate(mat4(), vec3(0.0f, -4.0f, 0.0f)) * scale(mat4(), vec3(5.0));
	mat4 view_matrix[4];
	view_matrix[0] = lookAt(vec3(25.0f, 10.0f, 25.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	view_matrix[1] = lookAt(vec3(25.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	view_matrix[2] = lookAt(vec3(0.0f, 0.0f, 40.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	view_matrix[3] = lookAt(vec3(0.0f, 40.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
	
	glUniformMatrix4fv(uniforms.render.model_matrix, 1, GL_FALSE, value_ptr(model_matrix));
	for(int i = 0; i < 4; ++i)
	{
		glUniformMatrix4fv(uniforms.render.view_matrix[i], 1, GL_FALSE, value_ptr(view_matrix[i]));
	}
	glUniformMatrix4fv(uniforms.render.proj_matrix, 1, GL_FALSE, value_ptr(proj_matrix));

	glViewportIndexedf(0, 0, 0, viewportSize.width / 2.0f, viewportSize.height / 2.0f);
	glViewportIndexedf(1, viewportSize.width / 2.0f, 0, viewportSize.width / 2.0f, viewportSize.height / 2.0f);
	glViewportIndexedf(2, viewportSize.width / 2.0f, viewportSize.height / 2.0f, viewportSize.width / 2.0f, viewportSize.height / 2.0f);
	glViewportIndexedf(3, 0, viewportSize.height / 2.0f, viewportSize.width / 2.0f, viewportSize.height / 2.0f);

	glBindVertexArray(bunny_vao);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glUniform4f(uniforms.render.color, 0.3f, 0.3f, 0.3f, 1.0f);
	glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUniform4f(uniforms.render.color, 0.0f, 0.0f, 0.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
	// ----- End Render Pass -----

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	//glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	viewportSize.width = width;
	viewportSize.height = height;
	proj_matrix = perspective(deg2rad(60.0f), viewportAspect, 0.1f, 1000.0f);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
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
	glutTimerFunc(16, My_Timer, 0);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}