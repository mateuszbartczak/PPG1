// Triangle.cpp
// Our first OpenGL program that will just draw a triangle on the screen.

#include <GLTools.h> // OpenGL toolkit
#include <GLFrame.h>
#include <GLFrustum.h>
#include <StopWatch.h>
#include <GLMatrixStack.h>

#ifdef __APPLE__
#include <glut/glut.h>          // OS X version of GLUT
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>            // Windows FreeGlut equivalent
#endif

GLuint shader;

GLint MVPMatrixLocation;
GLfloat matrix[16];

GLFrame mFrame;
M3DVector3f mEye;
M3DVector3f mAt;
M3DVector3f mUp;
M3DMatrix44f mCameraMatrix;

GLFrustum mFrustrum;
M3DMatrix44f mProjectionMatrix;
M3DMatrix44f mViewProjectionMatrix;
M3DMatrix44f mModelViewProjectionMatrix;

GLMatrixStack matrixStack;

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
	// Gray background
    glClearColor(0.50f, 0.50f, 0.50f, 0.50f);
    // White background
    // glClearColor(1.0f, 1.0f, 0.0f, 1.0f);

    shader = gltLoadShaderPairWithAttributes("pass_thru_shader.vp", "pass_thru_shader.fp",
            2, GLT_ATTRIBUTE_VERTEX, "vVertex", GLT_ATTRIBUTE_COLOR, "vColor");
    fprintf(stdout, "GLT_ATTRIBUTE_VERTEX : %d\nGLT_ATTRIBUTE_COLOR : %d \n",
            GLT_ATTRIBUTE_VERTEX, GLT_ATTRIBUTE_COLOR);

	MVPMatrixLocation=glGetUniformLocation(shader,"MVPMatrix");
	if(MVPMatrixLocation==-1)
	{
		fprintf(stderr,"uniform MVPMatrix could not be found\n");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw grid

void drawGrid() {
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.1f, 0.1f, 0.1f);
	glBegin(GL_LINES);
	for (int i = 0; i <= 20; i++) {
	glVertex3f(-10.0f + i, 10.0f, 0.0f);
	glVertex3f(-10.0f + i, -10.0f, 0.0f);
	glVertex3f(-10.0f, -10.0f + i, 0.0f);
	glVertex3f(10.0f, -10.0f + i, 0.0f);
	}
	glEnd();
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw pyramid

void drawPyramid() {

   // Czworokat
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.0f, 0.0f, 0.0f);
	glBegin(GL_QUADS);
		glVertex3f(-0.5f, -0.5f, 0.0f);
		glVertex3f(0.5f, -0.5f, 0.0f);
		glVertex3f(0.5f, 0.5f, 0.0f);
		glVertex3f(-0.5f, 0.5f, 0.0f);
	glEnd();

	// Trojkat 1
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 1.0f, 1.0f, 0.0f);
	glBegin(GL_TRIANGLES);
		glVertex3f(0.0f, 0.0f, 0.5f);
		glVertex3f(-0.5f, 0.5f, 0.0f);
		glVertex3f(0.5f, 0.5f, 0.0f);
	glEnd();

	// Trojkat 2
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 1.0f, 0.0f, 0.0f);
	glBegin(GL_TRIANGLES);
		glVertex3f(0.0f, 0.0f, 0.5f);
		glVertex3f(0.5f, 0.5f, 0.0f);
		glVertex3f(0.5f, -0.5f, 0.0f);
	glEnd();

	// Trojkat 3
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.0f, 1.0f, 0.0f);
	glBegin(GL_TRIANGLES);
		glVertex3f(0.0f, 0.0f, 0.5f);
		glVertex3f(0.5f, -0.5f, 0.0f);
		glVertex3f(-0.5f, -0.5f, 0.0f);
	glEnd();

	// Trojkat 4
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.0f, 0.0f, 1.0f);
	glBegin(GL_TRIANGLES);
		glVertex3f(0.0f, 0.0f, 0.5f);
		glVertex3f(-0.5f, -0.5f, 0.0f);
		glVertex3f(-0.5f, 0.5f, 0.0f);
	glEnd();
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw scene

void RenderScene(void) {
    // Clear the window with current clearing color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glUseProgram(shader);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	CStopWatch timer;
	float angle = timer.GetElapsedSeconds()*3.14f;

	M3DVector3f mAt={0,0,0};
	M3DVector3f mUp={0,0,1};
	M3DVector3f mEye;

	mEye[0]=6.8f*cos(angle);
	mEye[1]=6.0f*sin(angle);
	mEye[2]=5.0f; 
	LookAt(mFrame,mEye,mAt,mUp);
	mFrame.GetCameraMatrix(mCameraMatrix);

	matrixStack.LoadMatrix(mFrustrum.GetProjectionMatrix());
	matrixStack.MultMatrix(mCameraMatrix);

	glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, matrixStack.GetMatrix());

	drawGrid();

	matrixStack.Translate(1.0f,7.0f,0.0f);
	matrixStack.Rotate(30.0f,0.0,0.0,1.0);
	glUniformMatrix4fv(MVPMatrixLocation,1,GL_FALSE,matrixStack.GetMatrix());
	drawPyramid();

	matrixStack.PopMatrix();
	matrixStack.Translate(-7.0f,0.0f,0.0f);
	matrixStack.Scale(2.0f, 2.0f, 2.0f);
	glUniformMatrix4fv(MVPMatrixLocation,1,GL_FALSE,matrixStack.GetMatrix());
	drawPyramid();

	matrixStack.PopMatrix();

	// Perform the buffer swap to display back buffer
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