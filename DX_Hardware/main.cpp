#include <iostream>
#include <ctime>
#include <vector>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>//I might not need this later(it just had some collision alogithoms)

#include "Time.h"
#include "UserInput.h"

#include "VertexShader.csh"
#include "PixelShader.csh"

using namespace std;
using namespace DirectX;
using namespace DirectX::TriangleTests;//I might not need this later(it just had some collision alogithoms)

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
VECTOR3D Vector3Cross(VECTOR3D u, VECTOR3D v);
VECTOR3D Vector3Cross(VECTOR3D u, VECTOR3D v)
{
	VECTOR3D t;
	t.x = (u.y*v.z) - (v.y*u.z);
	t.y = -1 * ((u.x*v.z) - (v.x*u.z));
	t.z = (u.x*v.y) - (v.x*u.y);
	return t;
}
#pragma endregion

class DEMO_APP
{
	struct VERTEX { XMFLOAT4 xyzw; XMFLOAT4 color; XMFLOAT2 uv; XMFLOAT4 normal; };
	struct CAMERA { XMMATRIX cworld; XMMATRIX clocal; XMMATRIX cprojection; };
	struct TRANSFORM { XMMATRIX tworld; XMMATRIX tlocal; };

	enum COLLISION_STATE { NO_COLLISION, COLLISION, RESTING_CONTACT };

	ID3D11Device                   *device = nullptr;
	IDXGISwapChain                 *swapchain = nullptr;
	ID3D11DeviceContext            *context = nullptr;
	ID3D11RenderTargetView         *rtv = nullptr;
	ID3D11InputLayout              *layout = nullptr;
	ID3D11Texture2D                *depthstencil = nullptr;
	ID3D11DepthStencilView         *depthstencilview = nullptr;
	D3D11_VIEWPORT                  viewport;

	float                           BACKBUFFER_WIDTH = 600.0f, BACKBUFFER_HEIGHT = 600.0f, ASPECT_RATIO = ((float)(BACKBUFFER_HEIGHT) / (float)(BACKBUFFER_WIDTH)), FOV = 45.0f, ZNEAR = 1.0f, ZFAR = 1000.0f;

	ID3D11VertexShader             *vertexshader = nullptr;
	ID3D11PixelShader              *pixelshader = nullptr;

	ID3D11Buffer                   *cameraconstbuffer = nullptr;
	ID3D11Buffer                   *transformconstbuffer = nullptr;

	ID3D11Buffer                   *groundvertbuffer = nullptr;
	unsigned int                    groundmeshvertcount = 3;
	VERTEX                         *groundmesh = nullptr;

	ID3D11Buffer                   *movinggroundvertbuffer = nullptr;
	unsigned int                    movinggroundmeshvertcount = 3;
	VERTEX                         *movinggroundmesh = nullptr;

	ID3D11Buffer                   *cartesiancoordinatesvertbuffer = nullptr;
	unsigned int                    cartesiancoordinatesmeshvertcount = 6;
	VERTEX                         *cartesiancoordinatesmesh = nullptr;

	ID3D11Buffer                   *spherevertbuffer = nullptr;
	unsigned int                    spheremeshvertcount = 0;
	vector<VERTEX>                  spheremesh;

	ID3D11Buffer                   *pickinglinevertbuffer = nullptr;
	unsigned int                    pickinglinemeshvertcount = 2;
	VERTEX                         *pickinglinemesh = nullptr;

	ID3D11Buffer                   *redcubevertbuffer = nullptr;
	ID3D11Buffer                   *greencubevertbuffer = nullptr;
	ID3D11Buffer                   *bluecubevertbuffer = nullptr;
	unsigned int                    cubemeshvertcount = 0;
	vector<VERTEX>                  redcubemesh;
	vector<VERTEX>                  greencubemesh;
	vector<VERTEX>                  bluecubemesh;

	XMMATRIX                        cworld, clocal, cprojection;
	XMMATRIX                        pickinglineworld, startpickinglinelocal, endpickinglinelocal, collisionpickinglinelocal;
	XMMATRIX                        groundworld, groundlocal;
	XMMATRIX                        movinggroundworld, movinggroundlocal;
	XMMATRIX                        sphereworld, spherelocal;

