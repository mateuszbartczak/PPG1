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
#include <GL/gl.h>
#endif

GLuint shader;

GLMatrixStack modelView;
GLMatrixStack projectionMatrix;
GLGeometryTransform transformPipeline;
GLFrustum viewFrustum;
GLFrame cameraFrame;

GLint MVPMatrixLocation;
GLint MVMatrixLocation;
GLint normalMatrixLocation;
GLint shaderTextureLocation;
GLint shaderPositionLocation;
GLint shaderDiffuseColorLocation;
GLint shaderAmbientColorLocation;
GLint shaderSpecularColorLocation;
GLint alphaLocation;
float angle;
CStopWatch timer;

GLuint textureID[1];

void Texture2f(float s, float t) {
    glVertexAttrib2f(GLT_ATTRIBUTE_TEXTURE0, s, t);
}

///////////////////////////////////////////////////////////////////////////////
// Window has changed size, or has just been created. In either case, we need
// to use the window dimensions to set the viewport and the projection matrix.

void SetUpFrame(GLFrame &frame, const M3DVector3f origin, const M3DVector3f forward, const M3DVector3f up) {
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
	viewFrustum.SetPerspective(60.0f,w/h,1.0f,100.0f);
}

///////////////////////////////////////////////////////////////////////////////
// Load texture

bool LoadTGATexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode) {
    GLbyte *pBits;
    int nWidth, nHeight, nComponents;
    GLenum eFormat;

    pBits = gltReadTGABits(szFileName, &nWidth, &nHeight, &nComponents, &eFormat);
    if (pBits == NULL)
        return false;

    fprintf(stderr, "read texture from %s %dx%d\n", szFileName, nWidth, nHeight);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, nComponents, nWidth, nHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBits);

    free(pBits);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering context.
// This is the first opportunity to do any OpenGL related tasks.

void SetupRC() {
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glEnable(GL_DEPTH_TEST);
   shader = gltLoadShaderPairWithAttributes("pass_thru_shader.vp", "pass_thru_shader.fp", 4, GLT_ATTRIBUTE_VERTEX, "vVertex", GLT_ATTRIBUTE_COLOR, "vColor", GLT_ATTRIBUTE_TEXTURE0, "texCoord0", GLT_ATTRIBUTE_NORMAL, "vNormal");
   fprintf(stdout, "GLT_ATTRIBUTE_VERTEX : %d\nGLT_ATTRIBUTE_COLOR : %d \nGLT_ATTRIBUTE_TEXTURE0 : %d\n", GLT_ATTRIBUTE_VERTEX, GLT_ATTRIBUTE_COLOR, GLT_ATTRIBUTE_TEXTURE0);
    glGenTextures(1, textureID);
    glBindTexture(GL_TEXTURE_2D, textureID[0]);
    if (!LoadTGATexture("texture.tga", GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE)) {
        fprintf(stderr, "error loading texture\n");
    }
  	MVPMatrixLocation = glGetUniformLocation(shader, "MVPMatrix");
    shaderTextureLocation = glGetUniformLocation(shader, "texture0");
    shaderPositionLocation = glGetUniformLocation(shader, "lightPosition");
    shaderDiffuseColorLocation = glGetUniformLocation(shader, "diffuseColor");
    shaderAmbientColorLocation = glGetUniformLocation(shader, "ambientColor");
    shaderSpecularColorLocation = glGetUniformLocation(shader, "specularColor");
    MVMatrixLocation = glGetUniformLocation(shader, "MVMatrix");
    normalMatrixLocation = glGetUniformLocation(shader, "normalMatrix");
    alphaLocation = glGetUniformLocation(shader, "alpha");
	transformPipeline.SetMatrixStacks(modelView, projectionMatrix);
    M3DVector3f eye = {12.0f, -30.0f, 20.0f};
    M3DVector3f at = {0.0f, 0.0f, 0.0f};
    M3DVector3f up = {0.0f, 0.0f, 1.0f};
    LookAt(cameraFrame, eye, at, up);
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw pyramid

void TriangleFace(M3DVector3f a, M3DVector3f b, M3DVector3f c) {
    M3DVector3f normal, bMa, cMa;
    m3dSubtractVectors3(bMa, b, a);
    m3dSubtractVectors3(cMa, c, a);
    m3dCrossProduct3(normal, bMa, cMa);
    m3dNormalizeVector3(normal);
    glVertexAttrib3fv(GLT_ATTRIBUTE_NORMAL, normal);
	Texture2f(0, 0);
    glVertex3fv(a);
	Texture2f(1,1);
    glVertex3fv(b);
	Texture2f(1,0);
    glVertex3fv(c);
}

void DrawPyramid() {
	modelView.PushMatrix();
	modelView.PushMatrix();
    glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, transformPipeline.GetModelViewProjectionMatrix());
    glUniformMatrix4fv(MVMatrixLocation, 1, GL_FALSE, transformPipeline.GetModelViewMatrix());
    glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, transformPipeline.GetNormalMatrix());
	glBindTexture(GL_TEXTURE_2D, textureID[0]);
	M3DVector3f a = {-5.0f, -5.0f, 0.0f};
	M3DVector3f b = {5.0f, -5.0f, 0.0f};
	M3DVector3f c = {5.0f, 5.0f, 0.0f};
	M3DVector3f d = {-5.0f, 5.0f, 0.0f};
	M3DVector3f e = {0.0f, 0.0f, 1.0f};
	glBegin(GL_QUADS);
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.8f, 0.8f, 0.8f);
	glVertexAttrib2f(GLT_ATTRIBUTE_TEXTURE0, 0, 0);
	glVertex3fv(a);
	glVertexAttrib2f(GLT_ATTRIBUTE_TEXTURE0, 1, 1);
	glVertex3fv(b);
	glVertexAttrib2f(GLT_ATTRIBUTE_TEXTURE0, 1, 0);
	glVertex3fv(c);
	glVertexAttrib2f(GLT_ATTRIBUTE_TEXTURE0, 0, 1);
	glVertex3fv(d);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 1.0f, 0.0f, 0.0f);
	TriangleFace(e, a, b);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.0f, 1.0f, 0.0f);
	TriangleFace(e, b, c);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.0f, 0.0f, 1.0f);
	TriangleFace(e, c, d);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 1.0f, 1.0f, 0.0f);
	TriangleFace(e, d, a);
	glEnd();
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw floor

