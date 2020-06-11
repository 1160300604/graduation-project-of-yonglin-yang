#include "pch.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <limits>
#include "main.h"
#include "math.h"
#include <stdlib.h>
#include <time.h>
#include "stdio.h"
#include "GL/glut.h"
#include <queue>

#pragma  warning (disable:4996) 

using namespace std;

//窗口配置
int wid = 800.0;
int hei = 800.0;
const char* title = "曲面重建";

int model;
int C = 0;
int *** datac;
bool *** st;
int datax;
int datay;
int dataz;
int baseval = 50;    //默认50
double isolen = 0.1;    //默认0.3
double boxiso = 1.5;    //默认1.5
int intersectCount = 0;
int triangleCount = 0;
float isoSurface = 100;
float XRotate = 0;
float YRotate = 0;
float ZRotate = 0;
float XOffset = 0;
float YOffset = 0;
float ZOffset = 0;
float Zoom = 100.0;
float times = 1.0;
float XButtonDown;
float YButtonDown;
bool leftButtonDown = false;
bool rightButtonDown = true;

GLfloat light_ambient[] = { 0.25, 0.25, 0.25, 1.00 };

void windows_init(int argc, char * argv[], int wid, int hei, const char * title);
void keyboardHandlers(unsigned char key, int x, int y);
void mouseHandlers(int button, int state, int x, int y);
void mouseDraggers(int x, int y);
void render();
bool MarchingCube(float X, float Y, float Z);
void MarchingCubes();
bool MarchingCube_opt(queue<coordinate> &q, float X, float Y, float Z);
void MarchingCubes_opt();
void find_seed(queue<coordinate> &q);
void SetNormal(Point &Normal, int x, int y, int z);
void generateData(string fname, double ratio);
void deleteData();

int main(int argc, char * argv[])
{
	model = 1;
	generateData("pcdfile/rabbit.pcd", 0.3);//not ok
	//generateData("pcdfile/table.pcd", 12);//ok
	//generateData("pcdfile/scene_scan.pcd", 15);//ok
	//generateData("pcdfile/M.pcd", 0.3);
	//generateData("pcdfile/maize.pcd", 2);

	windows_init(argc, argv, wid, hei, title);	//窗口初始化
	glutMouseFunc(mouseHandlers);				//鼠标触发器
	glutKeyboardFunc(keyboardHandlers);			//键盘触发器
	glutMotionFunc(mouseDraggers);				//鼠标拖曳触发器
	glClearColor(0.0, 0.0, 0.0, 0.0);					//设置背景颜色
	glClearDepth(10);									//设置深度
	glEnable(GL_DEPTH_TEST);							//启用深度测试，根据坐标的远近自动隐藏被遮住的图形
	glEnable(GL_LIGHTING);								//启用灯源
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);	//配置环境光属性
	glEnable(GL_LIGHT0);								//启动环境光
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);			//设置图像双面绘图模式均为FILL，即绘制实心图形
	glutDisplayFunc(render);							//窗口刷新的消息处理函数
	glutMainLoop();
	deleteData();
	return 0;
}

void windows_init(int argc, char * argv[], int wid, int hei, const char * title) 
{
	glutInit(&argc, argv);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(wid, hei);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow(title);
}

void keyboardHandlers(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 'd':
			YRotate += 10;
			glutPostRedisplay();
			break;
		case 'a':
			YRotate -= 10;
			glutPostRedisplay();
			break;
		case 's':
			XRotate += 10;
			glutPostRedisplay();
			break;
		case 'w':
			XRotate -= 10;
			glutPostRedisplay();
			break;
		case '+':
			times += 0.8;
			glutPostRedisplay();
			break;
		case '-':
			times -= 0.8;
			glutPostRedisplay();
			break;
		case 27:
			exit(0);
	}
}

void mouseHandlers(int button, int state, int x, int y)
{
	switch (button){
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN) 
			{
				leftButtonDown = true;
				XButtonDown = x;
				YButtonDown = y;
			}
			else if (state == GLUT_UP)
			{
				leftButtonDown = false;
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN) {
				leftButtonDown = true;
				XButtonDown = x;
				YButtonDown = y;
			}
			else if (state == GLUT_UP)
			{
				leftButtonDown = false;
			}
			break;
		default:
			break;
	}

}

void mouseDraggers(int x, int y)
{
	if (leftButtonDown)
	{
		XRotate += (x - XButtonDown) * 0.1;
		YRotate += (y - YButtonDown) * 0.1;
		XButtonDown = x;
		YButtonDown = y;
	}
	if (rightButtonDown)
	{
		XOffset += (x - XButtonDown) * 0.002;
		YOffset += (y - YButtonDown) * 0.002;
		XButtonDown = x;
		YButtonDown = y;
	}

	glutPostRedisplay();
}

