#pragma once

#include "Windows.h"

#include "d3dx12.h"

#if defined(NTDDI_WIN10_RS2)
#define USES_DXGI6 1
#else
#define USES_DXGI6 0
#endif

#if USES_DXGI6
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <dxgidebug.h>

using Microsoft::WRL::ComPtr;

static const uint32_t MAX_FRAME_COUNT = 2;

class DeviceResources
{
public:
	enum InitFlags : uint32_t
	{
		IF_EnableDebugLayer = 1 << 0,
		IF_AllowTearing = 1 << 1,
		IF_EnableHDR = 1 << 2
	};

	void init(HWND window, uint32_t initFlags);
	bool IsTearingAllowed();
	void SetNextFenceValue();

	ID3D12Device*               GetD3DDevice() const { return m_d3dDevice.Get(); }
	IDXGISwapChain3*            GetSwapChain() const { return m_swapChain.Get(); }
	IDXGIFactory4*              GetDXGIFactory() const { return m_dxgiFactory.Get(); }
	D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
	uint64_t					GetCurrentFenceValue() const { return m_fenceValues[m_backBufferIndex]; }
	ID3D12Resource*             GetRenderTarget() const { return m_renderTargets[m_backBufferIndex].Get(); }
	ID3D12Resource*             GetDepthStencil() const { return m_depthStencil.Get(); }
	ID3D12CommandQueue*         GetCommandQueue() const { return m_commandQueue.Get(); }
	ID3D12CommandAllocator*     GetCommandAllocator() const { return m_commandAllocators[m_backBufferIndex].Get(); }
	ID3D12GraphicsCommandList*  GetCommandList() const { return m_commandList.Get(); }
	ID3D12Fence*				GetFence() const { return m_fence.Get(); }
	HANDLE						GetFenceEvent() { return m_fenceEvent.Get(); }
	DXGI_FORMAT                 GetBackBufferFormat() const { return m_backBufferFormat; }
	DXGI_FORMAT                 GetDepthBufferFormat() const { return m_depthBufferFormat; }
	D3D12_VIEWPORT              GetScreenViewport() const { return m_screenViewport; }
	D3D12_RECT                  GetScissorRect() const { return m_scissorRect; }
	UINT                        GetCurrentFrameIndex() const { return m_backBufferIndex; }
	UINT                        GetBackBufferCount() const { return m_backBufferCount; }
	DXGI_COLOR_SPACE_TYPE       GetColorSpace() const { return m_colorSpace; }

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

private:
	void enableDebugLayer();
	bool checkTearingSupport();
	void createDevice(bool bEnableDebugLayer);
	void checkFeatureLevel();
	void createCommandQueue();
	void createDescriptorHeaps();
	void createCommandAllocators();
	void createCommandList();
	void createEndOfFrameFence();

	void WaitForGpu();
	void initWindowSizeDependent();
	void resizeSwapChain(uint32_t width, uint32_t height, DXGI_FORMAT format);
	void createSwapChain(uint32_t width, uint32_t height, DXGI_FORMAT format);
	void updateColorSpace();
	void createBackBuffers();
	void createDepthBuffer(uint32_t width, uint32_t height);

	uint32_t							m_backBufferIndex;

	ComPtr<ID3D12Device>                m_d3dDevice;
	ComPtr<ID3D12CommandQueue>          m_commandQueue;
	ComPtr<ID3D12GraphicsCommandList>   m_commandList;
	ComPtr<ID3D12CommandAllocator>      m_commandAllocators[MAX_FRAME_COUNT];

	// Swap chain objects.
	ComPtr<IDXGIFactory4>               m_dxgiFactory;
	ComPtr<IDXGISwapChain3>             m_swapChain;
	ComPtr<ID3D12Resource>              m_renderTargets[MAX_FRAME_COUNT];
	ComPtr<ID3D12Resource>              m_depthStencil;

	// Presentation fence objects.
	ComPtr<ID3D12Fence>                 m_fence;
	uint64_t                            m_fenceValues[MAX_FRAME_COUNT];
	Microsoft::WRL::Wrappers::Event     m_fenceEvent;

	// Direct3D rendering objects.
	ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
	uint64_t                            m_rtvDescriptorSize;
	D3D12_VIEWPORT                      m_screenViewport;
	D3D12_RECT                          m_scissorRect;

	// Direct3D properties.
	DXGI_FORMAT                         m_backBufferFormat;
	DXGI_FORMAT                         m_depthBufferFormat;
	uint32_t                            m_backBufferCount;
	D3D_FEATURE_LEVEL                   m_d3dMinFeatureLevel;

