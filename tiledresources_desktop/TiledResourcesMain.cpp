//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "TiledResourcesMain.h"
#include "SampleSettings.h"

using namespace TiledResources;

using namespace concurrency;
using namespace DirectX;

#define START_IN_DEBUG 0

float g_minCamera;

// Loads and initializes application assets when the application is loaded.
TiledResourcesMain::TiledResourcesMain(const std::shared_ptr<DeviceResources>& deviceResources) :
    m_deviceResources(deviceResources),
    m_renderersLoaded(false),
    m_borderMode(false),
    m_lastInputWasDirect(false),
#if START_IN_DEBUG
    m_debugMode(true)
#else
    m_debugMode(false)
#endif
{
    // Create the renderers and the residency manager.
    m_terrainRenderer = std::unique_ptr<TerrainRenderer>(new TerrainRenderer(m_deviceResources));
    m_samplingRenderer = std::unique_ptr<SamplingRenderer>(new SamplingRenderer(m_deviceResources));
    m_residencyManager = std::unique_ptr<ResidencyManager>(new ResidencyManager(m_deviceResources));

    m_samplingRenderer->SetDebugMode(m_debugMode);
    m_residencyManager->SetDebugMode(m_debugMode);

    CreateDeviceDependentResourcesAsync().then([this]()
    {
        m_renderersLoaded = true;
    });

    XMFLOAT3 initialCamera(-0.149467558f, 1.04009187f, -0.145361549f);
    XMStoreFloat(&g_minCamera, XMVector3Length(XMLoadFloat3(&initialCamera)));
    float mult = 2.0f;
    initialCamera.x *= mult;
    initialCamera.y *= mult;
    initialCamera.z *= mult;
    m_camera.SetViewParameters(initialCamera, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-0.3f, 0.0f, 1.0f));
    m_camera.SetProjectionParameters(m_deviceResources->GetScreenViewport().Width, m_deviceResources->GetScreenViewport().Height);

    ZeroMemory(&m_controlState,sizeof(m_controlState));
}

task<void> TiledResourcesMain::CreateDeviceDependentResourcesAsync()
{
    auto terrainRendererCreationTask = m_terrainRenderer->CreateDeviceDependentResourcesAsync();
    auto samplingRendererCreationTask = m_samplingRenderer->CreateDeviceDependentResourcesAsync();
    auto residencyManagerCreationTask = m_residencyManager->CreateDeviceDependentResourcesAsync();

    // Register the terrain renderer's tiled resources with the residency manager.
    auto diffuseResidencyMap = m_residencyManager->ManageTexture(m_terrainRenderer->GetDiffuseTexture(), L"diffuse.bin");
    m_terrainRenderer->SetDiffuseResidencyMap(diffuseResidencyMap);
    auto normalResidencyMap = m_residencyManager->ManageTexture(m_terrainRenderer->GetNormalTexture(), L"normal.bin");
    m_terrainRenderer->SetNormalResidencyMap(normalResidencyMap);

    // Initialize the managed resources, pre-loading any necessary data (i.e. tiles in packed MIPs).
    auto managedResourceInitializationTask = m_residencyManager->InitializeManagedResourcesAsync();

    return (terrainRendererCreationTask && samplingRendererCreationTask && residencyManagerCreationTask && managedResourceInitializationTask);
}

