#include <iostream>
#include <ctime>
#include <vector>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>

#include "Time.h"
#include "UserInput.h"

#include "VertexShader.csh"
#include "PixelShader.csh"

using namespace std;
using namespace DirectX;
#pragma comment(lib,"d3d11.lib")
#pragma warning(disable : 4996)

UserInput                       userinput;

#pragma region mathlib

class VECTOR3D
{
public:
	float x, y, z;

	VECTOR3D();
	VECTOR3D(float a, float b, float c);
	VECTOR3D(const VECTOR3D& v);

	VECTOR3D& operator =(const VECTOR3D& v);
	VECTOR3D& operator +=(VECTOR3D& v);
	VECTOR3D& operator -=(VECTOR3D& v);
	VECTOR3D& operator *=(float f);
	VECTOR3D& operator /=(float f);
	VECTOR3D  operator  -();

	float Normal();
	float NormalSquared();
	VECTOR3D Normalize()const;
private:

};

VECTOR3D::VECTOR3D()
{
	x = y = z = 0.0f;
}
VECTOR3D::VECTOR3D(float a, float b, float c)
{
	x = a;
	y = b;
	z = c;
}
VECTOR3D::VECTOR3D(const VECTOR3D & v)
{
	x = v.x;
	y = v.y;
	z = v.z;
}
VECTOR3D & VECTOR3D::operator=(const VECTOR3D & v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	return *this;
}
VECTOR3D & VECTOR3D::operator+=(VECTOR3D & v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}
VECTOR3D & VECTOR3D::operator-=(VECTOR3D & v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}
VECTOR3D & VECTOR3D::operator*=(float f)
{
	x *= f;
	y *= f;
	z *= f;
	return *this;
}
VECTOR3D & VECTOR3D::operator/=(float f)
{
	assert(f != 0.0f);
	x /= f;
	y /= f;
	z /= f;
	return *this;
}
VECTOR3D VECTOR3D::operator-()
{
	return VECTOR3D(-x, -y, -z);
}

float VECTOR3D::Normal()
{
	//normal = length = magnitude
	return sqrt(x*x + y * y + z * z);
}
float VECTOR3D::NormalSquared()
{
	//normal = length = magnitude
	return (x*x + y * y + z * z);
}
VECTOR3D VECTOR3D::Normalize() const
{
	float m = sqrt(x*x + y * y + z * z);
	//assert(m!=0.0f);
	return(VECTOR3D(x / m, y / m, z / m));
}

VECTOR3D operator+(VECTOR3D u, VECTOR3D v);
VECTOR3D operator+(VECTOR3D u, VECTOR3D v)
{
	return VECTOR3D(u.x + v.x, u.y + v.y, u.z + v.z);
}
VECTOR3D operator-(VECTOR3D u, VECTOR3D v);
VECTOR3D operator-(VECTOR3D u, VECTOR3D v)
{
	return VECTOR3D(u.x - v.x, u.y - v.y, u.z - v.z);
}
VECTOR3D operator*(VECTOR3D v, float f);
VECTOR3D operator*(VECTOR3D v, float f)
{
	return VECTOR3D(v.x*f, v.y*f, v.z*f);
}
VECTOR3D operator*(float f, VECTOR3D v);
VECTOR3D operator*(float f, VECTOR3D v)
{
	return VECTOR3D(f*v.x, f*v.y, f*v.z);
}
float    operator*(VECTOR3D u, VECTOR3D v);
float operator*(VECTOR3D u, VECTOR3D v)
{
	return (float)(u.x*v.x + u.y*v.y + u.z*v.z);
}
bool     operator==(VECTOR3D u, VECTOR3D v);
bool operator==(VECTOR3D u, VECTOR3D v)
{
	return (u.x == v.x && u.y == v.y && u.z == v.z);
}
VECTOR3D operator/(VECTOR3D v, float f);
VECTOR3D operator/(VECTOR3D v, float f)
{
	assert(f != 0.0f);
	return VECTOR3D(v.x / f, v.y / f, v.z / f);
}

#pragma endregion

class DEMO_APP
{
	struct VERTEX { XMFLOAT4 xyzw; XMFLOAT4 color; XMFLOAT2 uv; XMFLOAT4 normal; };
	struct CAMERA { XMMATRIX cworld; XMMATRIX clocal; XMMATRIX cprojection; };
	struct TRANSFORM { XMMATRIX tworld; XMMATRIX tlocal; };

	enum COLLISION_STATE { NO_COLLISION, COLLISION, RESTING_CONTACT };