	bool                            pickinglinerender = false, collisionpickinglinerender = false;

#pragma region physics variables for ball bouncing
	//bool                            forceapplied = false;
	//float                           radius;
	//float                           mass;
	//float                           coefficientofrestitution;

	//VECTOR3D                        linearvelocity;
	//VECTOR3D                        tangentialvelocity;
	//VECTOR3D                        normalvelocity;
	//VECTOR3D                        linearacceleration;
	//VECTOR3D                        gravity;
	//VECTOR3D                        weight;
	//VECTOR3D                        force;
#pragma endregion

	void LoadWindow(HINSTANCE &hinst, WNDPROC &proc);
	void LoadPipeline();
	void LoadAssets();
	void LoadOBJ(char * fileName, vector<VERTEX> & FileMesh);
	DEMO_APP::COLLISION_STATE CheckSpherePlaneCollision();
	DEMO_APP::COLLISION_STATE RayIntersectsTriangle(XMVECTOR Origin, XMVECTOR Direction, XMVECTOR V0, XMVECTOR V1, XMVECTOR V2, float& Dist);
	DEMO_APP::COLLISION_STATE RayIntersectsTriangles(XMVECTOR Origin, XMVECTOR Direction, vector<VERTEX> &mesh, float &Dist);
public:
	Time                            time;
	HINSTANCE						application;
	WNDPROC							appWndProc;
	HWND							window;

	DEMO_APP(HINSTANCE hinst, WNDPROC proc);
	void Input();
	void Update();
	void Render();
	void ShutDown();
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
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
	D3D11_SUBRESOURCE_DATA id;
	ZeroMemory(&id, sizeof(D3D11_SUBRESOURCE_DATA));

#pragma region ground

	groundworld = XMMatrixIdentity();
	groundlocal = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	groundmesh = new VERTEX[groundmeshvertcount];
	groundmesh[0].xyzw = XMFLOAT4(10.0f, 0.0f, 10.0f, 1.0f);
	groundmesh[1].xyzw = XMFLOAT4(10.0f, 0.0f, -10.0f, 1.0f);
	groundmesh[2].xyzw = XMFLOAT4(-10.0f, 0.0f, 10.0f, 1.0f);

	groundmesh[0].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	groundmesh[1].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	groundmesh[2].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX) * groundmeshvertcount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(VERTEX);
	id.pSysMem = groundmesh;
	device->CreateBuffer(&bd, &id, &groundvertbuffer);

#pragma endregion

#pragma region moving ground

	movinggroundworld = XMMatrixIdentity();
	movinggroundlocal = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	movinggroundmesh = new VERTEX[movinggroundmeshvertcount];
	movinggroundmesh[0].xyzw = XMFLOAT4(-10.0f, 0.0f, -10.0f, 1.0f);
	movinggroundmesh[1].xyzw = XMFLOAT4(-10.0f, 0.0f, 10.0f, 1.0f);
	movinggroundmesh[2].xyzw = XMFLOAT4(10.0f, 0.0f, -10.0f, 1.0f);

	movinggroundmesh[0].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	movinggroundmesh[1].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	movinggroundmesh[2].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX) * movinggroundmeshvertcount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(VERTEX);
	id.pSysMem = movinggroundmesh;
	device->CreateBuffer(&bd, &id, &movinggroundvertbuffer);

#pragma endregion

#pragma region cartesiancoordinates
	cartesiancoordinatesmesh = new VERTEX[cartesiancoordinatesmeshvertcount];
	cartesiancoordinatesmesh[0].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[0].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[1].xyzw = XMFLOAT4(10.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[1].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[2].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[2].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[3].xyzw = XMFLOAT4(0.0f, 10.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[3].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[4].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	cartesiancoordinatesmesh[4].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	cartesiancoordinatesmesh[5].xyzw = XMFLOAT4(0.0f, 0.0f, 10.0f, 1.0f);
	cartesiancoordinatesmesh[5].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX) * cartesiancoordinatesmeshvertcount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(VERTEX);
	id.pSysMem = cartesiancoordinatesmesh;
	device->CreateBuffer(&bd, &id, &cartesiancoordinatesvertbuffer);
#pragma endregion

#pragma region sphere physics variables
	/*radius = 1.0f;
	mass = 10;
	coefficientofrestitution = 0.7f;
	linearacceleration = VECTOR3D(0.0f, 0.0f, 0.0f);
	gravity = VECTOR3D(0.0f, -9.81f, 0.0f);
	weight = gravity * mass;*/
#pragma endregion

#pragma region sphere
	sphereworld = XMMatrixIdentity();
	spherelocal = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		1.1f, 5.1f, 1.1f, 1.0f
	};

	LoadOBJ("sphere.obj", spheremesh);
	spheremeshvertcount = (unsigned int)spheremesh.size();

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX) * spheremeshvertcount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(VERTEX);
	id.pSysMem = spheremesh.data();
	device->CreateBuffer(&bd, &id, &spherevertbuffer);
