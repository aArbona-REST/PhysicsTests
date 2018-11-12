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

#define BACKBUFFER_WIDTH	500.0f
#define BACKBUFFER_HEIGHT	500.0f
#define RS_HEIGHT BACKBUFFER_WIDTH
#define RS_WIDTH BACKBUFFER_HEIGHT
#define ASPECT_RATIO ((float)(RS_HEIGHT)/(float)(RS_WIDTH))
#define FOV 45.0f
#define ZNEAR 0.01f
#define ZFAR 10000.0f

UserInput                       userinput;

class DEMO_APP
{
	struct VERTEX { XMFLOAT4 xyzw; XMFLOAT4 color; XMFLOAT2 uv; XMFLOAT4 normal; };
	struct CAMERA { XMMATRIX cworld; XMMATRIX clocal; XMMATRIX cprojection; };
	struct TRANSFORM { XMMATRIX tworld; XMMATRIX tlocal; };

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

	ID3D11VertexShader             *vertexshader = nullptr;
	ID3D11PixelShader              *pixelshader = nullptr;

	ID3D11Buffer                   *objvertbuffer = nullptr;
	unsigned int                    objvertcount;

	ID3D11Buffer                   *cartesiancoordinatesvertbuffer = nullptr;
	ID3D11Buffer                   *xycirclevertbuffer = nullptr;
	ID3D11Buffer                   *xzcirclevertbuffer = nullptr;
	ID3D11Buffer                   *yzcirclevertbuffer = nullptr;
	ID3D11Buffer                   *cubevertbuffer = nullptr;
	ID3D11Buffer                   *cameraconstbuffer = nullptr;
	ID3D11Buffer                   *transformconstbuffer = nullptr;

	XMMATRIX cworld, clocal, cprojection;
	XMMATRIX tworld, tlocal;

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
	device->CreatePixelShader(PixelShader, sizeof(PixelShader), NULL, &pixelshader);
	context->VSSetShader(vertexshader, NULL, NULL);
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

#pragma region sphere vert data and vert buffer

	vector<VERTEX> objverts;
	LoadOBJ("pyramid.obj", objverts);
	objvertcount = objverts.size();

