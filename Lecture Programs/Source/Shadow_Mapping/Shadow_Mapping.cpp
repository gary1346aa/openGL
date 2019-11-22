#include "../../Include/Common.h"

using namespace glm;
using namespace std;

#define SHADOW_MAP_SIZE 4096

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	int materialId;
	int indexCount;
	mat4 model_matrix;

	GLuint position_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;

} Shape;
Shape dragon;
Shape torus;

struct
{
	struct
	{
		mat4 view;
		mat4 proj;
	} eye;
} matrices;

struct
{
	struct
	{
		GLint   mvp;
	} light;
	struct
	{
        GLuint  shadow_tex;
		GLint   mv_matrix;
		GLint   proj_matrix;
		GLint   shadow_matrix;
		GLint   full_shading;
		GLint   light_matrix;
	} view;
} uniforms;

struct
{
	GLuint fbo;
	GLuint depthMap;
} shadowBuffer;

struct
{
	int width;
	int height;
} viewportSize;

GLuint depthProg;
GLuint blinnPhongProg;

const char *depth_vs[] = 
{
	"#version 410 core                         \n"
	"                                          \n"
	"uniform mat4 mvp;                         \n"
	"                                          \n"
	"layout (location = 0) in vec4 position;   \n"
	"                                          \n"
	"void main(void)                           \n"
	"{                                         \n"
	"    gl_Position = mvp * position;         \n"
	"}                                         \n"
};

const char *depth_fs[] =
{
    "#version 410 core                                \n"
	"                                                 \n"
    "out vec4 fragColor;                              \n"
	"                                                 \n"
    "void main()                                      \n"
	"{                                                \n"
	"    fragColor = vec4(vec3(gl_FragCoord.z), 1.0); \n"
	"}                                                \n"
};

const char *blinnphong_fs_glsl[] = 
{
	"#version 410 core                                                                          \n"
	"                                                                                           \n"
	"layout (location = 0) out vec4 color;	                                                    \n"
	"uniform sampler2DShadow shadow_tex;	                                                    \n"
	"	                                                                                        \n"
	"in VS_OUT	                                                                                \n"
	"{	                                                                                        \n"
	"    vec4 shadow_coord;	                                                                    \n"
	"    vec3 N;	                                                                            \n"
	"    vec3 L;	                                                                            \n"
	"    vec3 V;	                                                                            \n"
	"} fs_in;	                                                                                \n"
	"	                                                                                        \n"
	"// Material properties	                                                                    \n"
	"uniform vec3 ambient = vec3(0.225, 0.2, 0.25);   	                                        \n"
	"uniform vec3 diffuse_albedo = vec3(0.9, 0.8, 1.0);	                                        \n"
	"uniform vec3 specular_albedo = vec3(0.7);	                                                \n"
	"uniform float specular_power = 300.0;	                                                    \n"
	"uniform bool full_shading = true;	                                                        \n"
	"	                                                                                        \n"
	"void main(void)	                                                                        \n"
	"{	                                                                                        \n"
	"    // Normalize the incoming N, L and V vectors	                                        \n"
	"    vec3 N = normalize(fs_in.N);	                                                        \n"
	"    vec3 L = normalize(fs_in.L);	                                                        \n"
	"    vec3 V = normalize(fs_in.V);	                                                        \n"
	"	 vec3 H = normalize(L + V);	                                                            \n"
	" 	                                                                                        \n"
	"    // Compute the diffuse and specular components for each fragment                    	\n"
	"    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo;	                                \n"
	"    vec3 specular = pow(max(dot(N, H), 0.0), specular_power) * specular_albedo;	        \n"
	"    float shadow_factor = textureProj(shadow_tex, fs_in.shadow_coord);                     \n"
	"    color = vec4(ambient, 1.0) + shadow_factor * vec4(diffuse + specular, 1.0);        	\n"
	"}	                                                                                        \n"

};

