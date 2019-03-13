#include "pch.h"
#include <cstdint>

#include <d3d11_3.h>
#include <dxgi1_5.h>
#include <wrl/client.h>

#include <winrt/base.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include <triangle_vertex.h>
#include <triangle_pixel.h>

#include "sys_profile_timer.h"

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace Microsoft::WRL;

struct exception : public std::exception
{
	exception(HRESULT h) : m_h(h)
	{

	}

	HRESULT m_h;
};

inline void ThrowIfFailed(HRESULT hr)
{
	if (hr != S_OK)
	{
		throw exception(hr);
	}
}

static ComPtr<ID3D11Device3> CreateDevice()
{
	ComPtr<ID3D11Device> r;
	D3D_FEATURE_LEVEL levels[]{ D3D_FEATURE_LEVEL_11_0 };
	ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &levels[0], 1, D3D11_SDK_VERSION, r.GetAddressOf(), nullptr, nullptr));

	ComPtr<ID3D11Device3> r0;
	ThrowIfFailed(r.As(&r0));
	return r0;
}

static ComPtr<ID3D11DeviceContext> CreateImmediateContext(ID3D11Device* d )
{
	ComPtr<ID3D11DeviceContext> r;
	d->GetImmediateContext(r.GetAddressOf());
	return r;
}

namespace sample
{
    template <typename to, typename from> to* copy_to_abi_private(const from& w)
    {
        void* v = nullptr;
        winrt::copy_to_abi(w, v);

        return reinterpret_cast<to*>(v);
    }

    template <typename to, typename from> winrt::com_ptr<to> copy_to_abi(const from& w)
    {
        winrt::com_ptr<to> v;
        v.attach(sample::copy_to_abi_private<IUnknown>(w));
        return v;
    }
}


static ComPtr<IDXGISwapChain1> CreateSwapChain(const CoreWindow& w, ID3D11Device* d)
{
	ComPtr<IDXGIFactory2> f;
	ComPtr<IDXGISwapChain1> r;
	
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,__uuidof(IDXGIFactory2), reinterpret_cast<void**>(f.GetAddressOf())));

	DXGI_SWAP_CHAIN_DESC1 desc = {};

	desc.BufferCount	= 2;
	desc.Format			= DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width			= static_cast<UINT>(w.Bounds().Width);
	desc.Height			= static_cast<UINT>(w.Bounds().Height);
	desc.SampleDesc.Count = 1;
	desc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.AlphaMode		= DXGI_ALPHA_MODE_IGNORE;
	desc.Scaling		= DXGI_SCALING_NONE;

	ThrowIfFailed(f->CreateSwapChainForCoreWindow(d, sample::copy_to_abi<IUnknown>(w).get(), &desc, nullptr, r.GetAddressOf()));
	return r;
}

static ComPtr<ID3D11RenderTargetView1> CreateSwapChainView(IDXGISwapChain1* swap_chain, ID3D11Device3* device)
{
	ComPtr<ID3D11Texture2D> texture;

	ThrowIfFailed(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(texture.GetAddressOf())));
	CD3D11_RENDER_TARGET_VIEW_DESC desc(texture.Get(), D3D11_RTV_DIMENSION_TEXTURE2D);
	ComPtr<ID3D11RenderTargetView1> r;
	ThrowIfFailed(device->CreateRenderTargetView1(texture.Get(), nullptr, r.GetAddressOf()));
	return r;
}

static ComPtr<ID3D11VertexShader> CreateTriangleVertexShader(ID3D11Device3* device)
{
	ComPtr<ID3D11VertexShader> r;
	ThrowIfFailed(device->CreateVertexShader(g_triangle_vertex, sizeof(g_triangle_vertex), nullptr, r.GetAddressOf()));
	return r;
}

static ComPtr<ID3D11PixelShader> CreateTrianglePixelShader(ID3D11Device3* device)
{
	ComPtr<ID3D11PixelShader> r;
	ThrowIfFailed(device->CreatePixelShader(g_triangle_pixel, sizeof(g_triangle_pixel), nullptr, r.GetAddressOf()));
	return r;
}

static ComPtr<ID3D11RasterizerState2> CreateRasterizerState(ID3D11Device3* device)
{
	ComPtr<ID3D11RasterizerState2> r;

	D3D11_RASTERIZER_DESC2 state = {};

	state.FillMode				= D3D11_FILL_SOLID;
	state.CullMode				= D3D11_CULL_NONE;
	state.FrontCounterClockwise = TRUE;
	state.DepthClipEnable		= TRUE;
	state.ScissorEnable			= TRUE;

	ThrowIfFailed(device->CreateRasterizerState2(&state, r.GetAddressOf()));
	return r;
}