	HINSTANCE						application;
	WNDPROC							appWndProc;
	HWND							window;

	ID3D11Device                   *device = nullptr;
	IDXGISwapChain                 *swapchain = nullptr;
	ID3D11DeviceContext            *context = nullptr;
	ID3D11RenderTargetView         *rtv = nullptr;
	ID3D11InputLayout              *layout = nullptr;
	ID3D11Texture2D                *depthstencil = nullptr;
	ID3D11DepthStencilView         *depthstencilview = nullptr;
	D3D11_VIEWPORT                  viewport;

	float                           BACKBUFFER_WIDTH = 600.0f, BACKBUFFER_HEIGHT = 600.0f, ASPECT_RATIO = ((float)(BACKBUFFER_HEIGHT) / (float)(BACKBUFFER_WIDTH)), FOV = 45.0f, ZNEAR = 0.01f, ZFAR = 10000.0f;

	ID3D11VertexShader             *vertexshader = nullptr;
	ID3D11PixelShader              *pixelshader = nullptr;

	ID3D11Buffer                   *groundvertbuffer = nullptr;
	unsigned int                    groundvertcount = 6;

	ID3D11Buffer                   *cartesiancoordinatesvertbuffer = nullptr;
	unsigned int                    cartesiancoordinatesvertcount = 6;

	ID3D11Buffer                   *spherevertbuffer = nullptr;
	unsigned int                    spherevertcount = 0;
	vector<VERTEX>                  mesh;

	ID3D11Buffer                   *cameraconstbuffer = nullptr;
	ID3D11Buffer                   *transformconstbuffer = nullptr;

	XMMATRIX                        cworld, clocal, cprojection;
	XMMATRIX                        groundworld, groundlocal;
	XMMATRIX                        sphereworld, spherelocal;

	float                           rho;
	float                           area;
	float                           radius;
	float                           mass;
	float                           speed;

	float                           dynamicfrictioncoefficient;
	float                           staticfrictioncoefficient;
	float                           dragcoefficient;
	float                           coefficientofrestitution;

	VECTOR3D                        linearvelocity;
	VECTOR3D                        tangentialvelocity;
	VECTOR3D                        normalvelocity;

	VECTOR3D                        linearacceleration;
	VECTOR3D                        gravity;
	VECTOR3D                        weight;

	VECTOR3D                        force;
	VECTOR3D                        totalforces;
	VECTOR3D                        frictionforce;
	VECTOR3D                        normalforce;
	VECTOR3D                        dragforce;

	bool                            forceapplied = false;

public:

	Time                            time;

	DEMO_APP(HINSTANCE hinst, WNDPROC proc);
	void LoadWindow(HINSTANCE &hinst, WNDPROC &proc);
	void LoadPipeline();
	void LoadAssets();
	void LoadOBJ(char * fileName, vector<VERTEX> & FileMesh);
	void Input();
	void Render();
	void ShutDown();

	void ApplyFriction();
	DEMO_APP::COLLISION_STATE CheckSpherePlaneCollision();
};

DEMO_APP::DEMO_APP(HINSTANCE hinst, WNDPROC proc)
{
	LoadWindow(hinst, proc);
	LoadPipeline();
	LoadAssets();
}

void DEMO_APP::LoadWindow(HINSTANCE & hinst, WNDPROC & proc)
{

	application = hinst;
	appWndProc = proc;

	WNDCLASSEX  wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpfnWndProc = appWndProc;
	wndClass.lpszClassName = L"DirectXApplication";
	wndClass.hInstance = application;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	//wndClass.hIcon			= LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_FSICON));
	RegisterClassEx(&wndClass);

	RECT windowsize = { 0, 0, (long)BACKBUFFER_WIDTH, (long)BACKBUFFER_HEIGHT };
	AdjustWindowRect(&windowsize, WS_OVERLAPPEDWINDOW, false);

	window = CreateWindow(L"DirectXApplication", L"CGS Hardware Project", WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT, windowsize.right - windowsize.left, windowsize.bottom - windowsize.top,
		NULL, NULL, application, this);

	ShowWindow(window, SW_SHOW);

}

void DEMO_APP::LoadPipeline()
{

#pragma region swap chain device context
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = window;
	scd.SampleDesc.Count = 1;
	scd.Windowed = TRUE;
	D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, D3D11_SDK_VERSION, &scd, &swapchain, &device, 0, &context);
#pragma endregion