const char *blinnphong_vs_glsl[] = 
{
	"#version 410 core                                          \n"
	"uniform mat4 mv_matrix;                                    \n"
	"uniform mat4 proj_matrix;                                  \n"
	"uniform mat4 shadow_matrix;                                \n"
	"layout (location = 0) in vec4 position;                    \n"
	"layout (location = 1) in vec3 normal;                      \n"
	"out VS_OUT                                                 \n"
	"{                                                          \n"
	"    vec4 shadow_coord;                                     \n"
	"    vec3 N;                                                \n"
	"    vec3 L;                                                \n"
	"    vec3 V;                                                \n"
	"} vs_out;                                                  \n"
	"// Position of light                                       \n"
	"uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);        \n"
	"void main(void)                                            \n"
	"{                                                          \n"
	"    // Calculate view-space coordinate                     \n"
	"    vec4 P = mv_matrix * position;	                        \n"
	"    // Calculate normal in view-space	                    \n"
	"    vs_out.N = mat3(mv_matrix) * normal;	                \n"
	"    // Calculate light vector	                            \n"
	"    vs_out.L = light_pos - P.xyz;	                        \n"
	"    // Calculate view vector	                            \n"
	"    vs_out.V = -P.xyz;	                                    \n"
	"    // Light-space coordinates                          	\n"
	"    vs_out.shadow_coord = shadow_matrix * position;	    \n"
	"    // Calculate the clip-space position of each vertex	\n"
	"    gl_Position = proj_matrix * P;	                        \n"
	"}	                                                        \n"

};

void My_Init()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearColor(0.5, 0.5, 0.5, 1.0);

	// ----- Begin Initialize Depth Shader Program -----
	GLuint shadow_vs;
    GLuint shadow_fs;
	shadow_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shadow_vs, 1, depth_vs, 0);
	glCompileShader(shadow_vs);
	printGLShaderLog(shadow_vs);
    shadow_fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shadow_fs, 1, depth_fs, 0);
    glCompileShader(shadow_fs);
    printGLShaderLog(shadow_fs);
	depthProg = glCreateProgram();
	glAttachShader(depthProg, shadow_vs);
    glAttachShader(depthProg, shadow_fs);
	glLinkProgram(depthProg);
	uniforms.light.mvp = glGetUniformLocation(depthProg, "mvp");
	// ----- End Initialize Depth Shader Program -----

	// ----- Begin Initialize Blinn-Phong Shader Program -----
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
	blinnPhongProg = glCreateProgram();
	glAttachShader(blinnPhongProg, vs);
	glAttachShader(blinnPhongProg, fs);
	glLinkProgram(blinnPhongProg);
	glUseProgram(blinnPhongProg);
	uniforms.view.proj_matrix = glGetUniformLocation(blinnPhongProg, "proj_matrix");
	uniforms.view.mv_matrix = glGetUniformLocation(blinnPhongProg, "mv_matrix");
	uniforms.view.shadow_matrix = glGetUniformLocation(blinnPhongProg, "shadow_matrix");
    uniforms.view.shadow_tex = glGetUniformLocation(blinnPhongProg, "shadow_tex");
	// ----- End Initialize Blinn-Phong Shader Program -----

	// ----- Begin Initialize Dragon Model -----
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string err;
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/dragon.obj");

	glGenVertexArrays(1, &dragon.vao);
	glBindVertexArray(dragon.vao);

	glGenBuffers(1, &dragon.position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, dragon.position_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.positions.size() * sizeof(float), shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &dragon.normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, dragon.normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.normals.size() * sizeof(float), shapes[0].mesh.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &dragon.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dragon.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[0].mesh.indices.size() * sizeof(unsigned int), shapes[0].mesh.indices.data(), GL_STATIC_DRAW);

	dragon.indexCount  = shapes[0].mesh.indices.size();
	// ----- End Initialize Dragon Model -----

	shapes.clear();
	materials.clear();

	// ----- Begin Initialize Torus Model -----
	tinyobj::LoadObj(shapes, materials, err, "../../Media/Objects/torus_nrms_tc.obj");

	glGenVertexArrays(1, &torus.vao);
	glBindVertexArray(torus.vao);

	glGenBuffers(1, &torus.position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, torus.position_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.positions.size() * sizeof(float), shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &torus.normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, torus.normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, shapes[0].mesh.normals.size() * sizeof(float), shapes[0].mesh.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &torus.index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torus.index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[0].mesh.indices.size() * sizeof(unsigned int), shapes[0].mesh.indices.data(), GL_STATIC_DRAW);

	torus.indexCount  = shapes[0].mesh.indices.size();
	// ----- End Initialize Torus Model -----

	// ----- Begin Initialize Shadow Framebuffer Object -----
	glGenFramebuffers(1, &shadowBuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);

	glGenTextures(1, &shadowBuffer.depthMap);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0,  GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowBuffer.depthMap, 0);
	// ----- End Initialize Shadow Framebuffer Object -----
}