void DrawFloor(){
	float i = -10;
	while(i<11){
		glBegin(GL_LINES);
		glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.0f, 0.0f, 0.0f);
		glVertex3f(i, -10.0f, 0.0f);
		glVertex3f(i, 10.0f, 0.0f);
		glEnd();
		glBegin(GL_LINES);
		glVertexAttrib3f(GLT_ATTRIBUTE_COLOR, 0.0f, 0.0f, 0.0f);
		glVertex3f(-10.0f, i, 0.0f);
		glVertex3f(10.0f, i, 0.0f);
		glEnd();
		i+=1;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw scene

void RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, textureID[0]);
	glUseProgram(shader);
	M3DVector3f position={10.0f, 10.0f, 3.0f};
	M3DMatrix44f mCamera;
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
	modelView.PushMatrix();
	cameraFrame.GetCameraMatrix(mCamera);
	modelView.LoadMatrix(mCamera);
	modelView.PushMatrix();
	glUniform4f(shaderDiffuseColorLocation, 1.0f, 1.0f, 1.0f, 1.0f);
	glUniform4f(shaderAmbientColorLocation, 0.3f, 0.30f, 0.30f, 1.0f);
	glUniform4f(shaderSpecularColorLocation, 0.4f, 0.4f, 0.4f, 1.0f);
	glUniform3fv(shaderPositionLocation, 1, position);
	glUniform1i(shaderTextureLocation, 0);
	glPolygonOffset(1.0f, 1.0f);
	DrawFloor();
	glEnable(GL_POLYGON_OFFSET_FILL);
	DrawFloor();
	glDisable(GL_POLYGON_OFFSET_FILL);
    modelView.PushMatrix();
	modelView.Scale(2.0f, 2.0f, 2.0f);
	DrawPyramid();
	modelView.PopMatrix();
	modelView.PushMatrix();
	modelView.Translate(5.0f, -5.0f, 0.0f);
	DrawPyramid();
    modelView.PopMatrix();
	modelView.PopMatrix();

	// Perform the buffer swap to display back buffer
    glutSwapBuffers();
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