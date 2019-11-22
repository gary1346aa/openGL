#include "../../Include/Common.h"
#define NUM_DRAWS 50000

using namespace glm;
using namespace std;

struct DrawElementsIndirectCommand
{
    GLuint  count;
    GLuint  primCount;
    GLuint  first;
	GLint   baseVertex;
    GLuint  baseInstance;
};

GLuint              render_program;
GLuint              indirect_draw_buffer;
GLuint              draw_index_buffer;
mat4				proj_matrix;
vector<unsigned int> firsts;
vector<unsigned int> indexCounts;
vector<int> baseVertices;
int shapeCount;

struct
{
    GLint           time;
    GLint           view_matrix;
    GLint           proj_matrix;
    GLint           viewproj_matrix;
} uniforms;

enum MODE
{
    MODE_FIRST,
    MODE_MULTIDRAW = 0,
    MODE_SEPARATE_DRAWS,
    MODE_MAX = MODE_SEPARATE_DRAWS
};

MODE                mode;
bool                paused;

static const char *render_fs_glsl[] = 
{
    "#version 410 core                          \n"
    "                                           \n"
    "out vec4 color;                            \n"
    "                                           \n"
    "in VS_OUT                                  \n"
    "{                                          \n"
    "    vec3 normal;                           \n"
    "    vec4 color;                            \n"
    "} fs_in;                                   \n"
    "                                           \n"
    "void main(void)                            \n"
    "{                                          \n"
    "    vec3 N = normalize(fs_in.normal);      \n"
    "                                           \n"
    "    color = fs_in.color * abs(N.z);        \n"
    "}                                          \n"
};

static const char *render_vs_glsl[] = 
{
    "#version 410 core                                                         \n"
    "                                                                          \n"
    "layout (location = 0) in vec3 position_3;                                 \n"
    "layout (location = 1) in vec3 normal;                                     \n"
    "layout (location = 2) in uint draw_id;                                    \n"
    "                                                                          \n"
    "out VS_OUT                                                                \n"
    "{                                                                         \n"
    "    vec3 normal;                                                          \n"
    "    vec4 color;                                                           \n"
    "} vs_out;                                                                 \n"
    "                                                                          \n"
    "uniform float time = 0.0;                                                 \n"
    "                                                                          \n"
    "uniform mat4 view_matrix;                                                 \n"
    "uniform mat4 proj_matrix;                                                 \n"
    "uniform mat4 viewproj_matrix;                                             \n"
    "                                                                          \n"
    "const vec4 color0 = vec4(0.29, 0.21, 0.18, 1.0);                          \n"
    "const vec4 color1 = vec4(0.58, 0.55, 0.51, 1.0);                          \n"
    "                                                                          \n"
    "void main(void)                                                           \n"
    "{                                                                         \n"
    "    vec4 position = vec4(position_3, 1.0);                                \n"
    "    mat4 m1;                                                              \n"
    "    mat4 m2;                                                              \n"
    "    mat4 m;                                                               \n"
    "    float t = time * 0.1;                                                 \n"
    "    float f = float(draw_id) / 30.0;                                      \n"
    "                                                                          \n"
    "    float st = sin(t * 0.5 + f * 5.0);                                    \n"
    "    float ct = cos(t * 0.5 + f * 5.0);                                    \n"
    "                                                                          \n"
    "    float j = fract(f);                                                   \n"
    "    float d = cos(j * 3.14159);                                           \n"
    "                                                                          \n"
    "    // Rotate around Y                                                    \n"
    "    m[0] = vec4(ct, 0.0, st, 0.0);\n"
    "    m[1] = vec4(0.0, 1.0, 0.0, 0.0);\n"
    "    m[2] = vec4(-st, 0.0, ct, 0.0);\n"
    "    m[3] = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "\n"
    "    // Translate in the XZ plane\n"
    "    m1[0] = vec4(1.0, 0.0, 0.0, 0.0);\n"
    "    m1[1] = vec4(0.0, 1.0, 0.0, 0.0);\n"
    "    m1[2] = vec4(0.0, 0.0, 1.0, 0.0);\n"
    "    m1[3] = vec4(260.0 + 30.0 * d, 5.0 * sin(f * 123.123), 0.0, 1.0);     \n"
    "\n"
    "    m = m * m1;\n"
    "\n"
    "    // Rotate around X\n"
    "    st = sin(t * 2.1 * (600.0 + f) * 0.01);\n"
    "    ct = cos(t * 2.1 * (600.0 + f) * 0.01);\n"
    "\n"
    "    m1[0] = vec4(ct, st, 0.0, 0.0);\n"
    "    m1[1] = vec4(-st, ct, 0.0, 0.0);\n"
    "    m1[2] = vec4(0.0, 0.0, 1.0, 0.0);\n"
    "    m1[3] = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "\n"
    "    m = m * m1;\n"
    "\n"
    "    // Rotate around Z\n"
    "    st = sin(t * 1.7 * (700.0 + f) * 0.01);\n"
    "    ct = cos(t * 1.7 * (700.0 + f) * 0.01);\n"
    "\n"
    "    m1[0] = vec4(1.0, 0.0, 0.0, 0.0);\n"
    "    m1[1] = vec4(0.0, ct, st, 0.0);\n"
    "    m1[2] = vec4(0.0, -st, ct, 0.0);\n"
    "    m1[3] = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "\n"
    "    m = m * m1;\n"
    "\n"
    "    // Non-uniform scale\n"
    "    float f1 = 0.65 + cos(f * 1.1) * 0.2;\n"
    "    float f2 = 0.65 + cos(f * 1.1) * 0.2;\n"
    "    float f3 = 0.65 + cos(f * 1.3) * 0.2;\n"
    "\n"
    "    m1[0] = vec4(f1, 0.0, 0.0, 0.0);\n"
    "    m1[1] = vec4(0.0, f2, 0.0, 0.0);\n"
    "    m1[2] = vec4(0.0, 0.0, f3, 0.0);\n"
    "    m1[3] = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "\n"
    "    m = m * m1;\n"
    "\n"
    "    gl_Position = viewproj_matrix * m * position;\n"
    "    vs_out.normal = mat3(view_matrix * m) * normal;\n"
    "    vs_out.color = mix(color0, color1, fract(j * 313.431));\n"
    "}\n"
    "\n"
};

