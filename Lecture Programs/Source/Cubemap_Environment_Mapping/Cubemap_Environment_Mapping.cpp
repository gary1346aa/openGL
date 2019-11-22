#include "../../Include/Common.h"

using namespace glm;
using namespace std;

GLuint render_prog;
GLuint skybox_prog;
GLuint tex_envmap;
GLuint skybox_vao;
GLuint dragon_vao;
size_t index_count;
mat4 proj_matrix;

struct
{
    struct
    {
        GLint mv_matrix;
        GLint proj_matrix;
    } render;
    struct
    {
        GLint view_matrix;
    } skybox;
} uniforms;

const char *render_fs_glsl[] = 
{
    "#version 410 core                                                 \n"
    "                                                                  \n"
    "uniform samplerCube tex_cubemap;                                  \n"
    "                                                                  \n"
    "in VS_OUT                                                         \n"
    "{                                                                 \n"
    "    vec3 normal;                                                  \n"
    "    vec3 view;                                                    \n"
    "} fs_in;                                                          \n"
    "                                                                  \n"
    "out vec4 color;                                                   \n"
    "                                                                  \n"
    "void main(void)                                                   \n"
    "{                                                                 \n"
    "    // Reflect view vector about the plane defined by the normal  \n"
    "    // at the fragment                                            \n"
    "    vec3 r = reflect(fs_in.view, normalize(fs_in.normal));        \n"
    "                                                                  \n"
    "    // Sample from scaled using reflection vector                 \n"
    "    color = texture(tex_cubemap, r) * vec4(0.95, 0.80, 0.45, 1.0);\n"
    "}                                                                 \n"
    "                                                                  \n"
};

const char *render_vs_glsl[] = 
{
    "#version 410 core                            \n"
    "                                             \n"
    "uniform mat4 mv_matrix;                      \n"
    "uniform mat4 proj_matrix;                    \n"
    "                                             \n"
    "layout (location = 0) in vec4 position;      \n"
    "layout (location = 1) in vec3 normal;        \n"
    "                                             \n"
    "out VS_OUT                                   \n"
    "{                                            \n"
    "    vec3 normal;                             \n"
    "    vec3 view;                               \n"
    "} vs_out;                                    \n"
    "                                             \n"
    "void main(void)                              \n"
    "{                                            \n"
    "    vec4 pos_vs = mv_matrix * position;      \n"
    "                                             \n"
    "    vs_out.normal = mat3(mv_matrix) * normal;\n"
    "    vs_out.view = pos_vs.xyz;                \n"
    "                                             \n"
    "    gl_Position = proj_matrix * pos_vs;      \n"
    "}                                            \n"
    "                                             \n"
};

const char *skybox_fs_glsl[] = 
{
    "#version 410 core                          \n"
    "                                           \n"
    "uniform samplerCube tex_cubemap;           \n"
    "                                           \n"
    "in VS_OUT                                  \n"
    "{                                          \n"
    "    vec3    tc;                            \n"
    "} fs_in;                                   \n"
    "                                           \n"
    "layout (location = 0) out vec4 color;      \n"
    "                                           \n"
    "void main(void)                            \n"
    "{                                          \n"
    "    color = texture(tex_cubemap, fs_in.tc);\n"
    "}                                          \n"
    "                                           \n"
};

const char *skybox_vs_glsl[] = 
{
    "#version 410 core                                         \n"
    "                                                          \n"
    "out VS_OUT                                                \n"
    "{                                                         \n"
    "    vec3    tc;                                           \n"
    "} vs_out;                                                 \n"
    "                                                          \n"
    "uniform mat4 view_matrix;                                 \n"
    "                                                          \n"
    "void main(void)                                           \n"
    "{                                                         \n"
    "    vec3[4] vertices = vec3[4](vec3(-1.0, -1.0, 1.0),     \n"
    "                               vec3( 1.0, -1.0, 1.0),     \n"
    "                               vec3(-1.0,  1.0, 1.0),     \n"
    "                               vec3( 1.0,  1.0, 1.0));    \n"
    "                                                          \n"
    "    vs_out.tc = mat3(view_matrix) * vertices[gl_VertexID];\n"
    "                                                          \n"
    "    gl_Position = vec4(vertices[gl_VertexID], 1.0);       \n"
    "}                                                         \n"
    "                                                          \n"
};

void My_Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

	skybox_prog = glCreateProgram();
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, skybox_fs_glsl, NULL);
	glCompileShader(fs);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, skybox_vs_glsl, NULL);
	glCompileShader(vs);

	glAttachShader(skybox_prog, vs);
	glAttachShader(skybox_prog, fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);

	glLinkProgram(skybox_prog);
	glUseProgram(skybox_prog);

	uniforms.skybox.view_matrix = glGetUniformLocation(skybox_prog, "view_matrix");

	render_prog = glCreateProgram();
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, render_fs_glsl, NULL);
	glCompileShader(fs);

	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, render_vs_glsl, NULL);
	glCompileShader(vs);

	glAttachShader(render_prog, vs);
	glAttachShader(render_prog, fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);

	glLinkProgram(render_prog);
	glUseProgram(render_prog);

    uniforms.render.mv_matrix = glGetUniformLocation(render_prog, "mv_matrix");
    uniforms.render.proj_matrix = glGetUniformLocation(render_prog, "proj_matrix");

    TextureData envmap_data = loadPNG("../../Media/Textures/mountaincube.png");
	glGenTextures(1, &tex_envmap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);
	for(int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, envmap_data.width, envmap_data.height / 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, envmap_data.data + i * (envmap_data.width * (envmap_data.height / 6) * sizeof(unsigned char) * 4));
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	delete[] envmap_data.data;

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/dragon.obj");

	glGenVertexArrays(1, &skybox_vao);
	glGenVertexArrays(1, &dragon_vao);
	glBindVertexArray(dragon_vao);

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
	static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	static const GLfloat ones[] = { 1.0f };
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

	mat4 view_matrix = lookAt(vec3(15.0f * sinf(currentTime), 0.0f, 15.0f * cosf(currentTime)), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 mv_matrix = view_matrix *
					 rotate(mat4(), deg2rad(currentTime), vec3(1.0f, 0.0f, 0.0f)) *
					 rotate(mat4(), deg2rad(currentTime) * 130.1f, vec3(0.0f, 1.0f, 0.0f)) *
					 translate(mat4(), vec3(0.0f, -4.0f, 0.0f));

	glClearBufferfv(GL_COLOR, 0, gray);
	glClearBufferfv(GL_DEPTH, 0, ones);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);

	glUseProgram(skybox_prog);
	glBindVertexArray(skybox_vao);

	glUniformMatrix4fv(uniforms.skybox.view_matrix, 1, GL_FALSE, &view_matrix[0][0]);

	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(render_prog);
	glBindVertexArray(dragon_vao);

	glUniformMatrix4fv(uniforms.render.mv_matrix, 1, GL_FALSE, &mv_matrix[0][0]);
	glUniformMatrix4fv(uniforms.render.proj_matrix, 1, GL_FALSE, &proj_matrix[0][0]);

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
