//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "DirectXHelper.h"
#include "SampleSettings.h"
#include "TerrainRenderer.h"

using namespace TiledResources;

using namespace concurrency;
using namespace DirectX;
using namespace Windows::Foundation;

TerrainRenderer::TerrainRenderer(const std::shared_ptr<DeviceResources>& deviceResources) :
    m_deviceResources(deviceResources),
    m_diffuseTextureResidencyView(nullptr),
    m_normalTextureResidencyView(nullptr),
    m_useLimitedSampler(false),
    m_sunx(0.0f),
    m_suny(0.0f),
    m_scaleFactor(10.0f)
{
    CreateDeviceDependentResources();
}

void TerrainRenderer::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Create a constant buffer for the vertex transformation matrices.
    D3D11_BUFFER_DESC vertexShaderConstantBufferDesc;
    ZeroMemory(&vertexShaderConstantBufferDesc, sizeof(vertexShaderConstantBufferDesc));
    vertexShaderConstantBufferDesc.ByteWidth = sizeof(XMFLOAT4X4) * 4;
    vertexShaderConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexShaderConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    vertexShaderConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    DX::ThrowIfFailed(device->CreateBuffer(&vertexShaderConstantBufferDesc, nullptr, &m_vertexShaderConstantBuffer));

    // Create a constant buffer for information about environment lighting.
    D3D11_BUFFER_DESC pixelShaderConstantBufferDesc;
    ZeroMemory(&pixelShaderConstantBufferDesc, sizeof(pixelShaderConstantBufferDesc));
    pixelShaderConstantBufferDesc.ByteWidth = sizeof(XMFLOAT4);
    pixelShaderConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    pixelShaderConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    pixelShaderConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    DX::ThrowIfFailed(device->CreateBuffer(&pixelShaderConstantBufferDesc, nullptr, &m_pixelShaderConstantBuffer));

    // Create a tiled texture and view for the diffuse layer.
    D3D11_TEXTURE2D_DESC diffuseTextureDesc;
    ZeroMemory(&diffuseTextureDesc, sizeof(diffuseTextureDesc));
    diffuseTextureDesc.Width = SampleSettings::TerrainAssets::Diffuse::DimensionSize;
    diffuseTextureDesc.Height = SampleSettings::TerrainAssets::Diffuse::DimensionSize;
    diffuseTextureDesc.ArraySize = 6;
    diffuseTextureDesc.Format = SampleSettings::TerrainAssets::Diffuse::Format;
    diffuseTextureDesc.SampleDesc.Count = 1;
    if (m_deviceResources->GetTiledResourcesTier() <= D3D11_TILED_RESOURCES_TIER_1)
    {
        // On Tier 1, texture arrays (including texture cubes) may not include packed MIPs.
        diffuseTextureDesc.MipLevels = SampleSettings::TerrainAssets::Diffuse::UnpackedMipCount;
    }
    diffuseTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    diffuseTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    diffuseTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_TILED;
    DX::ThrowIfFailed(device->CreateTexture2D(&diffuseTextureDesc, nullptr, &m_diffuseTexture));

    D3D11_SHADER_RESOURCE_VIEW_DESC diffuseTextureViewDesc;
    ZeroMemory(&diffuseTextureViewDesc,sizeof(diffuseTextureViewDesc));
    diffuseTextureViewDesc.Format = diffuseTextureDesc.Format;
    diffuseTextureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    diffuseTextureViewDesc.TextureCube.MipLevels = -1; // Full MIP chain.
    DX::ThrowIfFailed(device->CreateShaderResourceView(m_diffuseTexture.Get(), &diffuseTextureViewDesc, &m_diffuseTextureView));

    // Create a tiled texture and view for the normal layer.
    D3D11_TEXTURE2D_DESC normalTextureDesc;
    ZeroMemory(&normalTextureDesc, sizeof(normalTextureDesc));
    normalTextureDesc.Width = SampleSettings::TerrainAssets::Normal::DimensionSize;
    normalTextureDesc.Height = SampleSettings::TerrainAssets::Normal::DimensionSize;
    normalTextureDesc.ArraySize = 6;
    normalTextureDesc.Format = SampleSettings::TerrainAssets::Normal::Format;
    normalTextureDesc.SampleDesc.Count = 1;
    if (m_deviceResources->GetTiledResourcesTier() <= D3D11_TILED_RESOURCES_TIER_1)
    {
        // On Tier 1, texture arrays (including texture cubes) may not include packed MIPs.
        normalTextureDesc.MipLevels = SampleSettings::TerrainAssets::Normal::UnpackedMipCount;
    }
    normalTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    normalTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    normalTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_TILED;
    DX::ThrowIfFailed(device->CreateTexture2D(&normalTextureDesc, nullptr, &m_normalTexture));

    D3D11_SHADER_RESOURCE_VIEW_DESC normalTextureViewDesc;
    ZeroMemory(&normalTextureViewDesc,sizeof(normalTextureViewDesc));
    normalTextureViewDesc.Format = normalTextureDesc.Format;
    normalTextureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    normalTextureViewDesc.TextureCube.MipLevels = -1; // Full MIP chain.
    DX::ThrowIfFailed(device->CreateShaderResourceView(m_normalTexture.Get(), &normalTextureViewDesc, &m_normalTextureView));
    
    // Create a wrapping trilinear sampler.
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;// D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    DX::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &m_trilinearSampler));
    samplerDesc.MinLOD = 4;
    DX::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &m_trilinearSamplerLimited));

    // On Tier-2 devices, create a wrapping trilinear sampler with max-filter behavior.
    // On Tier-1 devices, this behavior is emulated in shader code.
    if (m_deviceResources->GetTiledResourcesTier() >= D3D11_TILED_RESOURCES_TIER_2)
    {
        samplerDesc.Filter = D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
    }
    else
    {
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    }
    DX::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &m_maxFilterSampler));
}

