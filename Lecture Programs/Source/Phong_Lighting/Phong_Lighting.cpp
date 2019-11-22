#include "../../Include/Common.h"

using namespace glm;
using namespace std;

#define MENU_PER_VERTEX 1
#define MENU_PER_FRAGMENT 2
#define MENU_EXIT 3

GLuint per_fragment_program;
GLuint per_vertex_program;
int index_count;
mat4 proj_matrix;
bool per_vertex = true;
GLuint uniforms_buffer;

struct uniforms_block
{
    mat4 mv_matrix;
    mat4 view_matrix;
    mat4 proj_matrix;
};

struct
{
    GLint diffuse_albedo;
    GLint specular_albedo;
    GLint specular_power;
} uniforms[2];

const char *per_fragment_phong_fs_glsl[] = 
{
    "#version 410 core                                                              \n"
    "                                                                               \n"
    "// Output                                                                      \n"
    "layout (location = 0) out vec4 color;                                          \n"
    "                                                                               \n"
    "// Input from vertex shader                                                    \n"
    "in VS_OUT                                                                      \n"
    "{                                                                              \n"
    "    vec3 N;                                                                    \n"
    "    vec3 L;                                                                    \n"
    "    vec3 V;                                                                    \n"
    "} fs_in;                                                                       \n"
    "                                                                               \n"
    "// Material properties                                                         \n"
    "uniform vec3 diffuse_albedo = vec3(0.5, 0.2, 0.7);                             \n"
    "uniform vec3 specular_albedo = vec3(0.7);                                      \n"
    "uniform float specular_power = 128.0;                                          \n"
    "uniform vec3 ambient = vec3(0.1, 0.1, 0.1);                                    \n"
    "                                                                               \n"
    "void main(void)                                                                \n"
    "{                                                                              \n"
    "    // Normalize the incoming N, L and V vectors                               \n"
    "    vec3 N = normalize(fs_in.N);                                               \n"
    "    vec3 L = normalize(fs_in.L);                                               \n"
    "    vec3 V = normalize(fs_in.V);                                               \n"
    "                                                                               \n"
    "    // Calculate R locally                                                     \n"
    "    vec3 R = reflect(-L, N);                                                   \n"
    "                                                                               \n"
    "    // Compute the diffuse and specular components for each fragment           \n"
    "    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;                       \n"
    "    vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;\n"
    "                                                                               \n"
    "    // Write final color to the framebuffer                                    \n"
    "    color = vec4(ambient + diffuse + specular, 1.0);                           \n"
    "}                                                                              \n"
    "                                                                               \n"
};

const char *per_fragment_phong_vs_glsl[] = 
{
    "#version 410 core                                      \n"
    "                                                       \n"
    "// Per-vertex inputs                                   \n"
    "layout (location = 0) in vec3 position;                \n"
    "layout (location = 1) in vec3 normal;                  \n"
    "                                                       \n"
    "// Matrices we'll need                                 \n"
    "layout (std140) uniform constants                      \n"
    "{                                                      \n"
    "    mat4 mv_matrix;                                    \n"
    "    mat4 view_matrix;                                  \n"
    "    mat4 proj_matrix;                                  \n"
    "};                                                     \n"
    "                                                       \n"
    "// Inputs from vertex shader                           \n"
    "out VS_OUT                                             \n"
    "{                                                      \n"
    "    vec3 N;                                            \n"
    "    vec3 L;                                            \n"
    "    vec3 V;                                            \n"
    "} vs_out;                                              \n"
    "                                                       \n"
    "// Position of light                                   \n"
    "uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);    \n"
    "                                                       \n"
    "void main(void)                                        \n"
    "{                                                      \n"
    "    // Calculate view-space coordinate                 \n"
	"    vec4 P = mv_matrix * vec4(position, 1.0);          \n"
    "                                                       \n"
    "    // Calculate normal in view-space                  \n"
    "    vs_out.N = mat3(mv_matrix) * normal;               \n"
    "                                                       \n"
    "    // Calculate light vector                          \n"
    "    vs_out.L = light_pos - P.xyz;                      \n"
    "                                                       \n"
    "    // Calculate view vector                           \n"
    "    vs_out.V = -P.xyz;                                 \n"
    "                                                       \n"
    "    // Calculate the clip-space position of each vertex\n"
    "    gl_Position = proj_matrix * P;                     \n"
    "}                                                      \n"
    "                                                       \n"
};

const char *per_vertex_phong_fs_glsl[] = 
{
    "#version 410 core                             \n"
    "                                              \n"
    "// Output                                     \n"
    "layout (location = 0) out vec4 color;         \n"
    "                                              \n"
    "// Input from vertex shader                   \n"
    "in VS_OUT                                     \n"
    "{                                             \n"
    "    vec3 color;                               \n"
    "} fs_in;                                      \n"
    "                                              \n"
    "void main(void)                               \n"
    "{                                             \n"
    "    // Write incoming color to the framebuffer\n"
    "    color = vec4(fs_in.color, 1.0);           \n"
    "}                                             \n"
    "                                              \n"
};