void My_Display()
{
	// ----- Begin Initialize Time-Related Model Matrices -----
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	mat4 scale_bias_matrix = 
		translate(mat4(), vec3(0.5f, 0.5f, 0.5f)) *
		scale(mat4(), vec3(0.5f, 0.5f, 0.5f));
	dragon.model_matrix =
		rotate(mat4(), radians(time * 14.5f), vec3(0.0f, 1.0f, 0.0f)) *
		rotate(mat4(), radians(20.0f), vec3(1.0f, 0.0f, 0.0f)) *
		translate(mat4(), vec3(0.0f, -4.0f, 0.0f));
	torus.model_matrix = 
		rotate(mat4(), radians(time * 3.7f), vec3(0.0f, 1.0f, 0.0f)) * 
		translate(mat4(), vec3(sinf(time * 0.37f) * 12.0f, cosf(time * 0.37f) * 12.0f, 0.0f)) *
		scale(mat4(), vec3(2.0f));
	// ----- End Initialize Time-Related Model Matrices -----

	// ----- Begin Shadow Map Pass -----
	const float shadow_range = 15.0f;
	mat4 light_proj_matrix = ortho(-shadow_range, shadow_range, -shadow_range, shadow_range, 0.0f, 50.0f);
	mat4 light_view_matrix = lookAt(vec3(20.0f, 20.0f, 20.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 light_vp_matrix = light_proj_matrix * light_view_matrix;

	mat4 shadow_sbpv_matrix = scale_bias_matrix * light_vp_matrix;

	glUseProgram(depthProg);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 4.0f);
	glUniformMatrix4fv(uniforms.light.mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * dragon.model_matrix));
	glBindVertexArray(dragon.vao);
	glDrawElements(GL_TRIANGLES, dragon.indexCount, GL_UNSIGNED_INT, 0);
	glUniformMatrix4fv(uniforms.light.mvp, 1, GL_FALSE, value_ptr(light_vp_matrix * torus.model_matrix));
	glBindVertexArray(torus.vao);
	glDrawElements(GL_TRIANGLES, torus.indexCount, GL_UNSIGNED_INT, 0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	// ----- End Shadow Map Pass -----

	// ----- Begin Blinn-Phong Shading Pass -----
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, viewportSize.width, viewportSize.height);
	glUseProgram(blinnPhongProg);

	glUniformMatrix4fv(uniforms.view.proj_matrix, 1, GL_FALSE, value_ptr(matrices.eye.proj));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthMap);
    glUniform1i(uniforms.view.shadow_tex, 0);

	mat4 dragon_mv = matrices.eye.view * dragon.model_matrix;
	mat4 shadow_matrix = shadow_sbpv_matrix * dragon.model_matrix;

	glUniformMatrix4fv(uniforms.view.mv_matrix, 1, GL_FALSE, value_ptr(dragon_mv));
	glUniformMatrix4fv(uniforms.view.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));

	glBindVertexArray(dragon.vao);
	glDrawElements(GL_TRIANGLES, dragon.indexCount, GL_UNSIGNED_INT, 0);

	mat4 torus_mv = matrices.eye.view * torus.model_matrix;
	shadow_matrix = shadow_sbpv_matrix * torus.model_matrix;

	glUniformMatrix4fv(uniforms.view.mv_matrix, 1, GL_FALSE, value_ptr(torus_mv));
	glUniformMatrix4fv(uniforms.view.shadow_matrix, 1, GL_FALSE, value_ptr(shadow_matrix));

	glBindVertexArray(torus.vao);
	glDrawElements(GL_TRIANGLES, torus.indexCount, GL_UNSIGNED_INT, 0);
	// ----- End Blinn-Phong Shading Pass -----

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	viewportSize.width = width;
	viewportSize.height = height;
	float viewportAspect = (float)width / (float)height;
	matrices.eye.proj = perspective(deg2rad(50.0f), viewportAspect, 0.1f, 1000.0f);
	matrices.eye.view = lookAt(vec3(0.0f, 0.0f, 40.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
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
