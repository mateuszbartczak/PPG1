// Triangle.cpp
// Our first OpenGL program that will just draw a triangle on the screen.

#include <GLTools.h> // OpenGL toolkit
#include <GLFrame.h>
#include <GLFrustum.h>
#include <StopWatch.h>
#include <GLMatrixStack.h>
#include <GLGeometryTransform.h>
#include <math3d.h>

#ifdef __APPLE__
#include <glut/glut.h>          // OS X version of GLUT
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>            // Windows FreeGlut equivalent
#endif

GLuint shader;

float ico_vertices[3 * 12] = {
      0., 0., -0.9510565162951536,
      0., 0., 0.9510565162951536,
      -0.85065080835204, 0., -0.42532540417601994,
      0.85065080835204, 0., 0.42532540417601994,
      0.6881909602355868, -0.5, -0.42532540417601994,
      0.6881909602355868, 0.5, -0.42532540417601994,
      -0.6881909602355868, -0.5, 0.42532540417601994,
      -0.6881909602355868, 0.5, 0.42532540417601994,
      -0.2628655560595668, -0.8090169943749475, -0.42532540417601994,
      -0.2628655560595668, 0.8090169943749475, -0.42532540417601994,
      0.2628655560595668, -0.8090169943749475, 0.42532540417601994,
      0.2628655560595668, 0.8090169943749475, 0.42532540417601994
};

int ico_faces[3 * 20] = {
      1 ,			 11 ,			 7 ,
      1 ,			 7 ,			 6 ,
      1 ,			 6 ,			 10 ,
      1 ,			 10 ,			 3 ,
      1 ,			 3 ,			 11 ,
      4 ,			 8 ,			 0 ,
      5 ,			 4 ,			 0 ,
      9 ,			 5 ,			 0 ,
      2 ,			 9 ,			 0 ,
      8 ,			 2 ,			 0 ,
      11 ,			 9 ,			 7 ,
      7 ,			 2 ,			 6 ,
      6 ,			 8 ,			 10 ,
      10 ,			 4 ,			 3 ,
      3 ,			 5 ,			 11 ,
      4 ,			 10 ,			 8 ,
      5 ,			 3 ,			 4 ,
      9 ,			 11 ,			 5 ,
      2 ,			 7 ,			 9 ,
      8 ,			 6 ,			 2 
};

struct point_light {
   float position[3];
   float intensity_diffuse[3];
   float intensity_specular[3];
   float attenuation[3];

   void set_position(float x, float y, float z) {
  	position[0] = x;
  	position[1] = y;
  	position[2] = z;
   }

   void set_intensity_diffuse(float r, float g, float b) {
  	intensity_diffuse[0] = r;
  	intensity_diffuse[1] = g;
  	intensity_diffuse[2] = b;
   }

   void set_intensity_specular(float r, float g, float b) {
  	intensity_specular[0] = r;
  	intensity_specular[1] = g;
  	intensity_specular[2] = b;
   }

     void set_attenuation(float attenuation_0, float attenuation_1, float attenuation_2) {
  	attenuation[0] = attenuation_0;
  	attenuation[1] = attenuation_1;
  	attenuation[2] = attenuation_2;
   }
};

struct material {
   float r_ambient; 
   float r_diffuse; 
   float r_spectacular;  
   float alpha;

   void set_parameters(float r_ambient, float r_diffuse, float r_spectacular, float alpha) {
  	this->r_ambient = r_ambient;
  	this->r_diffuse = r_diffuse;
  	this->r_spectacular = r_spectacular;
  	this->alpha = alpha;
   }
};

GLuint shader_light,
   	ShaderColor,
   	MVPMatrixLocation_ShaderColor,
   	MVPMatrixLocation,
   	mvMatrix_location,
   	vMatrix_location,
   	NormalMatrix_location,
   	intensity_ambient_component_location,
   	light_0_position_location,
   	light_0_intensity_diffuse_location,
   	light_0_intensity_specular_location,
   	light_0_attenuation_location,
   	material_0_r_ambient_location,
   	material_0_r_diffuse_location,
   	material_0_r_spectacular_location,
   	material_0_alpha_location;
GLGeometryTransform geometry_pipeline;
GLMatrixStack p_stack;
GLMatrixStack mv_stack;
GLFrame cameraFrame;
GLFrustum mFrustrum;

float location[] = {1.5f, -1.0f, -1.5f};
float target[] = {0.0f, 0.0f, 0.0f};
float up_dir[] = {0.0f, 0.0f, 1.0f};
float camera_matrix[16];
float intensity_ambient_component[] = {0.2f, 0.2f, 0.2f};

CStopWatch timer;
point_light light_0;
material material_0;

///////////////////////////////////////////////////////////////////////////////
// Window has changed size, or has just been created. In either case, we need
// to use the window dimensions to set the viewport and the projection matrix.

