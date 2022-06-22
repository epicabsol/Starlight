#include "Starlight.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "d3dx12.h"
#include <d3d12.h>

#include "UVVertexShader.h"
#include "PointLightPixelShader.h"

#include "LoadedMesh.hpp"

// Three descriptors per mesh - constant buffer, diffuse texture, NormalRoughnessMetallic texture
#define DESCRIPTORS_PER_MESH (3)

inline unsigned int Align(unsigned int size, unsigned int alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

Starlight::Starlight(ID3D12Device* device, const MeshFormat& meshFormat) : _Device(device), _MeshFormat(meshFormat)
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

	// Pipeline State for point lights
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* signatureErrors = nullptr;
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureErrors);
	this->_Device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&this->_RootSignature));
	D3D12_INPUT_ELEMENT_DESC meshInputElements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, meshFormat.PositionOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, meshFormat.NormalOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, meshFormat.TexCoordOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, meshFormat.LightmapCoordOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { meshInputElements, sizeof(meshInputElements) / sizeof(meshInputElements[0]) };
	psoDesc.pRootSignature = this->_RootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(Shaders::UVVertexShaderBytes, Shaders::UVVertexShaderLength);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(Shaders::PointLightPixelShaderBytes, Shaders::PointLightPixelShaderLength);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, false, 0, 0.0f, 0.0f, false, false, false, 0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	this->_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&this->_PointLightPipelineState));

	this->_MeshCount = 0;
	this->_Meshes = new LoadedMesh[STARLIGHT_MAX_MESHES];
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
	this->TransitionResource(this->_LightmaskBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

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

ID3D12Resource* Starlight::CreateBuffer(size_t size, const char* data)
{
	D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	return this->CreateResource(&bufferDesc, 1, size, data);
}

ID3D12Resource* Starlight::CreateTexture(size_t width, size_t height, size_t stride, DXGI_FORMAT format, const char* data)
{
	D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0);
	return this->CreateResource(&textureDesc, stride, stride * height, data);
}

ID3D12Resource* Starlight::CreateResource(const D3D12_RESOURCE_DESC* description, size_t stride, size_t dataSize, const char* data)
{
	assert(description != nullptr);
	assert(data != nullptr);

	// Creating a new upload buffer each time this method is called isn't ideal, but it's not going to have any performance impact compared to
	// the most expensive things we're doing (raytracing lol)
	size_t uploadStride = description->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER ? 1 : Align(stride, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	size_t uploadSize = uploadStride * (dataSize / stride);

	// Create upload buffer
	D3D12_HEAP_PROPERTIES uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
	ID3D12Resource* uploadBuffer = nullptr;
	this->_Device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));

	// Create actual buffer
	D3D12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ID3D12Resource* buffer = nullptr;
	this->_Device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&buffer));

	// Copy data to the upload buffer
	D3D12_RANGE emptyRange = CD3DX12_RANGE(0, 0);
	char* uploadBytes = nullptr;
	uploadBuffer->Map(0, &emptyRange, (void**)&uploadBytes);
	if (description->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || stride == uploadStride)
	{
		memcpy_s(uploadBytes, uploadSize, data, dataSize);
	}
	else
	{
		for (size_t row = 0; row < dataSize / stride; row++)
		{
			memcpy_s(uploadBytes + row * uploadStride, uploadStride, data + row * stride, stride);
		}
	}
	uploadBuffer->Unmap(0, nullptr);

	// Copy data from the upload buffer to the final buffer
	if (description->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		this->_CommandList->CopyResource(buffer, uploadBuffer);
	}
	else
	{
		D3D12_TEXTURE_COPY_LOCATION destination = CD3DX12_TEXTURE_COPY_LOCATION(buffer, 0);
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT sourceFootprint = { 0, { description->Format, description->Width, description->Height, 1, uploadStride } };
		D3D12_TEXTURE_COPY_LOCATION source = CD3DX12_TEXTURE_COPY_LOCATION(uploadBuffer, sourceFootprint);
		this->_CommandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
	}
	this->ExecuteAndReset();

	uploadBuffer->Release();

	return buffer;
}

Starlight::~Starlight()
{
	for (size_t i = 0; i < this->_MeshCount; i++)
	{
		this->_Meshes[i].VertexBuffer->Release();
		this->_Meshes[i].VertexBuffer = nullptr;
		this->_Meshes[i].IndexBuffer->Release();
		this->_Meshes[i].IndexBuffer = nullptr;
		this->_Meshes[i].ConstantBuffer->Release();
		this->_Meshes[i].ConstantBuffer = nullptr;
		this->_Meshes[i].DiffuseTexture->Release();
		this->_Meshes[i].DiffuseTexture = nullptr;
		this->_Meshes[i].NormalRoughnessMetallicTexture->Release();
		this->_Meshes[i].NormalRoughnessMetallicTexture = nullptr;
	}

	this->_PointLightPipelineState->Release();
	this->_PointLightPipelineState = nullptr;

	this->_RootSignature->Release();
	this->_RootSignature = nullptr;

	this->_MeshDescriptorHeap->Release();
	this->_MeshDescriptorHeap = nullptr;

	this->_RTVDescriptorHeap->Release();
	this->_RTVDescriptorHeap = nullptr;

	this->_Fence->Release();
	this->_Fence = nullptr;

	this->_CommandList->Release();
	this->_CommandList = nullptr;

	this->_CommandAllocator->Release();
	this->_CommandAllocator = nullptr;

	this->_CommandQueue->Release();
	this->_CommandQueue = nullptr;

	this->_Device->Release();
	this->_Device = nullptr;
}