#pragma endregion

#pragma region picking line
	pickinglineworld = XMMatrixIdentity();
	startpickinglinelocal = XMMatrixIdentity();
	endpickinglinelocal = XMMatrixIdentity();
	collisionpickinglinelocal = XMMatrixIdentity();

	pickinglinemesh = new VERTEX[pickinglinemeshvertcount];
	pickinglinemesh[0].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	pickinglinemesh[0].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pickinglinemesh[1].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	pickinglinemesh[1].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX) * pickinglinemeshvertcount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(VERTEX);
	id.pSysMem = pickinglinemesh;
	device->CreateBuffer(&bd, &id, &pickinglinevertbuffer);
#pragma endregion

#pragma region rgb cubes

	LoadOBJ("cube.obj", redcubemesh);
	cubemeshvertcount = (unsigned int)redcubemesh.size();
	redcubemesh.resize(cubemeshvertcount);
	greencubemesh.resize(cubemeshvertcount);
	bluecubemesh.resize(cubemeshvertcount);
	for (size_t i = 0; i < cubemeshvertcount; i++)
	{
		redcubemesh[i].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
		greencubemesh[i] = redcubemesh[i];
		greencubemesh[i].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
		bluecubemesh[i] = redcubemesh[i];
		bluecubemesh[i].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	}
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX) * cubemeshvertcount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(VERTEX);
	id.pSysMem = redcubemesh.data();
	device->CreateBuffer(&bd, &id, &redcubevertbuffer);

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX) * cubemeshvertcount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(VERTEX);
	id.pSysMem = greencubemesh.data();
	device->CreateBuffer(&bd, &id, &greencubevertbuffer);

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX) * cubemeshvertcount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(VERTEX);
	id.pSysMem = bluecubemesh.data();
	device->CreateBuffer(&bd, &id, &bluecubevertbuffer);
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
	//XMMATRIX yrotation = {
	//	cosf(XM_PI),   0.0f, sinf(XM_PI), 0.0f,
	//	0.0f,          1.0f, 0.0f,        0.0f,
	//	-sinf(XM_PI), 0.0f, cosf(XM_PI), 0.0f,
	//	0.0f,          0.0f, 0.0f,        1.0f
	//};
	//clocal = yrotation * clocal;

	cworld = XMMatrixIdentity();
	clocal = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f , 0.0f,
		10.0f, 10.0f, -25.0f, 1.0f
	};

	CAMERA c;
	ZeroMemory(&c, sizeof(CAMERA));
	cprojection = XMMatrixPerspectiveFovLH(FOV, ASPECT_RATIO, ZNEAR, ZFAR);
	c.cworld = XMMatrixTranspose(cworld);
	c.clocal = XMMatrixTranspose(XMMatrixInverse(0, clocal));
	c.cprojection = XMMatrixTranspose(cprojection);

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(CAMERA);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(CAMERA);
	id.pSysMem = &c;
	device->CreateBuffer(&bd, &id, &cameraconstbuffer);
#pragma endregion

#pragma region transform
	TRANSFORM t;
	ZeroMemory(&t, sizeof(TRANSFORM));
	t.tworld = XMMatrixTranspose(XMMatrixIdentity());
	t.tlocal = XMMatrixTranspose(XMMatrixIdentity());

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(TRANSFORM);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = NULL;
	bd.StructureByteStride = sizeof(TRANSFORM);
	id.pSysMem = &t;
	device->CreateBuffer(&bd, &id, &transformconstbuffer);
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

#pragma region camera movement
	XMMATRIX newcamera = clocal;
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
	if (userinput.left_click && userinput.mouse_move)
	{
		XMVECTOR pos = newcamera.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newcamera.r[3] = XMLoadFloat4(&zero);
		newcamera = XMMatrixRotationX(userinput.diffy * (float)time.Delta() * 10.0f) * newcamera * XMMatrixRotationY(userinput.diffx * (float)time.Delta() * 10.0f);
		newcamera.r[3] = pos;
	}
	clocal = newcamera;
