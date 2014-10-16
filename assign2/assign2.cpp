#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include "pic.h"
#include "math.h"
using namespace std;

int g_iMenuId,i=0;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
typedef enum { POINT, LINE, SOLID, TRIANGLE} MODE; //4 modes of display. Extra triangle mode to display triangles
typedef enum { RED, GREEN, BLUE, GRAY} COLOR;

CONTROLSTATE g_ControlState = ROTATE;
MODE mode = SOLID;
COLOR color = GRAY;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};
char filename[10];
int dimension = 200;

/* represents one control point along the spline */
struct point {
    double x;
    double y;
    double z;
    point()
    {
        x=0;y=0;z=0;
    }
};

float controlPos=0; //controlPos used to determine next controlpoints
point drawPos,drawTan,drawNormal,drawBinormal,drawNextPos,drawNextTan,drawNextNormal,drawNextBinormal,cameraPos,focusPoint,normal,binormal,nextTangent,nextNormal,nextBinormal,prevPos,prevNormal,prevBinormal,v,v0,v1,v2,v3,v4,v5,v6,v7; //all vectors necessary for rendering and vertices for cross-rail intersection
GLuint skyTexture; //textures
GLuint groundTexture;

/* spline struct which contains how many control points, and an array of control points */
struct spline {
    int numControlPoints;
    struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

int loadSplines(char *argv) {
    char *cName = (char *)malloc(128 * sizeof(char));
    FILE *fileList;
    FILE *fileSpline;
    int iType, i = 0, j, iLength;
    
    /* load the track file */
    fileList = fopen(argv, "r");
    if (fileList == NULL) {
        printf ("can't open file\n");
        exit(1);
    }
    
    /* stores the number of splines in a global variable */
    fscanf(fileList, "%d", &g_iNumOfSplines);
    
    g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));
    
    /* reads through the spline files */
    for (j = 0; j < g_iNumOfSplines; j++) {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");
        
        if (fileSpline == NULL) {
            printf ("can't open file\n");
            exit(1);
        }
        
        /* gets length for spline file */
        fscanf(fileSpline, "%d %d", &iLength, &iType);
        
        /* allocate memory for all the points */
        g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
        g_Splines[j].numControlPoints = iLength;
        
        /* saves the data to the struct */
        while (fscanf(fileSpline, "%lf %lf %lf",
                      &g_Splines[j].points[i].x,
                      &g_Splines[j].points[i].y,
                      &g_Splines[j].points[i].z) != EOF) {
            i++;
        }
    }
    
    free(cName);
    
    return 0;
}

void reshape(int w, int h)
{
    glViewport (0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluPerspective(40,w/h,0.01,1000);
    glMatrixMode (GL_MODELVIEW);
}

/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
    int i, j;
    Pic *in = NULL;
    
    if (filename == NULL)
        return;
    
    /* Allocate a picture buffer */
    in = pic_alloc(640, 480, 3, NULL);
    
    printf("File to save to: %s\n", filename);
    
    for (i=479; i>=0; i--) {
        glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                     &in->pix[i*in->nx*in->bpp]);
    }
    
    if (jpeg_write(filename, in))
        printf("File saved Successfully\n");
    else
        printf("Error in Saving\n");
    
    pic_free(in);
}

void keyboard(unsigned char key, int x, int y) //Handles keyboard input for changing rendering modes
{
    switch(key)
    {
        case 'p': mode = POINT; break;
        case 'l': mode = LINE; break;
        case 's': mode = SOLID; break;
        case 't': mode = TRIANGLE; break;
        case 'a': if (i<10)sprintf(filename,"%s%d.jpg","00",i); else sprintf(filename,"%s%d.jpg","0",i);saveScreenshot(filename);i++; break;
        case 'r': color = RED; break;
        case 'g': color = GREEN; break;
        case 'b': color = BLUE; break;
        case 'd': color = GRAY; break;
    };
}

void menufunc(int value)
{
    switch (value)
    {
        case 0:
            exit(0);
            break;
    }
}

