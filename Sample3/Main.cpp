/*
* Joseph Despain
* This is a simple only diffuse hlsl shader.
* I am using the DXUT example as a base.
*/


#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include <WinBase.h>


#pragma warning( disable : 4100 )

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CModelViewerCamera          g_Camera;               // A model viewing camera
CDXUTDirectionWidget        g_LightControl;
CD3DSettingsDlg             g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                 g_HUD;                  // manages the 3D   
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
XMMATRIX                    g_mModelMatrix;



CDXUTSDKMesh                g_TeapotMesh;

ID3D11InputLayout*          g_pVertexLayout = nullptr;
ID3D11VertexShader*         g_pVertexShader = nullptr;
ID3D11PixelShader*          g_pPixelShader = nullptr;
ID3D11VertexShader*         g_pVertexShader1 = nullptr;
ID3D11PixelShader*          g_pPixelShader1 = nullptr;
ID3D11SamplerState*         g_pSamLinear = nullptr;

ID3D11Texture2D*			g_pRenderTexture = nullptr;
ID3D11RenderTargetView*		g_pRenderTargetView = nullptr;
ID3D11ShaderResourceView*	g_pShaderResouceView = nullptr;

struct CB_VS_PER_OBJECT
{
	XMFLOAT4X4 m_WorldViewProj;
	XMFLOAT4X4 m_World;
};
UINT                        g_iCBVSPerObjectBind = 0;

struct CB_PS_PER_OBJECT
{
	XMFLOAT4 m_vObjectColor;
};
UINT                        g_iCBPSPerObjectBind = 0;

struct CB_PS_PER_FRAME
{
	XMFLOAT4 m_vLightDir;
	XMFLOAT4 m_vViewDir;
};
UINT                        g_iCBPSPerFrameBind = 1;

ID3D11Buffer*               g_pcbVSPerObject = nullptr;
ID3D11Buffer*               g_pcbPSPerObject = nullptr;
ID3D11Buffer*               g_pcbPSPerFrame = nullptr;

#define IDC_TOGGLEFULLSCREEN    1

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext);
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext);
void CALLBACK OnD3D11DestroyDevice(void* pUserContext);
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext);

void InitApp();