#pragma endregion

#pragma region ground movement
	XMMATRIX newmatrix = movinggroundlocal;
	if (userinput.buttons['J'])
		newmatrix.r[3] = newmatrix.r[3] + newmatrix.r[0] * ((-(float)time.Delta()) * 10.0f);
	if (userinput.buttons['L'])
		newmatrix.r[3] = newmatrix.r[3] + newmatrix.r[0] * ((+(float)time.Delta()) * 10.0f);
	if (userinput.buttons['Y'])
		newmatrix.r[3] = newmatrix.r[3] + newmatrix.r[1] * ((+(float)time.Delta()) * 10.0f);
	if (userinput.buttons['H'])
		newmatrix.r[3] = newmatrix.r[3] + newmatrix.r[1] * ((-(float)time.Delta()) * 10.0f);
	if (userinput.buttons['I'])
		newmatrix.r[3] = newmatrix.r[3] + newmatrix.r[2] * ((+(float)time.Delta()) * 10.0f);
	if (userinput.buttons['K'])
		newmatrix.r[3] = newmatrix.r[3] + newmatrix.r[2] * ((-(float)time.Delta()) * 10.0f);
	if (userinput.right_click && userinput.mouse_move)
	{
		XMVECTOR pos = newmatrix.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newmatrix.r[3] = XMLoadFloat4(&zero);
		newmatrix = XMMatrixRotationX(userinput.diffy * (float)time.Delta() * 10.0f) * newmatrix * XMMatrixRotationY(userinput.diffx * (float)time.Delta() * 10.0f);
		newmatrix.r[3] = pos;
	}
	movinggroundlocal = newmatrix;
#pragma endregion

#pragma region ground picking

	if (userinput.buttons['1'])
	{
		//unproject screen click
		pickinglinerender = true;
		XMVECTOR start = XMVectorSet(userinput.x, userinput.y, ZFAR, 1.0f);
		startpickinglinelocal.r[3] = XMVector3Unproject(start, 0.0f, 0.0f, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, 0.0f, ZNEAR, cprojection, XMMatrixInverse(0, clocal), cworld);
		endpickinglinelocal.r[3] = XMVector3Unproject(start, 0.0f, 0.0f, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, 0.0f, ZFAR, cprojection, XMMatrixInverse(0, clocal), cworld);
		pickinglinemesh[0].xyzw = XMFLOAT4(XMVectorGetX(startpickinglinelocal.r[3]), XMVectorGetY(startpickinglinelocal.r[3]), XMVectorGetZ(startpickinglinelocal.r[3]), 1.0f);
		pickinglinemesh[1].xyzw = XMFLOAT4(XMVectorGetX(endpickinglinelocal.r[3]), XMVectorGetY(endpickinglinelocal.r[3]), XMVectorGetZ(endpickinglinelocal.r[3]), 1.0f);

		float distance = 0.0f;
		bool intersection = false;

		//test collision with stationary triangle
		intersection = RayIntersectsTriangle(
			startpickinglinelocal.r[3],
			endpickinglinelocal.r[3] - startpickinglinelocal.r[3],
			XMVectorSet(groundmesh[2].xyzw.x, groundmesh[2].xyzw.y, groundmesh[2].xyzw.z, 1.0f),
			XMVectorSet(groundmesh[1].xyzw.x, groundmesh[1].xyzw.y, groundmesh[1].xyzw.z, 1.0f),
			XMVectorSet(groundmesh[0].xyzw.x, groundmesh[0].xyzw.y, groundmesh[0].xyzw.z, 1.0f),
			distance);

		if (intersection)
		{
			collisionpickinglinelocal.r[3] = ((endpickinglinelocal.r[3] - startpickinglinelocal.r[3]) * distance) + startpickinglinelocal.r[3];
			collisionpickinglinerender = true;
		}
		else
			collisionpickinglinerender = false;

		//test collision with moving triangle
		if (!intersection)
		{
			XMVECTOR newstart = startpickinglinelocal.r[3] - movinggroundlocal.r[3];
			XMVECTOR newend = endpickinglinelocal.r[3] - movinggroundlocal.r[3];
			intersection = RayIntersectsTriangle(
				newstart,
				newend - newstart,
				XMVectorSet(movinggroundmesh[2].xyzw.x, movinggroundmesh[2].xyzw.y, movinggroundmesh[2].xyzw.z, 1.0f),
				XMVectorSet(movinggroundmesh[1].xyzw.x, movinggroundmesh[1].xyzw.y, movinggroundmesh[1].xyzw.z, 1.0f),
				XMVectorSet(movinggroundmesh[0].xyzw.x, movinggroundmesh[0].xyzw.y, movinggroundmesh[0].xyzw.z, 1.0f),
				distance);

			if (intersection)
			{
				collisionpickinglinelocal.r[3] = ((endpickinglinelocal.r[3] - startpickinglinelocal.r[3]) * distance) + startpickinglinelocal.r[3];
				collisionpickinglinerender = true;
			}
			else
				collisionpickinglinerender = false;

		}

	}


	//if (userinput.buttons['2'] && userinput.mouse_move)
	//{
		//XMVECTOR mouseposition = XMVectorSet(userinput.x, userinput.y, ZFAR, 1.0f);
		//XMVECTOR start = XMVector3Unproject(mouseposition, 0.0f, 0.0f, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, 0.0f, ZNEAR, cprojection, XMMatrixInverse(0, clocal), cworld);
		//XMVECTOR end = XMVector3Unproject(mouseposition, 0.0f, 0.0f, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, 0.0f, ZFAR, cprojection, XMMatrixInverse(0, clocal), cworld);
		//float distance = 0.0f;
	//	COLLISION_STATE intersection = RayIntersectsTriangles(mouseposition, end - start, redcubemesh, distance);
	//	if (intersection)
	//	{
	//		spherelocal.r[0] += XMVectorSet(abs(userinput.diffx) + abs(userinput.diffy), 0.0f, 0.0f, 1.0f) * time.Delta();
	//	}
	//}

