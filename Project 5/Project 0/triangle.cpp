// Triangle.cpp
// Our first OpenGL program that will just draw a triangle on the screen.

#include <GLTools.h> // OpenGL toolkit
#include <GLFrame.h>
#include <GLFrustum.h>
#include <StopWatch.h>
#include <GLMatrixStack.h>
#include <GLGeometryTransform.h>
#include <math3d.h>
#include <StopWatch.h>
#include <vector>

#ifdef __APPLE__
#include <glut/glut.h>          // OS X version of GLUT
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>            // Windows FreeGlut equivalent
#endif

GLuint shader;

std::vector<float> vertices;
GLuint n_vertices;
std::vector<GLuint> faces;
GLuint n_faces;

GLuint shaderAmbientColorLocation;
GLuint shaderDiffuseColorLocation;
GLuint shaderSpecularColorLocation;
GLuint shaderSpecularExponentLocation;

GLuint shaderAttenuation0Location;
GLuint shaderAttenuation1Location;
GLuint shaderAttenuation2Location;

GLuint normalMatrixLocation;
GLuint MVPMatrixLocation;
GLuint MVMatrixLocation;

GLuint shaderPositionLocation;
GLuint shaderColorLocation;
GLuint shaderAngleLocation;

GLFrustum frustum;
GLFrame cameraFrame;

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
}

void LookAt(GLFrame &frame, const M3DVector3f eye, const M3DVector3f at, const M3DVector3f up) {
    M3DVector3f forward;
    m3dSubtractVectors3(forward, at, eye);
    SetUpFrame(frame, eye, forward, up);
}

void ChangeSize(int w, int h) {
    glViewport(0, 0, w, h);
	frustum.SetPerspective(50.0f,(float)(w)/h,2.0f,80.0f);
}

///////////////////////////////////////////////////////////////////////////////
// Load data

void LoadVertices()
{
   FILE *fvertices=fopen("geode_vertices.dat","r");
   if(fvertices==NULL) {
   fprintf(stderr,"cannot open vertices file for reading\n");
   exit(-1);
   }
   char line[120];
   
   while(fgets(line,120,fvertices)!=NULL) {
   float x,y,z;
   double norm;
   sscanf(line,"%f %f %f",&x,&y,&z);
  
   norm=x*x+y*y+z*z;
   norm=sqrt(norm);
   n_vertices++;
   vertices.push_back(x);
   vertices.push_back(y);
   vertices.push_back(z);
   vertices.push_back(1.0f);   vertices.push_back(x/norm);
   vertices.push_back(y/norm);
   vertices.push_back(z/norm);
   } fprintf(stderr,"nv = %u %u\n",n_vertices,vertices.size());
}

void LoadFaces()
{
	FILE *ffaces=fopen("geode_faces.dat","r");
   if(ffaces==NULL) {
   fprintf(stderr,"cannot open faces file for reading\n");
   exit(-1);
   }

   char line[120];
   while(fgets(line,120,ffaces)!=NULL) {
   GLuint  i,j,k;
   
   if(3!=sscanf(line,"%u %u %u",&i,&j,&k)){
   fprintf(stderr,"error reading faces\n"); 
   exit(-1);
   }
   n_faces++;
   faces.push_back(i-1);
   faces.push_back(j-1);
   faces.push_back(k-1);
   
   } 	fprintf(stderr,"nf = %u\n",n_faces);
}

///////////////////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering context.
// This is the first opportunity to do any OpenGL related tasks.

void SetupRC() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	shader = gltLoadShaderPairWithAttributes("pass_thru_shader.vp", "pass_thru_shader.fp", 2, GLT_ATTRIBUTE_VERTEX, "vVertex", GLT_ATTRIBUTE_NORMAL, "vNormal");
    fprintf(stdout, "GLT_ATTRIBUTE_VERTEX : %d\nGLT_ATTRIBUTE_COLOR : %d \n", GLT_ATTRIBUTE_VERTEX, GLT_ATTRIBUTE_COLOR);

	MVPMatrixLocation = glGetUniformLocation(shader,"MVPMatrix");
	normalMatrixLocation = glGetUniformLocation(shader,"normalMatrix");

	MVMatrixLocation = glGetUniformLocation(shader,"MVMatrix");
	shaderPositionLocation = glGetUniformLocation(shader, "light1.position");
	shaderColorLocation = glGetUniformLocation(shader, "light1.color");
	shaderAngleLocation = glGetUniformLocation(shader, "light1.angle");

	shaderAttenuation0Location = glGetUniformLocation(shader, "light1.attenuation0");
	shaderAttenuation1Location = glGetUniformLocation(shader, "light1.attenuation1");
	shaderAttenuation2Location = glGetUniformLocation(shader, "light1.attenuation2");

	shaderAmbientColorLocation = glGetUniformLocation(shader, "material.ambientColor");
	shaderDiffuseColorLocation = glGetUniformLocation(shader, "material.diffuseColor");
	shaderSpecularColorLocation = glGetUniformLocation(shader, "material.specularColor");
	shaderSpecularExponentLocation = glGetUniformLocation(shader, "material.specularExponent");

	glEnable(GL_DEPTH_TEST);
	LoadVertices();
	LoadFaces();
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
// Called to draw scene

void RenderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(shader);  
	static CStopWatch Timer;

	float angle = Timer.GetElapsedSeconds() * 3/7;
	GLfloat position[] = { 1.0f, 1.0f, .5f };
	GLfloat color[] = { 1.0f, 1.0f, 1.0f };
	GLfloat angle2 = 90.0f;

	GLfloat attenuation0 = 0.1f;
	GLfloat attenuation1 = 0.1f;
	GLfloat attenuation2 = 0.1f;

	GLfloat ambientColor[] = { 0.1f, 0.5f, 0.1f };
	GLfloat diffuseColor[] = { 1.0f, 1.0f, 0.0f };
	GLfloat specularColor[] = { 0.0f, 0.0f, 1.0f };
	GLfloat specularExponent = 100.0f;

	GLMatrixStack modelView;
	GLMatrixStack projection;

	M3DVector3f at={0,0,0};
    M3DVector3f up={0,0,1};
    M3DVector3f eye;

	eye[0] = 2 * cos(angle);
	eye[1] = 2 * sin(angle);
	eye[2] = 10;

    LookAt(cameraFrame,eye,at,up);

	GLGeometryTransform geometryPipeline;
	geometryPipeline.SetMatrixStacks(modelView,projection);
	projection.LoadMatrix(frustum.GetProjectionMatrix());

	M3DMatrix44f mCamera;
	cameraFrame.GetCameraMatrix(mCamera);
	modelView.LoadMatrix(mCamera);
	modelView.PushMatrix();

	
	for(int i=-10; i<=10; ++i)
		{
			glBegin(GL_LINES);
			glVertex3f(-10, i, 0);
			glVertex3f(10, i, 0);
			glEnd();
			glBegin(GL_LINES);
			glVertex3f(i, -10, 0);
			glVertex3f(i, 10, 0);
			glEnd();
		}

	modelView.PopMatrix();

	modelView.PushMatrix();
	modelView.Translate(0,0.0,0.0);

	glUniformMatrix4fv(MVMatrixLocation,1,GL_FALSE,geometryPipeline.GetModelViewProjectionMatrix());
	glUniformMatrix4fv(MVPMatrixLocation,1,GL_FALSE,geometryPipeline.GetModelViewMatrix());
	glUniformMatrix3fv(normalMatrixLocation,1,GL_FALSE,geometryPipeline.GetNormalMatrix());
	glUniform3fv(shaderPositionLocation, 1, position);
	glUniform3fv(shaderColorLocation, 1, color);
	glUniform1f(shaderAngleLocation, angle2);
	glUniform1f(shaderAttenuation0Location, attenuation0);
	glUniform1f(shaderAttenuation1Location, attenuation1);
	glUniform1f(shaderAttenuation2Location, attenuation2);
	glUniform3fv(shaderAmbientColorLocation, 1, ambientColor);
	glUniform3fv(shaderDiffuseColorLocation, 1, diffuseColor);
	glUniform3fv(shaderSpecularColorLocation, 1, specularColor);
	glUniform1f(shaderSpecularExponentLocation, specularExponent);

	GLuint vertex_buffer;
	glGenBuffers(1,&vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,n_vertices*sizeof(float)*7,&vertices[0],GL_STATIC_DRAW);
	if(glGetError()!=GL_NO_ERROR) {
   	fprintf(stderr,"error copying vertices\n");
	}
	glVertexAttribPointer(GLT_ATTRIBUTE_VERTEX,4,GL_FLOAT,GL_FALSE,sizeof(float)*7,(const GLvoid *)0);
	glVertexAttribPointer(GLT_ATTRIBUTE_NORMAL,3,GL_FLOAT,GL_FALSE,sizeof(float)*7,(const GLvoid *)(4*sizeof(float)) );
	glEnableVertexAttribArray(GLT_ATTRIBUTE_VERTEX);
	glEnableVertexAttribArray(GLT_ATTRIBUTE_NORMAL);

	GLuint faces_buffer;
	glGenBuffers(1,&faces_buffer);
	if(glGetError()!=GL_NO_ERROR) {
	fprintf(stderr,"faces_buffer invalid\n");
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,faces_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,n_faces*sizeof(GLuint)*3,&faces[0],GL_STATIC_DRAW);
	if(glGetError()!=GL_NO_ERROR) {
	fprintf(stderr,"error copying faces\n");
	}
	glDrawElements(GL_TRIANGLES,3*n_faces,GL_UNSIGNED_INT,0);

	modelView.PopMatrix();

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