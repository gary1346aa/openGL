#include "../../Include/Common.h"

using namespace glm;
using namespace std;

static const char * vs_source[] =
{
	"#version 410 core                                                                     \n"
	"                                                                                      \n"
	"out VS_OUT{ vec2 tc;} vs_out;                                                         \n"
	"                                                                                      \n"
	"void main(void)                                                                       \n"
	"{                                                                                     \n"
	"    const vec4 vertices[] = vec4[](vec4(-0.5, 0.0, -0.5, 1.0),                        \n"
	"                                   vec4( 0.5, 0.0, -0.5, 1.0),                        \n"
	"                                   vec4(-0.5, 0.0,  0.5, 1.0),                        \n"
	"                                   vec4( 0.5, 0.0,  0.5, 1.0));                       \n"
	"                                                                                      \n"
	"    int x = gl_InstanceID & 63;                                                       \n"
	"    int y = gl_InstanceID >> 6;                                                       \n"
	"    vec2 offs = vec2(x, y);                                                           \n"
	"                                                                                      \n"
	"    vs_out.tc = (vertices[gl_VertexID].xz + offs + vec2(0.5)) / 64.0;                 \n"
	"    gl_Position = vertices[gl_VertexID] + vec4(float(x - 32), 0.0,float(y - 32), 0.0);\n"
	"}                                                                                     \n"

};

static const char * tcs_source[] =
{
	"#version 410 core                                                             \n"
	"                                                                              \n"
	"layout (vertices = 4) out;                                                    \n"
	"                                                                              \n"
	"in VS_OUT{ vec2 tc;} tcs_in[];                                                \n"
	"out TCS_OUT{ vec2 tc;} tcs_out[];                                             \n"
	"                                                                              \n"
	"uniform mat4 mvp_matrix;                                                      \n"
	"uniform float ndc_clip;                                                       \n"
	"                                                                              \n"
	"void main(void)                                                               \n"
	"{                                                                             \n"
	"    if (gl_InvocationID == 0)                                                 \n"
	"    {                                                                         \n"
	"        vec4 p0 = mvp_matrix * gl_in[0].gl_Position;                          \n"
	"        vec4 p1 = mvp_matrix * gl_in[1].gl_Position;                          \n"
	"        vec4 p2 = mvp_matrix * gl_in[2].gl_Position;                          \n"
	"        vec4 p3 = mvp_matrix * gl_in[3].gl_Position;                          \n"
	"        p0 /= p0.w;                                                           \n"
	"        p1 /= p1.w;                                                           \n"
	"        p2 /= p2.w;                                                           \n"
	"        p3 /= p3.w;                                                           \n"
	"        if (p0.z <= -1.0 &&                                                   \n"
	"            p1.z <= -1.0 &&                                                   \n"
	"            p2.z <= -1.0 &&                                                   \n"
	"            p3.z <= -1.0)                                                     \n"
	"         {                                                                    \n"
	"              gl_TessLevelOuter[0] = 0.0;                                     \n"
	"              gl_TessLevelOuter[1] = 0.0;                                     \n"
	"              gl_TessLevelOuter[2] = 0.0;                                     \n"
	"              gl_TessLevelOuter[3] = 0.0;                                     \n"
	"         }                                                                    \n"
	"         else                                                                 \n"
	"         {                                                                    \n"
	"            float l0 = length(p2.xy - p0.xy) * 16.0 + 1.0;                    \n"
	"            float l1 = length(p3.xy - p2.xy) * 16.0 + 1.0;                    \n"
	"            float l2 = length(p3.xy - p1.xy) * 16.0 + 1.0;                    \n"
	"            float l3 = length(p1.xy - p0.xy) * 16.0 + 1.0;                    \n"
	"            gl_TessLevelOuter[0] = l0;                                        \n"
	"            gl_TessLevelOuter[1] = l1;                                        \n"
	"            gl_TessLevelOuter[2] = l2;                                        \n"
	"            gl_TessLevelOuter[3] = l3;                                        \n"
	"            gl_TessLevelInner[0] = min(l1, l3);                               \n"
	"            gl_TessLevelInner[1] = min(l0, l2);                               \n"
	"        }                                                                     \n"
	"    }                                                                         \n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position; \n"
	"    tcs_out[gl_InvocationID].tc = tcs_in[gl_InvocationID].tc;                 \n"
	"}                                                                             \n"

};

