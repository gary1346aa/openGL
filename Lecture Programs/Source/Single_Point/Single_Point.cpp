#include "../../Include/Common.h"

#include <cstdlib>
#include <ctime>

using namespace glm;
using namespace std;

GLuint render_prog;
mat4 proj_matrix;
mat4 view_matrix;
mat4 mvp_matrix;

static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const GLfloat grey[] = { 0.5f, 0.5f, 0.5f, 1.0f };
static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat ones[] = { 1.0f };

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	int materialId;
	int indexCount;
} Shape;

typedef struct
{
	GLuint texId;
} Material;

vector<Shape> sceneShapes;
vector<Material> sceneMaterials;

struct
{
	struct
	{
		GLint mvp_matrix;
		GLint s_texture;
	} render;
} uniforms;

struct
{
	GLfloat height = 500.0f;
	GLfloat fb = 0.0f;
	GLfloat lr = 0.0f;
	GLfloat rho = 5000;
	GLfloat phi = deg2rad(90);
	GLfloat theta = 0;
} spherical;

struct
{
	struct
	{
		GLint x;
		GLint y;
	} position;
	struct
	{
		GLint x;
		GLint y;
	} press;
	struct
	{
		GLint x = 0;
		GLint y = 0;
	} release;
	struct
	{
		GLint x;
		GLint y;
	} diff;
	bool pressed = false;
} mouse;

vec3 spherical2cartesian(GLfloat rho, GLfloat phi, GLfloat theta)
{
	return  vec3(rho*sin(phi)*cos(theta), rho*cos(phi), rho*sin(phi)*sin(theta));
}

void My_Init()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	GLuint fs, vs;

	render_prog = glCreateProgram();
	vs = glCreateShader(GL_VERTEX_SHADER);
	fs = glCreateShader(GL_FRAGMENT_SHADER);

	string vs_script = 
		#include "shader.v"
		;
	string fs_script =
		#include "shader.f"
		;

	const char* vs_scriptc = vs_script.c_str();
	const char* fs_scriptc = fs_script.c_str();
	glShaderSource(vs, 1, &vs_scriptc, NULL);
	glShaderSource(fs, 1, &fs_scriptc, NULL);
	glCompileShader(vs);
	glCompileShader(fs);
	printGLShaderLog(vs);
	printGLShaderLog(fs);
	glAttachShader(render_prog, vs);
	glAttachShader(render_prog, fs);
	glLinkProgram(render_prog);
	glUseProgram(render_prog);

	uniforms.render.mvp_matrix = glGetUniformLocation(render_prog, "mvp_matrix");
	uniforms.render.s_texture = glGetUniformLocation(render_prog, "s_texture");
}

void loadScene()
{
	string err;
	string obj_dir = "../../Media/Objects/sponza.obj";
	string base_dir = "../../Media/Objects/";

	vector<tinyobj::material_t> materials;
	vector<tinyobj::shape_t> shapes;

	tinyobj::LoadObj(shapes, materials, err, obj_dir.c_str(), base_dir.c_str());

	for (int i = 0; i < shapes.size(); i++)
	{
		for (int j = 1; j < shapes[i].mesh.texcoords.size(); j += 2)
			shapes[i].mesh.texcoords[j] = 1 - shapes[i].mesh.texcoords[j];

		Shape shape;
		shape.materialId = shapes[i].mesh.material_ids[0];
		shape.indexCount = shapes[i].mesh.indices.size();

		glGenBuffers(3, &shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float), shapes[i].mesh.positions.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vboTex);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.texcoords.size() * sizeof(float), shapes[i].mesh.texcoords.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);

		glGenVertexArrays(1, &shape.vao);

		glBindVertexArray(shape.vao);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ebo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, shape.vboTex);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindVertexArray(0);

		sceneShapes.push_back(shape);
	}

	for (int i = 0; i < materials.size(); i++)
	{
		string texname = materials[i].diffuse_texname;
		Material material;

		int width, height, comp;
		string texpath = base_dir + texname;
		replace(texpath.begin(), texpath.end(), '\\', '/');

		unsigned char* data = stbi_load(texpath.c_str(), &width, &height, &comp, STBI_default);

		GLuint texId;
		glGenTextures(1, &texId);
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(data);
		material.texId = texId;
		sceneMaterials.push_back(material);
	}
}


void My_Display()
{

	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, ones);
	glUseProgram(render_prog);

	view_matrix = lookAt(spherical2cartesian(spherical.rho, spherical.phi, spherical.theta) + vec3(spherical.fb, spherical.height, spherical.lr), 
						 vec3(spherical.fb, spherical.height, spherical.lr),
						 vec3(0.0f, 1.0f, 0.0f));
	
	mvp_matrix = proj_matrix * view_matrix;
	glUniformMatrix4fv(uniforms.render.mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
	
	glLineWidth(1.5f);

	for (int i = 0; i < sceneShapes.size(); i++)
	{
		glBindVertexArray(sceneShapes[i].vao);
		glActiveTexture(GL_TEXTURE0);

		glBindTexture(GL_TEXTURE_2D, sceneMaterials[sceneShapes[i].materialId].texId);
		glUniform1i(uniforms.render.s_texture, 0);

		glDrawElements(GL_TRIANGLES, sceneShapes[i].indexCount, GL_UNSIGNED_INT, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix = perspective(deg2rad(60.0f), viewportAspect, 0.1f, 100000.0f);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		{
			mouse.press.x = x;
			mouse.press.y = y;
			mouse.pressed = true;
		}
		else
		{
			mouse.release.x += mouse.diff.x;
			mouse.release.y += mouse.diff.y;
			mouse.diff.x = 0;
			mouse.diff.y = 0;
			mouse.pressed = false;
		}
		break;
	}
}

void My_Wheel(int button, int dir, int x, int y)
{
	if (dir > 0)
	{
		spherical.rho = (spherical.rho - 150 > 100)? spherical.rho - 150 : 100;
	}
	else
	{
		spherical.rho += 150;
	}
}

void My_Motion(int x, int y)
{
	mouse.position.x = x;
	mouse.position.y = y;
	if (mouse.pressed)
	{
		mouse.diff.x = x - mouse.press.x;
		mouse.diff.y = y - mouse.press.y;
		if ((mouse.release.y + mouse.diff.y) <= -90)
			spherical.phi = deg2rad(180);
		else if ((mouse.release.y + mouse.diff.y) >= 90)
			spherical.phi = 0.1;
		else
			spherical.phi = deg2rad(90 - mouse.release.y - mouse.diff.y);
		spherical.theta = deg2rad(mouse.release.x + mouse.diff.x);
	}
}

void My_keyboard(unsigned char key, int x, int y)
{
	if (key == 'r')
		spherical.height += 100;
	if (key == 'f')
		spherical.height -= 100;
	if (key == 'a')
		spherical.lr += 100;
	if (key == 'd')
		spherical.lr -= 100;
	if (key == 'w')
		spherical.fb -= 100;
	if (key == 's')
		spherical.fb += 100;

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
	loadScene();


	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMouseWheelFunc(My_Wheel);
	glutMotionFunc(My_Motion);
	glutPassiveMotionFunc(My_Motion);
	glutKeyboardFunc(My_keyboard);
	glutTimerFunc(16, My_Timer, 0);

	glutMainLoop();

	return 0;
}