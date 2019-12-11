#include "../../Include/Common.h"

#include <cstdlib>
#include <ctime>

using namespace glm;
using namespace std;

GLuint render_prog;
GLuint *vao;
size_t index_count;

mat4 proj_matrix;
mat4 view_matrix;
mat4 mvp_matrix;
vector<tinyobj::shape_t> shapes;
vector<tinyobj::material_t> materials;

GLuint *position_buffer;
GLuint *index_buffer;
GLuint *texcoord_buffer;
map<string, GLuint> textures;

struct
{
	struct
	{
		GLint color;
		GLint mvp_matrix;
	} render;
} uniforms;

struct
{
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

const char *render_fs[] =
{
	"#version 410 core                \n"
	"                                 \n"
	"uniform vec4 color;              \n"
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
	"layout(triangles, invocations = 1) in;                                                 \n"
	"layout(line_strip, max_vertices = 2) out;                                              \n"
	"                                                                                       \n"
	"uniform mat4 mvp_matrix;                                                               \n"
	"                                                                                       \n"
	"void main()                                                                            \n"
	"{                                                                                      \n"
	"    vec4 v10 = gl_in[1].gl_Position - gl_in[0].gl_Position;                            \n"
	"    vec4 v20 = gl_in[2].gl_Position - gl_in[0].gl_Position;                            \n"
	"    vec3 normal = normalize(cross(v10.xyz, v20.xyz));                                  \n"
	"    vec4 center = gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position;  \n"
	"    center /= 3.0;                                                                     \n"
	"    gl_Position = mvp_matrix * center;                                                 \n"
	"    EmitVertex();                                                                      \n"
	"    center.xyz += normal * 0.2;                                                        \n"
	"    gl_Position = mvp_matrix * center;                                                 \n"
	"    EmitVertex();                                                                      \n"
	"    EndPrimitive();                                                                    \n"
	"}                                                                                      \n"
};

const char *render_vs[] =
{
	"#version 410 core                                 \n"
	"                                                  \n"
	"uniform mat4 mvp_matrix;                          \n"
	"layout (location = 0) in vec3 position;           \n"
	"                                                  \n"
	"void main(void)                                   \n"
	"{                                                 \n"
	"    gl_Position = mvp_matrix*vec4(position, 1.0); \n"
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
	//glAttachShader(render_prog, gs);
	glAttachShader(render_prog, fs);

	glLinkProgram(render_prog);
	glUseProgram(render_prog);
	uniforms.render.mvp_matrix = glGetUniformLocation(render_prog, "mvp_matrix");
	uniforms.render.color = glGetUniformLocation(render_prog, "color");
	// ----- End Initialize Depth-Normal Only Program -----

	// ----- Begin Initialize Input Mesh -----

	string err;
	string obj_dir = "../../Media/Objects/sponza.obj";
	string base_dir = "../../Media/Objects/";
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/sponza.obj", "../../Media/Objects/");

	vao = (GLuint*)malloc(shapes.size() * sizeof(GLuint));
	position_buffer = (GLuint*)malloc(shapes.size() * sizeof(GLuint));
	index_buffer = (GLuint*)malloc(shapes.size() * sizeof(GLuint));
	texcoord_buffer = (GLuint*)malloc(shapes.size() * sizeof(GLuint));

	for (int i = 0; i < shapes.size(); i++) {
		glGenBuffers(1, &position_buffer[i]);
		glBindBuffer(GL_ARRAY_BUFFER, position_buffer[i]);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float), shapes[i].mesh.positions.data(), GL_STATIC_DRAW);
		glGenBuffers(1, &index_buffer[i]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);
		glGenBuffers(1, &texcoord_buffer[i]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, texcoord_buffer[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.texcoords.size() * sizeof(unsigned int), shapes[i].mesh.texcoords.data(), GL_STATIC_DRAW);
	}
	
	{
		for (size_t m = 0; m < materials.size(); m++) {
			tinyobj::material_t* mp = &materials[m];

			if (mp->diffuse_texname.length() > 0) {
				// Only load the texture if it is not already loaded
				if (textures.find(mp->diffuse_texname) == textures.end()) {
					GLuint texture_id;
					int w, h;
					int comp;

					std::string texture_filename = base_dir + mp->diffuse_texname;

					unsigned char* image =
						stbi_load(texture_filename.c_str(), &w, &h, &comp, STBI_default);
					if (!image) {
						std::cerr << "Unable to load texture: " << texture_filename
							<< std::endl;
						exit(1);
					}
					std::cout << m << " Loaded texture: " << texture_filename << ", w = " << w
						<< ", h = " << h << ", comp = " << comp << std::endl;

					glGenTextures(1, &texture_id);
					glBindTexture(GL_TEXTURE_2D, texture_id);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					if (comp == 3) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
							GL_UNSIGNED_BYTE, image);
					}
					else if (comp == 4) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
							GL_UNSIGNED_BYTE, image);
					}
					else {
						assert(0);  // TODO
					}
					glBindTexture(GL_TEXTURE_2D, 0);
					stbi_image_free(image);
					textures.insert(std::make_pair(mp->diffuse_texname, texture_id));
				}
			}
		}
	}

	for (int i = 0; i < shapes.size(); i++)
	{
		glGenVertexArrays(1, &vao[i]);
		glBindVertexArray(vao[i]);

		glBindBuffer(GL_ARRAY_BUFFER, position_buffer[i]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer[i]);
	}

	// ----- End Initialize Input Mesh -----
}
void My_Display()
{
	static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat grey[] =  { 0.5f, 0.5f, 0.5f, 1.0f };
	static const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat ones[] = { 1.0f };
	float currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.0005f;

	// ----- Begin Render Pass -----
	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_DEPTH, 0, ones);
	glUseProgram(render_prog);
	glActiveTexture(GL_TEXTURE0);

	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST || GL_CULL_FACE);


	view_matrix = lookAt(spherical2cartesian(spherical.rho, spherical.phi, spherical.theta), 
						 vec3(0.0f, 0.0f, 0.0f), 
						 vec3(0.0f, 1.0f, 0.0f));
	
	mvp_matrix = proj_matrix * view_matrix;
	glUniformMatrix4fv(uniforms.render.mvp_matrix, 1, GL_FALSE, &mvp_matrix[0][0]);
	
	glLineWidth(1.5f);

	for (int i = 0; i < shapes.size(); i++)
	{
		index_count = shapes[i].mesh.indices.size();

		glUniform4fv(uniforms.render.color, 1, black);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(vao[i]);
		for (int j = 0; j < shapes[i].mesh.material_ids.size(); j++)
		{
			glBindTexture(GL_TEXTURE_2D, textures[materials[shapes[i].mesh.material_ids[j]].diffuse_texname]);
		}
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		/*
		glUniform4fv(uniforms.render.color, 1, grey);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(vao[i]);
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
		*/
	}
	// ----- End Render Pass -----

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
	glutMouseFunc(My_Mouse);
	glutMouseWheelFunc(My_Wheel);
	glutMotionFunc(My_Motion);
	glutPassiveMotionFunc(My_Motion);
	glutTimerFunc(16, My_Timer, 0);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	free(index_buffer);
	free(position_buffer);
	//////////////
	return 0;
}