static const char * tes_source[] =
{
	"#version 410 core                                                              \n"
	"                                                                               \n"
	"layout(quads, fractional_odd_spacing) in;                                      \n"
	"                                                                               \n"
	"uniform sampler2D tex_displacement;                                            \n"
	"                                                                               \n"
	"uniform mat4 mv_matrix;                                                        \n"
	"uniform mat4 proj_matrix;                                                      \n"
	"uniform float dmap_depth;                                                      \n"
	"                                                                               \n"
	"in TCS_OUT { vec2 tc;} tes_in[];                                               \n"
	"                                                                               \n"
	"out TES_OUT                                                                    \n"
	"{                                                                              \n"
	"    vec2 tc;                                                                   \n"
	"    vec3 world_coord;                                                          \n"
	"    vec3 eye_coord;                                                            \n"
	"} tes_out;                                                                     \n"
	"                                                                               \n"
	"void main(void)                                                                \n"
	"{                                                                              \n"
	"    vec2 tc1 = mix(tes_in[0].tc, tes_in[1].tc, gl_TessCoord.x);                \n"
	"    vec2 tc2 = mix(tes_in[2].tc, tes_in[3].tc, gl_TessCoord.x);                \n"
	"    vec2 tc = mix(tc2, tc1, gl_TessCoord.y);                                   \n"
	"                                                                               \n"
	"    vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x); \n"
	"    vec4 p2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x); \n"
	"    vec4 p = mix(p2, p1, gl_TessCoord.y);                                      \n"
	"    p.y += texture(tex_displacement, tc).r * dmap_depth;                       \n"
	"                                                                               \n"
	"    vec4 P_eye = mv_matrix * p;                                                \n"
	"                                                                               \n"
	"    tes_out.tc = tc;                                                           \n"
	"    tes_out.world_coord = p.xyz;                                               \n"
	"    tes_out.eye_coord = P_eye.xyz;                                             \n"
	"                                                                               \n"
	"    gl_Position = proj_matrix * P_eye;                                         \n"
	"}                                                                              \n"
};

static const char * fs_source[] =
{
	"#version 410 core                                                          \n"
	"                                                                           \n"
	"out vec4 color;                                                            \n"
	"                                                                           \n"
	"uniform sampler2D tex_color;                                               \n"
	"                                                                           \n"
	"uniform bool enable_fog = true;                                            \n"
	"uniform vec4 fog_color = vec4(0.7, 0.8, 0.9, 0.0);                         \n"
	"                                                                           \n"
	"in TES_OUT                                                                 \n"
	"{                                                                          \n"
	"    vec2 tc;                                                               \n"
	"    vec3 world_coord;                                                      \n"
	"    vec3 eye_coord;                                                        \n"
	"} fs_in;                                                                   \n"
	"                                                                           \n"
	"vec4 fog(vec4 c)                                                           \n"
	"{                                                                          \n"
	"    float z = length(fs_in.eye_coord);                                     \n"
	"                                                                           \n"
	"    float de = 0.025 * smoothstep(0.0, 6.0, 10.0 - fs_in.world_coord.y);   \n"
	"    float di = 0.045 * (smoothstep(0.0, 40.0, 20.0 - fs_in.world_coord.y));\n"
	"                                                                           \n"
	"    float extinction   = exp(-z * de);                                     \n"
	"    float inscattering = exp(-z * di);                                     \n"
	"                                                                           \n"
	"    return c * extinction + fog_color * (1.0 - inscattering);              \n"
	"}                                                                          \n"
	"                                                                           \n"
	"void main(void)                                                            \n"
	"{                                                                          \n"
	"    vec4 landscape = texture(tex_color, fs_in.tc);                         \n"
	"                                                                           \n"
	"    if (enable_fog)                                                        \n"
	"    {                                                                      \n"
	"        color = fog(landscape);                                            \n"
	"    }                                                                      \n"
	"    else                                                                   \n"
	"    {                                                                      \n"
	"        color = landscape;                                                 \n"
	"    }                                                                      \n"
	"}                                                                          \n"

};

GLuint          program;
GLuint          vao;
GLuint          tex_displacement;
GLuint          tex_color;
float           dmap_depth;
bool            enable_displacement;
bool            wireframe;
bool            enable_fog;