	// Cached device properties.
	HWND                                m_window;
	D3D_FEATURE_LEVEL                   m_d3dFeatureLevel;
	DWORD                               m_dxgiFactoryFlags;
	RECT                                m_outputSize;

	// HDR Support
	DXGI_COLOR_SPACE_TYPE               m_colorSpace;

	uint32_t m_initFlags;
};

CD3DX12_CPU_DESCRIPTOR_HANDLE DeviceResources::GetRenderTargetView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_backBufferIndex, m_rtvDescriptorSize);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DeviceResources::GetDepthStencilView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DeviceResources::SetNextFenceValue()
{
	// Set the fence value for the next frame.
	m_fenceValues[m_backBufferIndex] = GetCurrentFenceValue() + 1;
}

bool DeviceResources::IsTearingAllowed()
{
	return m_initFlags & IF_AllowTearing;
}

// This method acquires the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, try WARP.
IDXGIAdapter1* getFirstAvailableHardwareAdapter(ComPtr<IDXGIFactory4> dxgiFactory, D3D_FEATURE_LEVEL minFeatureLevel)
{
	ComPtr<IDXGIAdapter1> adapter;

	uint32_t adapterIndex = 0;
	HRESULT getAdapterResult = S_OK;

	while (getAdapterResult != DXGI_ERROR_NOT_FOUND)
	{
		getAdapterResult = dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf());


		DXGI_ADAPTER_DESC1 desc;
		ASSERT_RESULT(adapter->GetDesc1(&desc));

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
		{
			LOG("Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
			break;
		}

		adapterIndex++;
	}

	if (!adapter)
	{
		// Try WARP12 instead
		if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
		{
			ASSERT_F(false, "WARP12 not available. Enable the 'Graphics Tools' optional feature");
		}

		LOG("Direct3D Adapter - WARP12\n");
	}

	ASSERT_F(adapter != nullptr, "No Direct3D 12 device found");
	return adapter.Detach();
}

void DeviceResources::init(HWND window, uint32_t initFlags)
{
	const bool bEnableDebugLayer = initFlags & IF_EnableDebugLayer;
	const bool bWantAllowTearing = initFlags & IF_AllowTearing;
	bool bAllowTearing = bWantAllowTearing;

	m_window = window;

	m_d3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	m_d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	m_backBufferIndex = 0;
	m_backBufferCount = 2;

	m_rtvDescriptorSize = 0;
	m_backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	m_depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

	m_dxgiFactoryFlags = 0;
	m_outputSize = { 0, 0, 1, 1 };
	m_colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

	if (bEnableDebugLayer)
	{
		enableDebugLayer();
	}

	HRESULT createFactoryResult = CreateDXGIFactory2(m_dxgiFactoryFlags, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()));
	ASSERT_RESULT(createFactoryResult);

	if (bWantAllowTearing)
	{
		bAllowTearing = checkTearingSupport();

		if (!bAllowTearing)
		{
			initFlags &= ~IF_AllowTearing;
		}
	}

	m_initFlags = initFlags;

	createDevice(bEnableDebugLayer);
	checkFeatureLevel();
	createCommandQueue();
	createDescriptorHeaps();
	createCommandAllocators();
	createCommandList();
	createEndOfFrameFence();

	initWindowSizeDependent();
}

DXGI_FORMAT formatSrgbToLinear(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
	default:                                return fmt;
	}
}

void DeviceResources::resizeSwapChain(uint32_t width, uint32_t height, DXGI_FORMAT format)
{
	HRESULT hr = m_swapChain->ResizeBuffers(
		m_backBufferCount,
		width,
		height,
		format,
		(m_initFlags & IF_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
	);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		char err[64] = {};
		sprintf_s(err, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? m_d3dDevice->GetDeviceRemovedReason() : hr);

		ASSERT_F(false, err);
	}
}

void DeviceResources::createSwapChain(uint32_t width, uint32_t height, DXGI_FORMAT format)
{

	// Create a descriptor for the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = format;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = m_backBufferCount;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = (m_initFlags & IF_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain1> swapChain;

	HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		m_window,
		&swapChainDesc,
		&fsSwapChainDesc,
		nullptr,
		swapChain.GetAddressOf()
	);

	ASSERT_RESULT(hr);

	hr = swapChain.As(&m_swapChain);
	ASSERT_RESULT(hr);

	// This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
	hr = m_dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);
	ASSERT_RESULT(hr);
}

