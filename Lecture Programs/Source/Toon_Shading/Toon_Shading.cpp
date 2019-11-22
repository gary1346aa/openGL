#include "../../Include/Common.h"

using namespace glm;
using namespace std;

GLuint program;
GLuint tex_toon;
int index_count;
mat4 proj_matrix;

struct
{
    GLint       mv_matrix;
    GLint       proj_matrix;
} uniforms;

const char *toonshading_fs_glsl[] = 
{
    "#version 410 core                                    \n"
    "                                                     \n"
    "uniform sampler1D tex_toon;                          \n"
    "                                                     \n"
    "uniform vec3 light_pos = vec3(30.0, 30.0, 100.0);    \n"
    "                                                     \n"
    "in VS_OUT                                            \n"
    "{                                                    \n"
    "    vec3 normal;                                     \n"
    "    vec3 view;                                       \n"
    "} fs_in;                                             \n"
    "                                                     \n"
    "out vec4 color;                                      \n"
    "                                                     \n"
    "void main(void)                                      \n"
    "{                                                    \n"
    "    // Calculate per-pixel normal and light vector   \n"
    "    vec3 N = normalize(fs_in.normal);                \n"
    "    vec3 L = normalize(light_pos - fs_in.view);      \n"
    "                                                     \n"
    "    // Simple N dot L diffuse lighting               \n"
    "    float tc = pow(max(0.0, dot(N, L)), 5.0);        \n"
    "                                                     \n"
    "    // Sample from cell shading texture              \n"
    "    color = texture(tex_toon, tc) * tc;              \n"
    "}                                                    \n"
    "                                                     \n"
};

const char *toonshading_vs_glsl[] = 
{
    "#version 410 core                                    \n"
    "                                                     \n"
    "uniform mat4 mv_matrix;                              \n"
    "uniform mat4 proj_matrix;                            \n"
    "                                                     \n"
    "layout (location = 0) in vec4 position;              \n"
    "layout (location = 1) in vec3 normal;                \n"
    "                                                     \n"
    "out VS_OUT                                           \n"
    "{                                                    \n"
    "    vec3 normal;                                     \n"
    "    vec3 view;                                       \n"
    "} vs_out;                                            \n"
    "                                                     \n"
    "void main(void)                                      \n"
    "{                                                    \n"
    "    vec4 pos_vs = mv_matrix * position;              \n"
    "                                                     \n"
    "    // Calculate eye-space normal and position       \n"
    "    vs_out.normal = mat3(mv_matrix) * normal;        \n"
    "    vs_out.view = pos_vs.xyz;                        \n"
    "                                                     \n"
    "    // Send clip-space position to primitive assembly\n"
    "    gl_Position = proj_matrix * pos_vs;              \n"
    "}                                                    \n"
    "                                                     \n"
};



void My_Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, toonshading_fs_glsl, NULL);
	glCompileShader(fs);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, toonshading_vs_glsl, NULL);
	glCompileShader(vs);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);

	glLinkProgram(program);
	glUseProgram(program);

    uniforms.mv_matrix = glGetUniformLocation(program, "mv_matrix");
    uniforms.proj_matrix = glGetUniformLocation(program, "proj_matrix");

	static const GLubyte toon_tex_data[] =
    {
        0x44, 0x00, 0x00, 0x00,
        0x88, 0x00, 0x00, 0x00,
        0xCC, 0x00, 0x00, 0x00,
        0xFF, 0x00, 0x00, 0x00
    };

    glGenTextures(1, &tex_toon);
    glBindTexture(GL_TEXTURE_1D, tex_toon);
    glTexImage1D(GL_TEXTURE_1D, 0,
                    GL_RGBA, sizeof(toon_tex_data) / 4, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE,
                    toon_tex_data);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/torus_nrms_tc.obj");

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
}

void My_Display()
{
	static const GLfloat gray[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    static const GLfloat ones[] = { 1.0f };

    glClearBufferfv(GL_COLOR, 0, gray);
    glClearBufferfv(GL_DEPTH, 0, ones);

	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

    glUseProgram(program);

    mat4 mv_matrix = translate(mat4(), vec3(0.0f, 0.0f, -3.0f)) *
                        rotate(mat4(), deg2rad(currentTime * 13.75f), vec3(0.0f, 1.0f, 0.0f)) *
                        rotate(mat4(), deg2rad(currentTime * 7.75f), vec3(0.0f, 0.0f, 1.0f)) *
                        rotate(mat4(), deg2rad(currentTime * 15.3f), vec3(1.0f, 0.0f, 0.0f));
	glBindTexture(GL_TEXTURE_1D, tex_toon);

    glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, &mv_matrix[0][0]);
    glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, &proj_matrix[0][0]);

    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);

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