#pragma region render target
	ID3D11Texture2D * BackBuffer;
	ZeroMemory(&BackBuffer, sizeof(ID3D11Texture2D));
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
	device->CreateRenderTargetView(BackBuffer, NULL, &rtv);
#pragma endregion

#pragma region depth stencel view
	CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, lround(BACKBUFFER_WIDTH), lround(BACKBUFFER_HEIGHT), 1, 1, D3D11_BIND_DEPTH_STENCIL);
	device->CreateTexture2D(&depthStencilDesc, nullptr, &depthstencil);
	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	device->CreateDepthStencilView(depthstencil, &depthStencilViewDesc, &depthstencilview);
#pragma endregion

#pragma region Render target
	context->OMSetRenderTargets(1, &rtv, depthstencilview);
#pragma endregion

#pragma region View Port
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.Height = BACKBUFFER_HEIGHT;
	viewport.Width = BACKBUFFER_WIDTH;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	context->RSSetViewports(1, &viewport);
#pragma endregion

#pragma region shaders
	device->CreateVertexShader(VertexShader, sizeof(VertexShader), NULL, &vertexshader);
	context->VSSetShader(vertexshader, NULL, NULL);

	device->CreatePixelShader(PixelShader, sizeof(PixelShader), NULL, &pixelshader);
	context->PSSetShader(pixelshader, NULL, NULL);
#pragma endregion

#pragma region imput layout
	D3D11_INPUT_ELEMENT_DESC vertlayout[] =
	{
		"POSITION", 0,DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0,
		"COLOR", 0,DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0,
		"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 ,
		"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 ,
	};

	UINT numElements = ARRAYSIZE(vertlayout);
	device->CreateInputLayout(vertlayout, numElements, VertexShader, sizeof(VertexShader), &layout);
	context->IASetInputLayout(layout);
#pragma endregion

}

