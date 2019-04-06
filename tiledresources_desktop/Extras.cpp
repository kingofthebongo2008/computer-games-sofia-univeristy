#include "pch.h"
#include "DirectXHelper.h"
#include "SampleSettings.h"
#include "DeviceResources.h"
#include "Extras.h"

using namespace TiledResources;

using namespace concurrency;
using namespace DirectX;
using namespace Windows::Foundation;

Extras::Extras(const std::shared_ptr<DeviceResources>& deviceResources) :
    m_deviceResources(deviceResources)
{
    auto dev = m_deviceResources->GetD3DDevice();
    auto con = m_deviceResources->GetD3DDeviceContext();


}

void Extras::RenderAtmosphere()
{

}