const char *per_vertex_phong_vs_glsl[] = 
{
    "#version 410 core                                                              \n"
    "                                                                               \n"
    "// Per-vertex inputs                                                           \n"
    "layout (location = 0) in vec3 position;                                        \n"
    "layout (location = 1) in vec3 normal;                                          \n"
    "                                                                               \n"
    "// Matrices we'll need                                                         \n"
    "layout (std140) uniform constants                                              \n"
    "{                                                                              \n"
    "    mat4 mv_matrix;                                                            \n"
    "    mat4 view_matrix;                                                          \n"
    "    mat4 proj_matrix;                                                          \n"
    "};                                                                             \n"
    "                                                                               \n"
    "// Light and material properties                                               \n"
    "uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);                            \n"
    "uniform vec3 diffuse_albedo = vec3(0.5, 0.2, 0.7);                             \n"
    "uniform vec3 specular_albedo = vec3(0.7);                                      \n"
    "uniform float specular_power = 128.0;                                          \n"
    "uniform vec3 ambient = vec3(0.1, 0.1, 0.1);                                    \n"
    "                                                                               \n"
    "// Outputs to the fragment shader                                              \n"
    "out VS_OUT                                                                     \n"
    "{                                                                              \n"
    "    vec3 color;                                                                \n"
    "} vs_out;                                                                      \n"
    "                                                                               \n"
    "void main(void)                                                                \n"
    "{                                                                              \n"
    "    // Calculate view-space coordinate                                         \n"
	"    vec4 P = mv_matrix * vec4(position, 1.0);                                  \n"
    "                                                                               \n"
    "    // Calculate normal in view space                                          \n"
    "    vec3 N = mat3(mv_matrix) * normal;                                         \n"
    "    // Calculate view-space light vector                                       \n"
    "    vec3 L = light_pos - P.xyz;                                                \n"
    "    // Calculate view vector (simply the negative of the view-space position)  \n"
    "    vec3 V = -P.xyz;                                                           \n"
    "                                                                               \n"
    "    // Normalize all three vectors                                             \n"
    "    N = normalize(N);                                                          \n"
    "    L = normalize(L);                                                          \n"
    "    V = normalize(V);                                                          \n"
    "    // Calculate R by reflecting -L around the plane defined by N              \n"
    "    vec3 R = reflect(-L, N);                                                   \n"
    "                                                                               \n"
    "    // Calculate the diffuse and specular contributions                        \n"
    "    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;                       \n"
    "    vec3 specular = pow(max(dot(R, V), 0.0), specular_power) * specular_albedo;\n"
    "                                                                               \n"
    "    // Send the color output to the fragment shader                            \n"
    "    vs_out.color = ambient + diffuse + specular;                               \n"
    "                                                                               \n"
    "    // Calculate the clip-space position of each vertex                        \n"
    "    gl_Position = proj_matrix * P;                                             \n"
    "}                                                                              \n"
    "                                                                               \n"
};


void My_Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

	GLuint vs;
	GLuint fs;

	vs = glCreateShader(GL_VERTEX_SHADER);
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vs, 1, per_fragment_phong_vs_glsl, 0);
	glShaderSource(fs, 1, per_fragment_phong_fs_glsl, 0);
	glCompileShader(vs);
	glCompileShader(fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);
    per_fragment_program = glCreateProgram();
    glAttachShader(per_fragment_program, vs);
    glAttachShader(per_fragment_program, fs);
    glLinkProgram(per_fragment_program);
	glUseProgram(per_fragment_program);

    uniforms[0].diffuse_albedo = glGetUniformLocation(per_fragment_program, "diffuse_albedo");
    uniforms[0].specular_albedo = glGetUniformLocation(per_fragment_program, "specular_albedo");
    uniforms[0].specular_power = glGetUniformLocation(per_fragment_program, "specular_power");

	vs = glCreateShader(GL_VERTEX_SHADER);
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vs, 1, per_vertex_phong_vs_glsl, 0);
	glShaderSource(fs, 1, per_vertex_phong_fs_glsl, 0);
	glCompileShader(vs);
	glCompileShader(fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);
    per_vertex_program = glCreateProgram();
    glAttachShader(per_vertex_program, vs);
    glAttachShader(per_vertex_program, fs);
    glLinkProgram(per_vertex_program);
	glUseProgram(per_vertex_program);

    uniforms[1].diffuse_albedo = glGetUniformLocation(per_vertex_program, "diffuse_albedo");
    uniforms[1].specular_albedo = glGetUniformLocation(per_vertex_program, "specular_albedo");
    uniforms[1].specular_power = glGetUniformLocation(per_vertex_program, "specular_power");

	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/sphere.obj");

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

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

	glGenBuffers(1, &uniforms_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, uniforms_buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(uniforms_block), 0, GL_DYNAMIC_DRAW);
}

void My_Display()
{
	static const GLfloat zeros[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 0.0f };
    static const GLfloat ones[] = { 1.0f };

    glUseProgram(per_vertex ? per_vertex_program : per_fragment_program);

    glClearBufferfv(GL_COLOR, 0, gray);
    glClearBufferfv(GL_DEPTH, 0, ones);

    vec3 view_position = vec3(0.0f, 0.0f, 50.0f);
    mat4 view_matrix = lookAt(view_position, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniforms_buffer);
    uniforms_block * block = (uniforms_block *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(uniforms_block), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    mat4 model_matrix = scale(mat4(), vec3(12.0f));
    block->mv_matrix = view_matrix * model_matrix;
    block->view_matrix = view_matrix;
    block->proj_matrix = proj_matrix;
    glUnmapBuffer(GL_UNIFORM_BUFFER);

    glUniform1f(uniforms[per_vertex ? 1 : 0].specular_power, 30.0f);
	vec3 specular = vec3(1.0f);
    glUniform3fv(uniforms[per_vertex ? 1 : 0].specular_albedo, 1, &specular[0]);

    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(deg2rad(50.0f), viewportAspect, 0.1f, 1000.0f);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_PER_VERTEX:
		per_vertex = true;
		break;
	case MENU_PER_FRAGMENT:
		per_vertex = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
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

	// Create a menu and bind it to mouse right button.
	////////////////////////////
	int menu_main = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddMenuEntry("(Gouraud Shading) Per Vertex", MENU_PER_VERTEX);
	glutAddMenuEntry("(Phong Shading) Per Fragment", MENU_PER_FRAGMENT);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	////////////////////////////

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