void render() {

	glViewport(0, 0, wid, hei);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLfloat fAspect = (GLfloat)(hei > wid ? hei / wid : wid / hei);
	glOrtho(-2 * Zoom, 2 * Zoom, -2 * fAspect*Zoom,
		2 * fAspect*Zoom, -10 * 2 * Zoom, 10 * 2 * Zoom);
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	glTranslatef(XOffset, YOffset, ZOffset);
	glRotatef(XRotate, 1.0, 0.0, 0.0);	//绕xyz轴旋转Xrotate，YRotate，ZRotate度
	glRotatef(YRotate, 0.0, 1.0, 0.0);
	glRotatef(ZRotate, 0.0, 0.0, 1.0);
	glScalef(times, times, times);		//将三维图像延xyz轴拉伸至原来的times倍
	glPushAttrib(GL_LIGHTING_BIT);
	glDisable(GL_LIGHTING);
	glPopAttrib();
	glBegin(GL_TRIANGLES);				//多组独立填充三角形
	if (model == 0)
	{
		MarchingCubes();
	}
	else
	{
		MarchingCubes_opt();
	}
	glEnd();
	glPopMatrix();
	glutSwapBuffers();
}

bool MarchingCube(float X, float Y, float Z) {

	if (X < 0 || X >= datax - 1
		|| Y < 0 || Y >= datay - 1
		|| Z < 0 || Z >= dataz - 1)
		return false;

	float CubeVertexValue[8];
	for (int i = 0; i < 8; i++)
	{
		CubeVertexValue[i] = (float)datac[(int)(X + CubeVertex[i][0])]
			[(int)(Y + CubeVertex[i][1])]
		[(int)(Z + CubeVertex[i][2])];
	}
	int intersectEdge = 0;
	int cube_conf = 0;
	Point intersectVertex[12];
	Point NormalVector[12];

	for (int i = 0; i < 8; i++)
		if (CubeVertexValue[i] <= isoSurface)
			cube_conf |= 1 << i;

	intersectEdge = CubeEdgeFlags[cube_conf];
	if (intersectEdge == 0) 
		return false;
	intersectCount++;

	for (int i = 0; i < 12; i++)
	{
		if (intersectEdge & (1 << i)) {
			float difference = CubeVertexValue[CubeEdge[i][1]] - CubeVertexValue[CubeEdge[i][0]];
			float offset = (isoSurface - CubeVertexValue[CubeEdge[i][0]]) / difference;
			intersectVertex[i].x = X + CubeVertex[CubeEdge[i][0]][0] + CubeEdgeDirection[i][0] * offset;
			intersectVertex[i].y = Y + CubeVertex[CubeEdge[i][0]][1] + CubeEdgeDirection[i][1] * offset;
			intersectVertex[i].z = Z + CubeVertex[CubeEdge[i][0]][2] + CubeEdgeDirection[i][2] * offset;

			SetNormal(NormalVector[i], (int)intersectVertex[i].x, (int)intersectVertex[i].y, (int)intersectVertex[i].z);

		}
	}

	for (int i = 0; i < 5; i++)
	{

		if (TriTable[cube_conf][3 * i] < 0)
			break;
		triangleCount++;
		for (int j = 0; j < 3; j++)
		{
			int edge = TriTable[cube_conf][3 * i + j];
			glColor3f(intersectVertex[edge].x, intersectVertex[edge].y, intersectVertex[edge].z);
			glNormal3f(NormalVector[edge].x, NormalVector[edge].y, NormalVector[edge].z);
			glVertex3f(intersectVertex[edge].x, intersectVertex[edge].y, intersectVertex[edge].z);

		}
	}

	return true;
}

void MarchingCubes()
{
	cout << "without optimization" << endl;
	clock_t starttime, endtime;
	starttime = clock();
	int i, j, k;
	for (i = 0; i < datax - 1; i++)
		for (j = 0; j < datay- 1; j++)
			for (k = 0; k < dataz - 1; k++)
				MarchingCube(i, j, k);
	endtime = clock();
	cout << "点云重建耗时：" << (double)(endtime - starttime) << "ms" << endl;
	cout << "等值面片个数：" << intersectCount << endl;
	cout << "三角面片个数：" << triangleCount << endl;
}


