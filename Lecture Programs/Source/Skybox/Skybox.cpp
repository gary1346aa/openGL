#include "../../Include/Common.h"

using namespace glm;
using namespace std;

GLuint skybox_prog;
GLuint tex_envmap;
GLuint skybox_vao;
mat4 proj_matrix;

struct
{
    struct
    {
        GLint inv_vp_matrix;
		GLint eye;
    } skybox;
} uniforms;

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
    "#version 410 core                                              \n"
    "                                                               \n"
    "out VS_OUT                                                     \n"
    "{                                                              \n"
    "    vec3    tc;                                                \n"
    "} vs_out;                                                      \n"
    "                                                               \n"
    "uniform mat4 inv_vp_matrix;                                    \n"
	"uniform vec3 eye;                                              \n"
    "                                                               \n"
    "void main(void)                                                \n"
    "{                                                              \n"
    "    vec4[4] vertices = vec4[4](vec4(-1.0, -1.0, 1.0, 1.0),     \n"
    "                               vec4( 1.0, -1.0, 1.0, 1.0),     \n"
    "                               vec4(-1.0,  1.0, 1.0, 1.0),     \n"
    "                               vec4( 1.0,  1.0, 1.0, 1.0));    \n"
    "                                                               \n"
	"    vec4 p = inv_vp_matrix * vertices[gl_VertexID];            \n"
	"    p /= p.w;                                                  \n"
	"    vs_out.tc = normalize(p.xyz - eye);                        \n"
    "                                                               \n"
    "    gl_Position = vertices[gl_VertexID];                       \n"
    "}                                                              \n"
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

	uniforms.skybox.inv_vp_matrix = glGetUniformLocation(skybox_prog, "inv_vp_matrix");
	uniforms.skybox.eye = glGetUniformLocation(skybox_prog, "eye");

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
    
    glGenVertexArrays(1, &skybox_vao);
}

void My_Display()
{
	static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	static const GLfloat ones[] = { 1.0f };
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.0005f;

	vec3 eye = vec3(0.0f, 0.0f, 0.0f);
	mat4 view_matrix = lookAt(eye, vec3(15.0f * sinf(currentTime), 7.0f * sinf(currentTime), 15.0f * cosf(currentTime)), vec3(0.0f, 1.0f, 0.0f));
	mat4 mv_matrix = view_matrix *
					 rotate(mat4(), deg2rad(currentTime), vec3(1.0f, 0.0f, 0.0f)) *
					 rotate(mat4(), deg2rad(currentTime) * 130.1f, vec3(0.0f, 1.0f, 0.0f)) *
					 translate(mat4(), vec3(0.0f, -4.0f, 0.0f));
	mat4 inv_vp_matrix = inverse(proj_matrix * view_matrix);

	glClearBufferfv(GL_COLOR, 0, gray);
	glClearBufferfv(GL_DEPTH, 0, ones);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);

	glUseProgram(skybox_prog);
	glBindVertexArray(skybox_vao);

	glUniformMatrix4fv(uniforms.skybox.inv_vp_matrix, 1, GL_FALSE, &inv_vp_matrix[0][0]);
	glUniform3fv(uniforms.skybox.eye, 1, &eye[0]);

	glDisable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_DEPTH_TEST);

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
	glutTimerFunc(20, My_Timer, val);
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