void SetUpFrame(GLFrame &frame,const M3DVector3f origin, const M3DVector3f forward, const M3DVector3f up) {
	frame.SetOrigin(origin);
	frame.SetForwardVector(forward);
	M3DVector3f side,oUp;
	m3dCrossProduct3(side,forward,up);
	m3dCrossProduct3(oUp,side,forward);
	frame.SetUpVector(oUp);
	frame.Normalize();
};

void LookAt(GLFrame &frame, const M3DVector3f eye, const M3DVector3f at, const M3DVector3f up) {
    M3DVector3f forward;
    m3dSubtractVectors3(forward, at, eye);
    SetUpFrame(frame, eye, forward, up);
}

void ChangeSize(int w, int h) {
    glViewport(0, 0, w, h);
	mFrustrum.SetPerspective(60.0f,w/h,1.0f,100.0f);
}

///////////////////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering context.
// This is the first opportunity to do any OpenGL related tasks.

void SetupRC() {
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   shader_light = gltLoadShaderPairWithAttributes("pass_thru_shader.vp", "pass_thru_shader.fp", 3, GLT_ATTRIBUTE_VERTEX, "position", GLT_ATTRIBUTE_COLOR, "vColor", GLT_ATTRIBUTE_NORMAL, "vVertex");
   ShaderColor = gltLoadShaderPairWithAttributes("pass_thru_shader.vp", "pass_thru_shader.fp", 2, GLT_ATTRIBUTE_VERTEX, "position", GLT_ATTRIBUTE_COLOR, "vColor");
   MVPMatrixLocation_ShaderColor = glGetUniformLocation(ShaderColor, "MVPMatrix");
   MVPMatrixLocation = glGetUniformLocation(shader_light, "MVPMatrix");
   mvMatrix_location = glGetUniformLocation(shader_light, "mvMatrix");
   vMatrix_location = glGetUniformLocation(shader_light, "MVMatrix");
   NormalMatrix_location = glGetUniformLocation(shader_light, "normalMatrix");
   intensity_ambient_component_location = glGetUniformLocation(shader_light, "intensity_ambient_component");
   light_0_position_location = glGetUniformLocation(shader_light, "light_0.position");
   light_0_intensity_diffuse_location = glGetUniformLocation(shader_light, "light_0.intensity_diffuse");
   light_0_intensity_specular_location = glGetUniformLocation(shader_light, "light_0.intensity_specular");
   light_0_attenuation_location = glGetUniformLocation(shader_light, "light_0.attenuation");
   material_0_r_ambient_location = glGetUniformLocation(shader_light, "material_0.r_ambient");
   material_0_r_diffuse_location = glGetUniformLocation(shader_light, "material_0.r_diffuse");
   material_0_r_spectacular_location = glGetUniformLocation(shader_light, "material_0.r_spectacular");
   material_0_alpha_location = glGetUniformLocation(shader_light, "material_0.alpha");
   light_0.set_position(0.0f, 0.0f, 0.0f);
   light_0.set_intensity_diffuse(1.0f, 1.0f, 1.0f);
   light_0.set_intensity_specular(0.0f, 0.0f, 0.0f);
   light_0.set_attenuation(0.1f, 0.1f, 0.0f);
   material_0.set_parameters(1.0f, 1.0f, 1.0f, 200.0f);
   geometry_pipeline.SetMatrixStacks(mv_stack, p_stack);
   glEnable(GL_CULL_FACE);
   glEnable(GL_DEPTH_TEST);
   glFrontFace(GL_CCW);
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw floor

void floor_tr_normal(const float A[], const float B[], const float C[], float normalVector[3], float r = 1.0f, float g = 1.0f, float b = 1.0f) {
   glBegin(GL_TRIANGLES);
  	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, r, g, b);
  	glVertexAttrib3fv(GLT_ATTRIBUTE_NORMAL, normalVector);
  	glVertex3fv(A);
  	glVertex3fv(B);
  	glVertex3fv(C);
   glEnd();
}

void floor_tr(const float A[], const float B[], const float C[], float r = 1.0f, float g = 1.0f, float b = 1.0f) {
   glBegin(GL_TRIANGLES);
  	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, r, g, b);
  	glVertex3fv(A);
  	glVertex3fv(B);
  	glVertex3fv(C);
   glEnd();
}

void normal(float result[], const float A[], const float B[], const float C[]) {
   float C_min_B[3], A_min_B[3];
   m3dSubtractVectors3(C_min_B, C, B);
   m3dSubtractVectors3(A_min_B, A, B);
   m3dCrossProduct3(result, C_min_B, A_min_B);
   m3dNormalizeVector3(result);
}