task<void> TerrainRenderer::CreateDeviceDependentResourcesAsync()
{
    // Load and create the vertex shader and input layout.
    auto vsTask = DX::ReadDataAsync(L"TerrainRenderer.vs.cso").then([this](std::vector<byte> fileData)
    {
        auto device = m_deviceResources->GetD3DDevice();
        D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] = 
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        DX::ThrowIfFailed(device->CreateInputLayout(inputLayoutDesc, ARRAYSIZE(inputLayoutDesc), fileData.data(), fileData.size(), &m_inputLayout));
        DX::ThrowIfFailed(device->CreateVertexShader(fileData.data(), fileData.size(), nullptr, &m_vertexShader));
    });

    // Load and create the pixel shader.
    auto psTask = DX::ReadDataAsync(
        m_deviceResources->GetTiledResourcesTier() >= D3D11_TILED_RESOURCES_TIER_2 ?
        L"TerrainRenderer.Tier2.ps.cso" : 
        L"TerrainRenderer.Tier1.ps.cso"
        ).then([this](std::vector<byte> fileData)
    {
        auto device = m_deviceResources->GetD3DDevice();
        DX::ThrowIfFailed(device->CreatePixelShader(fileData.data(), fileData.size(), nullptr, &m_pixelShader));
    });

    // Load and create the vertex buffer.
    auto vbTask = DX::ReadDataAsync(L"geometry.vb.bin").then([this](std::vector<byte> fileData)
    {
        auto device = m_deviceResources->GetD3DDevice();
        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.ByteWidth = static_cast<UINT>(fileData.size());
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vertexBufferInitialData = {fileData.data(), 0, 0};
        DX::ThrowIfFailed(device->CreateBuffer(&vertexBufferDesc, &vertexBufferInitialData, &m_vertexBuffer));
    });

    // Load and create the index buffer.
    auto ibTask = DX::ReadDataAsync(L"geometry.ib.bin").then([this](std::vector<byte> fileData)
    {
        auto device = m_deviceResources->GetD3DDevice();
        D3D11_BUFFER_DESC indexBufferDesc;
        ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
        indexBufferDesc.ByteWidth = static_cast<UINT>(fileData.size());
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA indexBufferInitialData = {fileData.data(), 0, 0};
        DX::ThrowIfFailed(device->CreateBuffer(&indexBufferDesc, &indexBufferInitialData, &m_indexBuffer));
        m_terrainIndexCount = static_cast<unsigned int>(fileData.size() / sizeof(unsigned int));
    });

    return (vsTask && psTask && vbTask && ibTask);
}

void TerrainRenderer::ReleaseDeviceDependentResources()
{
    m_indexBuffer.Reset();
    m_vertexBuffer.Reset();
    m_inputLayout.Reset();
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_vertexShaderConstantBuffer.Reset();
    m_pixelShaderConstantBuffer.Reset();
    m_trilinearSampler.Reset();
    m_maxFilterSampler.Reset();
    m_diffuseTexture.Reset();
    m_diffuseTextureView.Reset();
    m_normalTexture.Reset();
    m_normalTextureView.Reset();
}

