#include "../../Include/Common.h"

using namespace glm;

static const char * vs_source[] =
{
	"#version 410 core                                                      \n"
	"                                                                       \n"
	"out VS_OUT                                                             \n"
	"{                                                                      \n"
	"    vec2 tc;                                                           \n"
	"} vs_out;                                                              \n"
	"                                                                       \n"
	"uniform mat4 mvp;                                                      \n"
	"uniform float offset;                                                  \n"
	"                                                                       \n"
	"void main(void)                                                        \n"
	"{                                                                      \n"
	"    const vec2[4] position = vec2[4](vec2(-0.75, -0.75),               \n"
	"                                     vec2( 0.75, -0.75),               \n"
	"                                     vec2(-0.75,  0.75),               \n"
	"                                     vec2( 0.75,  0.75));              \n"
	"    vs_out.tc = (position[gl_VertexID].xy + vec2(offset, 0.5)) *       \n"
	"                vec2(30.0, 1.0);                                       \n"
	"    gl_Position = mvp * vec4(position[gl_VertexID], 0.0, 1.0);         \n"
	"}                                                                      \n"

};

static const char * fs_source[] =
{
	"#version 410 core                                                      \n"
	"                                                                       \n"
	"layout (location = 0) out vec4 color;                                  \n"
	"                                                                       \n"
	"in VS_OUT                                                              \n"
	"{                                                                      \n"
	"    vec2 tc;                                                           \n"
	"} fs_in;                                                               \n"
	"                                                                       \n"
	"uniform sampler2D tex;                                                 \n"
	"                                                                       \n"
	"void main(void)                                                        \n"
	"{                                                                      \n"
	"    color = texture(tex, fs_in.tc);                                    \n"
	"}                                                                      \n"
};

GLuint          program;
GLuint          vao;
mat4 proj_matrix;

struct
{
	GLint       mvp;
	GLint       offset;
} uniforms;

GLuint          tex_wall;
GLuint          tex_ceiling;
GLuint          tex_floor;

enum FilterTypes
{
	NEAREST = 1,
	LINEAR,
	LINEAR_MIPMAP,
	ANISOTROPIC
};

float maxAniso = 1.0f;
bool anisoSupport = false;

void My_Init()
{
    if (GL_EXT_texture_filter_anisotropic)
        anisoSupport = true;
	// anisoSupport = glewIsSupported("GL_EXT_texture_filter_anisotropic");
	if(anisoSupport)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
	}
	printf("Anisotropic Filtering is %ssupported\n", anisoSupport ? "" : "not ");
	printf("Max Anisotropy: %.1f\n", maxAniso);

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

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	uniforms.mvp = glGetUniformLocation(program, "mvp");
	uniforms.offset = glGetUniformLocation(program, "offset");

	TextureData tex = loadPNG("../../Media/Textures/brick.png");
	glGenTextures(1, &tex_wall);
	glBindTexture(GL_TEXTURE_2D, tex_wall);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] tex.data;

	tex = loadPNG("../../Media/Textures/ceiling.png");
	glGenTextures(1, &tex_ceiling);
	glBindTexture(GL_TEXTURE_2D, tex_ceiling);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] tex.data;

	tex = loadPNG("../../Media/Textures/floor.png");
	glGenTextures(1, &tex_floor);
	glBindTexture(GL_TEXTURE_2D, tex_floor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	delete[] tex.data;

	int i;
	GLuint textures[] = { tex_floor, tex_wall, tex_ceiling };

	for (i = 0; i < 3; i++)
	{
		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

	glUseProgram(program);

	glUniform1f(uniforms.offset, currentTime * 0.003f);
	mat4 Identy_Init(1.0);
	int i;
	GLuint textures[] = { tex_wall, tex_floor, tex_wall, tex_ceiling };
	for (i = 0; i < 4; i++)
	{
		mat4 mv_matrix = rotate(Identy_Init,deg2rad(90.0f * (float)i), vec3(0.0f, 0.0f, 1.0f));
		mv_matrix = translate(mv_matrix,vec3(-0.5f, 0.0f, -10.0f));
		mv_matrix = rotate(mv_matrix,deg2rad(90.0f),vec3( 0.0f, 1.0f, 0.0f));
		mv_matrix = scale(mv_matrix,vec3(50.0f, 1.0f, 1.0f));
		mat4 mvp = proj_matrix * mv_matrix;

		glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, &mvp[0][0]);

		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glDrawArrays(GL_TRIANGLES, 0, 3);

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

void My_Menu(int val)
{
	GLuint textures[] = { tex_floor, tex_wall, tex_ceiling };

	if(anisoSupport)
	{
		// Reset to default 1.0
		for (int i = 0; i < 3; i++)
		{
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f); 
		}
	}

	if(val == NEAREST)
	{
		for (int i = 0; i < 3; i++)
		{
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}
	else if(val == LINEAR)
	{
		for (int i = 0; i < 3; i++)
		{
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
	else if(val == LINEAR_MIPMAP)
	{
		for (int i = 0; i < 3; i++)
		{
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
	else if(val == ANISOTROPIC && anisoSupport)
	{
		for (int i = 0; i < 3; i++)
		{
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso); 
		}
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

	// Create GLUT menu.
	////////////////////
	glutCreateMenu(My_Menu);
	glutAddMenuEntry("Nearest", NEAREST);
	glutAddMenuEntry("Linear", LINEAR);
	glutAddMenuEntry("Linear Mipmap", LINEAR_MIPMAP);
	glutAddMenuEntry("Anisotropic", ANISOTROPIC);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
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