mat4 proj_matrix;
struct
{
	GLint mvp_matrix;
	GLint mv_matrix;
	GLint proj_matrix;
	GLint dmap_depth;
	GLint enable_fog;
	GLint tex_color;
} uniforms;

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	program = glCreateProgram();
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, vs_source, NULL);
	glCompileShader(vs);
	printGLShaderLog(vs);
	GLuint tcs = glCreateShader(GL_TESS_CONTROL_SHADER);
	glShaderSource(tcs, 1, tcs_source, NULL);
	glCompileShader(tcs);
	printGLShaderLog(tcs);
	GLuint tes = glCreateShader(GL_TESS_EVALUATION_SHADER);
	glShaderSource(tes, 1, tes_source, NULL);
	glCompileShader(tes);
	printGLShaderLog(tes);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, fs_source, NULL);
	glCompileShader(fs);
	printGLShaderLog(fs);

	glAttachShader(program, vs);
	glAttachShader(program, tcs);
	glAttachShader(program, tes);
	glAttachShader(program, fs);

	glLinkProgram(program);
	glUseProgram(program);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	uniforms.mv_matrix = glGetUniformLocation(program, "mv_matrix");
	uniforms.mvp_matrix = glGetUniformLocation(program, "mvp_matrix");
	uniforms.proj_matrix = glGetUniformLocation(program, "proj_matrix");
	uniforms.dmap_depth = glGetUniformLocation(program, "dmap_depth");
	uniforms.enable_fog = glGetUniformLocation(program, "enable_fog");
	uniforms.tex_color = glGetUniformLocation(program, "tex_color");
	dmap_depth = 6.0f;

	TextureData tdata =  loadPNG("../../Media/Textures/terragen.png");

	glEnable(GL_TEXTURE_2D);
	glGenTextures( 1, &tex_displacement );
	glBindTexture( GL_TEXTURE_2D, tex_displacement);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glActiveTexture(GL_TEXTURE1);
	TextureData tdata2 =  loadPNG("../../Media/Textures/terragen_color.png");
	
	glGenTextures( 1, &tex_color );
	glBindTexture( GL_TEXTURE_2D, tex_color);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, tdata2.width, tdata2.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata2.data );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	
	glPatchParameteri(GL_PATCH_VERTICES, 4);

	glEnable(GL_CULL_FACE);

	enable_displacement =true;
	wireframe = false;
	enable_fog = true;
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	static const GLfloat black[] = { 0.85f, 0.95f, 1.0f, 1.0f };
	static const GLfloat one = 1.0f;

	float f_timer_cnt = glutGet(GLUT_ELAPSED_TIME);
	float currentTime = f_timer_cnt* 0.001f;
	float f = (float)currentTime;

	static double last_time = 0.0;
	static double total_time = 0.0;
	
	total_time += (currentTime - last_time);
	last_time = currentTime;

	float t = (float)total_time * 0.03f;
	float r = sinf(t * 5.37f) * 15.0f + 16.0f;
	float h = cosf(t * 4.79f) * 2.0f + 3.2f;
	glBindVertexArray(vao);
	glClearBufferfv(GL_COLOR, 0, black);
	glClearBufferfv(GL_DEPTH, 0, &one);

	glUseProgram(program);

    mat4 mv_matrix = lookAt(vec3(sinf(t) * r, h, cosf(t) * r), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, value_ptr(mv_matrix));
	glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, value_ptr(proj_matrix));
	glUniformMatrix4fv(uniforms.mvp_matrix, 1, GL_FALSE, value_ptr(proj_matrix * mv_matrix));
	glUniform1f(uniforms.dmap_depth, enable_displacement ? dmap_depth : 0.0f);
	glUniform1i(uniforms.enable_fog, enable_fog ? 1 : 0);
	glUniform1i(uniforms.tex_color, 1);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDrawArraysInstanced(GL_PATCHES, 0, 4, 64 * 64);
	
	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	float viewportAspect = (float)width / (float)height;
	proj_matrix= perspective(deg2rad(60.0f), viewportAspect, 0.1f, 1000.0f);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void My_Keyboard( unsigned char key, int x, int y )
{
	switch (key)
	{
	case 'Q': dmap_depth += 0.1f;
		break;
	case 'A': dmap_depth -= 0.1f;
		break;
	case 'F': enable_fog = !enable_fog;
		break;
	case 'D': enable_displacement = !enable_displacement;
		break;
	case 'W': wireframe = !wireframe;
		break;
	default:
		break;
	};
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
	glutInitWindowSize(800, 600);
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
	glutKeyboardFunc(My_Keyboard);
	///////////////////////////////

	// Enter main event loop.
	//////////////
	glutMainLoop();
	//////////////
	return 0;
}