void TerrainRenderer::SetSourceGeometry(FreeCamera const& camera, bool applyOrientationTransform)
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Update the vertex shader constant buffer with the camera data.
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    DX::ThrowIfFailed(context->Map(m_vertexShaderConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
    static float arg = 0.0f;
    XMStoreFloat4x4(
        &static_cast<XMFLOAT4X4*>(mappedResource.pData)[2],
        XMMatrixRotationZ(arg)
        );
    arg += 1.0f / 600.0f;
    static_cast<XMFLOAT4X4*>(mappedResource.pData)[0] = camera.GetViewMatrix();
    if (applyOrientationTransform)
    {
        // When rendering directly to the swap chain render target, apply an orientation transform.
        // This is necessary as the swap chain is specified as pre-rotated, enabling better performance
        // but requiring that all source content be pre-rotated.
        XMFLOAT4X4 projection = camera.GetProjectionMatrix();
        XMStoreFloat4x4(
            &static_cast<XMFLOAT4X4*>(mappedResource.pData)[1],
            XMLoadFloat4x4(&projection)
            );
    }
    else
    {
        static_cast<XMFLOAT4X4*>(mappedResource.pData)[1] = camera.GetProjectionMatrix();
    }
    ((float*) mappedResource.pData)[16 * 3] = m_scaleFactor;
    context->Unmap(m_vertexShaderConstantBuffer.Get(), 0);

    // Set pipeline state from the Input Assembler through the Vertex Shader.
    UINT stride = sizeof(XMFLOAT3);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_vertexShaderConstantBuffer.GetAddressOf());
}


void TerrainRenderer::SetTargetsForRendering(FreeCamera const& camera)
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Update the pixel shader constant buffer with lighting information.
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    DX::ThrowIfFailed(context->Map(m_pixelShaderConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
    XMFLOAT3 sun(sinf(m_sunx), cosf(m_sunx), m_suny);
    XMStoreFloat3(&sun, XMVector3Normalize(XMLoadFloat3(&sun)));
    static_cast<XMFLOAT3*>(mappedResource.pData)[0] = sun;
    ((float*) mappedResource.pData)[3] = m_scaleFactor;
    context->Unmap(m_pixelShaderConstantBuffer.Get(), 0);

    // Set pipeline state from the Pixel Shader through the Output Merger.
    auto targetView = m_deviceResources->GetBackBufferRenderTargetView();
    context->OMSetRenderTargets(1, &targetView, m_deviceResources->GetDepthStencilView());
    //float clear [4] = { 0.92f, 0.65f, 0.41f, 1.0f };
    //XMFLOAT3 p = camera.GetPosition();
    //float camHeight;
    //XMStoreFloat(&camHeight, XMVector3Length(XMLoadFloat3(&p)));
    //for (int i = 0; i < 3; i++) clear[i] *= max(min(0.5f-p.x, 1.0f), 0) / (1.0f + camHeight * camHeight * camHeight);
    context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::Black);
    context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    D3D11_VIEWPORT screenViewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &screenViewport);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    ID3D11SamplerState* samplers[] = 
    {
        m_useLimitedSampler ? m_trilinearSamplerLimited.Get() : m_trilinearSampler.Get(),
        m_maxFilterSampler.Get()
    };
    context->PSSetSamplers(0, ARRAYSIZE(samplers), samplers);
    context->PSSetConstantBuffers(0, 1, m_pixelShaderConstantBuffer.GetAddressOf());
    ID3D11ShaderResourceView* textureViews[] = 
    {
        m_diffuseTextureView.Get(),
        m_diffuseTextureResidencyView,
        m_normalTextureView.Get(),
        m_normalTextureResidencyView
    };
    context->PSSetShaderResources(0, ARRAYSIZE(textureViews), textureViews);
}

void TerrainRenderer::Draw()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    context->DrawIndexed(m_terrainIndexCount, 0, 0);
}

ID3D11Texture2D* TerrainRenderer::GetDiffuseTexture()
{
    return m_diffuseTexture.Get();
}

void TerrainRenderer::SetDiffuseResidencyMap(ID3D11ShaderResourceView* view)
{
    m_diffuseTextureResidencyView = view;
}

ID3D11Texture2D* TerrainRenderer::GetNormalTexture()
{
    return m_normalTexture.Get();
}

void TerrainRenderer::SetNormalResidencyMap(ID3D11ShaderResourceView* view)
{
    m_normalTextureResidencyView = view;
}

void TerrainRenderer::ToggleLodLimit()
{
    m_useLimitedSampler = !m_useLimitedSampler;
}

void TerrainRenderer::RotateSun(float x, float y)
{
    float timeFactor = 1.0f / G_TIME_SCALE;
    m_sunx += x * timeFactor;
    if (m_sunx < -2.0f * XM_PI) m_sunx += 2.0f * XM_PI;
    if (m_sunx > 2.0f * XM_PI) m_sunx -= 2.0f * XM_PI;
    m_suny += y * timeFactor;
    if (m_suny > 4.0f) m_suny = 4.0f;
    if (m_suny < -4.0f) m_suny = -4.0f;
}

void TerrainRenderer::ChangeScale(float d)
{
    float timeFactor = 1.0f / G_TIME_SCALE;
    m_scaleFactor += d * timeFactor;
    if (m_scaleFactor < 1.0f) m_scaleFactor = 1.0f;
    if (m_scaleFactor > 50.0f) m_scaleFactor = 50.0f;
}