void Starlight::AddMesh(const MeshInfo* meshInfo, const MaterialInfo* materialInfo)
{
	assert(this->_MeshCount < STARLIGHT_MAX_MESHES);
	assert(meshInfo != nullptr);
	assert(materialInfo != nullptr);
	assert(meshInfo->VertexSize == this->_MeshFormat.VertexSize);

	// Pick the correct DXGI format for the indices
	DXGI_FORMAT indexFormat = DXGI_FORMAT_UNKNOWN;
	/*if (meshInfo->IndexSize == 1)
	{
		indexFormat = DXGI_FORMAT_R8_UINT;
	}
	else*/ if (meshInfo->IndexSize == 2)
	{
		indexFormat = DXGI_FORMAT_R16_UINT;
	}
	else if (meshInfo->IndexSize == 4)
	{
		indexFormat = DXGI_FORMAT_R32_UINT;
	}
	assert(indexFormat != DXGI_FORMAT_UNKNOWN);

	LoadedMesh* mesh = &this->_Meshes[this->_MeshCount];

	// Vertices
	mesh->VertexBuffer = this->CreateBuffer(meshInfo->VertexSize * meshInfo->VertexCount, (const char*)meshInfo->VertexData);
	mesh->VertexSize = meshInfo->VertexSize;
	mesh->VertexCount = meshInfo->VertexCount;
	
	// Indices
	mesh->IndexBuffer = this->CreateBuffer(meshInfo->IndexSize * meshInfo->IndexCount, (const char*)meshInfo->IndexData);
	mesh->IndexFormat = indexFormat;
	mesh->IndexCount = meshInfo->IndexCount;

	// Constants
	MeshConstants constants = {
		meshInfo->Transform,
		materialInfo->DiffuseTint,
	};
	mesh->ConstantBuffer = this->CreateBuffer(sizeof(constants), (const char*)&constants);

	// Textures
	mesh->DiffuseTexture = this->CreateTexture(materialInfo->DiffuseTexture.Width, materialInfo->DiffuseTexture.Height, materialInfo->DiffuseTexture.Stride, DXGI_FORMAT_R8G8B8A8_UNORM, (const char*)materialInfo->DiffuseTexture.PixelData);
	mesh->NormalRoughnessMetallicTexture = this->CreateTexture(materialInfo->NormalRoughnessMetallicTexture.Width, materialInfo->NormalRoughnessMetallicTexture.Height, materialInfo->NormalRoughnessMetallicTexture.Stride, DXGI_FORMAT_R8G8B8A8_UNORM, (const char*)materialInfo->NormalRoughnessMetallicTexture.PixelData);

	this->_MeshCount += 1;
}

void Starlight::BakePointLight(const PointLight* pointLight, const OutputBuffer* outputBuffer)
{
	//this->_CommandAllocator->Reset();
	//this->_CommandList->Reset(this->_CommandAllocator, nullptr);

	this->PrepareOutputBuffer(outputBuffer);

	this->_CommandList->SetPipelineState(this->_PointLightPipelineState);
	this->_CommandList->SetGraphicsRootSignature(this->_RootSignature);
	D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)outputBuffer->Width, (float)outputBuffer->Height);
	this->_CommandList->RSSetViewports(1, &viewport);
	D3D12_RECT scissor = { 0, 0, outputBuffer->Width, outputBuffer->Height };
	this->_CommandList->RSSetScissorRects(1, &scissor);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(this->_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, this->_RTVDescriptorSize);
	this->_CommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
	this->_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw each of the meshes
	for (size_t i = 0; i < this->_MeshCount; i++)
	{
		D3D12_VERTEX_BUFFER_VIEW vertexBuffer = { this->_Meshes[i].VertexBuffer->GetGPUVirtualAddress(), this->_Meshes[i].VertexCount * this->GetMeshFormat().VertexSize, this->GetMeshFormat().VertexSize };
		this->_CommandList->IASetVertexBuffers(0, 1, &vertexBuffer);
		D3D12_INDEX_BUFFER_VIEW indexBuffer = { this->_Meshes[i].IndexBuffer->GetGPUVirtualAddress(), this->_Meshes[i].IndexCount * this->GetMeshFormat().IndexSize, this->_Meshes[i].IndexFormat };
		this->_CommandList->IASetIndexBuffer(&indexBuffer);
		this->_CommandList->DrawIndexedInstanced(this->_Meshes[i].IndexCount, 1, 0, 0, 0);
	}
	this->ExecuteAndReset();

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

Starlight* Starlight::CreateStarlight(ID3D12Device* device, const MeshFormat& meshFormat)
{
	// TODO: Implement!

	return new Starlight(device, meshFormat);
}

