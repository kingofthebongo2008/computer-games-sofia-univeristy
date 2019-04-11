#include "pch.h"

#include "main_renderer_interface.h"
#include "window_environment.h"

#include "device_resources.h"
#include "sampling_renderer.h"
#include "residency_manager.h"

#include "free_camera.h"


//Main renderer of the app

namespace sample
{
	class MainRenderer : public IMainRenderer
	{
		public:

		void Initialize() override;
		void Uninitialize() override;
		void Load()  override;
		void Run()  override;

		void OnWindowSizeChanged(const sample::window_environment& envrionment)  override;
		void SetWindow(::IUnknown* w, const sample::window_environment& envrionment)  override;

		private:

		std::unique_ptr<sample::DeviceResources>    m_deviceResources;          //gpu, swapchain, queues, heaps for rtv
		std::unique_ptr<sample::SamplingRenderer>   m_samplingRenderer;         //render targets for residency
		std::unique_ptr<sample::ResidencyManager>	m_residencyManager;			//updates physical data

		std::mutex                                  m_blockRendering;           //block render thread for the swap chain resizes

		winrt::com_ptr <ID3D12CommandAllocator>   	m_command_allocator[2];		//one per frame
		winrt::com_ptr <ID3D12GraphicsCommandList1> m_command_list[2];			//one per frame

		uint32_t                                    m_frame_index	    = 0;
		uint64_t									m_fence_value[2]    = { 1, 1 };

		//Rendering
		winrt::com_ptr< ID3D12RootSignature>		m_root_signature;
		winrt::com_ptr< ID3D12PipelineState>		m_sampling_renderer_state;  //responsible for output of the parts that we want
		winrt::com_ptr< ID3D12PipelineState>		m_terrain_renderer_state;   //responsible to renderer the terrain

		winrt::com_ptr <ID3D12Resource1>   			m_geometry_vertex_buffer;	//planet geometry
		winrt::com_ptr <ID3D12Resource1>   			m_geometry_index_buffer;	//planet indices

		winrt::com_ptr <ID3D12Resource1>			m_diffuse;					//diffuse.bin
		winrt::com_ptr <ID3D12Resource1>			m_normal;					//normal.bin

		winrt::com_ptr <ID3D12Resource1>			m_diffuse_residency;		//diffuse residency -> Texture which holds which mip level is resident
		winrt::com_ptr <ID3D12Resource1>			m_normal_residency;			//normal  residency -> Texture which holds which mip level is resident

		uint32_t									m_diffuse_srt			= 0;
		uint32_t									m_normal_srt			= 1;

		uint32_t									m_diffuse_residency_srt = 2;
		uint32_t									m_normal_residency_srt	= 3;

		//view concepts
		D3D12_VERTEX_BUFFER_VIEW                    m_planet_vertex_view;       //vertices for render
		D3D12_INDEX_BUFFER_VIEW                     m_planet_index_view;		//indices for render

		FreeCamera									m_camera;
	};
}

