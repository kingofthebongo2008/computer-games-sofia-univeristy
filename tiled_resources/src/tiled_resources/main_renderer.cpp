#include "pch.h"
#include "main_renderer.h"

#include <concrt.h>
#include <ppl.h>

#include "d3dx12.h"
#include <DirectXMath.h>

#include "window_environment.h"
#include "device_resources.h"

#include "sampling_renderer.h"
#include "sample_settings.h"
#include "error.h"
#include "file_helper.h"
#include "free_camera.h"

namespace sample
{

	//Create the memory manager for the gpu commands
	static winrt::com_ptr <ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device1* device)
	{
		winrt::com_ptr<ID3D12CommandAllocator> r;
		sample::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), r.put_void()));
		return r;
	}

	//create an object that will record commands
	static winrt::com_ptr <ID3D12GraphicsCommandList1> CreateCommandList(ID3D12Device1* device, ID3D12CommandAllocator* a)
	{
		winrt::com_ptr<ID3D12GraphicsCommandList1> r;
		sample::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, a, nullptr, __uuidof(ID3D12GraphicsCommandList1), r.put_void()));

		r->Close();
		return r;
	}

	//create an object which represents what types of external data the shaders will use. You can imagine f(int x, float y); Root Signature is that we have two parameters on locations 0 and 1 types int and float
	static winrt::com_ptr< ID3D12RootSignature>	 CreateRootSignature(ID3D12Device1* device)
	{
		static
        #include <default_graphics_signature.h>

			winrt::com_ptr<ID3D12RootSignature> r;
		sample::ThrowIfFailed(device->CreateRootSignature(0, &g_default_graphics_signature[0], sizeof(g_default_graphics_signature), __uuidof(ID3D12RootSignature), r.put_void()));
		return r;
	}

	//sampling
	static winrt::com_ptr< ID3D12PipelineState>	 CreateSamplingRendererState(ID3D12Device1* device, ID3D12RootSignature* root)
	{
		static
        #include <sampling_renderer_pixel.h>

		static
        #include <sampling_renderer_vertex.h>

		D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
		state.pRootSignature = root;
		state.SampleMask = UINT_MAX;
		state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		state.NumRenderTargets = 1;
		state.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
		state.SampleDesc.Count = 1;
		state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		state.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		state.DepthStencilState.DepthEnable = TRUE;
		state.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		state.DepthStencilState.StencilEnable = FALSE;

		//Describe the format of the vertices. In the gpu they are going to be unpacked into the registers
	   //If you apply compression to then, you can always make them bytes
		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		state.InputLayout.NumElements = 1;
		state.InputLayout.pInputElementDescs = &inputLayoutDesc[0];


		state.VS = { &g_sampling_renderer_vertex[0], sizeof(g_sampling_renderer_vertex) };
		state.PS = { &g_sampling_renderer_pixel[0], sizeof(g_sampling_renderer_pixel) };

		winrt::com_ptr<ID3D12PipelineState> r;

		sample::ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
		return r;
	}

	//render sampled resouces
	static winrt::com_ptr< ID3D12PipelineState>	 CreateTerrainRendererState(ID3D12Device1* device, ID3D12RootSignature* root)
	{
		static
        #include <terrain_renderer_tier2_pixel.h>

        static
        #include <terrain_renderer_vertex.h>

        D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
		state.pRootSignature = root;
		state.SampleMask = UINT_MAX;
		state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		state.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		state.RasterizerState.FrontCounterClockwise = TRUE;

		state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		state.NumRenderTargets = 1;
		state.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
		state.SampleDesc.Count = 1;
		state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

		state.DepthStencilState.DepthEnable = FALSE;
		state.DepthStencilState.StencilEnable = FALSE;

		//Describe the format of the vertices. In the gpu they are going to be unpacked into the registers
		//If you apply compression to then, you can always make them bytes
		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		state.InputLayout.NumElements = 1;
		state.InputLayout.pInputElementDescs = &inputLayoutDesc[0];

		state.VS = { &g_terrain_renderer_vertex[0],      sizeof(g_terrain_renderer_vertex) };
		state.PS = { &g_terrain_renderer_tier2_pixel[0], sizeof(g_terrain_renderer_tier2_pixel) };

		winrt::com_ptr<ID3D12PipelineState> r;
		sample::ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
		return r;
	}

	//prepare for creation
	inline D3D12_RESOURCE_DESC DescribeBuffer(uint64_t elements, uint64_t elementSize = 1)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
		desc.DepthOrArraySize = 1;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Height = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = elements * elementSize;
		return desc;
	}

	//compute geometry sizes
	static D3D12_RESOURCE_DESC DescribeGeometryBuffer(size_t byte_size)
	{
		return DescribeBuffer(byte_size);
	}

	//create buffer for upload
	static winrt::com_ptr<ID3D12Resource1 > CreateGeometryUploadBuffer(ID3D12Device1* device, size_t size_in_bytes)
	{
		D3D12_RESOURCE_DESC d = DescribeGeometryBuffer(size_in_bytes);

		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_HEAP_PROPERTIES p = {};
		p.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_GENERIC_READ;

		sample::ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
		return r;
	}

	//create buffer in vram
	static winrt::com_ptr<ID3D12Resource1 > CreateGeometryBuffer(ID3D12Device1* device, size_t size_in_bytes)
	{
		D3D12_RESOURCE_DESC d = DescribeGeometryBuffer(size_in_bytes);

		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_HEAP_PROPERTIES p = {};
		p.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_COPY_DEST;

		sample::ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
		return r;
	}

	//Cube map
	inline D3D12_RESOURCE_DESC DescribeDiffuse()
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
		desc.DepthOrArraySize = 6;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Format = SampleSettings::TerrainAssets::Diffuse::Format;
		desc.Height = SampleSettings::TerrainAssets::Diffuse::DimensionSize;
		desc.Layout		= D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		desc.MipLevels	= SampleSettings::TerrainAssets::Diffuse::UnpackedMipCount;	
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = SampleSettings::TerrainAssets::Diffuse::DimensionSize;
		return desc;
	}

	static winrt::com_ptr<ID3D12Resource1 > CreateDiffuseTexture(ID3D12Device1* device)
	{
		D3D12_RESOURCE_DESC d = DescribeDiffuse();

		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_RESOURCE_STATES       state	= D3D12_RESOURCE_STATE_COPY_DEST;
		sample::ThrowIfFailed(device->CreateReservedResource(&d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));

		return r;
	}

	inline D3D12_SHADER_RESOURCE_VIEW_DESC DescribeDiffuseView()
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format					= SampleSettings::TerrainAssets::Diffuse::Format;
		desc.ViewDimension			= D3D12_SRV_DIMENSION_TEXTURECUBE;
		desc.TextureCube.MipLevels  = SampleSettings::TerrainAssets::Diffuse::UnpackedMipCount;
		return desc;
	}

	static void CreateDiffuseShaderResourceView(ID3D12Device1* d, ID3D12Resource1* r, D3D12_CPU_DESCRIPTOR_HANDLE h)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC v = DescribeDiffuseView();
		d->CreateShaderResourceView(r, &v, h);
	}

	//Diffuse Residency
	inline D3D12_RESOURCE_DESC DescribeResidency( uint32_t width, uint32_t height)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
		desc.DepthOrArraySize	= 6;
		desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Flags				= D3D12_RESOURCE_FLAG_NONE;
		desc.Format				= DXGI_FORMAT_R8_UNORM;
		desc.Height				= height;
		desc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.MipLevels			= 1;
		desc.SampleDesc.Count	= 1;
		desc.SampleDesc.Quality = 0;
		desc.Width				= width;
		return desc;
	}

	static winrt::com_ptr<ID3D12Resource1> CreateResidency(ID3D12Device1* device, uint32_t width, uint32_t height)
	{
		D3D12_RESOURCE_DESC d = DescribeResidency(width, height);

		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_HEAP_PROPERTIES p = {};
		p.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_COPY_SOURCE;

		ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
		return r;
	}

	inline D3D12_RESOURCE_DESC DescribeNormal()
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
		desc.DepthOrArraySize = 6;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Format = SampleSettings::TerrainAssets::Normal::Format;
		desc.Height = SampleSettings::TerrainAssets::Normal::DimensionSize;
		desc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		desc.MipLevels = SampleSettings::TerrainAssets::Normal::UnpackedMipCount;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = SampleSettings::TerrainAssets::Normal::DimensionSize;
		return desc;
	}

	static winrt::com_ptr<ID3D12Resource1 > CreateNormalTexture(ID3D12Device1* device)
	{
		D3D12_RESOURCE_DESC d = DescribeNormal();
		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_COPY_DEST;
		sample::ThrowIfFailed(device->CreateReservedResource(&d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
		return r;
	}

	//Directly maps constants
	struct PassConstants
	{
		DirectX::XMFLOAT4X4 m_View;
		DirectX::XMFLOAT4X4 m_Projection;
		DirectX::XMFLOAT3   m_SunPosition;
		float               m_ScaleFactor;
	};

	void MainRenderer::Initialize()
	{
		m_deviceResources = std::make_unique<sample::DeviceResources>();
		m_samplingRenderer = std::make_unique<sample::SamplingRenderer>();

		//if you have many threads that generate commands. 1 per thread per frame
		{
			ID3D12Device1* d = m_deviceResources->Device();

			m_command_allocator[0] = CreateCommandAllocator(d);
			m_command_allocator[1] = CreateCommandAllocator(d);

			m_command_list[0] = CreateCommandList(d, m_command_allocator[0].get());
			m_command_list[1] = CreateCommandList(d, m_command_allocator[1].get());
		}
	}

	void MainRenderer::Uninitialize()
	{

	}

	void MainRenderer::Load()
	{
		ID3D12Device1* d = m_deviceResources->Device();
		m_root_signature = CreateRootSignature(d);

		//Compile many shader during the loading time of the app

		//use concurrency runtime 
		concurrency::task_group g;

		//spawn many loading tasks
		g.run([this, d]
		{
			m_sampling_renderer_state = CreateSamplingRendererState(d, m_root_signature.get());
		});

		g.run([this, d]
		{
			//Pattern for uploading resources

			//Create resource on the upload heap. Example works with 1 heap per resource
			//Read the data and copy to the resource
			auto bytes0 = sample::ReadFileAsync(L"data1\\geometry.vb.bin").then([d](std::vector<uint8_t> b)
			{
				auto buf0Upload = CreateGeometryUploadBuffer(d, b.size());
				void* upload;
				CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
				sample::ThrowIfFailed(buf0Upload->Map(0, &readRange, reinterpret_cast<void**>(&upload)));
				memcpy(upload, &b[0], b.size());
				buf0Upload->Unmap(0, nullptr);
				return std::make_tuple(buf0Upload, b.size());
			});

			//Create resource on the upload heap. Example works with 1 heap per resource
			//Read the data and copy to the resource
			auto bytes1 = sample::ReadFileAsync(L"data1\\geometry.ib.bin").then([d](std::vector<uint8_t>  b)
			{
				auto buf0Upload = CreateGeometryUploadBuffer(d, b.size());

				void* upload;
				CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
				sample::ThrowIfFailed(buf0Upload->Map(0, &readRange, reinterpret_cast<void**>(&upload)));
				memcpy(upload, &b[0], b.size());
				buf0Upload->Unmap(0, nullptr);

				return std::make_tuple(buf0Upload, b.size());
			});

			//wait for the tasks
			//indices and vertices memory must be alive on execute
			auto   vertices = bytes0.get();
			auto   indices = bytes1.get();

			auto   verticesBuffer = std::get<0>(vertices).get();
			auto   indicesBuffer = std::get<0>(indices).get();

			auto   vericesSize = std::get<1>(vertices);
			auto   indicesSize = std::get<1>(indices);

			//Create resource on in the gpu memory

			m_geometry_vertex_buffer = CreateGeometryBuffer(d, vericesSize);
			m_geometry_index_buffer = CreateGeometryBuffer(d, indicesSize);

			//Set debugging names
			m_geometry_vertex_buffer->SetName(L"geometry.vb.bin");
			m_geometry_index_buffer->SetName(L"geometry.ib.bin");

			m_planet_vertex_view.BufferLocation = m_geometry_vertex_buffer->GetGPUVirtualAddress();
			m_planet_vertex_view.SizeInBytes = static_cast<uint32_t>(vericesSize);
			m_planet_vertex_view.StrideInBytes = 3 * sizeof(float);  //spacing between every two vertices x,y,z for this demo

			m_planet_index_view.BufferLocation = m_geometry_index_buffer->GetGPUVirtualAddress();
			m_planet_index_view.SizeInBytes = static_cast<uint32_t>(indicesSize);
			m_planet_index_view.Format = DXGI_FORMAT_R32_UINT;

			//Copy the resources to the gpu memory
			//and prepare them for usage by the gpu
			ID3D12CommandAllocator* allocator = m_command_allocator[m_frame_index].get();
			ID3D12GraphicsCommandList1* commandList = m_command_list[m_frame_index].get();
			allocator->Reset();
			commandList->Reset(allocator, nullptr);

			commandList->CopyResource(m_geometry_vertex_buffer.get(), verticesBuffer);
			commandList->CopyResource(m_geometry_index_buffer.get(), indicesBuffer);

			//Fix barriers, prepare for reading
			{
				D3D12_RESOURCE_BARRIER barrier[2] = {};

				barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier[0].Transition.pResource = m_geometry_vertex_buffer.get();
				barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
				barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier[1].Transition.pResource = m_geometry_index_buffer.get();
				barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
				barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barrier[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				commandList->ResourceBarrier(2, &barrier[0]);
			}

			commandList->Close();


			//Execute what we have so far
			{
				//form group of several command lists
				ID3D12CommandList* lists[] = { commandList };
				m_deviceResources->Queue()->ExecuteCommandLists(1, lists); //Execute what we have, submission of commands to the gpu
			}

			//Insert in the gpu a command after all submitted commands so far.
			const uint64_t fence_value = m_fence_value[m_frame_index];
			m_deviceResources->SignalFenceValue(fence_value);
			m_deviceResources->WaitForFenceValue(fence_value); //block the cpu
			m_fence_value[m_frame_index] = fence_value + 1;    //increase the fence
		});

		g.run([this, d]
		{
			m_residencyManager	= std::make_unique<ResidencyManager>();
			/*
			{
				m_diffuse = CreateDiffuseTexture(d);	//Create the reserved resource
				m_diffuse->SetName(L"diffuse.bin");

				auto managed		= m_residencyManager->ManageTexture(d, m_diffuse.get(), L"data1\\diffuse.bin");
				m_diffuse_residency = CreateResidency(d, managed->ResidencyWidth(), managed->ResidencyHeight());
				//CreateDiffuseShaderResourceView(d, m_diffuse.get(), CpuView( d, m_deviceResources->ShaderHeap()) + m_diffuse_srt);
			}

			{
				m_normal = CreateNormalTexture(d);	//Create the reserved resource
				m_normal->SetName(L"normal.bin");

				auto managed		= m_residencyManager->ManageTexture(d, m_normal.get(), L"data1\\normal.bin");
				m_normal_residency	= CreateResidency(d, managed->ResidencyWidth(), managed->ResidencyHeight());

			}
			*/
		});

		g.run([this, d]
		{
			//m_residencyManager	= std::make_unique<ResidencyManager>();
			

		});

		//let the waiting thread do some work also
		g.run_and_wait([this, d]
		{
			m_terrain_renderer_state = CreateTerrainRendererState(d, m_root_signature.get());
		});
	}

	void MainRenderer::Run()
	{
		std::lock_guard lock(m_blockRendering);

		//reset the command generators for this frame if they have data, which was already used by the gpu
		ID3D12CommandAllocator* allocator = m_command_allocator[m_frame_index].get();
		ID3D12GraphicsCommandList1* commandList = m_command_list[m_frame_index].get();
		allocator->Reset();
		commandList->Reset(allocator, nullptr);

		//Do the main depth
		{

			//get the pointer to the gpu memory
			D3D12_CPU_DESCRIPTOR_HANDLE back_buffer = m_deviceResources->SwapChainHandle(m_frame_index);

			//Transition resources for writing. flush caches
			{
				D3D12_RESOURCE_BARRIER barrier = {};

				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = m_deviceResources->SwapChainBuffer(m_frame_index);
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				commandList->ResourceBarrier(1, &barrier);
			}

			//Get Depth Buffer
			D3D12_CPU_DESCRIPTOR_HANDLE depth_buffer = m_deviceResources->SwapChainDepthHandle(m_frame_index);

			//Mark the resources in the rasterizer output
			{
				commandList->OMSetRenderTargets(1, &back_buffer, TRUE, &depth_buffer);
			}

			//do the clear, fill the memory with a value
			{
				FLOAT c[4] = { 0.0f, 0.f,0.f,0.f };
				commandList->ClearRenderTargetView(back_buffer, c, 0, nullptr);
				commandList->ClearDepthStencilView(depth_buffer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			}

			{
				//set the type of the parameters that we will use in the shader
				commandList->SetGraphicsRootSignature(m_root_signature.get());

				//Now map constants for the two passes
				//Root constants are used to put there access to most commonly used data
				PassConstants constants;

				constants.m_View = m_camera.GetViewMatrix();
				constants.m_Projection = m_camera.GetProjectionMatrix();
				constants.m_ScaleFactor = 10.0f;
				DirectX::XMStoreFloat3(&constants.m_SunPosition, DirectX::g_XMOne3);

				//Constants are important and must match;
				static_assert(sizeof(PassConstants) == 36 * 4);
				commandList->SetGraphicsRoot32BitConstants(7, 36, &constants, 0);

				//set the raster pipeline state as a whole, it was prebuilt before
				commandList->SetPipelineState(m_sampling_renderer_state.get());

				uint32_t  w = m_deviceResources->SwapChainWidth();
				uint32_t  h = m_deviceResources->SwapChainHeight();

				//set the scissor test separately (which parts of the view port will survive)
				{
					D3D12_RECT r = { 0, 0, static_cast<int32_t>(w), static_cast<int32_t>(h) };
					commandList->RSSetScissorRects(1, &r);
				}

				//set the viewport. 
				{
					D3D12_VIEWPORT  v;
					v.TopLeftX = 0;
					v.TopLeftY = 0;
					v.MinDepth = 0.0f;
					v.MaxDepth = 1.0f;
					v.Width = static_cast<float>(w);
					v.Height = static_cast<float>(h);
					commandList->RSSetViewports(1, &v);
				}

				//set the types of the triangles we will use
				{
					commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					commandList->IASetIndexBuffer(&m_planet_index_view);
					commandList->IASetVertexBuffers(0, 1, &m_planet_vertex_view);

				}

				//draw the triangles indices to draw
				commandList->DrawIndexedInstanced(m_planet_index_view.SizeInBytes / 4, 1, 0, 0, 0);
			}

			//Transition resources for presenting, flush the gpu caches
			{
				D3D12_RESOURCE_BARRIER barrier = {};

				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = m_deviceResources->SwapChainBuffer(m_frame_index);
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				commandList->ResourceBarrier(1, &barrier);
			}
		}

        //Note, how transition barriers are not scheduled well.

		//Now do the sampling renderer
		{
			//get the pointer to the gpu memory
			D3D12_CPU_DESCRIPTOR_HANDLE back_buffer = m_samplingRenderer->SamplingHandle(m_frame_index);

			//Transition resources for writing. flush caches
			{
				D3D12_RESOURCE_BARRIER barrier = {};

				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = m_samplingRenderer->SamplingRenderTarget(m_frame_index);
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				commandList->ResourceBarrier(1, &barrier);
			}

			//Get Depth Buffer
			D3D12_CPU_DESCRIPTOR_HANDLE depth_buffer = m_samplingRenderer->SamplingDepthHandle(m_frame_index);

			//Mark the resources in the rasterizer output
			{
				commandList->OMSetRenderTargets(1, &back_buffer, TRUE, &depth_buffer);
			}

			//do the clear, fill the memory with a value
			{
				FLOAT c[4] = { 0.0f, 0.f,0.f,0.f };
				commandList->ClearRenderTargetView(back_buffer, c, 0, nullptr);
				commandList->ClearDepthStencilView(depth_buffer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
			}

			{
				//set the type of the parameters that we will use in the shader
				commandList->SetGraphicsRootSignature(m_root_signature.get());

				//Now map constants for the two passes
				//Root constants are used to put there access to most commonly used data
				PassConstants constants;

				constants.m_View = m_camera.GetViewMatrix();
				constants.m_Projection = m_camera.GetProjectionMatrix();
				constants.m_ScaleFactor = 10.0f;
				DirectX::XMStoreFloat3(&constants.m_SunPosition, DirectX::g_XMOne3);

				//Constants are important and must match;
				static_assert(sizeof(PassConstants) == 36 * 4);
				commandList->SetGraphicsRoot32BitConstants(7, 36, &constants, 0);

				//set the raster pipeline state as a whole, it was prebuilt before
				commandList->SetPipelineState(m_sampling_renderer_state.get());

				uint32_t  w = m_samplingRenderer->SamplingWidth();
				uint32_t  h = m_samplingRenderer->SamplingHeight();

				//set the scissor test separately (which parts of the view port will survive)
				{
					D3D12_RECT r = { 0, 0, static_cast<int32_t>(w), static_cast<int32_t>(h) };
					commandList->RSSetScissorRects(1, &r);
				}

				//set the viewport. 
				{
					D3D12_VIEWPORT  v;
					v.TopLeftX = 0;
					v.TopLeftY = 0;
					v.MinDepth = 0.0f;
					v.MaxDepth = 1.0f;
					v.Width = static_cast<float>(w);
					v.Height = static_cast<float>(h);
					commandList->RSSetViewports(1, &v);
				}

				//set the types of the triangles we will use
				{
					commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					commandList->IASetIndexBuffer(&m_planet_index_view);
					commandList->IASetVertexBuffers(0, 1, &m_planet_vertex_view);

				}

				//draw the triangles indices to draw
				commandList->DrawIndexedInstanced(m_planet_index_view.SizeInBytes / 4, 1, 0, 0, 0);
			}

			//Transition resources for copying, flush the gpu caches
			{
				D3D12_RESOURCE_BARRIER barrier = {};

				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = m_samplingRenderer->SamplingRenderTarget(m_frame_index);
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				commandList->ResourceBarrier(1, &barrier);
			}

			//copy to back buffer
			{
				auto source			= m_samplingRenderer->SamplingRenderTarget(m_frame_index);
				auto destination	= m_samplingRenderer->SamplingStaging(m_frame_index);

				// The footprint may depend on the device of the resource, but we assume there is only one device.
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;

				m_deviceResources->Device()->GetCopyableFootprints(&source->GetDesc(), 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

				commandList->CopyTextureRegion(
					&CD3DX12_TEXTURE_COPY_LOCATION(destination, PlacedFootprint), 0, 0, 0,
					&CD3DX12_TEXTURE_COPY_LOCATION(source, 0), nullptr);
			}
		}

		commandList->Close();   //close the list

		{
			//form group of several command lists
			ID3D12CommandList* lists[] = { commandList };
			m_deviceResources->Queue()->ExecuteCommandLists(1, lists); //Execute what we have, submission of commands to the gpu
		}

		
		const uint64_t fence_value = m_fence_value[m_frame_index];

		{
			//Tell the gpu to signal the cpu after it finishes executing the commands that we have just submitted
			m_deviceResources->SignalFenceValue(fence_value);
		}

		m_deviceResources->SwapChain()->Present(1, 0);    //present the swap chain

		//prepare for the next frame
		{
			m_frame_index = m_deviceResources->SwapChain()->GetCurrentBackBufferIndex();
			m_deviceResources->WaitForFenceValue(m_fence_value[m_frame_index]);
			m_fence_value[m_frame_index] = fence_value + 1;
		}

		//Now we can readback the data from the previous frame
		{
			auto source = m_samplingRenderer->SamplingRenderTarget(m_frame_index);
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
			uint32_t rows = 0;
			uint64_t rowSizeInBytes = 0;
			uint64_t totalBytes = 0;
			m_deviceResources->Device()->GetCopyableFootprints(&source->GetDesc(), 0, 1, 0, &PlacedFootprint, &rows, &rowSizeInBytes, &totalBytes);

			SamplingRenderer::CollectParameters collect;

			collect.m_height	= PlacedFootprint.Footprint.Height;
			collect.m_width		= PlacedFootprint.Footprint.Width;
			collect.m_row_pitch = PlacedFootprint.Footprint.RowPitch;
			collect.m_total_bytes = totalBytes;

			m_samplingRenderer->CollectSamples(m_frame_index, collect);
		}
	}

	static inline uint32_t align8(uint32_t value)
	{
		return (value + 7) & ~7;
	}

	void MainRenderer::SetWindow(::IUnknown * w, const sample::window_environment & envrionment)
	{
		auto width = align8(static_cast<uint32_t>(envrionment.m_back_buffer_size.Width));
		auto height = align8(static_cast<uint32_t>(envrionment.m_back_buffer_size.Height));
		m_frame_index = m_deviceResources->CreateSwapChain(w, width, height);

		//Create the sampling renderer
		sample::ResizeSamplingRendererContext ctx = {};

		ctx.m_device = m_deviceResources->Device();
		ctx.m_width = width;
		ctx.m_height = height;
		ctx.m_depth_index = 2;
		ctx.m_render_target_index = 2;
		ctx.m_depth_heap = m_deviceResources->DepthHeap();
		ctx.m_render_target_heap = m_deviceResources->RenderTargetHeap();
		m_samplingRenderer->CreateSamplingRenderer(ctx);


		//Camera
		{
			using namespace DirectX;
			XMFLOAT3 initialCamera(-0.149467558f, 1.04009187f, -0.145361549f);
			float mult = 2.0f;
			initialCamera.x *= mult;
			initialCamera.y *= mult;
			initialCamera.z *= mult;
			m_camera.SetViewParameters(initialCamera, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-0.3f, 0.0f, 1.0f));
			m_camera.SetProjectionParameters(width, height);
		}
	}

	void MainRenderer::OnWindowSizeChanged(const sample::window_environment & envirionment)
	{
		//wait for the render thread to finish and block it so we can submit a command
		std::lock_guard lock(m_blockRendering);

		//Now wait for the gpu to finish what it has from the main thread

		//Insert in the gpu a command after all submitted commands so far.
		const uint64_t fence_value = m_fence_value[m_frame_index];

		m_deviceResources->SignalFenceValue(fence_value);
		m_deviceResources->WaitForFenceValue(fence_value);

		auto w = align8(static_cast<uint32_t>(envirionment.m_back_buffer_size.Width));
		auto h = align8(static_cast<uint32_t>(envirionment.m_back_buffer_size.Height));

		{
			m_frame_index = m_deviceResources->ResizeBuffers(w, h);
		}

		{
			//Create the sampling renderer
			sample::ResizeSamplingRendererContext ctx = {};

			ctx.m_device = m_deviceResources->Device();
			ctx.m_width = w;
			ctx.m_height = h;
			ctx.m_depth_index = 2;
			ctx.m_render_target_index = 2;
			ctx.m_depth_heap = m_deviceResources->DepthHeap();
			ctx.m_render_target_heap = m_deviceResources->RenderTargetHeap();
			m_samplingRenderer->CreateSamplingRenderer(ctx);
		}

		//Prepare to unblock the rendering
		m_fence_value[m_frame_index] = fence_value + 1;
	}
}

IMainRenderer* MakeMainRenderer()
{
	return new sample::MainRenderer();
}
