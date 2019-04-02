#pragma once

namespace sample
{
    //Helper class that assists us using the descriptors
    struct DescriptorHeapCpuView
    {
        DescriptorHeapCpuView(D3D12_CPU_DESCRIPTOR_HANDLE  base, uint64_t offset) : m_base(base), m_offset(offset)
        {

        }

        D3D12_CPU_DESCRIPTOR_HANDLE operator () (size_t index) const
        {
            return { m_base.ptr + index * m_offset };
        }

        D3D12_CPU_DESCRIPTOR_HANDLE operator + (size_t index) const
        {
            return { m_base.ptr + index * m_offset };
        }

        D3D12_CPU_DESCRIPTOR_HANDLE m_base = {};
        uint64_t                    m_offset;
    };

    DescriptorHeapCpuView CpuView(ID3D12Device* d, ID3D12DescriptorHeap* heap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
        return DescriptorHeapCpuView(heap->GetCPUDescriptorHandleForHeapStart(), d->GetDescriptorHandleIncrementSize(desc.Type));
    }
}