void My_Init()
{
    int i;

	render_program = glCreateProgram();
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vs, 1, render_vs_glsl, NULL);
    glShaderSource(fs, 1, render_fs_glsl, NULL);

    glCompileShader(vs);
    glCompileShader(fs);

    glAttachShader(render_program, vs);
    glAttachShader(render_program, fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);

    glLinkProgram(render_program);
    glDeleteShader(fs);
    glDeleteShader(vs);

    uniforms.time            = glGetUniformLocation(render_program, "time");
    uniforms.view_matrix     = glGetUniformLocation(render_program, "view_matrix");
    uniforms.proj_matrix     = glGetUniformLocation(render_program, "proj_matrix");
    uniforms.viewproj_matrix = glGetUniformLocation(render_program, "viewproj_matrix");

	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/asteroids.obj");
	shapeCount = shapes.size();

	size_t positionSize = 0;
	size_t normalSize = 0;
	size_t indexSize = 0;
	for(int i = 0; i < shapes.size(); ++i)
	{
		positionSize += shapes[i].mesh.positions.size();
		normalSize += shapes[i].mesh.normals.size();
		indexSize += shapes[i].mesh.indices.size();
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint positionBuffer;
	GLuint normalBuffer;
	GLuint indexBuffer;

	glGenBuffers(1, &positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, positionSize * sizeof(float), 0, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, normalSize * sizeof(float), 0, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize * sizeof(unsigned int), 0, GL_STATIC_DRAW);

	size_t positionOffset = 0;
	size_t normalOffset = 0;
	size_t indexOffset = 0;
	int baseVertex = 0;
	for(int i = 0; i < shapes.size(); ++i)
	{
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glBufferSubData(GL_ARRAY_BUFFER, positionOffset * sizeof(float), shapes[i].mesh.positions.size() * sizeof(float), shapes[i].mesh.positions.data());
		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glBufferSubData(GL_ARRAY_BUFFER, normalOffset * sizeof(float), shapes[i].mesh.normals.size() * sizeof(float), shapes[i].mesh.normals.data());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indexOffset * sizeof(unsigned int), shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data());

		firsts.push_back(indexOffset);
		positionOffset += shapes[i].mesh.positions.size();
		normalOffset += shapes[i].mesh.normals.size();
		indexOffset += shapes[i].mesh.indices.size();
		baseVertices.push_back(baseVertex);
		indexCounts.push_back(shapes[i].mesh.indices.size());
		baseVertex += shapes[i].mesh.positions.size() / 3;
	}

    glGenBuffers(1, &indirect_draw_buffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_draw_buffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER,
                 NUM_DRAWS * sizeof(DrawElementsIndirectCommand),
                 NULL,
                 GL_STATIC_DRAW);

    DrawElementsIndirectCommand * cmd = (DrawElementsIndirectCommand *)
        glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, NUM_DRAWS * sizeof(DrawElementsIndirectCommand), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    for (i = 0; i < NUM_DRAWS; i++)
    {
		cmd[i].first = firsts[i % shapeCount];
		cmd[i].count = shapes[i % shapeCount].mesh.indices.size();
        cmd[i].primCount = 1;
		cmd[i].baseVertex = baseVertices[i % shapeCount];
        cmd[i].baseInstance = i;
    }

    glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER);

    glGenBuffers(1, &draw_index_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, draw_index_buffer);
    glBufferData(GL_ARRAY_BUFFER, NUM_DRAWS * sizeof(GLuint), NULL, GL_STATIC_DRAW);

    GLuint * draw_index =
        (GLuint *)glMapBufferRange(GL_ARRAY_BUFFER, 0, NUM_DRAWS * sizeof(GLuint), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    for (i = 0; i < NUM_DRAWS; i++)
    {
        draw_index[i] = i;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, 0, NULL);
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
}

