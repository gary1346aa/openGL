#include "../../Include/Common.h"

using namespace glm;
using namespace std;

GLuint          program;
GLuint          tex_object[2];
GLuint          tex_index;
int index_count;

struct
{
    GLint       mv_matrix;
    GLint       proj_matrix;
} uniforms;

static const char *render_fs_glsl[] = 
{
    "#version 410 core                                            \n"
    "                                                             \n"
    "uniform sampler2D tex_object;                                \n"
    "                                                             \n"
    "in VS_OUT                                                    \n"
    "{                                                            \n"
    "    vec2 tc;                                                 \n"
    "} fs_in;                                                     \n"
    "                                                             \n"
    "out vec4 color;                                              \n"
    "                                                             \n"
    "void main(void)                                              \n"
    "{                                                            \n"
    "    color = texture(tex_object, fs_in.tc * vec2(3.0, 1.0));  \n"
    "}                                                            \n"
};

static const char *render_vs_glsl[] = 
{
    "#version 410 core                            \n"
    "                                             \n"
    "uniform mat4 mv_matrix;                      \n"
    "uniform mat4 proj_matrix;                    \n"
    "                                             \n"
    "layout (location = 0) in vec4 position;      \n"
    "layout (location = 1) in vec2 tc;            \n"
    "                                             \n"
    "out VS_OUT                                   \n"
    "{                                            \n"
    "    vec2 tc;                                 \n"
    "} vs_out;                                    \n"
    "                                             \n"
    "void main(void)                              \n"
    "{                                            \n"
    "    vec4 pos_vs = mv_matrix * position;      \n"
    "                                             \n"
    "    vs_out.tc = tc;                          \n"
    "                                             \n"
    "    gl_Position = proj_matrix * pos_vs;      \n"
    "}                                            \n"
};

void My_Init()
{
	program = glCreateProgram();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, render_fs_glsl, NULL);
	glCompileShader(fs);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, render_vs_glsl, NULL);
	glCompileShader(vs);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);

	glLinkProgram(program);
	glUseProgram(program);

    uniforms.mv_matrix = glGetUniformLocation(program, "mv_matrix");
    uniforms.proj_matrix = glGetUniformLocation(program, "proj_matrix");

#define B 0x00, 0x00, 0x00, 0x00
#define W 0xFF, 0xFF, 0xFF, 0xFF
    static const GLubyte tex_data[] =
    {
        B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
        W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
        B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
        W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
        B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
        W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
        B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
        W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
        B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
        W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
        B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
        W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
        B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
        W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
        B, W, B, W, B, W, B, W, B, W, B, W, B, W, B, W,
        W, B, W, B, W, B, W, B, W, B, W, B, W, B, W, B,
    };
#undef B
#undef W

    glGenTextures(1, &tex_object[0]);
    glBindTexture(GL_TEXTURE_2D, tex_object[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    TextureData tex = loadPNG("../../Media/Textures/pattern1.png");
	glGenTextures(1, &tex_object[1]);
	glBindTexture(GL_TEXTURE_2D, tex_object[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/torus_nrms_tc.obj");

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint position_buffer;
	GLuint texcoord_buffer;
	GLuint index_buffer;

	glGenBuffers(1, &position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.positions.size() * sizeof(float), shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &texcoord_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.texcoords.size() * sizeof(float), shapes[0].mesh.texcoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[0].mesh.indices.size() * sizeof(unsigned int), shapes[0].mesh.indices.data(), GL_STATIC_DRAW);
	index_count = shapes[0].mesh.indices.size();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
}

void My_Display()
{
    static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    static const GLfloat ones[] = { 1.0f };

    glClearBufferfv(GL_COLOR, 0, gray);
    glClearBufferfv(GL_DEPTH, 0, ones);

	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

    glBindTexture(GL_TEXTURE_2D, tex_object[tex_index]);

    glUseProgram(program);

    mat4 proj_matrix = perspective(deg2rad(60.0f), 1.0f, 0.1f, 1000.0f);
    mat4 mv_matrix = translate(mat4(), vec3(0.0f, 0.0f, -3.0f)) *
                            rotate(mat4(), deg2rad((float)currentTime * 19.3f), vec3(0.0f, 1.0f, 0.0f)) *
                            rotate(mat4(), deg2rad((float)currentTime * 21.1f), vec3(0.0f, 0.0f, 1.0f));

    glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, &mv_matrix[0][0]);
    glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, &proj_matrix[0][0]);

    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);

	glutSwapBuffers();
}

void My_Keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'T':
		case 't':
            tex_index++;
            if (tex_index > 1)
                tex_index = 0;
			glutPostRedisplay();
            break;
    }
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
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
	glutKeyboardFunc(My_Keyboard);
	glutTimerFunc(16, My_Timer, 0);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}