void DrawTeapot(ID3D11DeviceContext* pd3dImmediateContext);


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Set DXUT callbacks
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackFrameMove(OnFrameMove);

	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	InitApp();
	DXUTInit(true, true, nullptr); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
	DXUTCreateWindow(L"Simple Diffuse");
	DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, 800, 600);
	DXUTMainLoop(); // Enter into the DXUT render loop

	return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	static const XMVECTORF32 s_vLightDir = { -1000.f, 1, -1.f, 0.f };
	XMVECTOR vLightDir = XMVector3Normalize(s_vLightDir);
	g_LightControl.SetLightDirection(vLightDir);

	// Initialize dialogs
	g_D3DSettingsDlg.Init(&g_DialogResourceManager);
	g_HUD.Init(&g_DialogResourceManager);
	g_SampleUI.Init(&g_DialogResourceManager);

	g_HUD.SetCallback(OnGUIEvent); int iY = 10;
	g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 23);

	g_SampleUI.SetCallback(OnGUIEvent); iY = 10;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	// Update the camera's position based on user input 
	g_Camera.FrameMove(fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
	void* pUserContext)
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	// Pass messages to settings dialog if its active
	if (g_D3DSettingsDlg.IsActive())
	{
		g_D3DSettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;
	*pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	g_LightControl.HandleMessages(hWnd, uMsg, wParam, lParam);

	// Pass all remaining windows messages to camera so it can respond to user input
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	switch (nControlID)
	{
	case IDC_TOGGLEFULLSCREEN:
		DXUTToggleFullScreen();
		break;
	}

}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;

	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN(g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	V_RETURN(g_D3DSettingsDlg.OnD3D11CreateDevice(pd3dDevice));


	XMFLOAT3 vCenter(0.0f, 0.0f, 0.0f);

	g_mModelMatrix = XMMatrixTranslation(-vCenter.x, -vCenter.y, -vCenter.z);
	g_mModelMatrix = XMMatrixScaling(10.0f, 10.0f, 10.0f);


	// Best practice would be to compile these offline as part of the build, but it's more convienent for experimentation to compile at runtime.
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pVertexShaderBuffer = nullptr;
	V_RETURN(DXUTCompileFromFile(L"VertexShader.hlsl", nullptr, "main", "vs_5_0", dwShaderFlags, 0, &pVertexShaderBuffer));

	ID3DBlob* pPixelShaderBuffer = nullptr;
	V_RETURN(DXUTCompileFromFile(L"PixelShader.hlsl", nullptr, "main", "ps_5_0", dwShaderFlags, 0, &pPixelShaderBuffer));

	// Create the shaders
	V_RETURN(pd3dDevice->CreateVertexShader(pVertexShaderBuffer->GetBufferPointer(), pVertexShaderBuffer->GetBufferSize(), nullptr, &g_pVertexShader));
	DXUT_SetDebugName(g_pVertexShader, "VSMain");

	V_RETURN(pd3dDevice->CreatePixelShader(pPixelShaderBuffer->GetBufferPointer(), pPixelShaderBuffer->GetBufferSize(), nullptr, &g_pPixelShader));
	DXUT_SetDebugName(g_pPixelShader, "PSMain");


	ID3DBlob* pVertexShaderBuffer1 = nullptr;
	V_RETURN(DXUTCompileFromFile(L"VertexShader1.hlsl", nullptr, "main", "vs_5_0", dwShaderFlags, 0, &pVertexShaderBuffer1));

	ID3DBlob* pPixelShaderBuffer1 = nullptr;
	V_RETURN(DXUTCompileFromFile(L"PixelShader1.hlsl", nullptr, "main", "ps_5_0", dwShaderFlags, 0, &pPixelShaderBuffer1));

	// Create the shaders
	V_RETURN(pd3dDevice->CreateVertexShader(pVertexShaderBuffer1->GetBufferPointer(), pVertexShaderBuffer1->GetBufferSize(), nullptr, &g_pVertexShader1));
	DXUT_SetDebugName(g_pVertexShader1, "VSMain1");

	V_RETURN(pd3dDevice->CreatePixelShader(pPixelShaderBuffer1->GetBufferPointer(), pPixelShaderBuffer1->GetBufferSize(), nullptr, &g_pPixelShader1));
	DXUT_SetDebugName(g_pPixelShader1, "PSMain1");


	// Create our vertex input layout
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	V_RETURN(pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pVertexShaderBuffer->GetBufferPointer(), pVertexShaderBuffer->GetBufferSize(), &g_pVertexLayout));
	DXUT_SetDebugName(g_pVertexLayout, "Primary");

	SAFE_RELEASE(pVertexShaderBuffer);
	SAFE_RELEASE(pPixelShaderBuffer);
	SAFE_RELEASE(pVertexShaderBuffer1);
	SAFE_RELEASE(pPixelShaderBuffer1);

	// Load the mesh
	V_RETURN(g_TeapotMesh.Create(pd3dDevice, L"teapot\\wt_teapot.sdkmesh"));

	// Create a sampler state
	// I don't need to sample anything.
	

	D3D11_SAMPLER_DESC SamDesc;
	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.MipLODBias = 0.0f;
	SamDesc.MaxAnisotropy = 1;
	SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
	SamDesc.MinLOD = 0;
	SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &g_pSamLinear));
	DXUT_SetDebugName(g_pSamLinear, "Primary");
	

	// Setup constant buffers
	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;

	Desc.ByteWidth = sizeof(CB_VS_PER_OBJECT);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &g_pcbVSPerObject));
	DXUT_SetDebugName(g_pcbVSPerObject, "CB_VS_PER_OBJECT");

	Desc.ByteWidth = sizeof(CB_PS_PER_OBJECT);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &g_pcbPSPerObject));
	DXUT_SetDebugName(g_pcbPSPerObject, "CB_PS_PER_OBJECT");

	Desc.ByteWidth = sizeof(CB_PS_PER_FRAME);
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, nullptr, &g_pcbPSPerFrame));
	DXUT_SetDebugName(g_pcbPSPerFrame, "CB_PS_PER_FRAME");

	// Setup the camera's view parameters
	static const XMVECTORF32 s_vecEye = { 0.0f, 0.0f, -100.0f, 0.0f };
	g_Camera.SetViewParams(s_vecEye, g_XMZero);
	//g_Camera.SetRadius(fObjectRadius * 3.0f, fObjectRadius * 0.5f, fObjectRadius * 10.0f);



	D3D11_TEXTURE2D_DESC TexDesc;
	TexDesc.Usage = D3D11_USAGE_DEFAULT;
	TexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	TexDesc.ArraySize = 1;
	TexDesc.CPUAccessFlags = 0;
	TexDesc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
	TexDesc.Width = DXUTGetWindowWidth();
	TexDesc.Height = DXUTGetWindowHeight();
	TexDesc.MipLevels = 1;
	TexDesc.MiscFlags = 0;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	V_RETURN(pd3dDevice->CreateTexture2D(&TexDesc, 0, &g_pRenderTexture));

	D3D11_RENDER_TARGET_VIEW_DESC renderDesc;
	ZeroMemory(&renderDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	renderDesc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
	renderDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderDesc.Texture2D.MipSlice = 0;
	V_RETURN(pd3dDevice->CreateRenderTargetView(g_pRenderTexture, &renderDesc, &g_pRenderTargetView));

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResDesc;
	shaderResDesc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
	shaderResDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResDesc.Texture2D.MostDetailedMip = 0;
	shaderResDesc.Texture2D.MipLevels = 1;

	V_RETURN(pd3dDevice->CreateShaderResourceView(g_pRenderTexture, &shaderResDesc, &g_pShaderResouceView));



	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;

	V_RETURN(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	V_RETURN(g_D3DSettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(XM_PI / 4, fAspectRatio, 2.0f, 4000.0f);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_Camera.SetButtonMasks(MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON);

	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	g_HUD.SetSize(170, 170);
	g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300);
	g_SampleUI.SetSize(170, 300);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
	float fElapsedTime, void* pUserContext)
{
	HRESULT hr;

	// If the settings dialog is being shown, then render it instead of rendering the app's scene
	if (g_D3DSettingsDlg.IsActive())
	{
		g_D3DSettingsDlg.OnRender(fElapsedTime);
		return;
	}

	auto pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, pDSV);

	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
	pd3dImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::Red);

	// Get the light direction
	// Times a 1000 because our teapot is pretty big.
	XMVECTOR vLightDir = 10000 * g_LightControl.GetLightDirection();

	// Per frame cb update
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V(pd3dImmediateContext->Map(g_pcbPSPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	auto pPerFrame = reinterpret_cast<CB_PS_PER_FRAME*>(MappedResource.pData);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&pPerFrame->m_vLightDir), vLightDir);
	XMVECTOR eyePos = g_Camera.GetEyePt();
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&pPerFrame->m_vViewDir), eyePos);
	pd3dImmediateContext->Unmap(g_pcbPSPerFrame, 0);

	pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPerFrameBind, 1, &g_pcbPSPerFrame);

	//Get the mesh
	//IA setup
	pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);
	UINT Strides[1];
	UINT Offsets[1];
	ID3D11Buffer* pVB[1];
	pVB[0] = g_TeapotMesh.GetVB11(0, 0);
	Strides[0] = (UINT)g_TeapotMesh.GetVertexStride(0, 0);
	Offsets[0] = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
	pd3dImmediateContext->IASetIndexBuffer(g_TeapotMesh.GetIB11(0), g_TeapotMesh.GetIBFormat11(0), 0);

	// Set the shaders
	pd3dImmediateContext->VSSetShader(g_pVertexShader1, nullptr, 0);
	pd3dImmediateContext->PSSetShader(g_pPixelShader1, nullptr, 0);


	// Get the projection & view matrix from the camera class
	XMMATRIX mProj = g_Camera.GetProjMatrix();
	XMMATRIX mView = g_Camera.GetViewMatrix();


	// Set the per object constant data
	XMMATRIX mWorld = g_mModelMatrix * g_Camera.GetWorldMatrix();
	XMMATRIX mWorldViewProjection = mWorld * mView * mProj;

	// VS Per object
	V(pd3dImmediateContext->Map(g_pcbVSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	auto pVSPerObject = reinterpret_cast<CB_VS_PER_OBJECT*>(MappedResource.pData);
	XMMATRIX mt = XMMatrixTranspose(mWorldViewProjection);
	XMStoreFloat4x4(&pVSPerObject->m_WorldViewProj, mt);
	mt = XMMatrixTranspose(mWorld);
	XMStoreFloat4x4(&pVSPerObject->m_World, mt);
	pd3dImmediateContext->Unmap(g_pcbVSPerObject, 0);

	pd3dImmediateContext->VSSetConstantBuffers(g_iCBVSPerObjectBind, 1, &g_pcbVSPerObject);

	// PS Per object
	V(pd3dImmediateContext->Map(g_pcbPSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	auto pPSPerObject = reinterpret_cast<CB_PS_PER_OBJECT*>(MappedResource.pData);
	XMStoreFloat4(&pPSPerObject->m_vObjectColor, Colors::White);
	pd3dImmediateContext->Unmap(g_pcbPSPerObject, 0);

	pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPerObjectBind, 1, &g_pcbPSPerObject);

	//Render
	pd3dImmediateContext->PSSetSamplers(0, 1, &g_pSamLinear);


	DrawTeapot(pd3dImmediateContext);


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Clear the render target and depth stencil
	auto pRTV = DXUTGetD3D11RenderTargetView();
	pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, DXUTGetD3D11DepthStencilView());
	pd3dImmediateContext->ClearRenderTargetView(pRTV, Colors::MidnightBlue);
	pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	// Get the light direction
	// Times a 1000 because our teapot is pretty big.
	vLightDir = 10000 * g_LightControl.GetLightDirection();

	// Per frame cb update
	MappedResource;
	V(pd3dImmediateContext->Map(g_pcbPSPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	pPerFrame = reinterpret_cast<CB_PS_PER_FRAME*>(MappedResource.pData);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&pPerFrame->m_vLightDir), vLightDir);
	eyePos = g_Camera.GetEyePt();
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&pPerFrame->m_vViewDir), eyePos);
	pd3dImmediateContext->Unmap(g_pcbPSPerFrame, 0);

	pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPerFrameBind, 1, &g_pcbPSPerFrame);

	//Get the mesh
	//IA setup
	pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);
	Strides[1];
	Offsets[1];
	pVB[1];
	pVB[0] = g_TeapotMesh.GetVB11(0, 0);
	Strides[0] = (UINT)g_TeapotMesh.GetVertexStride(0, 0);
	Offsets[0] = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
	pd3dImmediateContext->IASetIndexBuffer(g_TeapotMesh.GetIB11(0), g_TeapotMesh.GetIBFormat11(0), 0);

	// Set the shaders
	pd3dImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	pd3dImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

	pd3dImmediateContext->PSSetShaderResources(1, 1, &g_pShaderResouceView);

	// Get the projection & view matrix from the camera class
	mProj = g_Camera.GetProjMatrix();
	mView = g_Camera.GetViewMatrix();


	// Set the per object constant data
	mWorld = g_mModelMatrix * g_Camera.GetWorldMatrix();
	mWorldViewProjection = mWorld * mView * mProj;

	// VS Per object
	V(pd3dImmediateContext->Map(g_pcbVSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	pVSPerObject = reinterpret_cast<CB_VS_PER_OBJECT*>(MappedResource.pData);
	mt = XMMatrixTranspose(mWorldViewProjection);
	XMStoreFloat4x4(&pVSPerObject->m_WorldViewProj, mt);
	mt = XMMatrixTranspose(mWorld);
	XMStoreFloat4x4(&pVSPerObject->m_World, mt);
	pd3dImmediateContext->Unmap(g_pcbVSPerObject, 0);

	pd3dImmediateContext->VSSetConstantBuffers(g_iCBVSPerObjectBind, 1, &g_pcbVSPerObject);

	// PS Per object
	V(pd3dImmediateContext->Map(g_pcbPSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	pPSPerObject = reinterpret_cast<CB_PS_PER_OBJECT*>(MappedResource.pData);
	XMStoreFloat4(&pPSPerObject->m_vObjectColor, Colors::White);
	pd3dImmediateContext->Unmap(g_pcbPSPerObject, 0);

	pd3dImmediateContext->PSSetConstantBuffers(g_iCBPSPerObjectBind, 1, &g_pcbPSPerObject);

	//Render
	pd3dImmediateContext->PSSetSamplers(0, 1, &g_pSamLinear);

	DrawTeapot(pd3dImmediateContext);

	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
	g_HUD.OnRender(fElapsedTime);
	DXUT_EndPerfEvent();

	
}

void DrawTeapot(ID3D11DeviceContext* pd3dImmediateContext)
{
	for (UINT subset = 0; subset < g_TeapotMesh.GetNumSubsets(0); ++subset)
	{
		// Get the subset
		auto pSubset = g_TeapotMesh.GetSubset(0, subset);

		auto PrimType = CDXUTSDKMesh::GetPrimitiveType11((SDKMESH_PRIMITIVE_TYPE)pSubset->PrimitiveType);
		pd3dImmediateContext->IASetPrimitiveTopology(PrimType);

		// Ignores most of the material information in them mesh to use only a simple shader
		auto pDiffuseRV = g_TeapotMesh.GetMaterial(pSubset->MaterialID)->pDiffuseRV11;
		pd3dImmediateContext->PSSetShaderResources(0, 1, &pDiffuseRV);

		pd3dImmediateContext->DrawIndexed((UINT)pSubset->IndexCount, 0, (UINT)pSubset->VertexStart);
	}
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
	g_D3DSettingsDlg.OnD3D11DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();

	g_TeapotMesh.Destroy();

	SAFE_RELEASE(g_pVertexLayout);
	SAFE_RELEASE(g_pVertexShader);
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pVertexShader1);
	SAFE_RELEASE(g_pPixelShader1);
	SAFE_RELEASE(g_pSamLinear);

	SAFE_RELEASE(g_pcbVSPerObject);
	SAFE_RELEASE(g_pcbPSPerObject);
	SAFE_RELEASE(g_pcbPSPerFrame);

	SAFE_RELEASE(g_pRenderTexture);
	SAFE_RELEASE(g_pRenderTargetView);
	SAFE_RELEASE(g_pShaderResouceView);
}