void DEMO_APP::LoadAssets()
{

#pragma region ground

	groundworld = XMMatrixIdentity();
	groundlocal = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	VERTEX * ground = new VERTEX[groundvertcount];
	ground[0].xyzw = XMFLOAT4(100.0f, 0.0f, 100.0f, 1.0f);
	ground[1].xyzw = XMFLOAT4(100.0f, 0.0f, -100.0f, 1.0f);
	ground[2].xyzw = XMFLOAT4(-100.0f, 0.0f, 100.0f, 1.0f);

	ground[3].xyzw = XMFLOAT4(-100.0f, 0.0f, -100.0f, 1.0f);
	ground[4].xyzw = XMFLOAT4(-100.0f, 0.0f, 100.0f, 1.0f);
	ground[5].xyzw = XMFLOAT4(100.0f, 0.0f, -100.0f, 1.0f);

	ground[0].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	ground[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	ground[2].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	ground[3].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	ground[4].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	ground[5].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	D3D11_BUFFER_DESC groundbufferdescription;
	ZeroMemory(&groundbufferdescription, sizeof(D3D11_BUFFER_DESC));
	groundbufferdescription.Usage = D3D11_USAGE_DEFAULT;
	groundbufferdescription.ByteWidth = sizeof(VERTEX) * groundvertcount;
	groundbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	groundbufferdescription.CPUAccessFlags = 0;
	groundbufferdescription.MiscFlags = NULL;
	groundbufferdescription.StructureByteStride = sizeof(VERTEX);

	D3D11_SUBRESOURCE_DATA groundinitdata;
	ZeroMemory(&groundinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	groundinitdata.pSysMem = ground;
	device->CreateBuffer(&groundbufferdescription, &groundinitdata, &groundvertbuffer);

#pragma endregion

#pragma region cartesiancoordinates
	VERTEX * cartesiancoordinatesxyz = new VERTEX[cartesiancoordinatesvertcount];
	cartesiancoordinatesxyz[0].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[0].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[1].xyzw = XMFLOAT4(10.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[1].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[2].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[2].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[3].xyzw = XMFLOAT4(0.0f, 10.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[3].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[4].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesxyz[4].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	cartesiancoordinatesxyz[5].xyzw = XMFLOAT4(0.0f, 0.0f, 10.0f, 1.0f);
	cartesiancoordinatesxyz[5].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	D3D11_BUFFER_DESC cartesiancoordinatesvertbufferdescription;
	ZeroMemory(&cartesiancoordinatesvertbufferdescription, sizeof(D3D11_BUFFER_DESC));
	cartesiancoordinatesvertbufferdescription.Usage = D3D11_USAGE_DEFAULT;
	cartesiancoordinatesvertbufferdescription.ByteWidth = sizeof(VERTEX) * cartesiancoordinatesvertcount;
	cartesiancoordinatesvertbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	cartesiancoordinatesvertbufferdescription.CPUAccessFlags = 0;
	cartesiancoordinatesvertbufferdescription.MiscFlags = NULL;
	cartesiancoordinatesvertbufferdescription.StructureByteStride = sizeof(VERTEX);
	D3D11_SUBRESOURCE_DATA cartesiancoordinatesinitdata;
	ZeroMemory(&cartesiancoordinatesinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	cartesiancoordinatesinitdata.pSysMem = cartesiancoordinatesxyz;
	device->CreateBuffer(&cartesiancoordinatesvertbufferdescription, &cartesiancoordinatesinitdata, &cartesiancoordinatesvertbuffer);
#pragma endregion

#pragma region sphere
	sphereworld = XMMatrixIdentity();
	spherelocal = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 1.1f, 0.0f, 1.0f
	};

	mass = 10;
	radius = 1.0f;
	linearacceleration = VECTOR3D(0.0f, 0.0f, 0.0f);
	gravity = VECTOR3D(0.0f, -9.81f, 0.0f);
	dynamicfrictioncoefficient = 0.03f;
	staticfrictioncoefficient = 0.7f;
	dragcoefficient = 0.04f;
	coefficientofrestitution = 0.8f;
	weight = gravity * mass;
	normalforce = -weight;
	area = radius * radius * 4 * XM_PI;
	rho = 1.0f;

	LoadOBJ("spheresizeone.obj", mesh);
	spherevertcount = (unsigned int)mesh.size();

	D3D11_BUFFER_DESC pointbufferdescription;
	ZeroMemory(&pointbufferdescription, sizeof(D3D11_BUFFER_DESC));
	pointbufferdescription.Usage = D3D11_USAGE_DYNAMIC;
	pointbufferdescription.ByteWidth = sizeof(VERTEX) * spherevertcount;
	pointbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	pointbufferdescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	pointbufferdescription.MiscFlags = NULL;
	pointbufferdescription.StructureByteStride = sizeof(VERTEX);

	D3D11_SUBRESOURCE_DATA pointinitdata;
	ZeroMemory(&pointinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	pointinitdata.pSysMem = mesh.data();
	device->CreateBuffer(&pointbufferdescription, &pointinitdata, &spherevertbuffer);

#pragma endregion

#pragma region camera

	//X Rotation
	//		1, 0, 0,
	//		0, cos, -sin,
	//		0, sin, cos,
	//Y Rotation
	//		cos, 0, sin,
	//		0, 1, 0,
	//		-sin, 0, cos,
	//Z Rotation
	//		cos, -sin, 0,
	//		sin, cos, 0,
	//		0, 0, 1,


	CAMERA camera;
	ZeroMemory(&camera, sizeof(CAMERA));
	cworld = XMMatrixIdentity();
	clocal = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f , 0.0f,
		5.0f, 5.0f, 20.0f, 1.0f
	};
	XMMATRIX yrotation = {
		cosf(XM_PI),   0.0f, sinf(XM_PI), 0.0f,
		0.0f,          1.0f, 0.0f,        0.0f,
		-sinf(XM_PI), 0.0f, cosf(XM_PI), 0.0f,
		0.0f,          0.0f, 0.0f,        1.0f
	};

	clocal = yrotation * clocal;
	cprojection = XMMatrixPerspectiveFovLH(FOV, ASPECT_RATIO, ZNEAR, ZFAR);
	camera.cworld = XMMatrixTranspose(cworld);
	camera.clocal = XMMatrixTranspose(XMMatrixInverse(0, clocal));
	camera.cprojection = XMMatrixTranspose(cprojection);

	D3D11_BUFFER_DESC cameraconstbufferdescription;
	ZeroMemory(&cameraconstbufferdescription, sizeof(D3D11_BUFFER_DESC));
	cameraconstbufferdescription.Usage = D3D11_USAGE_DYNAMIC;
	cameraconstbufferdescription.ByteWidth = sizeof(CAMERA);
	cameraconstbufferdescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraconstbufferdescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraconstbufferdescription.MiscFlags = NULL;
	cameraconstbufferdescription.StructureByteStride = sizeof(CAMERA);
	D3D11_SUBRESOURCE_DATA camerainitdata;
	ZeroMemory(&camerainitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	camerainitdata.pSysMem = &camera;
	device->CreateBuffer(&cameraconstbufferdescription, &camerainitdata, &cameraconstbuffer);
#pragma endregion

#pragma region transform
	TRANSFORM transform;
	ZeroMemory(&transform, sizeof(TRANSFORM));
	transform.tworld = XMMatrixTranspose(XMMatrixIdentity());
	transform.tlocal = XMMatrixTranspose(XMMatrixIdentity());

	D3D11_BUFFER_DESC transformconstbufferdescription;
	ZeroMemory(&transformconstbufferdescription, sizeof(D3D11_BUFFER_DESC));
	transformconstbufferdescription.Usage = D3D11_USAGE_DYNAMIC;
	transformconstbufferdescription.ByteWidth = sizeof(TRANSFORM);
	transformconstbufferdescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	transformconstbufferdescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	transformconstbufferdescription.MiscFlags = NULL;
	transformconstbufferdescription.StructureByteStride = sizeof(TRANSFORM);
	D3D11_SUBRESOURCE_DATA transforminitdata;
	ZeroMemory(&transforminitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	transforminitdata.pSysMem = &transform;
	device->CreateBuffer(&transformconstbufferdescription, &transforminitdata, &transformconstbuffer);
#pragma endregion

}

void DEMO_APP::LoadOBJ(char * fileName, vector<VERTEX> & FileMesh)
{
	vector<unsigned int> vertexindices, uvindices, normalindices;
	vector<XMFLOAT4>vertices;
	vector<XMFLOAT2>uvs;
	vector<XMFLOAT4>normals;
	FILE * file;
	fopen_s(&file, fileName, "r");
	if (file == nullptr)
	{
		OutputDebugString(L"File is nullptr");
		return;
	}
	while (true)
	{
		char lineheader[128]{};
		int res = fscanf(file, "%s", lineheader);
		if (res == EOF)
		{
			break;
		}
		if (strcmp(lineheader, "v") == 0)
		{
			XMFLOAT4 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			vertex.w = 1;
			vertices.push_back(vertex);
		}
		else if (strcmp(lineheader, "vt") == 0)
		{
			XMFLOAT2 uv;
			ZeroMemory(&uv, sizeof(XMFLOAT2));
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uvs.push_back(uv);
		}
		else if (strcmp(lineheader, "vn") == 0)
		{
			XMFLOAT4 normal;
			ZeroMemory(&normal, sizeof(XMFLOAT4));
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			normals.push_back(normal);
		}
		else if (strcmp(lineheader, "f") == 0)
		{
			unsigned int vertexindex[3], uvindex[3], normalindex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexindex[0], &uvindex[0], &normalindex[0], &vertexindex[1], &uvindex[1], &normalindex[1], &vertexindex[2], &uvindex[2], &normalindex[2]);
			if (matches != 9)
			{
				OutputDebugString(L"file corrupt or just can not be read");
				return;
			}
			vertexindices.push_back(vertexindex[0]);
			vertexindices.push_back(vertexindex[1]);
			vertexindices.push_back(vertexindex[2]);
			uvindices.push_back(uvindex[0]);
			uvindices.push_back(uvindex[1]);
			uvindices.push_back(uvindex[2]);
			normalindices.push_back(normalindex[0]);
			normalindices.push_back(normalindex[1]);
			normalindices.push_back(normalindex[2]);
		}
	}
	for (size_t i = 0; i < vertexindices.size(); i++)
	{
		VERTEX vertex;
		unsigned int vertexindex = vertexindices[i];
		vertex.xyzw = vertices[vertexindex - 1];
		vertex.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		unsigned int uvindex = uvindices[i];
		vertex.uv = uvs[uvindex - 1];
		unsigned int normalindex = normalindices[i];
		vertex.normal = normals[normalindex - 1];
		FileMesh.push_back(vertex);
	}
}

void DEMO_APP::Input()
{

	XMMATRIX newcamera = clocal;

#pragma region translation camera movement
	if (userinput.buttons['W'])
		newcamera.r[3] = newcamera.r[3] + newcamera.r[2] * ((+(float)time.Delta()) * 10.0f);
	if (userinput.buttons['S'])
		newcamera.r[3] = newcamera.r[3] + newcamera.r[2] * ((-(float)time.Delta()) * 10.0f);
	if (userinput.buttons['R'])
		newcamera.r[3] = newcamera.r[3] + newcamera.r[1] * ((+(float)time.Delta()) * 10.0f);
	if (userinput.buttons['F'])
		newcamera.r[3] = newcamera.r[3] + newcamera.r[1] * ((-(float)time.Delta()) * 10.0f);
	if (userinput.buttons['A'])
		newcamera.r[3] = newcamera.r[3] + newcamera.r[0] * ((-(float)time.Delta()) * 10.0f);
	if (userinput.buttons['D'])
		newcamera.r[3] = newcamera.r[3] + newcamera.r[0] * ((+(float)time.Delta()) * 10.0f);
#pragma endregion

#pragma region rotation camera movement
	if (userinput.buttons['Q'])
	{
		XMVECTOR pos = newcamera.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newcamera.r[3] = XMLoadFloat4(&zero);
		newcamera = XMMatrixRotationZ(-(float)time.Delta() * 1.0f) * newcamera;
		newcamera.r[3] = pos;
	}
	if (userinput.buttons['E'])
	{
		XMVECTOR pos = newcamera.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newcamera.r[3] = XMLoadFloat4(&zero);
		newcamera = XMMatrixRotationZ(+(float)time.Delta() * 1.0f) * newcamera;
		newcamera.r[3] = pos;
	}
	if (userinput.left_click && userinput.mouse_move)
	{
		XMVECTOR pos = newcamera.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newcamera.r[3] = XMLoadFloat4(&zero);
		newcamera = XMMatrixRotationX(userinput.diffy * (float)time.Delta() * 10.0f) * newcamera * XMMatrixRotationY(userinput.diffx * (float)time.Delta() * 10.0f);
		newcamera.r[3] = pos;
	}
#pragma endregion

	clocal = newcamera;

	XMMATRIX newtestpoint = spherelocal;

#pragma region translation testpoint movement
	if (userinput.buttons['I'])
		newtestpoint.r[3] = newtestpoint.r[3] + newtestpoint.r[2] * ((+(float)time.Delta()) * 2.0f);
	if (userinput.buttons['K'])
		newtestpoint.r[3] = newtestpoint.r[3] + newtestpoint.r[2] * ((-(float)time.Delta()) * 2.0f);
	if (userinput.buttons['Y'])
		newtestpoint.r[3] = newtestpoint.r[3] + newtestpoint.r[1] * ((+(float)time.Delta()) * 2.0f);
	if (userinput.buttons['H'])
		newtestpoint.r[3] = newtestpoint.r[3] + newtestpoint.r[1] * ((-(float)time.Delta()) * 2.0f);
	if (userinput.buttons['J'])
		newtestpoint.r[3] = newtestpoint.r[3] + newtestpoint.r[0] * ((-(float)time.Delta()) * 2.0f);
	if (userinput.buttons['L'])
		newtestpoint.r[3] = newtestpoint.r[3] + newtestpoint.r[0] * ((+(float)time.Delta()) * 2.0f);
#pragma endregion

#pragma region rotation testpoint movement
	if (userinput.buttons['U'])
	{
		XMVECTOR pos = newtestpoint.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newtestpoint.r[3] = XMLoadFloat4(&zero);
		newtestpoint = XMMatrixRotationZ(-(float)time.Delta() * 10.0f) * newtestpoint;
		newtestpoint.r[3] = pos;
	}
	if (userinput.buttons['O'])
	{
		XMVECTOR pos = newtestpoint.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newtestpoint.r[3] = XMLoadFloat4(&zero);
		newtestpoint = XMMatrixRotationZ(+(float)time.Delta() * 10.0f) * newtestpoint;
		newtestpoint.r[3] = pos;
	}
	if (userinput.right_click && userinput.mouse_move)
	{
		XMVECTOR pos = newtestpoint.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newtestpoint.r[3] = XMLoadFloat4(&zero);
		newtestpoint = XMMatrixRotationX(userinput.diffy * (float)time.Delta() * 2.0f) * newtestpoint * XMMatrixRotationY(userinput.diffx * (float)time.Delta() * 2.0f);
		newtestpoint.r[3] = pos;
	}
#pragma endregion

	userinput.mouse_move = false;
	spherelocal = newtestpoint;

}

void DEMO_APP::Render()
{

	float color[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(rtv, color);
	context->ClearDepthStencilView(depthstencilview, D3D11_CLEAR_DEPTH, 1.0f, 0);

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	D3D11_MAPPED_SUBRESOURCE msr;
	ZeroMemory(&msr, sizeof(D3D11_MAPPED_SUBRESOURCE));

#pragma region camera
	CAMERA camera;
	ZeroMemory(&camera, sizeof(CAMERA));
	camera.cworld = XMMatrixTranspose(cworld);
	camera.clocal = XMMatrixTranspose(XMMatrixInverse(0, clocal));
	camera.cprojection = XMMatrixTranspose(cprojection);
	context->Map(cameraconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(CAMERA), &camera, sizeof(CAMERA));
	context->Unmap(cameraconstbuffer, 0);
	context->VSSetConstantBuffers(0, 1, &cameraconstbuffer);
#pragma endregion

#pragma region ground



	TRANSFORM tground;
	ZeroMemory(&tground, sizeof(TRANSFORM));
	tground.tworld = XMMatrixTranspose(groundworld);
	tground.tlocal = XMMatrixTranspose(groundlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &tground, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &groundvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(groundvertcount, 0);

#pragma endregion

#pragma region cartesiancoordinates
	TRANSFORM tcartesiancoordinates;
	ZeroMemory(&tcartesiancoordinates, sizeof(TRANSFORM));
	tcartesiancoordinates.tworld = XMMatrixTranspose(XMMatrixIdentity());
	tcartesiancoordinates.tlocal = XMMatrixTranspose(XMMatrixIdentity());
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &tcartesiancoordinates, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &cartesiancoordinatesvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->Draw(cartesiancoordinatesvertcount, 0);
#pragma endregion

#pragma region sphere physics

	if (!forceapplied)
	{
		force = VECTOR3D(100.0f, 500.0f, 100.0f);
		forceapplied = true;
	}
	else
	{
		force = 0.99f * force;
		if (abs(force.x) < 0.01f) { force.x = 0; }
		if (abs(force.y) < 0.01f) { force.y = 0; }
		if (abs(force.z) < 0.01f) { force.z = 0; }
	}
	speed = linearvelocity * normalforce.Normalize();
	dragforce = -0.5f * rho * speed * dragcoefficient * area * linearvelocity;
	switch (CheckSpherePlaneCollision())
	{
	case NO_COLLISION:
	{
		totalforces = force + weight + dragforce;
		linearacceleration = totalforces / mass;
		linearvelocity = linearvelocity + linearacceleration * (float)time.Delta();
		break;
	}
	case COLLISION:
	{
		linearvelocity = -coefficientofrestitution * normalvelocity + tangentialvelocity;
		break;
	}
	case RESTING_CONTACT:
	{
		ApplyFriction();
		break;
	}
	}



	VECTOR3D pos = VECTOR3D(XMVectorGetX(spherelocal.r[3]), XMVectorGetY(spherelocal.r[3]), XMVectorGetZ(spherelocal.r[3]));
	pos = pos + linearvelocity * (float)time.Delta();
	if (pos.y <= radius)
		pos.y = radius;
	spherelocal.r[3] = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);

#pragma endregion

#pragma region sphere

	TRANSFORM tsphere;
	ZeroMemory(&tsphere, sizeof(TRANSFORM));
	tsphere.tworld = XMMatrixTranspose(sphereworld);
	tsphere.tlocal = XMMatrixTranspose(spherelocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &tsphere, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &spherevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(spherevertcount, 0);
	TRANSFORM tspherecartesiancoordinates;
	ZeroMemory(&tspherecartesiancoordinates, sizeof(TRANSFORM));
	tspherecartesiancoordinates.tworld = XMMatrixTranspose(spherelocal);
	tspherecartesiancoordinates.tlocal = XMMatrixTranspose(XMMatrixIdentity());
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &tspherecartesiancoordinates, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &cartesiancoordinatesvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->Draw(cartesiancoordinatesvertcount, 0);

#pragma endregion

	swapchain->Present(0, 0);

}

void DEMO_APP::ShutDown()
{

	device->Release();
	swapchain->Release();
	context->Release();
	rtv->Release();
	layout->Release();
	depthstencil->Release();
	depthstencilview->Release();

	vertexshader->Release();
	pixelshader->Release();

	groundvertbuffer->Release();
	cartesiancoordinatesvertbuffer->Release();
	spherevertbuffer->Release();

	cameraconstbuffer->Release();
	transformconstbuffer->Release();

	UnregisterClass(L"DirectXApplication", application);
}

void DEMO_APP::ApplyFriction()
{
	//stable static state//not moving and not going to move
	if (linearvelocity.Normal() == 0.0f)
	{
		linearvelocity = VECTOR3D(0.0f, 0.0f, 0.0f);
		linearacceleration = VECTOR3D(0.0f, 0.0f, 0.0f);
	}
	//dynamic state//already moving
	if (linearvelocity.Normal() > 0.0f)
	{
		frictionforce = -dynamicfrictioncoefficient * normalforce.Normal()*linearvelocity.Normalize();
		totalforces = force + weight + normalforce + frictionforce + dragforce;
		linearacceleration = totalforces / mass;
		linearvelocity = linearvelocity + linearacceleration * (float)time.Delta();
	}
	//unstable static state//not moving and will move
	if (linearvelocity.Normal() == 0.0f && force.Normal() >= (/*static friction*/staticfrictioncoefficient * normalforce.Normal()))
	{
		float slidingfactor = force.Normal() / (staticfrictioncoefficient * normalforce.Normal());
		linearvelocity = slidingfactor * force.Normalize();
		frictionforce = -dynamicfrictioncoefficient * normalforce.Normal() * linearvelocity.Normalize();
		totalforces = force + weight + normalforce + frictionforce + dragforce;
		linearacceleration + totalforces / mass;
		linearvelocity = linearvelocity + linearacceleration * (float)time.Delta();
	}
	if (abs(linearacceleration.x) < 0.01f) { linearacceleration.x = 0; }
	if (abs(linearacceleration.y) < 0.01f) { linearacceleration.y = 0; }
	if (abs(linearacceleration.z) < 0.01f) { linearacceleration.z = 0; }
}

DEMO_APP::COLLISION_STATE DEMO_APP::CheckSpherePlaneCollision()
{
	float vrn;
	float planedir;
	float r;
	VECTOR3D n = VECTOR3D(XMVectorGetX(groundlocal.r[1]), XMVectorGetY(groundlocal.r[1]), XMVectorGetZ(groundlocal.r[1]));
	VECTOR3D p = VECTOR3D(XMVectorGetX(groundlocal.r[3]), XMVectorGetY(groundlocal.r[3]), XMVectorGetZ(groundlocal.r[3]));
	VECTOR3D c = VECTOR3D(XMVectorGetX(spherelocal.r[3]), XMVectorGetY(spherelocal.r[3]), XMVectorGetZ(spherelocal.r[3]));
	planedir = -n * p;
	r = radius;
	normalvelocity = (linearvelocity * n) * n;
	tangentialvelocity = linearvelocity - normalvelocity;
	vrn = linearvelocity * n;
	if (normalvelocity.Normal() <= 0.1f)
	{
		normalvelocity = VECTOR3D(0.0f, 0.0f, 0.0f);
		vrn = 0.0f;
	}
	COLLISION_STATE collisionstate = NO_COLLISION;
	if (vrn < 0 && abs(n*c + planedir) <= r)
		collisionstate = COLLISION;
	else if (vrn == 0 && abs(n*c + planedir) == r)
		collisionstate = RESTING_CONTACT;
	else if (vrn > 0.0f || abs(n*c*planedir) > r)
		collisionstate = NO_COLLISION;
	return collisionstate;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
	srand((unsigned int)time(0));

	DEMO_APP                        myApp(hInstance, (WNDPROC)WndProc);
	MSG                             msg; ZeroMemory(&msg, sizeof(msg));

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		myApp.time.Signal();
		myApp.Input();
		myApp.Render();
	}

	myApp.ShutDown();
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (GetAsyncKeyState(VK_ESCAPE))
		message = WM_DESTROY;
	switch (message)
	{
	case (WM_DESTROY):
	{
		PostQuitMessage(0);
		break;
	};
	case (WM_KEYDOWN):
	{
		if (wParam)
		{
			userinput.buttons[wParam] = true;
		}
		break;
	};
	case (WM_KEYUP):
	{
		if (wParam)
		{
			userinput.buttons[wParam] = false;
		}
		break;
	};
	case (WM_LBUTTONDOWN):
	{
		userinput.diffx = 0.0f;
		userinput.diffy = 0.0f;
		userinput.left_click = true;
		break;
	};
	case (WM_LBUTTONUP):
	{
		userinput.diffx = 0.0f;
		userinput.diffy = 0.0f;
		userinput.left_click = false;
		userinput.mouse_move = false;
		break;
	};
	case (WM_RBUTTONDOWN):
	{
		userinput.right_click = true;
		break;
	};
	case (WM_RBUTTONUP):
	{
		userinput.right_click = false;
		break;
	};
	case (WM_MOUSEMOVE):
	{
		userinput.mouse_move = true;
		userinput.x = (float)GET_X_LPARAM(lParam);
		userinput.y = (float)GET_Y_LPARAM(lParam);
		userinput.diffx = userinput.x - userinput.prevX;
		userinput.diffy = userinput.y - userinput.prevY;
		userinput.prevX = userinput.x;
		userinput.prevY = userinput.y;
		break;
	};
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}