// Notifies renderers that device resources need to be released.
void TiledResourcesMain::OnDeviceLost()
{
    m_renderersLoaded = false;

    m_terrainRenderer->ReleaseDeviceDependentResources();
    m_samplingRenderer->ReleaseDeviceDependentResources();
    m_residencyManager->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be re-created.
void TiledResourcesMain::OnDeviceRestored()
{
    m_terrainRenderer->CreateDeviceDependentResources();
    m_samplingRenderer->CreateDeviceDependentResources();
    m_residencyManager->CreateDeviceDependentResources();

    CreateDeviceDependentResourcesAsync().then([this]()
    {
        m_renderersLoaded = true;
    });

    UpdateForWindowSizeChange();
}

// Updates application state when the window size changes (e.g. device orientation change)
void TiledResourcesMain::UpdateForWindowSizeChange() 
{
    m_samplingRenderer->CreateWindowSizeDependentResources();
    m_camera.SetProjectionParameters(m_deviceResources->GetScreenViewport().Width, m_deviceResources->GetScreenViewport().Height);
}

// Updates the application state once per frame.
void TiledResourcesMain::Update() 
{
    // Update camera control.
    float timeFactor = 1.0f / G_TIME_SCALE;

    if (m_lastInputWasDirect)
    {
        m_camera.ApplyTranslation(XMFLOAT3(
            m_directControlState.vx * timeFactor,
            m_directControlState.vy * timeFactor,
            m_directControlState.vz * timeFactor
            ));

        m_camera.ApplyRotation(XMFLOAT3(
            m_directControlState.vrx * timeFactor,
            m_directControlState.vry * timeFactor,
            m_directControlState.vrz * timeFactor
            ));
    }
    else
    {
        m_camera.ApplyTranslation(XMFLOAT3(
            ((m_controlState.vxp ? 1.0f : 0.0f) - (m_controlState.vxn ? 1.0f : 0.0f)) * SampleSettings::CameraDynamics::TranslationSpeed * timeFactor,
            ((m_controlState.vyp ? 1.0f : 0.0f) - (m_controlState.vyn ? 1.0f : 0.0f)) * SampleSettings::CameraDynamics::TranslationSpeed * timeFactor,
            ((m_controlState.vzp ? 1.0f : 0.0f) - (m_controlState.vzn ? 1.0f : 0.0f)) * SampleSettings::CameraDynamics::TranslationSpeed * timeFactor
            ));

        m_camera.ApplyRotation(XMFLOAT3(
            m_controlState.vrx * SampleSettings::CameraDynamics::TransientRotationMultiplier,
            m_controlState.vry * SampleSettings::CameraDynamics::TransientRotationMultiplier,
            ((m_controlState.vrzp ? 1.0f : 0.0f) - (m_controlState.vrzn ? 1.0f : 0.0f)) * SampleSettings::CameraDynamics::RotationSpeed * timeFactor
            ));
    }

    // Clear transient control state.
    m_controlState.vrx = 0.0f;
    m_controlState.vry = 0.0f;
    m_lastInputWasDirect = false;
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool TiledResourcesMain::Render()
{
    // Don't try to render anything before the first Update.
    if (!m_renderersLoaded)
    {
        return false;
    }

    // Render the scene objects.
    m_terrainRenderer->SetSourceGeometry(m_camera, false);
    m_samplingRenderer->SetTargetsForSampling();
    m_terrainRenderer->Draw();
    m_terrainRenderer->SetSourceGeometry(m_camera, true);
    m_terrainRenderer->SetTargetsForRendering(m_camera);
    m_terrainRenderer->Draw();

    //m_terrainRenderer->ToggleLodLimit();
    //UINT num;
    //D3D11_VIEWPORT vp[16];
    //m_deviceResources->GetD3DDeviceContext()->RSGetViewports(&num, NULL);
    //m_deviceResources->GetD3DDeviceContext()->RSGetViewports(&num, vp);
    //D3D11_VIEWPORT temp;
    //temp.TopLeftX = vp[0].Width / 2;
    //temp.TopLeftY = vp[0].TopLeftY;
    //temp.Width = vp[0].Width - temp.TopLeftX;
    //temp.Height = vp[0].Height;
    //temp.MinDepth = vp[0].MinDepth;
    //temp.MaxDepth = vp[0].MaxDepth;
    //m_deviceResources->GetD3DDeviceContext()->RSSetViewports(1, &temp);
    //m_terrainRenderer->SetTargetsForRendering(m_camera);
    //m_terrainRenderer->Draw();
    //m_terrainRenderer->ToggleLodLimit();
    //m_deviceResources->GetD3DDeviceContext()->RSSetViewports(1, &vp[0]);

    m_samplingRenderer->RenderVisualization();
    auto samples = m_samplingRenderer->CollectSamples();
    m_residencyManager->EnqueueSamples(samples);
    m_residencyManager->ProcessQueues();
    m_residencyManager->RenderVisualization();

    return true;
}

void TiledResourcesMain::OnPointerMoved(float dx, float dy)
{
    m_controlState.vrx += -dy;
    m_controlState.vry += -dx;
}

void TiledResourcesMain::OnKeyChanged(WPARAM key, bool down)
{
    switch(key)
    {
    case SampleSettings::Controls::Right:
        m_controlState.vxp = down;
        break;
    case SampleSettings::Controls::Left:
        m_controlState.vxn = down;
        break;
    case SampleSettings::Controls::Up:
        m_controlState.vyp = down;
        break;
    case SampleSettings::Controls::Down:
        m_controlState.vyn = down;
        break;
    case SampleSettings::Controls::Back:
        m_controlState.vzp = down;
        break;
    case SampleSettings::Controls::Forward:
        m_controlState.vzn = down;
        break;
    case SampleSettings::Controls::RollLeft:
        m_controlState.vrzp = down;
        break;
    case SampleSettings::Controls::RollRight:
        m_controlState.vrzn = down;
        break;
    case SampleSettings::Controls::ToggleDebug:
        if (down)
        {
            m_debugMode = !m_debugMode;
            m_samplingRenderer->SetDebugMode(m_debugMode);
            m_residencyManager->SetDebugMode(m_debugMode);
        }
        break;
    case SampleSettings::Controls::ResetMappings:
        if (down)
        {
            m_residencyManager->Reset();
        }
        break;
    case SampleSettings::Controls::ToggleBorder:
        if (down)
        {
            m_borderMode = !m_borderMode;
            m_residencyManager->SetBorderMode(m_borderMode);
            m_residencyManager->Reset();
        }
        break;
    case SampleSettings::Controls::ToggleLodLimit:
        if (down)
        {
            m_terrainRenderer->ToggleLodLimit();
        }
        break;
    default:
        break;
    }
}

void TiledResourcesMain::OnRightClick(float x, float y)
{
    m_samplingRenderer->DebugSample(x, y);
}

void TiledResourcesMain::SetControlState(float vx, float vy, float vz, float vrx, float vry, float vrz)
{
    m_directControlState.vx = vx;
    m_directControlState.vy = vy;
    m_directControlState.vz = vz;
    m_directControlState.vrx = vrx;
    m_directControlState.vry = vry;
    m_directControlState.vrz = vrz;
    m_lastInputWasDirect = true;
}

void TiledResourcesMain::RotateSun(float x, float y)
{
    m_terrainRenderer->RotateSun(x, y);
}

void TiledResourcesMain::ChangeScale(float d)
{
    m_terrainRenderer->ChangeScale(d);
}