bool MarchingCube_opt(queue<coordinate> &q, float X, float Y, float Z)
{
	if (X < 0 || X >= datax - 1
		|| Y < 0 || Y >= datay - 1
		|| Z < 0 || Z >= dataz - 1)
		return false;

	float CubeVertexValue[8];
	for (int i = 0; i < 8; i++)
	{
		CubeVertexValue[i] = (float)datac[(int)(X + CubeVertex[i][0])]
			[(int)(Y + CubeVertex[i][1])]
		[(int)(Z + CubeVertex[i][2])];
	}
	int intersectEdge = 0;
	int cube_conf = 0;
	Point intersectVertex[12];
	Point NormalVector[12];

	for (int i = 0; i < 8; i++)
		if (CubeVertexValue[i] <= isoSurface)
			cube_conf |= 1 << i;

	intersectEdge = CubeEdgeFlags[cube_conf];
	if (intersectEdge == 0)
		return false;
	intersectCount++;

	int nb[] = { 0, 0, 0, 0, 0, 0 };

	for (int i = 0; i < 12; i++)
	{
		if (intersectEdge & (1 << i)) {
			float difference = CubeVertexValue[CubeEdge[i][1]] - CubeVertexValue[CubeEdge[i][0]];
			float offset = (isoSurface - CubeVertexValue[CubeEdge[i][0]]) / difference;
			intersectVertex[i].x = X + CubeVertex[CubeEdge[i][0]][0] + CubeEdgeDirection[i][0] * offset;
			intersectVertex[i].y = Y + CubeVertex[CubeEdge[i][0]][1] + CubeEdgeDirection[i][1] * offset;
			intersectVertex[i].z = Z + CubeVertex[CubeEdge[i][0]][2] + CubeEdgeDirection[i][2] * offset;
			SetNormal(NormalVector[i], (int)intersectVertex[i].x, (int)intersectVertex[i].y, (int)intersectVertex[i].z);
			for (int j = 0; j < 6; j++) {
				nb[j] += neibor[i][j];
			}
		}
	}

	int XX, YY, ZZ;
	for (int i = 0; i < 6; i++) {
		XX = (int)X + deta[i][0];
		YY = (int)Y + deta[i][1];
		ZZ = (int)Z + deta[i][2];
		if (nb[i] && !st[XX][YY][ZZ]) {
			q.push({ XX, YY, ZZ });
			st[XX][YY][ZZ] = 1;
		}
	}

	for (int i = 0; i < 5; i++)
	{

		if (TriTable[cube_conf][3 * i] < 0)
			break;
		triangleCount++;
		int edge;
		for (int j = 0; j < 3; j++)
		{
			edge = TriTable[cube_conf][3 * i + j];
			glColor3f(intersectVertex[edge].x, intersectVertex[edge].y, intersectVertex[edge].z);
			glVertex3f(intersectVertex[edge].x, intersectVertex[edge].y, intersectVertex[edge].z);

		}
		glNormal3f(NormalVector[edge].x, NormalVector[edge].y, NormalVector[edge].z);
	}

	return true;
}

void MarchingCubes_opt() {
	cout << "optimized" << endl; 
	clock_t starttime, endtime;
	starttime = clock();
	/*-------------------------------------*/
	st = new bool**[datax+10];
	for (int i = 0; i < datax + 10; i++) {
		st[i] = new bool*[datay + 10];
		for (int j = 0; j < datay + 10; j++) {
			st[i][j] = new bool[dataz + 10];
			for (int k = 0; k < dataz + 10; k++) {
				st[i][j][k] = false;
			}
		}
	}
	queue<coordinate> q;
	find_seed(q);
	while (!q.empty()) {
		coordinate c = q.front();
		q.pop();
		MarchingCube_opt(q, c.x, c.y, c.z);
	}


	/*-------------------------------------*/

	endtime = clock();
	cout << "点云重建耗时：" << (double)(endtime - starttime) << "ms" << endl;
	cout << "等值面片个数：" << intersectCount << endl;
	cout << "三角面片个数：" << triangleCount << endl;
}

void find_seed(queue<coordinate> &q) {
	int i, j, k;
	int threshold = 0;
	for (i = 0; i < datax - 1; i++)
		for (j = 0; j < datay - 1; j++)
			for (k = 0; k < dataz - 1; k++)
				if (MarchingCube(i, j, k)) {
					q.push({ i, j, k });
					if (++threshold == 20)
						return;
				}
}

void SetNormal(Point &Normal, int x, int y, int z)
{
	if (x > 1 && x < datax - 1 && y>1 && y < datay- 1 && z>1 && z < dataz - 1)
	{
		Normal.x = (float)(datac[x - 1][y][z] - datac[x + 1][y][z]);
		Normal.y = (float)(datac[x][y - 1][z] - datac[x][y + 1][z]);
		Normal.z = (float)(datac[x][y][z - 1] - datac[x][y][z + 1]);
		float length = sqrtf((Normal.x * Normal.x) + (Normal.y * Normal.y) + (Normal.z * Normal.z));
		Normal.x = Normal.x * 1.0 / length;
		Normal.y = Normal.y * 1.0 / length;
		Normal.z = Normal.z * 1.0 / length;
	}
}