	D3D11_BUFFER_DESC objvertbufferdescription;
	ZeroMemory(&objvertbufferdescription, sizeof(D3D11_BUFFER_DESC));
	objvertbufferdescription.Usage = D3D11_USAGE_DEFAULT;
	objvertbufferdescription.ByteWidth = sizeof(VERTEX) * objvertcount;
	objvertbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	objvertbufferdescription.CPUAccessFlags = 0;
	objvertbufferdescription.MiscFlags = NULL;
	objvertbufferdescription.StructureByteStride = sizeof(VERTEX);
	D3D11_SUBRESOURCE_DATA objvertinitdata;
	ZeroMemory(&objvertinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	objvertinitdata.pSysMem = objverts.data();
	device->CreateBuffer(&objvertbufferdescription, &objvertinitdata, &objvertbuffer);

#pragma endregion

#pragma region cartesiancoordinates vert data and vert buffer
	VERTEX xyz[6]{};
	xyz[0].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	xyz[0].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	xyz[1].xyzw = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	xyz[1].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	xyz[2].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	xyz[2].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	xyz[3].xyzw = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	xyz[3].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	xyz[4].xyzw = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	xyz[4].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	xyz[5].xyzw = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	xyz[5].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	D3D11_BUFFER_DESC cartesiancoordinatesvertbufferdescription;
	ZeroMemory(&cartesiancoordinatesvertbufferdescription, sizeof(D3D11_BUFFER_DESC));
	cartesiancoordinatesvertbufferdescription.Usage = D3D11_USAGE_DEFAULT;
	cartesiancoordinatesvertbufferdescription.ByteWidth = sizeof(VERTEX) * 6;
	cartesiancoordinatesvertbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	cartesiancoordinatesvertbufferdescription.CPUAccessFlags = 0;
	cartesiancoordinatesvertbufferdescription.MiscFlags = NULL;
	cartesiancoordinatesvertbufferdescription.StructureByteStride = sizeof(VERTEX);
	D3D11_SUBRESOURCE_DATA cartesiancoordinatesinitdata;
	ZeroMemory(&cartesiancoordinatesinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	cartesiancoordinatesinitdata.pSysMem = xyz;
	device->CreateBuffer(&cartesiancoordinatesvertbufferdescription, &cartesiancoordinatesinitdata, &cartesiancoordinatesvertbuffer);
#pragma endregion

#pragma region xycircle vert data and vert buffer
	VERTEX xycircle[366]{};
	for (size_t i = 0; i < 366; i++)
	{
		xycircle[i].xyzw = XMFLOAT4((sin((i * (3.14f / 180))))*1.0f, (cos((i * (3.14f / 180))))*1.0f, 0.0f, 1.0f);
		xycircle[i].color = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	}
	D3D11_BUFFER_DESC xycirclevertbufferdescription;
	ZeroMemory(&xycirclevertbufferdescription, sizeof(D3D11_BUFFER_DESC));
	xycirclevertbufferdescription.Usage = D3D11_USAGE_DEFAULT;
	xycirclevertbufferdescription.ByteWidth = sizeof(VERTEX) * 366;
	xycirclevertbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	xycirclevertbufferdescription.CPUAccessFlags = 0;
	xycirclevertbufferdescription.MiscFlags = NULL;
	xycirclevertbufferdescription.StructureByteStride = sizeof(VERTEX);
	D3D11_SUBRESOURCE_DATA xycircleinitdata;
	ZeroMemory(&xycircleinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	xycircleinitdata.pSysMem = xycircle;
	device->CreateBuffer(&xycirclevertbufferdescription, &xycircleinitdata, &xycirclevertbuffer);
#pragma endregion

#pragma region xzcircle vert data and vert buffer
	VERTEX xzcircle[366]{};
	for (size_t i = 0; i < 366; i++)
	{
		xzcircle[i].xyzw = XMFLOAT4((sin((i * (3.14f / 180))))*1.0f, 0.0f, (cos((i * (3.14f / 180))))*1.0f, 1.0f);
		xzcircle[i].color = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);
	}
	D3D11_BUFFER_DESC xzcirclevertbufferdescription;
	ZeroMemory(&xzcirclevertbufferdescription, sizeof(D3D11_BUFFER_DESC));
	xzcirclevertbufferdescription.Usage = D3D11_USAGE_DEFAULT;
	xzcirclevertbufferdescription.ByteWidth = sizeof(VERTEX) * 366;
	xzcirclevertbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	xzcirclevertbufferdescription.CPUAccessFlags = 0;
	xzcirclevertbufferdescription.MiscFlags = NULL;
	xzcirclevertbufferdescription.StructureByteStride = sizeof(VERTEX);
	D3D11_SUBRESOURCE_DATA xzcircleinitdata;
	ZeroMemory(&xzcircleinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	xzcircleinitdata.pSysMem = xzcircle;
	device->CreateBuffer(&xzcirclevertbufferdescription, &xzcircleinitdata, &xzcirclevertbuffer);
#pragma endregion

#pragma region yzcircle vert data and vert buffer
	VERTEX yzcircle[366]{};
	for (size_t i = 0; i < 366; i++)
	{
		yzcircle[i].xyzw = XMFLOAT4(0.0f, (sin((i * (3.14f / 180))))*1.0f, (cos((i * (3.14f / 180))))*1.0f, 1.0f);
		yzcircle[i].color = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);
	}
	D3D11_BUFFER_DESC yzcirclevertbufferdescription;
	ZeroMemory(&yzcirclevertbufferdescription, sizeof(D3D11_BUFFER_DESC));
	yzcirclevertbufferdescription.Usage = D3D11_USAGE_DEFAULT;
	yzcirclevertbufferdescription.ByteWidth = sizeof(VERTEX) * 366;
	yzcirclevertbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	yzcirclevertbufferdescription.CPUAccessFlags = 0;
	yzcirclevertbufferdescription.MiscFlags = NULL;
	yzcirclevertbufferdescription.StructureByteStride = sizeof(VERTEX);
	D3D11_SUBRESOURCE_DATA yzcircleinitdata;
	ZeroMemory(&yzcircleinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	yzcircleinitdata.pSysMem = yzcircle;
	device->CreateBuffer(&yzcirclevertbufferdescription, &yzcircleinitdata, &yzcirclevertbuffer);
#pragma endregion

#pragma region cube vert data and vert buffer

	unsigned int index = 0;

	VERTEX cube[36]{};

	for (size_t i = 0; i < 36; i++)
		cube[i].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, +0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, -0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, -0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, -0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, +0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, +0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, +0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, +0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, +0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(-0.25f, +0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(-0.25f, -0.25f, +0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, +0.25f, 1.0f);

	cube[index++].xyzw = XMFLOAT4(+0.25f, +0.25f, -0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, +0.25f, 1.0f);
	cube[index++].xyzw = XMFLOAT4(+0.25f, -0.25f, -0.25f, 1.0f);


	D3D11_BUFFER_DESC cubevertbufferdescription;
	ZeroMemory(&cubevertbufferdescription, sizeof(D3D11_BUFFER_DESC));
	cubevertbufferdescription.Usage = D3D11_USAGE_DEFAULT;
	cubevertbufferdescription.ByteWidth = sizeof(VERTEX) * 36;
	cubevertbufferdescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	cubevertbufferdescription.CPUAccessFlags = 0;
	cubevertbufferdescription.MiscFlags = NULL;
	cubevertbufferdescription.StructureByteStride = sizeof(VERTEX);
	D3D11_SUBRESOURCE_DATA cubeinitdata;
	ZeroMemory(&cubeinitdata, sizeof(D3D11_SUBRESOURCE_DATA));
	cubeinitdata.pSysMem = &cube;
	device->CreateBuffer(&cubevertbufferdescription, &cubeinitdata, &cubevertbuffer);
#pragma endregion

#pragma region camera constbuffer

	CAMERA camera;
	ZeroMemory(&camera, sizeof(CAMERA));
	cworld = XMMatrixIdentity();
	clocal = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -5.0f, 1.0f
	};
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
	ZeroMemory(&camerainitdata, sizeof(cubeinitdata));
	camerainitdata.pSysMem = &camera;
	device->CreateBuffer(&cameraconstbufferdescription, &camerainitdata, &cameraconstbuffer);
#pragma endregion

#pragma region transform constbuffer
	TRANSFORM transform;
	ZeroMemory(&transform, sizeof(TRANSFORM));
	tworld = XMMatrixIdentity();
	tlocal = XMMatrixIdentity();
	transform.tworld = XMMatrixTranspose(tworld);
	transform.tlocal = XMMatrixTranspose(tlocal);

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
	transforminitdata.pSysMem = &camera;
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
		vertex.color = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f);
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
	if (userinput.mouse_move && userinput.left_click)
	{
		XMVECTOR pos = newcamera.r[3];
		XMFLOAT4 zero = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		newcamera.r[3] = XMLoadFloat4(&zero);
		newcamera = XMMatrixRotationX(userinput.diffy * (float)time.Delta() * 10.0f) * newcamera * XMMatrixRotationY(userinput.diffx * (float)time.Delta() * 10.0f);
		newcamera.r[3] = pos;
	}
	userinput.mouse_move = false;
#pragma endregion

	clocal = newcamera;

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

#pragma region obj file 
	TRANSFORM tobjfile;
	ZeroMemory(&tobjfile, sizeof(TRANSFORM));
	tworld = XMMatrixIdentity();
	tlocal = XMMatrixIdentity();
	tobjfile.tworld = XMMatrixTranspose(tworld);
	tobjfile.tlocal = XMMatrixTranspose(tlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &tobjfile, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);

	context->IASetVertexBuffers(0, 1, &objvertbuffer, &stride, &offset);

	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(objvertcount, 0);
#pragma endregion

#pragma region cartesiancoordinates
	TRANSFORM tcartesiancoordinates;
	ZeroMemory(&tcartesiancoordinates, sizeof(TRANSFORM));
	tworld = XMMatrixIdentity();
	tlocal = XMMatrixIdentity();
	tcartesiancoordinates.tworld = XMMatrixTranspose(tworld);
	tcartesiancoordinates.tlocal = XMMatrixTranspose(tlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &tcartesiancoordinates, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);

	context->IASetVertexBuffers(0, 1, &cartesiancoordinatesvertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	context->Draw(6, 0);
#pragma endregion

#pragma region xycircle
	TRANSFORM txycircle;
	ZeroMemory(&txycircle, sizeof(TRANSFORM));
	tworld = XMMatrixIdentity();
	tlocal = XMMatrixIdentity();
	txycircle.tworld = XMMatrixTranspose(tworld);
	txycircle.tlocal = XMMatrixTranspose(tlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &txycircle, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);

	context->IASetVertexBuffers(0, 1, &xycirclevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	context->Draw(366, 0);
#pragma endregion

#pragma region xzcircle
	TRANSFORM txzcircle;
	ZeroMemory(&txzcircle, sizeof(TRANSFORM));
	tworld = XMMatrixIdentity();
	tlocal = XMMatrixIdentity();
	txzcircle.tworld = XMMatrixTranspose(tworld);
	txzcircle.tlocal = XMMatrixTranspose(tlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &txzcircle, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);

	context->IASetVertexBuffers(0, 1, &xzcirclevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	context->Draw(366, 0);
#pragma endregion

#pragma region yzcircle
	TRANSFORM tyzcircle;
	ZeroMemory(&tyzcircle, sizeof(TRANSFORM));
	tworld = XMMatrixIdentity();
	tlocal = XMMatrixIdentity();
	tyzcircle.tworld = XMMatrixTranspose(tworld);
	tyzcircle.tlocal = XMMatrixTranspose(tlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &tyzcircle, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);

	context->IASetVertexBuffers(0, 1, &yzcirclevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	context->Draw(366, 0);
#pragma endregion

#pragma region xzmoon
	TRANSFORM txzmoon;
	ZeroMemory(&txzmoon, sizeof(TRANSFORM));
	XMMATRIX mtxzOrbit = XMMatrixRotationY((float)time.TotalTimeExact());
	XMMATRIX mtxzTranslate = XMMatrixTranslation(1.0f, 0.0f, 0.0f);
	XMMATRIX mtxzScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	tlocal = mtxzScale * mtxzTranslate * mtxzOrbit;
	txzmoon.tworld = XMMatrixTranspose(tworld);
	txzmoon.tlocal = XMMatrixTranspose(tlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &txzmoon, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);

	context->IASetVertexBuffers(0, 1, &cubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(36, 0);
#pragma endregion

#pragma region xymoon
	TRANSFORM txymoon;
	ZeroMemory(&txymoon, sizeof(TRANSFORM));
	XMMATRIX mtxyOrbit = XMMatrixRotationZ((float)time.TotalTimeExact());
	XMMATRIX mtxyTranslate = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	XMMATRIX mtxyScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	tlocal = mtxyScale * mtxyTranslate * mtxyOrbit;
	txymoon.tworld = XMMatrixTranspose(tworld);
	txymoon.tlocal = XMMatrixTranspose(tlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &txymoon, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);

	context->IASetVertexBuffers(0, 1, &cubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(36, 0);
#pragma endregion

#pragma region yzmoon
	TRANSFORM tyzmoon;
	ZeroMemory(&tyzmoon, sizeof(TRANSFORM));
	XMMATRIX mtyzOrbit = XMMatrixRotationX((float)time.TotalTimeExact());
	XMMATRIX mtyzTranslate = XMMatrixTranslation(0.0f, 0.0f, 1.0f);
	XMMATRIX mtyzScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	tlocal = mtyzScale * mtyzTranslate * mtyzOrbit;
	tyzmoon.tworld = XMMatrixTranspose(tworld);
	tyzmoon.tlocal = XMMatrixTranspose(tlocal);
	context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy_s(msr.pData, sizeof(TRANSFORM), &tyzmoon, sizeof(TRANSFORM));
	context->Unmap(transformconstbuffer, 0);

	context->IASetVertexBuffers(0, 1, &cubevertbuffer, &stride, &offset);
	context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(36, 0);
#pragma endregion

#pragma region earth
	//TRANSFORM tearth;
	//ZeroMemory(&tearth, sizeof(TRANSFORM));
	//tlocal = XMMatrixRotationY((float)time.TotalTimeExact());
	//tearth.tworld = XMMatrixTranspose(tworld);
	//tearth.tlocal = XMMatrixTranspose(tlocal);
	//context->Map(transformconstbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	//memcpy_s(msr.pData, sizeof(TRANSFORM), &tearth, sizeof(TRANSFORM));
	//context->Unmap(transformconstbuffer, 0);

	//context->IASetVertexBuffers(0, 1, &cubevertbuffer, &stride, &offset);
	//context->VSSetConstantBuffers(1, 1, &transformconstbuffer);
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//context->Draw(36, 0);
#pragma endregion

	swapchain->Present(0, 0);

}

void DEMO_APP::ShutDown()
{

	//this is missing ID311Buffer->Release();
	device->Release();
	context->Release();
	rtv->Release();
	swapchain->Release();
	UnregisterClass(L"DirectXApplication", application);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
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