#pragma once

#include "DeviceResources.h"

namespace TiledResources
{
    class Extras
    {
    public:
        Extras(const std::shared_ptr<DeviceResources>& deviceResources);
        void RenderAtmosphere();
    private:
        std::shared_ptr<DeviceResources> m_deviceResources;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_quadVB;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_quadIB;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_quadIL;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_quadVS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_quadPS;
    };
}