void generateData(string fname, double ratio)
{
	clock_t starttime, endtime;
	starttime = clock();
	ifstream infile(fname);
	if (!infile.is_open())
	{
		cout << "不存在该文件！" << endl;
		exit(-1);
	}
	cout << "文件" << fname << "读取成功！" << endl;

	char tmpline[100];
	for (int i = 1; i <= 11; i++)
	{
		if (i != 10)
		{
			infile.getline(tmpline, 100);
		}
		else 
		{
			string s;
			infile >> s;
			infile >> C;
			infile.getline(tmpline, 100);
		}
	}

	double xmin = 0, xmax = 0;
	double ymin = 0, ymax = 0;
	double zmin = 0, zmax = 0;

	vector<double*> cloud;

	int c = 0;
	while (c++ < C)
	{	
		double *tmp = new double[3];
		infile >> tmp[0] >> tmp[1] >> tmp[2];
		tmp[0] *= ratio; 
		tmp[1] *= ratio;
		tmp[2] *= ratio;
		xmin = min(tmp[0], xmin);
		xmax = max(tmp[0], xmax);
		ymin = min(tmp[1], ymin);
		ymax = max(tmp[1], ymax);
		zmin = min(tmp[2], zmin);
		zmax = max(tmp[2], zmax);
		cloud.push_back(tmp);
	}
	infile.close();

	cout << "x方向上的点云边界：xmin=" << xmin << "  xmax=" << xmax << endl;
	cout << "y方向上的点云边界：ymin=" << ymin << "  ymax=" << ymax << endl;
	cout << "z方向上的点云边界：zmin=" << zmin << "  zmax=" << zmax << endl;
	
	xmin -= (boxiso + 3.5) * isolen;
	xmax += (boxiso + 3.5) * isolen;
	ymin -= (boxiso + 3.5) * isolen;
	ymax += (boxiso + 3.5) * isolen;
	zmin -= (boxiso + 3.5) * isolen;
	zmax += (boxiso + 3.5) * isolen;

	datax = (int)((xmax - xmin) / isolen);
	datay = (int)((ymax - ymin) / isolen);
	dataz = (int)((zmax - zmin) / isolen);
	cout << "体素规模：" << datax << " * " << datay << " * " << dataz << endl;
	cout << "数据点数：" << C << endl;

	datac = new int**[datax];

	try
	{
		for (int i = 0; i < datax; i++)
		{
			datac[i] = new int*[datay];
			for (int j = 0; j < datay; j++)
			{
				datac[i][j] = new int[dataz];
				for (int k = 0; k < dataz; k++)
				{
					datac[i][j][k] = 0;
				}
			}
		}
	}
	catch (const bad_alloc &e)
	{
		std::cout << "内存条炸裂" << std::endl;
		exit(-1);
	}
	
	double xoff, yoff, zoff;        //点云中点相对于整个点云六面体左下角点的偏移量
	int xleft, xright;
	int yleft, yright;
	int zleft, zright;        //点云包围盒的边界
	double tmpsqroff;
	
	for (int i = 0; i < (int)(cloud.size()); i++)
	{
		xoff = (cloud[i][0] - xmin) / isolen;
		yoff = (cloud[i][1] - ymin) / isolen;
		zoff = (cloud[i][2] - zmin) / isolen;
		xleft = (int)(xoff - boxiso) + 1;
		xright = (int)(xoff + boxiso);
		yleft = (int)(yoff - boxiso) + 1;
		yright = (int)(yoff + boxiso);
		zleft = (int)(zoff - boxiso) + 1;
		zright = (int)(zoff + boxiso);

		for (int j = xleft; j <= xright; j++){
			for (int k = yleft; k <= yright; k++){
				for (int l = zleft; l <= zright; l++){
					tmpsqroff = (xoff - j) * (xoff - j) + (yoff - k) * (yoff - k) + (zoff - l) * (zoff - l);
					datac[j][k][l] += (int)((baseval * baseval) / (tmpsqroff * isolen * isolen));
				}
			}
		}
	}
	endtime = clock();
	std::cout << "点云转换耗时：" << (double)(endtime - starttime) << "ms" << std::endl;

	for (int i = 0; i < C; i++)
	{
		delete[] cloud[i];
	}

	return;
}

void deleteData()
{
	for (int i = 0; i < datax; i++)
	{
		for (int j = 0; j < datay; j++)
		{
			delete[] datac[i][j];
		}
		delete[] datac[i];
	}
	delete[] datac;

	return;
}