void DeviceResources::updateColorSpace()
{
	DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	bool bIsDisplayHDR10 = false;
	HRESULT hr = S_OK;

#if USES_DXGI6
	ASSERT(m_swapChain);

	ComPtr<IDXGIOutput> output;
	hr = m_swapChain->GetContainingOutput(output.GetAddressOf());
	ASSERT_RESULT(hr);

	ComPtr<IDXGIOutput6> output6;
	ASSERT_RESULT(output.As(&output6));

	DXGI_OUTPUT_DESC1 desc;
	ASSERT_RESULT(output6->GetDesc1(&desc));

	bIsDisplayHDR10 = desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
#endif

	if ((m_initFlags & InitFlags::IF_EnableHDR) && bIsDisplayHDR10)
	{
		switch (m_backBufferFormat)
		{
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		{
			// The application creates the HDR10 signal.
			colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
			break;
		}
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		{
			// The system creates the HDR10 signal; application uses linear values.
			colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
			break;
		}
		default:
		{
			// Not sure if this is a valid case.
			ASSERT(false);
			break;
		}
		}
	}

	m_colorSpace = colorSpace;

	uint32_t colorSpaceSupport = 0;
	hr = m_swapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport);

	if (SUCCEEDED(hr) && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
	{
		hr = m_swapChain->SetColorSpace1(colorSpace);
		ASSERT_RESULT(hr);
	}

}

void DeviceResources::initWindowSizeDependent()
{
	ASSERT(m_window);

	WaitForGpu();

	// Release resources that are tied to the swap chain and update fence values.
	for (uint32_t n = 0; n < m_backBufferCount; n++)
	{
		m_renderTargets[n].Reset();
		m_fenceValues[n] = m_fenceValues[m_backBufferIndex];
	}

	const uint32_t backBufferWidth = max(static_cast<uint32_t>(m_outputSize.right - m_outputSize.left), 1u);
	const uint32_t backBufferHeight = max(static_cast<uint32_t>(m_outputSize.bottom - m_outputSize.top), 1u);
	const DXGI_FORMAT backBufferFormat = formatSrgbToLinear(m_backBufferFormat);

	// If the swap chain already exists, resize it, otherwise create one.
	if (m_swapChain)
	{
		resizeSwapChain(backBufferWidth, backBufferHeight, backBufferFormat);
	}
	else
	{
		createSwapChain(backBufferWidth, backBufferHeight, backBufferFormat);
	}

	updateColorSpace();

	createBackBuffers();

	// Reset the index to the current back buffer.
	m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
	{
		createDepthBuffer(backBufferWidth, backBufferHeight);
	}

	// Set the 3D rendering viewport and scissor rectangle to target the entire window.
	m_screenViewport.TopLeftX = m_screenViewport.TopLeftY = 0.f;
	m_screenViewport.Width = static_cast<float>(backBufferWidth);
	m_screenViewport.Height = static_cast<float>(backBufferHeight);
	m_screenViewport.MinDepth = D3D12_MIN_DEPTH;
	m_screenViewport.MaxDepth = D3D12_MAX_DEPTH;

	m_scissorRect.left = m_scissorRect.top = 0;
	m_scissorRect.right = backBufferWidth;
	m_scissorRect.bottom = backBufferHeight;
}

void DeviceResources::createBackBuffers()
{
	for (uint32_t i = 0; i < m_backBufferCount; i++)
	{
		HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_renderTargets[i].GetAddressOf()));
		ASSERT_RESULT(hr);

		wchar_t name[25] = {};
		swprintf_s(name, L"Render target %d", i);
		m_renderTargets[i]->SetName(name);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = m_backBufferFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), i, static_cast<UINT>(m_rtvDescriptorSize));
		m_d3dDevice->CreateRenderTargetView(m_renderTargets[i].Get(), &rtvDesc, rtvDescriptor);
	}
}

void DeviceResources::createDepthBuffer(uint32_t width, uint32_t height)
{

	// Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
	// on this surface.
	CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		m_depthBufferFormat,
		width,
		height,
		1, // This depth stencil view has only one texture.
		1  // Use a single mipmap level.
	);
	depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = m_depthBufferFormat;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	HRESULT hr = m_d3dDevice->CreateCommittedResource(
		&depthHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf())
	);

	ASSERT_RESULT(hr);

	m_depthStencil->SetName(L"Depth stencil");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = m_depthBufferFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DeviceResources::WaitForGpu()
{
	if (!m_commandQueue || !m_fence || !m_fenceEvent.IsValid())
	{
		return;
	}

	// Schedule a Signal command in the GPU queue.
	uint64_t fenceValue = m_fenceValues[m_backBufferIndex];

	if (!SUCCEEDED(m_commandQueue->Signal(m_fence.Get(), fenceValue)))
	{
		return;
	}

	// Wait until the Signal has been processed.
	if (!SUCCEEDED(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent.Get())))
	{
		return;
	}

	WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_fenceValues[m_backBufferIndex]++;
}