#pragma endregion

	userinput.mouse_move = false;
}

void DEMO_APP::Update()
{

#pragma region physics calculations

	//if (!forceapplied)
	//{
	//	force = VECTOR3D(0.0f, 0.0f, 0.0f);
	//	forceapplied = true;
	//}
	//else
	//{
	//	force = 0.99f * force;
	//	if (abs(force.x) < 0.09f) { force.x = 0; }
	//	if (abs(force.y) < 0.09f) { force.y = 0; }
	//	if (abs(force.z) < 0.09f) { force.z = 0; }
	//	if (abs(linearacceleration.x) < 0.09f) { linearacceleration.x = 0; }
	//	if (abs(linearacceleration.y) < 0.09f) { linearacceleration.y = 0; }
	//	if (abs(linearacceleration.z) < 0.09f) { linearacceleration.z = 0; }
	//}
	//normalvelocity = (linearvelocity * VECTOR3D(XMVectorGetX(groundlocal.r[1]), XMVectorGetY(groundlocal.r[1]), XMVectorGetZ(groundlocal.r[1]))) * VECTOR3D(XMVectorGetX(groundlocal.r[1]), XMVectorGetY(groundlocal.r[1]), XMVectorGetZ(groundlocal.r[1]));
	//VECTOR3D pos = VECTOR3D(XMVectorGetX(spherelocal.r[3]), XMVectorGetY(spherelocal.r[3]), XMVectorGetZ(spherelocal.r[3]));

	//switch (CheckSpherePlaneCollision())
	//{
	//case NO_COLLISION:
	//{
	//	linearacceleration = (force + weight) / mass;
	//	linearvelocity += linearacceleration * (float)time.Delta();
	//	break;
	//}
	//case COLLISION:
	//{
	//	linearvelocity = -coefficientofrestitution * normalvelocity + (linearvelocity - normalvelocity);
	//	break;
	//}
	//case RESTING_CONTACT:
	//{
	//	break;
	//}
	//}
	//if (pos.y < radius)
	//	pos.y = radius;
	//pos += linearvelocity * (float)time.Delta();

	//spherelocal.r[3] = XMVectorSet(pos.x, pos.y, pos.z, 1.0f);

#pragma endregion

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
	TRANSFORM t;
	ZeroMemory(&t, sizeof(TRANSFORM));
	XMMATRIX m;
	ZeroMemory(&m, sizeof(XMMATRIX));

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

#pragma region cartesiancoordinates
	t.tworld = XMMatrixTranspose(XMMatrixIdentity());
	t.tlocal = XMMatrixTranspose(XMMatrixIdentity());
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &cartesiancoordinatesvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->Draw(cartesiancoordinatesmeshvertcount, 0);
#pragma endregion

#pragma region ground
	t.tworld = XMMatrixTranspose(groundworld);
	t.tlocal = XMMatrixTranspose(groundlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &groundvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(groundmeshvertcount, 0);
#pragma endregion

#pragma region moving ground
	t.tworld = XMMatrixTranspose(movinggroundworld);
	t.tlocal = XMMatrixTranspose(movinggroundlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &movinggroundvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(movinggroundmeshvertcount, 0);

	//cartesiancoordinates lines
	t.tworld = XMMatrixTranspose(movinggroundlocal);
	t.tlocal = XMMatrixTranspose(XMMatrixIdentity());
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &cartesiancoordinatesvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->Draw(cartesiancoordinatesmeshvertcount, 0);

	//cartesiancoordinates red cube
	m = {
		0.25f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.25f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.25f, 0.0f,
		cartesiancoordinatesmesh[1].xyzw.x, cartesiancoordinatesmesh[1].xyzw.y, cartesiancoordinatesmesh[1].xyzw.z, 1.0f
	};
	t.tworld = XMMatrixTranspose(movinggroundlocal);
	t.tlocal = XMMatrixTranspose(m);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &redcubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(cubemeshvertcount, 0);

	//cartesiancoordinates green cube
	m = {
		0.25f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.25f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.25f, 0.0f,
		cartesiancoordinatesmesh[3].xyzw.x, cartesiancoordinatesmesh[3].xyzw.y, cartesiancoordinatesmesh[3].xyzw.z, 1.0f
	};
	t.tworld = XMMatrixTranspose(movinggroundlocal);
	t.tlocal = XMMatrixTranspose(m);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &greencubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(cubemeshvertcount, 0);

	//cartesiancoordinates blue cube
	m = {
		0.25f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.25f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.25f, 0.0f,
		cartesiancoordinatesmesh[5].xyzw.x, cartesiancoordinatesmesh[5].xyzw.y, cartesiancoordinatesmesh[5].xyzw.z, 1.0f
	};
	t.tworld = XMMatrixTranspose(movinggroundlocal);
	t.tlocal = XMMatrixTranspose(m);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &bluecubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(cubemeshvertcount, 0);
#pragma endregion

#pragma region sphere
	//sphere
	t.tworld = XMMatrixTranspose(sphereworld);
	t.tlocal = XMMatrixTranspose(spherelocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &spherevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(spheremeshvertcount, 0);

	//cartesiancoordinates lines
	t.tworld = XMMatrixTranspose(spherelocal);
	t.tlocal = XMMatrixTranspose(XMMatrixIdentity());
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &cartesiancoordinatesvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->Draw(cartesiancoordinatesmeshvertcount, 0);

	//cartesiancoordinates red cube
	m = {
		0.25f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.25f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.25f, 0.0f,
		cartesiancoordinatesmesh[1].xyzw.x, cartesiancoordinatesmesh[1].xyzw.y, cartesiancoordinatesmesh[1].xyzw.z, 1.0f
	};
	t.tworld = XMMatrixTranspose(spherelocal);
	t.tlocal = XMMatrixTranspose(m);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &redcubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(cubemeshvertcount, 0);

	//cartesiancoordinates green cube
	m = {
		0.25f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.25f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.25f, 0.0f,
		cartesiancoordinatesmesh[3].xyzw.x, cartesiancoordinatesmesh[3].xyzw.y, cartesiancoordinatesmesh[3].xyzw.z, 1.0f
	};
	t.tworld = XMMatrixTranspose(spherelocal);
	t.tlocal = XMMatrixTranspose(m);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &greencubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(cubemeshvertcount, 0);

	//cartesiancoordinates blue cube
	m = {
		0.25f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.25f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.25f, 0.0f,
		cartesiancoordinatesmesh[5].xyzw.x, cartesiancoordinatesmesh[5].xyzw.y, cartesiancoordinatesmesh[5].xyzw.z, 1.0f
	};
	t.tworld = XMMatrixTranspose(spherelocal);
	t.tlocal = XMMatrixTranspose(m);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &bluecubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(cubemeshvertcount, 0);
#pragma endregion

#pragma region picking line segment

	//line
	context->Map(pickinglinevertbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(VERTEX) * pickinglinemeshvertcount, pickinglinemesh, sizeof(VERTEX) * pickinglinemeshvertcount);
	context->Unmap(pickinglinevertbuffer, 0);

	t.tworld = XMMatrixTranspose(pickinglineworld);
	t.tlocal = XMMatrixTranspose(pickinglineworld);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);
	context->IASetVertexBuffers(0, 1, &pickinglinevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->Draw(pickinglinemeshvertcount, 0);

	//start sphere
	if (pickinglinerender)
	{
		t.tworld = XMMatrixTranspose(pickinglineworld);
		t.tlocal = XMMatrixTranspose(XMMatrixScaling(0.2f, 0.2f, 0.2f) * startpickinglinelocal);
		context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
		context->Unmap(transformconstbuffer, 0);
		context->IASetVertexBuffers(0, 1, &spherevertbuffer, &stride, &offset);
		context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->Draw(spheremeshvertcount, 0);
		//end sphere
		t.tworld = XMMatrixTranspose(pickinglineworld);
		t.tlocal = XMMatrixTranspose(XMMatrixScaling(0.2f, 0.2f, 0.2f) * endpickinglinelocal);
		context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
		context->Unmap(transformconstbuffer, 0);
		context->IASetVertexBuffers(0, 1, &spherevertbuffer, &stride, &offset);
		context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->Draw(spheremeshvertcount, 0);
	}
	//collision sphere
	if (collisionpickinglinerender)
	{
		t.tworld = XMMatrixTranspose(pickinglineworld);
		t.tlocal = XMMatrixTranspose(XMMatrixScaling(0.2f, 0.2f, 0.2f) * collisionpickinglinelocal);
		context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy_s(msr.pData, sizeof(TRANSFORM), &t, sizeof(TRANSFORM));
		context->Unmap(transformconstbuffer, 0);
		context->IASetVertexBuffers(0, 1, &spherevertbuffer, &stride, &offset);
		context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->Draw(spheremeshvertcount, 0);
	}

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

	cameraconstbuffer->Release();
	transformconstbuffer->Release();

	groundvertbuffer->Release();
	delete groundmesh;

	movinggroundvertbuffer->Release();
	delete movinggroundmesh;

	cartesiancoordinatesvertbuffer->Release();
	delete cartesiancoordinatesmesh;

	spherevertbuffer->Release();

	redcubevertbuffer->Release();
	greencubevertbuffer->Release();
	bluecubevertbuffer->Release();

	pickinglinevertbuffer->Release();
	delete pickinglinemesh;

	UnregisterClass(L"DirectXApplication", application);
}

DEMO_APP::COLLISION_STATE DEMO_APP::CheckSpherePlaneCollision()
{
	//VECTOR3D centerpoint = VECTOR3D(XMVectorGetX(spherelocal.r[3]), XMVectorGetY(spherelocal.r[3]), XMVectorGetZ(spherelocal.r[3]));
	//VECTOR3D pointonplane = VECTOR3D(XMVectorGetX(groundlocal.r[3]), XMVectorGetY(groundlocal.r[3]), XMVectorGetZ(groundlocal.r[3]));
	//VECTOR3D planenormal = VECTOR3D(XMVectorGetX(groundlocal.r[1]), XMVectorGetY(groundlocal.r[1]), XMVectorGetZ(groundlocal.r[1]));
	//float vrn = linearvelocity * planenormal;
	//if (normalvelocity.Normal() <= 0.1f)
	//{
	//	normalvelocity = VECTOR3D(0.0f, 0.0f, 0.0f);
	//	vrn = 0.0f;
	//}
	//COLLISION_STATE collisionstate = NO_COLLISION;
	//if (vrn < 0.0f && abs(planenormal * (centerpoint - pointonplane)) <= radius)
	//	collisionstate = COLLISION;
	//else if (vrn == 0.0f && abs(planenormal * (centerpoint - pointonplane)) == radius)
	//	collisionstate = RESTING_CONTACT;
	//else if (vrn > 0.0f || abs(planenormal * (centerpoint - pointonplane)) > radius)
	//	collisionstate = NO_COLLISION;
	//return collisionstate;
	return NO_COLLISION;
}

DEMO_APP::COLLISION_STATE DEMO_APP::RayIntersectsTriangle(XMVECTOR Origin, XMVECTOR Direction, XMVECTOR V0, XMVECTOR V1, XMVECTOR V2, float& Dist)
{

	XMVECTOR e1 = XMVectorSubtract(V1, V0);
	XMVECTOR e2 = XMVectorSubtract(V2, V0);

	// p = Direction ^ e2;
	XMVECTOR p = XMVector3Cross(Direction, e2);

	// det = e1 * p;
	XMVECTOR det = XMVector3Dot(e1, p);

	XMVECTOR u, v, t;

	if (XMVector3GreaterOrEqual(det, g_RayEpsilon))
	{
		// Determinate is positive (front side of the triangle).
		XMVECTOR s = XMVectorSubtract(Origin, V0);

		// u = s * p;
		u = XMVector3Dot(s, p);

		XMVECTOR NoIntersection = XMVectorLess(u, XMVectorZero());
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(u, det));

		// q = s ^ e1;
		XMVECTOR q = XMVector3Cross(s, e1);

		// v = Direction * q;
		v = XMVector3Dot(Direction, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(v, XMVectorZero()));
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAdd(u, v), det));

		// t = e2 * q;
		t = XMVector3Dot(e2, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(t, XMVectorZero()));

		if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
		{
			Dist = 0.0f;
			//return false;
			return NO_COLLISION;
		}
	}
	else if (XMVector3LessOrEqual(det, g_RayNegEpsilon))
	{
		// Determinate is negative (back side of the triangle).
		XMVECTOR s = XMVectorSubtract(Origin, V0);

		// u = s * p;
		u = XMVector3Dot(s, p);

		XMVECTOR NoIntersection = XMVectorGreater(u, XMVectorZero());
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(u, det));

		// q = s ^ e1;
		XMVECTOR q = XMVector3Cross(s, e1);

		// v = Direction * q;
		v = XMVector3Dot(Direction, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(v, XMVectorZero()));
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(XMVectorAdd(u, v), det));

		// t = e2 * q;
		t = XMVector3Dot(e2, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(t, XMVectorZero()));

		if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
		{
			Dist = 0.0f;
			//return false;
			return NO_COLLISION;
		}
	}
	else
	{
		// Parallel ray.
		Dist = 0.0f;
		//return false;
		return NO_COLLISION;
	}

	t = XMVectorDivide(t, det);

	// (u / det) and (v / dev) are the barycentric cooridinates of the intersection.

	// Store the x-component to *pDist
	XMStoreFloat(&Dist, t);

	//return true;
	return COLLISION;

}