void vertex(float out_array[], float a0, float a1,float a2) {
   out_array[0] = a0;
   out_array[1] = a1;
   out_array[2] = a2;
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw pyramid

void drawPyramid() {
	float A[3], B[3], C[3], normalVector[3];
	vertex(A, -0.5f,-0.5f, 0.0f);
	vertex(B, 0.5f,-0.5f,  0.0f);
	vertex(C, 0.0f, 0.0f, 1.0f);
	normal(normalVector, A, B, C);
	floor_tr_normal(A, B, C, normalVector, 0.5f, 0.5f, 0.5f);
	vertex(A, -0.5f, 0.5f, 0.0f);
	vertex(B,-0.5f,-0.5f, 0.0f);
	vertex(C, 0.0f, 0.0f, 1.0f);
	normal(normalVector, A, B, C);
	floor_tr_normal(A, B, C, normalVector, 0.5f, 0.3f, 0.3f);
	vertex(A, 0.5f, 0.5f, 0.0f);
	vertex(B, -0.5f, 0.5f, 0.0f);
	vertex(C, 0.0f, 0.0f, 1.0f);
	normal(normalVector, A, B, C);
	floor_tr_normal(A, B, C, normalVector, 0.0f, 0.9f, 0.9f);
	vertex(A, 0.5f,-0.5f, 0.0f);
	vertex(B, 0.5f, 0.5f, 0.0f);
	vertex(C,  0.0f, 0.0f,  1.0f);
	normal(normalVector, A, B, C);
	floor_tr_normal(A, B, C, normalVector, 0.0f, 0.9f, 0.1f);
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw light

void floor_light() {
   float A[3], B[3], C[3], normalVector[3];

	vertex(A, 1.0f, -1.0f, 0.0f);
	vertex(B, -1.0f, -1.0f, 0.0f);
	vertex(C,  1.0f,  1.0f, 0.0f);
	floor_tr(A, B, C, 1.0f, 0.5f, 0.2f);
	vertex(A, -1.0f, 1.0f, 0.0f);
	vertex(B, 1.0f, 1.0f, 0.0f);
	vertex(C,-1.0f,-1.0f, 0.0f);
	floor_tr(A, B, C, 1.0f, 0.5f, 0.2f);

	vertex(A, 1.0f,-1.0f, 0.0f);
	vertex(B, 1.0f, 1.0f, 0.0f);
	vertex(C, 0.0f, 0.0f, 3.0f);
	floor_tr(A, B, C, 1.0f, 0.5f, 0.2f);

	vertex(A, 1.0f, 1.0f, 0.0f);
	vertex(B,-1.0f, 1.0f, 0.0f);
	vertex(C, 0.0f, 0.0f, 3.0f);
	floor_tr(A, B, C, 1.0f, 0.5f, 0.2f);

	vertex(A,-1.0f, -1.0f, 0.0f);
	vertex(B, 0.0f, 0.0f, 3.0f);
	vertex(C, -1.0f, 1.0f, 0.0f);
	floor_tr(A, B, C,  1.0f, 0.5f, 0.2f);
 
	vertex(A, 1.0f, -1.0f, 0.0f);
	vertex(B, 0.0f, 0.0f, 3.0f);
	vertex(C, -1.0f, -1.0f, 0.0f);
	floor_tr(A, B, C,  1.0f, 0.5f, 0.2f);
}

void draw_icosahedron(int n_faces, float * vertices, int * faces) {
   float normalVector[3];
   for(int i = 0; i < n_faces; ++i) {
  	normal(normalVector, vertices + 3 * faces[3 * i], vertices + 3 * faces[3 * i + 1], vertices + 3 * faces[3 * i + 2]);
  	floor_tr_normal(vertices + 3 * faces[3 * i], vertices + 3 * faces[3 * i + 1], vertices + 3 * faces[3 * i + 2], normalVector, 0.0f, 0.0f, 0.9f);
   }
}

void draw_icosahedron_smooth(int n_faces, float *vertices, int *faces) {
   float normalVector[3];
   for(int i = 0; i < n_faces; ++i) {
  	for(int j=0 ; j < 3 ; ++j) {
     	m3dCopyVector3(normalVector, vertices + 3 * faces[i * 3 + j]);
     	m3dNormalizeVector3(normalVector);
     	floor_tr_normal(vertices + 3 * faces[3 * i], vertices + 3 * faces[3 * i + 1], vertices + 3 * faces[3 * i + 2], normalVector, 1.0f, 0.0f, 0.0f);
  	}
   }
}

void floor_floor() {
   float A[3], B[3], C[3], normalVector[] = {0.0f, 0.0f, 1.0f};
   for(int i = -5; i < 6; i += 2) {
  	for(int j = -5; j < 6; j += 2) {
     	vertex(A, (float)i + -1.0f, (float)j + -1.0f, -0.0f);
     	vertex(B, (float)i + 1.0f, (float)j + -1.0f, -0.0f);
     	vertex(C, (float)i + 1.0f, (float)j + 1.0f, -0.0f);
     	floor_tr_normal(A, B, C, normalVector, 0.05f, 0.0f, 0.8f);
     	vertex(A, (float)i + 1.0f, (float)j + 1.0f, -0.0f);
     	vertex(B, (float)i + -1.0f, (float)j + 1.0f, -0.0f);
     	vertex(C, (float)i + -1.0f, (float)j + -1.0f, -0.0f);
     	floor_tr_normal(A, B, C, normalVector, 0.05f, 0.8f, 0.0f);
  	}
   }
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw scene

void RenderScene(void) {
   float angle = timer.GetElapsedSeconds() * 3.14f / 5.0f;
   location[0] = 10.0f * cos(angle / 2.0f);
   location[1] = 10.0f * sin(angle / 2.0f);
   location[2] = 8.0f;

   light_0.position[0] = 5.0f;
   light_0.position[1] = 5.0f;

   light_0.position[0] = 5.0f * cos(angle);
   light_0.position[1] = 5.0f * sin(angle);
   light_0.position[2] = 5.0f;
   LookAt(cameraFrame, location, target, up_dir);
   cameraFrame.GetCameraMatrix(camera_matrix);
   p_stack.LoadMatrix(mFrustrum.GetProjectionMatrix());
   mv_stack.LoadMatrix(camera_matrix);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
 
   glUseProgram(ShaderColor);
   mv_stack.PushMatrix();
   mv_stack.Translate(light_0.position[0], light_0.position[1], light_0.position[2]);
   mv_stack.Scale(0.25f, 0.25f, 0.25f);
   glUniformMatrix4fv(MVPMatrixLocation_ShaderColor, 1, GL_FALSE, geometry_pipeline.GetModelViewProjectionMatrix());
   floor_light();
   mv_stack.PopMatrix();
 
   glUseProgram(shader_light);
   glUniformMatrix3fv(NormalMatrix_location, 1, GL_FALSE, geometry_pipeline.GetNormalMatrix());
   glUniformMatrix4fv(vMatrix_location, 1, GL_FALSE, camera_matrix);
   glUniform3fv(intensity_ambient_component_location, 1, intensity_ambient_component);
   glUniform3fv(light_0_position_location, 1, light_0.position);
   glUniform3fv(light_0_intensity_diffuse_location, 1, light_0.intensity_diffuse);
   glUniform3fv(light_0_intensity_specular_location, 1, light_0.intensity_specular);
   glUniform3fv(light_0_attenuation_location, 1, light_0.attenuation);
   glUniform1f(material_0_r_ambient_location, material_0.r_ambient);
   glUniform1f(material_0_r_diffuse_location, material_0.r_diffuse);
   glUniform1f(material_0_r_spectacular_location, material_0.r_spectacular);
   glUniform1f(material_0_alpha_location, material_0.alpha);
 
   glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, geometry_pipeline.GetModelViewProjectionMatrix());
   glUniformMatrix4fv(mvMatrix_location, 1, GL_FALSE, geometry_pipeline.GetModelViewMatrix());
 
		floor_floor();

     	mv_stack.PushMatrix();
     	mv_stack.Translate(0.0f, 0.0f, 0.0f);
     	glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, geometry_pipeline.GetModelViewProjectionMatrix());
     	glUniformMatrix4fv(mvMatrix_location, 1, GL_FALSE, geometry_pipeline.GetModelViewMatrix());
     	drawPyramid();
   	    mv_stack.PopMatrix();

		mv_stack.PushMatrix();
     	mv_stack.Translate(3.0f, 2.0f, 0.0f);
		mv_stack.Scale(2.0f, 2.0f, 2.0f);
     	glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, geometry_pipeline.GetModelViewProjectionMatrix());
     	glUniformMatrix4fv(mvMatrix_location, 1, GL_FALSE, geometry_pipeline.GetModelViewMatrix());
     	drawPyramid();
   	    mv_stack.PopMatrix();

		mv_stack.PushMatrix();
		mv_stack.Translate(0.0f, 0.0f, 2.0f);
		glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, geometry_pipeline.GetModelViewProjectionMatrix());
		glUniformMatrix4fv(mvMatrix_location, 1, GL_FALSE, geometry_pipeline.GetModelViewMatrix());
		draw_icosahedron(20, ico_vertices, ico_faces);
		
	mv_stack.PopMatrix();

	// Perform the buffer swap to display back buffer
	glUseProgram(0);
    glutSwapBuffers();
	glutPostRedisplay();
}

///////////////////////////////////////////////////////////////////////////////
// Main entry point for GLUT based programs

int main(int argc, char* argv[]) {

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Triangle");
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }

    SetupRC();

    glutMainLoop();
    return 0;
}