static ComPtr<ID3D11BlendState1> CreateBlendState(ID3D11Device3* device)
{
	ComPtr<ID3D11BlendState1> r;

	D3D11_BLEND_DESC1 state = {};
	state.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	ThrowIfFailed(device->CreateBlendState1(&state, r.GetAddressOf()));
	return r;
}

static ComPtr<ID3D11DepthStencilState> CreateDepthStencilState(ID3D11Device3* device)
{
	ComPtr<ID3D11DepthStencilState> r;

	D3D11_DEPTH_STENCIL_DESC state = {};
	state.DepthEnable = FALSE;
	state.DepthFunc = D3D11_COMPARISON_LESS;
	ThrowIfFailed(device->CreateDepthStencilState(&state, r.GetAddressOf()));
	return r;
}

static const float pi = 3.14159265358979323846f;
static const float initial_angles[4] =
{
	0.0f,
	pi / 2.0f,
	pi / 4.0f,
	3.0f * pi / 4.0f
};

 ComPtr<ID3D11Buffer> CreateRotationAnglesBuffer(ID3D11Device3* device)
{
	ComPtr<ID3D11Buffer> r;
	D3D11_BUFFER_DESC desc = {};

	desc.ByteWidth = sizeof(float) * 4;
	desc.StructureByteStride = sizeof(float);
	desc.Usage				= D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags	= D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags			= D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA d = {};
	d.pSysMem = &initial_angles[0];
	d.SysMemPitch = sizeof(initial_angles);

	ThrowIfFailed(device->CreateBuffer(&desc, &d, r.GetAddressOf()));
	return r;
}

static ComPtr<ID3D11ShaderResourceView1> CreateRotationAnglesView(ID3D11Device3* d, ID3D11Buffer* buffer)
{
	ComPtr<ID3D11ShaderResourceView1> r;
	D3D11_SHADER_RESOURCE_VIEW_DESC1 desc = {};

	desc.ViewDimension			= D3D11_SRV_DIMENSION_BUFFER;
	desc.Format					= DXGI_FORMAT_R32_FLOAT;
	desc.Buffer.ElementWidth	= sizeof(float);
	desc.Buffer.NumElements		= 4;
	

	ThrowIfFailed(d->CreateShaderResourceView1(buffer, &desc, r.GetAddressOf()));
	return r;
}

class ViewProvider : public winrt::implements<ViewProvider, IFrameworkView, IFrameworkViewSource>
{
	public:

	IFrameworkView CreateView()
	{
			return *this;
	}

	void Initialize(const CoreApplicationView& v)
	{
		m_activated			= v.Activated(winrt::auto_revoke, { this, &ViewProvider::OnActivated });
		m_device			= CreateDevice();
		m_device_context	= CreateImmediateContext(m_device.Get());

		m_rotation_angles	= CreateRotationAnglesBuffer(m_device.Get());
		m_rotation_angles_view = CreateRotationAnglesView(m_device.Get(), m_rotation_angles.Get());

		for (auto i = 0; i < 4; ++i)
		{
			m_angles[i] = initial_angles[i];
		}
	}

	void Uninitialize() 
	{

	}