DEMO_APP::COLLISION_STATE DEMO_APP::RayIntersectsTriangles(XMVECTOR Origin, XMVECTOR Direction, vector<VERTEX> &mesh, float &Dist)
{

	float distance = (float)INT32_MAX;
	COLLISION_STATE intersection = NO_COLLISION;
	for (size_t index = 0; index < mesh.size() / 3; index += 3)
	{
		XMVECTOR V0 = XMVectorSet(mesh[index].xyzw.x, mesh[index].xyzw.y, mesh[index].xyzw.z, 1.0f);
		XMVECTOR V1 = XMVectorSet(mesh[index + 1].xyzw.x, mesh[index + 1].xyzw.y, mesh[index + 1].xyzw.z, 1.0f);
		XMVECTOR V2 = XMVectorSet(mesh[index + 1].xyzw.x, mesh[index + 1].xyzw.y, mesh[index + 1].xyzw.z, 1.0f);
		float d = 0.0f;
		if (RayIntersectsTriangle(Origin, Direction, V0, V1, V2, d) == COLLISION && distance > d)
		{
			intersection = COLLISION;
			distance = d;
		}
	}
	return intersection;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
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
		myApp.Update();
		myApp.Render();
	}

	myApp.ShutDown();
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case (WM_DESTROY):
	{
		PostQuitMessage(0);
		break;
	};
	case (WM_KEYDOWN):
	{
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		if (wParam)
			userinput.buttons[wParam] = true;
		break;
	};
	case (WM_KEYUP):
	{
		if (wParam)
			userinput.buttons[wParam] = false;
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