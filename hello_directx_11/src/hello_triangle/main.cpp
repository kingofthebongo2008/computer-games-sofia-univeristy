#include "pch.h"
#include <cstdint>

#include <triangle_vertex.h>
#include <triangle_pixel.h>

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace Microsoft::WRL;

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

static winrt::com_ptr<ID3D11Device3> CreateDevice()
{
    winrt::com_ptr<ID3D11Device> r;
	D3D_FEATURE_LEVEL levels[]{ D3D_FEATURE_LEVEL_11_0 };
	ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, &levels[0], 1, D3D11_SDK_VERSION, r.put(), nullptr, nullptr));
	return r.as<ID3D11Device3>();
}

static winrt::com_ptr<ID3D11DeviceContext> CreateImmediateContext(ID3D11Device* d )
{
    winrt::com_ptr<ID3D11DeviceContext> r;
	d->GetImmediateContext(r.put());
	return r;
}


static winrt::com_ptr<IDXGISwapChain1> CreateSwapChain(const CoreWindow& w, ID3D11Device* d)
{
    winrt::com_ptr<IDXGIFactory2> f;
    winrt::com_ptr<IDXGISwapChain1> r;
	
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,__uuidof(IDXGIFactory2), reinterpret_cast<void**>(f.put())));

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

    ThrowIfFailed(f->CreateSwapChainForCoreWindow(d, sample::copy_to_abi<IUnknown>(w).get(), &desc, nullptr, r.put()));

	return r;
}

static winrt::com_ptr<ID3D11RenderTargetView1> CreateSwapChainView(IDXGISwapChain1* swap_chain, ID3D11Device3* device)
{
    winrt::com_ptr<ID3D11Texture2D> texture;

	ThrowIfFailed(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(texture.put())));
	CD3D11_RENDER_TARGET_VIEW_DESC desc(texture.get(), D3D11_RTV_DIMENSION_TEXTURE2D);
    winrt::com_ptr<ID3D11RenderTargetView1> r;
	ThrowIfFailed(device->CreateRenderTargetView1(texture.get(), nullptr, r.put()));
	return r;
}

static winrt::com_ptr <ID3D11VertexShader> CreateTriangleVertexShader(ID3D11Device3* device)
{
    winrt::com_ptr<ID3D11VertexShader> r;
	ThrowIfFailed(device->CreateVertexShader(g_triangle_vertex, sizeof(g_triangle_vertex), nullptr, r.put()));
	return r;
}

static winrt::com_ptr <ID3D11PixelShader> CreateTrianglePixelShader(ID3D11Device3* device)
{
    winrt::com_ptr <ID3D11PixelShader> r;
	ThrowIfFailed(device->CreatePixelShader(g_triangle_pixel, sizeof(g_triangle_pixel), nullptr, r.put()));
	return r;
}

static winrt::com_ptr<ID3D11RasterizerState2> CreateRasterizerState(ID3D11Device3* device)
{
    winrt::com_ptr<ID3D11RasterizerState2> r;

	D3D11_RASTERIZER_DESC2 state = {};

	state.FillMode				= D3D11_FILL_SOLID;
	state.CullMode				= D3D11_CULL_NONE;
	state.FrontCounterClockwise = TRUE;
	state.DepthClipEnable		= TRUE;
	state.ScissorEnable			= TRUE;

	ThrowIfFailed(device->CreateRasterizerState2(&state, r.put()));
	return r;
}

static winrt::com_ptr<ID3D11BlendState1> CreateBlendState(ID3D11Device3* device)
{
    winrt::com_ptr<ID3D11BlendState1> r;

	D3D11_BLEND_DESC1 state = {};
	state.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	ThrowIfFailed(device->CreateBlendState1(&state, r.put()));
	return r;
}

static winrt::com_ptr<ID3D11DepthStencilState> CreateDepthStencilState(ID3D11Device3* device)
{
    winrt::com_ptr <ID3D11DepthStencilState> r;

	D3D11_DEPTH_STENCIL_DESC state = {};
	state.DepthEnable = FALSE;
	state.DepthFunc = D3D11_COMPARISON_LESS;
	ThrowIfFailed(device->CreateDepthStencilState(&state, r.put()));
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
		m_device_context	= CreateImmediateContext(m_device.get());
	}

	void Uninitialize() 
	{

	}

	void Run()
	{
		while (m_window_running)
		{
			CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			{
                winrt::com_ptr<ID3D11RenderTargetView1> m_swap_chain_view = CreateSwapChainView(m_swap_chain.get(), m_device.get());

				m_device_context->ClearState();

				{
					float clear_value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					m_device_context->ClearRenderTargetView(m_swap_chain_view.get(), clear_value);
				}

				{
					ID3D11RenderTargetView* views[1] = { m_swap_chain_view.get() };
					m_device_context->OMSetRenderTargets(1, views, nullptr);
					m_device_context->OMSetDepthStencilState(m_depth_stencil_state.get(), 0);
				}

				{
					m_device_context->OMSetBlendState(m_blend_state.get(), nullptr, 0xFFFFFFFF);
				}

				{
					m_device_context->RSSetState(m_rasterizer_state.get());

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
					m_device_context->VSSetShader(m_triangle_vertex.get(), nullptr, 0);
				}

				{
					m_device_context->PSSetShader(m_triangle_pixel.get(), nullptr, 0);
				}
				{
					m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				}

				{
					m_device_context->Draw(3, 0);
				}
				
			}
			m_swap_chain->Present(0, 0);
		}
	}

	void Load(winrt::hstring h)
	{
		m_triangle_vertex = CreateTriangleVertexShader(m_device.get());
		m_triangle_pixel = CreateTrianglePixelShader(m_device.get());

		m_blend_state = CreateBlendState(m_device.get());
		m_rasterizer_state = CreateRasterizerState(m_device.get());
		m_depth_stencil_state = CreateDepthStencilState(m_device.get());
	}

	void SetWindow(const CoreWindow& w)
	{
		m_closed			= w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
		m_size_changed		= w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });

		m_swap_chain		= CreateSwapChain(w, m_device.get());

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
	
    winrt::com_ptr <ID3D11Device3>				m_device;
    winrt::com_ptr <ID3D11DeviceContext>		m_device_context;
    winrt::com_ptr <IDXGISwapChain1>			m_swap_chain;

    winrt::com_ptr <ID3D11VertexShader>			m_triangle_vertex;
    winrt::com_ptr <ID3D11PixelShader>			m_triangle_pixel;
    winrt::com_ptr <ID3D11RasterizerState2>		m_rasterizer_state;
    winrt::com_ptr <ID3D11BlendState1>			m_blend_state;
    winrt::com_ptr <ID3D11DepthStencilState>	m_depth_stencil_state;

	uint32_t									m_back_buffer_width = 0;
	uint32_t									m_back_buffer_height = 0;

};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	CoreApplication::Run(ViewProvider());
	CoUninitialize();
	return 0;
}