	void Run()
	{
		while (m_window_running)
		{
			CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			

			//game update
			{
				double frame_time	= m_frame_timer.seconds();
				m_frame_timer.reset();


				const float total			= 3.14159265358979323846f * 2.0f;
				const  uint32_t steps		= 144;
				const float step_rotation	= total / steps;

				for (auto i = 0; i < 4; ++i)
				{
					m_angles[i] = static_cast<float>(m_angles[i] + 100.0f * frame_time * step_rotation);
				}
			}

			//game render
			{
				ComPtr<ID3D11RenderTargetView1> m_swap_chain_view = CreateSwapChainView(m_swap_chain.Get(), m_device.Get());

				m_device_context->ClearState();

				{
					float clear_value[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
					m_device_context->ClearRenderTargetView(m_swap_chain_view.Get(), clear_value);
				}

				{
					ID3D11RenderTargetView* views[1] = { m_swap_chain_view.Get() };
					m_device_context->OMSetRenderTargets(1, views, nullptr);
					m_device_context->OMSetDepthStencilState(m_depth_stencil_state.Get(), 0);
				}

				{
					m_device_context->OMSetBlendState(m_blend_state.Get(), nullptr, 0xFFFFFFFF);
				}

				{
					m_device_context->RSSetState(m_rasterizer_state.Get());

					D3D11_RECT r = { 0, 0, m_back_buffer_width, m_back_buffer_height };
					m_device_context->RSSetScissorRects(1, &r);
					
					D3D11_VIEWPORT v;
					v.TopLeftX = 0;
					v.TopLeftY = 0;
					v.MinDepth = 0.0f;
					v.MaxDepth = 1.0f;
					v.Width = static_cast<float>(m_back_buffer_width);
					v.Height = static_cast<float>(m_back_buffer_height);

					m_device_context->RSSetViewports(1, &v);
				}

				{
					D3D11_MAPPED_SUBRESOURCE mapped = {};
					ThrowIfFailed(m_device_context->Map(m_rotation_angles.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));

					std::memcpy(mapped.pData, &m_angles[0], sizeof(m_angles));

					m_device_context->Unmap(m_rotation_angles.Get(), 0 );
					

					ID3D11ShaderResourceView* v[1] = { m_rotation_angles_view.Get() };
					m_device_context->VSSetShaderResources(0, 1, v);
					m_device_context->VSSetShader(m_triangle_vertex.Get(), nullptr, 0);
				}

				{
					m_device_context->PSSetShader(m_triangle_pixel.Get(), nullptr, 0);
				}
				{
					m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				}

				{
					m_device_context->DrawInstanced(3,4,0, 0);
				}
			}
			m_swap_chain->Present(0, 0);
		}
	}

	void Load(winrt::hstring h)
	{
		m_triangle_vertex = CreateTriangleVertexShader(m_device.Get());
		m_triangle_pixel = CreateTrianglePixelShader(m_device.Get());

		m_blend_state = CreateBlendState(m_device.Get());
		m_rasterizer_state = CreateRasterizerState(m_device.Get());
		m_depth_stencil_state = CreateDepthStencilState(m_device.Get());
	}

	void SetWindow(const CoreWindow& w)
	{
		m_closed			= w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
		m_size_changed		= w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });

		m_swap_chain		= CreateSwapChain(w, m_device.Get());

		m_back_buffer_width = static_cast<UINT>(w.Bounds().Width);
		m_back_buffer_height = static_cast<UINT>(w.Bounds().Height);
	}

	void OnWindowClosed(const CoreWindow&w, const CoreWindowEventArgs& a)
	{
		m_window_running = false;
	}

	void OnActivated(const CoreApplicationView&, const IActivatedEventArgs&)
	{
		CoreWindow::GetForCurrentThread().Activate();
	}

	void OnWindowSizeChanged(const CoreWindow& w, const WindowSizeChangedEventArgs& a)
	{
		ThrowIfFailed(m_swap_chain->ResizeBuffers(3, static_cast<UINT>(a.Size().Width), static_cast<UINT>(a.Size().Height), DXGI_FORMAT_R8G8B8A8_UNORM, 0));

		m_back_buffer_width = static_cast<UINT>(a.Size().Width);
		m_back_buffer_height = static_cast<UINT>(a.Size().Height);
	}

	bool m_window_running = true;
	CoreWindow::Closed_revoker					m_closed;
	CoreWindow::SizeChanged_revoker				m_size_changed;
	CoreApplicationView::Activated_revoker		m_activated;
	
	ComPtr<ID3D11Device3>						m_device;
	ComPtr<ID3D11DeviceContext>					m_device_context;
	ComPtr<IDXGISwapChain1>						m_swap_chain;

	ComPtr<ID3D11VertexShader>					m_triangle_vertex;
	ComPtr<ID3D11PixelShader>					m_triangle_pixel;
	ComPtr<ID3D11RasterizerState2>				m_rasterizer_state;
	ComPtr<ID3D11BlendState1>					m_blend_state;
	ComPtr<ID3D11DepthStencilState>				m_depth_stencil_state;

	uint32_t									m_back_buffer_width = 0;
	uint32_t									m_back_buffer_height = 0;

	ComPtr<ID3D11Buffer>						m_rotation_angles;
	ComPtr<ID3D11ShaderResourceView1>			m_rotation_angles_view;

	float										m_angles[4];
	sys::profile_timer							m_frame_timer;

};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	CoreApplication::Run(ViewProvider());
	CoUninitialize();
	return 0;
}