/* converts mouse drags into information about
 rotation/translation/scaling */
void mousedrag(int x, int y)
{
    int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
    
    switch (g_ControlState)
    {
        case TRANSLATE:
            if (g_iLeftMouseButton)
            {
                g_vLandTranslate[0] += vMouseDelta[0]*0.01;
                g_vLandTranslate[1] -= vMouseDelta[1]*0.01;
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandTranslate[2] += vMouseDelta[1]*0.01;
            }
            break;
        case ROTATE:
            if (g_iLeftMouseButton)
            {
                g_vLandRotate[0] += vMouseDelta[1];
                g_vLandRotate[1] += vMouseDelta[0];
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandRotate[2] += vMouseDelta[1];
            }
            break;
        case SCALE:
            if (g_iLeftMouseButton)
            {
                g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
                g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
            }
            if (g_iMiddleMouseButton)
            {
                g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
            }
            break;
    }
    //cout << "Translation: " << g_vLandTranslate[0] << " " << g_vLandTranslate[1] << " " << g_vLandTranslate[2] << endl;
    //cout << "Rotation: " << g_vLandRotate[0] << " " << g_vLandRotate[1] << " " << g_vLandRotate[2] << endl;
    
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{
    switch (button)
    {
        case GLUT_LEFT_BUTTON:
            g_iLeftMouseButton = (state==GLUT_DOWN);
            break;
        case GLUT_MIDDLE_BUTTON:
            g_iMiddleMouseButton = (state==GLUT_DOWN);
            break;
        case GLUT_RIGHT_BUTTON:
            g_iRightMouseButton = (state==GLUT_DOWN);
            break;
    }
    
    switch(glutGetModifiers())
    {
        case GLUT_ACTIVE_CTRL:
            g_ControlState = TRANSLATE;
            break;
        case GLUT_ACTIVE_SHIFT:
            g_ControlState = SCALE;
            break;
        default:
            g_ControlState = ROTATE;
            break;
    }
    
    g_vMousePos[0] = x;
    g_vMousePos[1] = y;
}

void initTexture (char *filename, GLuint &Texture) //Simple initialization of texture from slides
{
	Pic *pic = jpeg_read(filename, NULL);
    
	glGenTextures(1, &Texture);
	glBindTexture(GL_TEXTURE_2D, Texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pic->nx, pic->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, pic->pix);
}

void generateEnvironment() //Simple production of 6 quads in different directions to make a cube to give an illusion of world
{
    glColor3f(1,1,1);
	glEnable(GL_TEXTURE_2D);
    
    //Sky on top and its four sides
	glBindTexture(GL_TEXTURE_2D, skyTexture); //In this case use texture in outer space. If not in outerspace, ground texture can be used.
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3f(-dimension, dimension, dimension);
	glTexCoord2f(0,1); glVertex3f(-dimension, dimension, -dimension);
	glTexCoord2f(1,0); glVertex3f(dimension, dimension, -dimension);
	glTexCoord2f(1,1); glVertex3f(dimension, dimension, dimension);
	glEnd();
    
	//Left
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3f(-dimension, -dimension, dimension);
	glTexCoord2f(0,1); glVertex3f(-dimension, -dimension, -dimension);
	glTexCoord2f(1,0); glVertex3f(-dimension, dimension, -dimension);
	glTexCoord2f(1,1); glVertex3f(-dimension, dimension, dimension);
	glEnd();
    
	//Right
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3f(dimension, dimension, dimension);
	glTexCoord2f(0,1); glVertex3f(dimension, dimension, -dimension);
	glTexCoord2f(1,0); glVertex3f(dimension, -dimension, -dimension);
	glTexCoord2f(1,1); glVertex3f(dimension, -dimension, dimension);
	glEnd();
    
	//Front
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3f(-dimension, dimension, -dimension);
	glTexCoord2f(0,1); glVertex3f(-dimension, -dimension, -dimension);
	glTexCoord2f(1,0); glVertex3f(dimension, -dimension, -dimension);
	glTexCoord2f(1,1); glVertex3f(dimension, dimension, -dimension);
	glEnd();
    
	//Back
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3f(dimension, dimension, dimension);
	glTexCoord2f(0,1); glVertex3f(dimension, -dimension, dimension);
	glTexCoord2f(1,0); glVertex3f(-dimension, -dimension, dimension);
	glTexCoord2f(1,1); glVertex3f(-dimension, dimension, dimension);
	glEnd();
    
	//Ground
	//glBindTexture(GL_TEXTURE_2D, groundTexture);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3f(dimension, -dimension, dimension);
	glTexCoord2f(0,1); glVertex3f(dimension, -dimension, -dimension);
	glTexCoord2f(1,0); glVertex3f(-dimension, -dimension, -dimension);
	glTexCoord2f(1,1); glVertex3f(-dimension, -dimension, dimension);
	glEnd();
    
	glDisable(GL_TEXTURE_2D);
}

point Pos(float u, point ctrl1, point ctrl2, point ctrl3, point ctrl4, int i) //Simply find the x,y,z coordinates of the spline given an u and 4 control points. This function can evalute points, 1st derivative, and second derivative
{
    float u3=u*u*u,u2=u*u,x,y,z;
    float basis[4];
    
    if (i==1)
    {
        basis[0] =-u3+2*u2-u;
        basis[1] = 3*u3-5*u2+2;
        basis[2] =-3*u3+4*u2+u;
        basis[3] = u3-u2;
    }
    else if (i==2)
    {
        basis[0] =-3*u2+4*u-1;
        basis[1] = 9*u2-10*u;
        basis[2] =-9*u2+8*u+1;
        basis[3] = 3*u2-2*u;
    }
    else
    {
        basis[0] =-6*u+4;
        basis[1] = 18*u-10;
        basis[2] =-18*u+8;
        basis[3] = 6*u-2;
    }
    
    x = basis[0]*ctrl1.x+basis[1]*ctrl2.x+basis[2]*ctrl3.x+basis[3]*ctrl4.x;
    y = basis[0]*ctrl1.y+basis[1]*ctrl2.y+basis[2]*ctrl3.y+basis[3]*ctrl4.y;
    z = basis[0]*ctrl1.z+basis[1]*ctrl2.z+basis[2]*ctrl3.z+basis[3]*ctrl4.z;
    
    point p;
    p.x=x;p.y=y;p.z=z;
    
    return p;
}

point Cross(point p1, point p2) //Cross product
{
    point product;
    product.x = p1.y*p2.z-p1.z*p2.y;
    product.y = p1.z*p2.x-p1.x*p2.z;
    product.z = p1.x*p2.y-p1.y*p2.x;
    return product;
}

float Dot(point p1, point p2) //Dot product
{
    return p1.x*p2.x+p1.y*p2.y+p1.z*p2.z;
}

point Normalize (point &p) //Normalize a vector
{
    float length = sqrt(Dot(p,p));
    p.x /= length; p.y /= length; p.z /= length;
    return p;
}

void railCrossSection(point &v0, point &v1, point &v2, point &v3, point cameraPos, point normal, point binormal) //Calculates v0-v7 for rail-cross section rendering
{
    int scale = 1000;
    v0.x = cameraPos.x + (normal.x-binormal.x)/scale;
    v0.y = cameraPos.y + (normal.y-binormal.y)/scale;
    v0.z = cameraPos.z + (normal.z-binormal.z)/scale;
    
    v1.x = cameraPos.x + (normal.x+binormal.x)/scale;
    v1.y = cameraPos.y + (normal.y+binormal.y)/scale;
    v1.z = cameraPos.z + (normal.z+binormal.z)/scale;
    
    v2.x = cameraPos.x + (-normal.x+binormal.x)/scale;
    v2.y = cameraPos.y + (-normal.y+binormal.y)/scale;
    v2.z = cameraPos.z + (-normal.z+binormal.z)/scale;
    
    v3.x = cameraPos.x + (-normal.x-binormal.x)/scale;
    v3.y = cameraPos.y + (-normal.y-binormal.y)/scale;
    v3.z = cameraPos.z + (-normal.z-binormal.z)/scale;
}

void drawSpline(point ctrl1, point ctrl2, point ctrl3, point ctrl4, float disp) //draw splines using positions, tangents, normals, and binormals for sophisticated railroad
{
    //glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, skyTexture);
	glBegin(GL_LINES);
    glColor3f(1,0,0);
	for(float u = 0; u<1.0; u+=0.01)
	{
        float u3 = u*u*u,u2=u*u,x,y,z;
		float basis[4];
		basis[0] =-u3+2*u2-u;
		basis[1] = 3*u3-5*u2+2;
		basis[2] =-3*u3+4*u2+u;
		basis[3] = u3-u2;
        
		x = basis[0]*ctrl1.x+basis[1]*ctrl2.x+basis[2]*ctrl3.x+basis[3]*ctrl4.x;
		y = basis[0]*ctrl1.y+basis[1]*ctrl2.y+basis[2]*ctrl3.y+basis[3]*ctrl4.y;
		z = basis[0]*ctrl1.z+basis[1]*ctrl2.z+basis[2]*ctrl3.z+basis[3]*ctrl4.z;
        
        //glVertex3f(x-disp, y, z);
        
        drawPos.x = x;
        drawPos.y = y;
        drawPos.z = z;
        drawTan = Pos(u,ctrl1,ctrl2,ctrl3,ctrl4,2);
        Normalize(drawTan);
        drawNormal = Cross(drawTan,v);
        Normalize(drawNormal);
        drawBinormal = Cross(drawTan,drawNormal);
    
        railCrossSection(v0,v1,v2,v3,drawPos,drawNormal,drawBinormal);
        
        drawNextPos = Pos(u+0.01,ctrl1,ctrl2,ctrl3,ctrl4,1);
        drawNextTan = Pos(u+0.01,ctrl1,ctrl2,ctrl3,ctrl4,2);
        Normalize(drawNextTan);
        drawNextNormal = Cross(drawBinormal,drawNextTan);
        Normalize(drawNextNormal);
        drawNextBinormal = Cross(drawNextTan,drawNextNormal);
        
        railCrossSection(v4,v5,v6,v7,drawNextPos,drawNextNormal,drawNextBinormal);
        
        /*glVertex3f(v0.x-disp,v0.y,v0.z);
        glVertex3f(v1.x-disp,v1.y,v1.z);
        glVertex3f(v0.x-disp,v0.y,v0.z);
        glVertex3f(v3.x-disp,v3.y,v3.z);
        glVertex3f(v2.x-disp,v2.y,v2.z);
        glVertex3f(v1.x-disp,v1.y,v1.z);
        glVertex3f(v2.x-disp,v2.y,v2.z);
        glVertex3f(v3.x-disp,v3.y,v3.z);
        
        glVertex3f(v4.x-disp,v4.y,v4.z);
        glVertex3f(v5.x-disp,v5.y,v5.z);
        glVertex3f(v4.x-disp,v4.y,v4.z);
        glVertex3f(v7.x-disp,v7.y,v7.z);
        glVertex3f(v6.x-disp,v6.y,v6.z);
        glVertex3f(v5.x-disp,v5.y,v5.z);
        glVertex3f(v6.x-disp,v6.y,v6.z);
        glVertex3f(v7.x-disp,v7.y,v7.z);*/
        
        glVertex3f(v0.x-disp,v0.y,v0.z);
        glVertex3f(v4.x-disp,v4.y,v4.z);
        glVertex3f(v1.x-disp,v1.y,v1.z);
        glVertex3f(v5.x-disp,v5.y,v5.z);
        glVertex3f(v2.x-disp,v2.y,v2.z);
        glVertex3f(v6.x-disp,v6.y,v6.z);
        glVertex3f(v3.x-disp,v3.y,v3.z);
        glVertex3f(v7.x-disp,v7.y,v7.z);
        
	}
    //glDisable(GL_TEXTURE_2D);
	glEnd();
}

void myinit()
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel (GL_SMOOTH);
    
    controlPos = 0; //Initialize when u=0 and its position, tangent, normal, and binormal. Store those values into prev variables to be retrieved in the update Camera function.
    v.x=1;v.y=2;v.z=-1; //arbitrary v (1,2,-1)
    cameraPos = Pos(0,g_Splines->points[(int)controlPos],g_Splines->points[(int)controlPos+1],g_Splines->points[(int)controlPos+2],g_Splines->points[(int)controlPos+3],1);
    prevPos = cameraPos;
    focusPoint = Pos(0,g_Splines->points[(int)controlPos],g_Splines->points[(int)controlPos+1],g_Splines->points[(int)controlPos+2],g_Splines->points[(int)controlPos+3],2);
    Normalize(focusPoint);
    normal = Cross(focusPoint,v);
    Normalize(normal);
    prevNormal = normal;
    binormal = Cross(focusPoint,normal);
    prevBinormal = binormal;
    initTexture( "sky.jpg", skyTexture );
	initTexture( "ground.jpg", groundTexture );
}

