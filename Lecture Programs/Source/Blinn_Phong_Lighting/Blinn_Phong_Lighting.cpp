#include "../../Include/Common.h"

using namespace glm;
using namespace std;

GLuint blinn_program;
int index_count;
mat4 proj_matrix;
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
} uniform;

const char *blinnphong_fs_glsl[] = 
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
    "uniform float specular_power = 200.0;                                          \n"
    "                                                                               \n"
    "void main(void)                                                                \n"
    "{                                                                              \n"
    "    // Normalize the incoming N, L and V vectors                               \n"
    "    vec3 N = normalize(fs_in.N);                                               \n"
    "    vec3 L = normalize(fs_in.L);                                               \n"
    "    vec3 V = normalize(fs_in.V);                                               \n"
    "    vec3 H = normalize(L + V);                                                 \n"
    "                                                                               \n"
    "    // Compute the diffuse and specular components for each fragment           \n"
    "    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;                       \n"
    "    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;\n"
    "                                                                               \n"
    "    // Write final color to the framebuffer                                    \n"
    "    color = vec4(diffuse + specular, 1.0);                                     \n"
    "}                                                                              \n"
    "                                                                               \n"
};

const char *blinnphong_vs_glsl[] = 
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

void My_Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

	GLuint vs;
	GLuint fs;

	vs = glCreateShader(GL_VERTEX_SHADER);
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vs, 1, blinnphong_vs_glsl, 0);
	glShaderSource(fs, 1, blinnphong_fs_glsl, 0);
	glCompileShader(vs);
	glCompileShader(fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);
    blinn_program = glCreateProgram();
    glAttachShader(blinn_program, vs);
    glAttachShader(blinn_program, fs);
    glLinkProgram(blinn_program);
	glUseProgram(blinn_program);

    uniform.diffuse_albedo = glGetUniformLocation(blinn_program, "diffuse_albedo");
    uniform.specular_albedo = glGetUniformLocation(blinn_program, "specular_albedo");
    uniform.specular_power = glGetUniformLocation(blinn_program, "specular_power");

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

    glUseProgram(blinn_program);

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

    glUniform1f(uniform.specular_power, 60.0f);
	vec3 specular = vec3(1.0f);
    glUniform3fv(uniform.specular_albedo, 1, &specular[0]);

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