void My_Display()
{
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    int j;
    static const float one = 1.0f;
    static const float black[] = { 0.0f, 0.0f, 0.0f, 0.0f };

        
    static double last_time = 0.0;
    static double total_time = 0.0;

    if (!paused)
        total_time += (currentTime - last_time);
    last_time = currentTime;

    float t = float(total_time);
    int i = int(total_time * 3.0f);

    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_DEPTH, 0, &one);

    const mat4 view_matrix = lookAt(vec3(100.0f * cosf(t * 0.023f), 100.0f * cosf(t * 0.023f), 300.0f * sinf(t * 0.037f) - 600.0f),
                                                  vec3(0.0f, 0.0f, 260.0f),
                                                  normalize(vec3(0.1f - cosf(t * 0.1f) * 0.3f, 1.0f, 0.0f)));

    glUseProgram(render_program);

    glUniform1f(uniforms.time, t);
    glUniformMatrix4fv(uniforms.view_matrix, 1, GL_FALSE, &view_matrix[0][0]);
    glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, &proj_matrix[0][0]);
    glUniformMatrix4fv(uniforms.viewproj_matrix, 1, GL_FALSE, &(proj_matrix * view_matrix)[0][0]);

    if (mode == MODE_MULTIDRAW)
    {
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, NUM_DRAWS, 0);
    }
    else if (mode == MODE_SEPARATE_DRAWS)
    {
        for (j = 0; j < NUM_DRAWS; j++)
        {
            glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indexCounts[j % shapeCount], GL_UNSIGNED_INT, (const void*)(firsts[j % shapeCount] * sizeof(unsigned int)), 1, baseVertices[j % shapeCount], j);
        }
    }

	glutSwapBuffers();
}

void My_Keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'P':
		case 'p':
            paused = !paused;
            break;
        case 'D':
		case 'd':
            mode = MODE(mode + 1);
            if (mode > MODE_MAX)
                mode = MODE_FIRST;
            break;
    }
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(deg2rad(50.0f), viewportAspect, 1.0f, 2000.0f);
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