void DeviceResources::enableDebugLayer()
{
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
	{
		debugController->EnableDebugLayer();
	}
	else
	{
		LOG("WARNING: Direct3D Debug Device is not available\n");
	}

	ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
	{
		m_dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
	}
}

bool DeviceResources::checkTearingSupport()
{
	BOOL allowTearing = FALSE;
	ComPtr<IDXGIFactory5> factory5;
	HRESULT hr = m_dxgiFactory.As(&factory5);

	if (SUCCEEDED(hr))
	{
		hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
	}

	bool bAllowTearing = SUCCEEDED(hr) && allowTearing;

	if (!bAllowTearing)
	{
		LOG("Variable refresh rate displays not supported");
	}

	return bAllowTearing;
}

void DeviceResources::createDevice(bool bEnableDebugLayer)
{
	ComPtr<IDXGIAdapter1> adapter;
	*adapter.GetAddressOf() = getFirstAvailableHardwareAdapter(m_dxgiFactory, m_d3dMinFeatureLevel);

	D3D12CreateDevice(
		adapter.Get(),
		m_d3dMinFeatureLevel,
		IID_PPV_ARGS(m_d3dDevice.ReleaseAndGetAddressOf())
	);

	m_d3dDevice->SetName(L"DeviceResources");

	// Configure debug device (if active).
	ComPtr<ID3D12InfoQueue> d3dInfoQueue;
	if (SUCCEEDED(m_d3dDevice.As(&d3dInfoQueue)))
	{
		if (bEnableDebugLayer)
		{
			d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		}

		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		d3dInfoQueue->AddStorageFilterEntries(&filter);
	}
}

void DeviceResources::checkFeatureLevel()
{
	// Determine maximum supported feature level for this device
	static const D3D_FEATURE_LEVEL s_featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
	{
		_countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
	};

	HRESULT hr = m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
	if (SUCCEEDED(hr))
	{
		m_d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
	}
	else
	{
		m_d3dFeatureLevel = m_d3dMinFeatureLevel;
	}
}

void DeviceResources::createCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT queueCreated = m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf()));
	ASSERT_RESULT(queueCreated);

	m_commandQueue->SetName(L"DeviceResources");
}

void DeviceResources::createDescriptorHeaps()
{
	// Render targets
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
	rtvDescriptorHeapDesc.NumDescriptors = m_backBufferCount;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	HRESULT descrHeapCreated = m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf()));
	ASSERT_RESULT(descrHeapCreated);

	m_rtvDescriptorHeap->SetName(L"DeviceResources");
	m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Depth Stencil Views
	ASSERT(m_depthBufferFormat != DXGI_FORMAT_UNKNOWN);

	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	descrHeapCreated = m_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf()));

	m_dsvDescriptorHeap->SetName(L"DeviceResources");
}

void DeviceResources::createCommandAllocators()
{
	for (uint32_t n = 0; n < m_backBufferCount; n++)
	{
		HRESULT createdAllocator = m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocators[n].ReleaseAndGetAddressOf()));
		ASSERT_RESULT(createdAllocator);

		wchar_t name[25] = {};
		swprintf_s(name, L"Render target %u", n);
		m_commandAllocators[n]->SetName(name);
	}
}

void DeviceResources::createCommandList()
{
	HRESULT cmdListCreated = m_d3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocators[0].Get(),
		nullptr,
		IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf()));

	ASSERT_RESULT(cmdListCreated);
	ASSERT_RESULT(m_commandList->Close());

	m_commandList->SetName(L"DeviceResources");
}

void DeviceResources::createEndOfFrameFence()
{
	// Create a fence for tracking GPU execution progress.
	HRESULT createdFence = m_d3dDevice->CreateFence(m_fenceValues[m_backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()));
	ASSERT_RESULT(createdFence);
	m_fenceValues[m_backBufferIndex]++;

	m_fence->SetName(L"DeviceResources");

	m_fenceEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
	ASSERT(m_fenceEvent.IsValid());
}