void updateCamera()
{
    if (controlPos >= g_Splines->numControlPoints-1)
    {
        myinit();
    }
    controlPos += (sqrt(2*9.8*(dimension-cameraPos.y)/2000000000)); //Simulation of gravity based on class's equation
    float u = controlPos-(int)controlPos;
    cameraPos = Pos(u,g_Splines->points[(int)controlPos],g_Splines->points[(int)controlPos+1],g_Splines->points[(int)controlPos+2],g_Splines->points[(int)controlPos+3],1);
    nextTangent = Pos(u,g_Splines->points[(int)controlPos],g_Splines->points[(int)controlPos+1],g_Splines->points[(int)controlPos+2],g_Splines->points[(int)controlPos+3],2);
    Normalize(nextTangent);
    nextNormal = Cross(prevBinormal,nextTangent);
    Normalize(nextNormal);
    nextBinormal = Cross(nextTangent,nextNormal);
    prevPos = cameraPos;
    prevNormal = nextNormal;
    prevBinormal = nextBinormal;
}

void doIdle()
{
    glutPostRedisplay();
}

void display()
{
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(0,0,0,nextTangent.x,nextTangent.y,nextTangent.z,nextNormal.x,nextNormal.y,nextNormal.z); //GluLookAt provides accurate view of LookAt and Up vector
    glTranslatef(-cameraPos.x+0.01,-cameraPos.y,-cameraPos.z); //Accurate simulation of roller coaster ride
    for (int i=0;i<g_Splines->numControlPoints;i++)
    {
        drawSpline(g_Splines->points [i],g_Splines->points [i+1],g_Splines->points [i+2],g_Splines->points [i+3],0);
        drawSpline(g_Splines->points [i],g_Splines->points [i+1],g_Splines->points [i+2],g_Splines->points [i+3],0.1);
    }
    generateEnvironment();
    updateCamera();
    glutSwapBuffers();
}

int main (int argc, char ** argv)
{
    if (argc<2)
    {
        printf ("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }
    loadSplines(argv[1]);
    glutInit(&argc,argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize (640, 480);
    glutInitWindowPosition (100, 100);
    glutCreateWindow (argv[1]);
    glEnable(GL_DEPTH_TEST);
    /* tells glut to use a particular display function to redraw */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    /* allow the user to quit using the right mouse button menu */
    g_iMenuId = glutCreateMenu(menufunc);
    glutSetMenu(g_iMenuId);
    glutAddMenuEntry("Quit",0);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    /* replace with any animate code */
    glutIdleFunc(doIdle);
    glutKeyboardFunc(keyboard);
    
    /* callback for mouse drags */
    glutMotionFunc(mousedrag);
    /* callback for idle mouse movement */
    glutPassiveMotionFunc(mouseidle);
    /* callback for mouse button changes */
    glutMouseFunc(mousebutton);
    
    /* do initialization */
    myinit();
    
    glutMainLoop();
    return 0;
}