#include "Starlight.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "d3dx12.h"
#include <d3d12.h>

// Three descriptors per mesh - constant buffer, diffuse texture, NormalRoughnessMetallic texture
#define DESCRIPTORS_PER_MESH (3)

inline unsigned int Align(unsigned int size, unsigned int alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

Starlight::Starlight(ID3D12Device* device) : _Device(device)
{
	device->AddRef();
	this->_RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	this->_SRVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&this->_CommandQueue));

	// Command allocator
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->_CommandAllocator));

	// Command list
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->_CommandAllocator, nullptr, IID_PPV_ARGS(&this->_CommandList));

	// Fence and event
	device->CreateFence(this->_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->_Fence));
	this->_FenceValue++;
	this->_FenceEvent = CreateEventW(nullptr, false, false, nullptr);

	// RTV descriptor heap (one single descriptor)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&this->_RTVDescriptorHeap));

	// Mesh descriptor heap (three descriptors per mesh)
	D3D12_DESCRIPTOR_HEAP_DESC meshHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, STARLIGHT_MAX_MESHES * DESCRIPTORS_PER_MESH, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	device->CreateDescriptorHeap(&meshHeapDesc, IID_PPV_ARGS(&this->_MeshDescriptorHeap));
}

void Starlight::PrepareOutputBuffer(const OutputBuffer* outputBuffer)
{
	float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor);

	// Create lightmask buffer
	D3D12_RESOURCE_DESC lightmaskDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, (unsigned int)outputBuffer->Width, (unsigned int)outputBuffer->Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_HEAP_PROPERTIES lightmaskHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	this->_Device->CreateCommittedResource(&lightmaskHeap, D3D12_HEAP_FLAG_NONE, &lightmaskDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue, IID_PPV_ARGS(&this->_LightmaskBuffer));
	this->_LightmaskBuffer->SetName(L"LightmaskBuffer");

	// Put the lightmask descriptor in slot 0 of the RTV descriptor heap
	this->_Device->CreateRenderTargetView(this->_LightmaskBuffer, nullptr, this->_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	
	// Create readback buffer
	//D3D12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, (unsigned int)outputBuffer->Width, (unsigned int)outputBuffer->Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
	D3D12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(outputBuffer->Stride * outputBuffer->Height);
	D3D12_HEAP_PROPERTIES readbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	this->_Device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&this->_ReadbackBuffer));
	this->_ReadbackBuffer->SetName(L"ReadbackBuffer");

	// Clear the lightmask
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)outputBuffer->Width, (float)outputBuffer->Height, 0.0f, 1.0f };
	this->_CommandList->RSSetViewports(1, &viewport);
	this->_CommandList->ClearRenderTargetView(this->_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), clearColor, 0, nullptr);

	this->ExecuteAndReset();
}

void Starlight::CleanupOutputBuffer(const OutputBuffer* outputBuffer)
{
	// TODO: Implement!
	this->TransitionResource(this->_LightmaskBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	//this->_CommandList->CopyResource(this->_ReadbackBuffer, this->_LightmaskBuffer);

	D3D12_TEXTURE_COPY_LOCATION source = CD3DX12_TEXTURE_COPY_LOCATION(this->_LightmaskBuffer, 0);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT destinationFootprint = { 0, { DXGI_FORMAT_R8G8B8A8_UNORM, (unsigned int)outputBuffer->Width, (unsigned int)outputBuffer->Height, 1, (unsigned int)outputBuffer->Stride } };
	D3D12_TEXTURE_COPY_LOCATION destination = CD3DX12_TEXTURE_COPY_LOCATION(this->_ReadbackBuffer, destinationFootprint);
	this->_CommandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

	this->ExecuteAndReset();

	D3D12_RANGE readbackRange = { 0, outputBuffer->Stride * outputBuffer->Height };
	void* readbackData = nullptr;
	this->_ReadbackBuffer->Map(0, &readbackRange, &readbackData);
	memcpy(outputBuffer->PixelData, readbackData, outputBuffer->Stride * outputBuffer->Height);
	D3D12_RANGE emptyRange = { 0, 0 };
	this->_ReadbackBuffer->Unmap(0, &emptyRange);

	this->_ReadbackBuffer->Release();
	this->_ReadbackBuffer = nullptr;

	this->_LightmaskBuffer->Release();
	this->_LightmaskBuffer = nullptr;
}

void Starlight::ExecuteAndReset()
{
	// Execute the command list
	this->_CommandList->Close();
	ID3D12CommandList* commandLists[] = { this->_CommandList };
	this->_CommandQueue->ExecuteCommandLists(sizeof(commandLists) / sizeof(commandLists[0]), commandLists);

	// Wait for the command lists in the queue to be completed
	uint64_t fenceValue = this->_FenceValue;
	this->_CommandQueue->Signal(this->_Fence, fenceValue);
	this->_Fence->SetEventOnCompletion(fenceValue, this->_FenceEvent);
	WaitForSingleObjectEx(this->_FenceEvent, INFINITE, false);
	this->_FenceValue++;

	// Recreate the command list
	this->_CommandAllocator->Reset();
	this->_CommandList->Reset(this->_CommandAllocator, nullptr);
}

void Starlight::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
	this->_CommandList->ResourceBarrier(1, &barrier);
}

Starlight::~Starlight()
{
	this->_MeshDescriptorHeap->Release();
	this->_MeshDescriptorHeap = nullptr;

	this->_RTVDescriptorHeap->Release();
	this->_RTVDescriptorHeap = nullptr;

	this->_CommandAllocator->Release();
	this->_CommandAllocator = nullptr;

	this->_CommandQueue->Release();
	this->_CommandQueue = nullptr;

	this->_Device->Release();
	this->_Device = nullptr;
}

void Starlight::AddMesh(const MeshInfo* meshInfo, const MaterialInfo* materialInfo)
{
	// TODO: Implement!
}

void Starlight::BakePointLight(const PointLight* pointLight, const OutputBuffer* outputBuffer)
{
	//this->_CommandAllocator->Reset();
	//this->_CommandList->Reset(this->_CommandAllocator, nullptr);

	this->PrepareOutputBuffer(outputBuffer);

	// TODO: Implement!

	this->CleanupOutputBuffer(outputBuffer);
}

void Starlight::BakeDirectionalLight(const DirectionalLight* directionalLight, const OutputBuffer* outputBuffer)
{
	// TODO: Implement!
}

void Starlight::BakeSkyLight(const SkyLight* skyLight, const OutputBuffer* outputBuffer)
{
	// TODO: Implement!
}

Starlight* Starlight::CreateStarlight(ID3D12Device* device)
{
	// TODO: Implement!

	return new Starlight(device);
}

