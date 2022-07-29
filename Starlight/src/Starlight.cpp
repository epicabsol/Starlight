#include "Starlight.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "d3dx12.h"
#include <d3d12.h>
#include "pix3.h"

#include "UVVertexShader.h"
#include "PointLightPixelShader.h"
#include "DirectionalLightPixelShader.h"

#include "LoadedMesh.hpp"

#define BAKE_RESOURCE_RANGE(Name) Name##s, Last##Name = Name##s + STARLIGHT_MAX_MESHES - 1

enum class BakeResource
{
	PointLightConstantBuffer,
	DirectionalLightConstantBuffer,
	LightmaskBuffers,
	LastLightMaskBuffer = LightmaskBuffers + STARLIGHT_BUFFER_COUNT - 1,
	BAKE_RESOURCE_RANGE(VertexBuffer),
	BAKE_RESOURCE_RANGE(IndexBuffer),
	BAKE_RESOURCE_RANGE(ConstantBuffer),
	Count,
};

enum class MeshResource
{
	VertexBuffer,
	IndexBuffer,
	ConstantBuffer,
	DiffuseTexture,
	NormalRoughnessMetallicTexture,
	Count,
};

struct PointLightConstants
{
	PointLight Light;
	float _Padding;
};

struct DirectionalLightConstants
{
	DirectionalLight Light;
	float _Padding;
};

inline unsigned int Align(unsigned int size, unsigned int alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

Starlight::Starlight(ID3D12Device* device, const MeshFormat& meshFormat) : _MeshFormat(meshFormat)
{
#if _DEBUG
	PIXCaptureParameters capture = PIXCaptureParameters();
	capture.GpuCaptureParameters.FileName = L"capture.wpix";
	PIXBeginCapture(0, &capture);
#endif

	device->QueryInterface<ID3D12Device5>(&this->_Device);
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
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, STARLIGHT_BUFFER_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&this->_RTVDescriptorHeap));
	this->_RTVDescriptorHeap->SetName(L"RTVDescriptorHeap");

	// Resource descriptor heap (one descriptor per BakeResource enum member)
	D3D12_DESCRIPTOR_HEAP_DESC resourceHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, (unsigned int)BakeResource::Count, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
	device->CreateDescriptorHeap(&resourceHeapDesc, IID_PPV_ARGS(&this->_ResourceDescriptorHeap));
	this->_ResourceDescriptorHeap->SetName(L"ResourceDescriptorHeap");

	// Sampler descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
	device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&this->_SamplerDescriptorHeap));
	this->_SamplerDescriptorHeap->SetName(L"SamplerDescriptorHeap");
	D3D12_SAMPLER_DESC samplerDesc = { D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 0, D3D12_COMPARISON_FUNC_ALWAYS, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, 100000.0f };
	device->CreateSampler(&samplerDesc, this->_SamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Allocate the top-level acceleration structure buffer and associated scratch buffer
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = { };
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = STARLIGHT_MAX_MESHES;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = { };
	this->_Device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
	this->_SceneAccelerationStructure = this->CreateUAV(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	this->_SceneBuildScratchBuffer = this->CreateUAV(topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	
	// Allocate and map the instance buffer used to build the top-level acceleration structure
	size_t instanceBufferSize = sizeof(this->_SceneInstanceData[0]) * STARLIGHT_MAX_MESHES;
	D3D12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize);
	D3D12_HEAP_PROPERTIES uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	this->_Device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &instanceBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&this->_SceneInstanceBuffer));
	D3D12_RANGE emptyRange = CD3DX12_RANGE(0, 0);
	this->_SceneInstanceBuffer->Map(0, &emptyRange, (void**)&this->_SceneInstanceData);

	// Constant buffer for point light baking
	this->_PointLightConstantBuffer = this->CreateConstantBuffer(Align(sizeof(PointLightConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
	this->_PointLightConstantBuffer->SetName(L"PointLightConstantBuffer");
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = { this->_PointLightConstantBuffer->GetGPUVirtualAddress(), Align(sizeof(PointLightConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) };
	this->_Device->CreateConstantBufferView(&cbvDesc, this->GetBakeResourceCPU(BakeResource::PointLightConstantBuffer));
	this->_PointLightConstantBuffer->Map(0, &emptyRange, (void**)&this->_PointLightConstantData);

	// Pipeline State for point lights
	CD3DX12_ROOT_PARAMETER rootParameters[6] = { };
	rootParameters[0].InitAsShaderResourceView(0);
	D3D12_DESCRIPTOR_RANGE pointLightConstantsRange = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	rootParameters[1].InitAsDescriptorTable(1, &pointLightConstantsRange);
	D3D12_DESCRIPTOR_RANGE previousIrradianceRange = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	rootParameters[2].InitAsDescriptorTable(1, &previousIrradianceRange);
	D3D12_DESCRIPTOR_RANGE samplerRange = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
	rootParameters[3].InitAsDescriptorTable(1, &samplerRange);
	D3D12_DESCRIPTOR_RANGE meshVerticesRange = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, STARLIGHT_MAX_MESHES, 0, 1);
	rootParameters[4].InitAsDescriptorTable(1, &meshVerticesRange);
	D3D12_DESCRIPTOR_RANGE meshIndicesRange = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, STARLIGHT_MAX_MESHES, 0, 2);
	rootParameters[5].InitAsDescriptorTable(1, &meshIndicesRange);
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(sizeof(rootParameters) / sizeof(rootParameters[0]), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	this->_PointLightRootSignature = this->CreateRootSignature(&rootSignatureDesc);
	
	D3D12_INPUT_ELEMENT_DESC meshInputElements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, meshFormat.PositionOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, meshFormat.NormalOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, meshFormat.TangentOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, meshFormat.TexCoordOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, meshFormat.LightmapCoordOffset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { meshInputElements, sizeof(meshInputElements) / sizeof(meshInputElements[0]) };
	psoDesc.pRootSignature = this->_PointLightRootSignature;
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

	this->_DirectionalLightConstantBuffer = this->CreateConstantBuffer(Align(sizeof(DirectionalLightConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
	this->_DirectionalLightConstantBuffer->SetName(L"DirectionalLightConstantBuffer");
	cbvDesc = { this->_DirectionalLightConstantBuffer->GetGPUVirtualAddress(), Align(sizeof(DirectionalLightConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) };
	this->_Device->CreateConstantBufferView(&cbvDesc, this->GetBakeResourceCPU(BakeResource::DirectionalLightConstantBuffer));
	this->_DirectionalLightConstantBuffer->Map(0, &emptyRange, (void**)&this->_DirectionalLightConstantData);

	// Use the same root signature for directional lights as point lights
	this->_DirectionalLightRootSignature = this->CreateRootSignature(&rootSignatureDesc);
	psoDesc.pRootSignature = this->_DirectionalLightRootSignature;
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(Shaders::DirectionalLightPixelShaderBytes, Shaders::DirectionalLightPixelShaderLength);
	this->_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&this->_DirectionalLightPipelineState));

	this->_MeshCount = 0;
	this->_Meshes = new LoadedMesh[STARLIGHT_MAX_MESHES];
}

void Starlight::PrepareOutputBuffer(const OutputBuffer* outputBuffer)
{
	float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor);
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)outputBuffer->Width, (float)outputBuffer->Height, 0.0f, 1.0f };
	this->_CommandList->RSSetViewports(1, &viewport);

	// TODO: Keep the lightmask buffer around and reuse it if its dimensions and format match

	// Create lightmask buffers
	for (size_t i = 0; i < STARLIGHT_BUFFER_COUNT; i++)
	{
		D3D12_RESOURCE_DESC lightmaskDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, (unsigned int)outputBuffer->Width, (unsigned int)outputBuffer->Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		D3D12_HEAP_PROPERTIES lightmaskHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		this->_Device->CreateCommittedResource(&lightmaskHeap, D3D12_HEAP_FLAG_NONE, &lightmaskDesc, i == 0 ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&this->_LightmaskBuffers[i]));
		
		wchar_t bufferName[255] = { '\0' };
		wsprintfW(bufferName, L"LightMaskBuffer[%u]", (int)i);
		
		this->_LightmaskBuffers[i]->SetName(bufferName);

		// Create the RTV descriptor in slot i of the RTV descriptor heap
		D3D12_CPU_DESCRIPTOR_HANDLE lightmaskRTVDescriptorHandle = { this->_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + this->_RTVDescriptorSize * i };
		this->_Device->CreateRenderTargetView(this->_LightmaskBuffers[i], nullptr, lightmaskRTVDescriptorHandle);

		// Create the SRV descriptor
		D3D12_CPU_DESCRIPTOR_HANDLE lightmaskSRVDescriptorHandle = { this->_ResourceDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + this->_SRVDescriptorSize * ((size_t)BakeResource::LightmaskBuffers + i) };
		this->_Device->CreateShaderResourceView(this->_LightmaskBuffers[i], nullptr, lightmaskSRVDescriptorHandle);
	}
	
	// Create readback buffer
	//D3D12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, (unsigned int)outputBuffer->Width, (unsigned int)outputBuffer->Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
	D3D12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(outputBuffer->Stride * outputBuffer->Height);
	D3D12_HEAP_PROPERTIES readbackHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	this->_Device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&this->_ReadbackBuffer));
	this->_ReadbackBuffer->SetName(L"ReadbackBuffer");

	this->ExecuteAndReset();
}

void Starlight::CleanupOutputBuffer(const OutputBuffer* outputBuffer)
{
	// Download the pixel data from the latest lightmask buffer
	this->TransitionResource(this->_LightmaskBuffers[this->_LatestLightmaskBuffer], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

	D3D12_TEXTURE_COPY_LOCATION source = CD3DX12_TEXTURE_COPY_LOCATION(this->_LightmaskBuffers[this->_LatestLightmaskBuffer], 0);
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

	for (size_t i = 0; i < STARLIGHT_BUFFER_COUNT; i++)
	{
		this->_LightmaskBuffers[i]->Release();
		this->_LightmaskBuffers[i] = nullptr;
	}
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

void Starlight::RebuildTopLevelIfNeeded()
{
	if (!this->_RebuildTopLevel)
	{
		return;
	}

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = { };
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = STARLIGHT_MAX_MESHES;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.InstanceDescs = this->_SceneInstanceBuffer->GetGPUVirtualAddress();
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = { };
	buildDesc.Inputs = topLevelInputs;
	buildDesc.DestAccelerationStructureData = this->_SceneAccelerationStructure->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = this->_SceneBuildScratchBuffer->GetGPUVirtualAddress();
	this->_CommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	this->ExecuteAndReset();

	this->_RebuildTopLevel = false;
}

void Starlight::BakeLight(ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature, BakeResource constantBuffer, const OutputBuffer* outputBuffer)
{
	this->_CommandList->SetPipelineState(pipelineState);
	this->_CommandList->SetGraphicsRootSignature(rootSignature);
	this->_CommandList->SetGraphicsRootShaderResourceView(0, this->_SceneAccelerationStructure->GetGPUVirtualAddress());
	ID3D12DescriptorHeap* descriptorHeaps[] = {
		this->_ResourceDescriptorHeap,
		this->_SamplerDescriptorHeap,
	};
	this->_CommandList->SetDescriptorHeaps(sizeof(descriptorHeaps) / sizeof(descriptorHeaps[0]), descriptorHeaps);
	this->_CommandList->SetGraphicsRootDescriptorTable(1, this->GetBakeResourceGPU(constantBuffer));
	this->_CommandList->SetGraphicsRootDescriptorTable(3, this->_SamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	this->_CommandList->SetGraphicsRootDescriptorTable(4, this->GetMeshResourceGPU(0, MeshResource::VertexBuffer));
	this->_CommandList->SetGraphicsRootDescriptorTable(5, this->GetMeshResourceGPU(0, MeshResource::IndexBuffer));

	D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)outputBuffer->Width, (float)outputBuffer->Height);
	this->_CommandList->RSSetViewports(1, &viewport);
	D3D12_RECT scissor = { 0, 0, outputBuffer->Width, outputBuffer->Height };
	this->_CommandList->RSSetScissorRects(1, &scissor);

	this->_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const size_t bounces = 5;
	for (size_t bounce = 0; bounce < bounces; bounce++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(this->_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (this->_LatestLightmaskBuffer + 1) % STARLIGHT_BUFFER_COUNT, this->_RTVDescriptorSize);
		this->_CommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		this->_CommandList->SetGraphicsRootDescriptorTable(2, this->GetBakeResourceGPU((BakeResource)((size_t)BakeResource::LightmaskBuffers + this->_LatestLightmaskBuffer)));

		// Draw each of the meshes
		for (size_t i = 0; i < this->_MeshCount; i++)
		{
			D3D12_VERTEX_BUFFER_VIEW vertexBuffer = { this->_Meshes[i].VertexBuffer->GetGPUVirtualAddress(), this->_Meshes[i].VertexCount * this->GetMeshFormat().VertexSize, this->GetMeshFormat().VertexSize };
			this->_CommandList->IASetVertexBuffers(0, 1, &vertexBuffer);
			D3D12_INDEX_BUFFER_VIEW indexBuffer = { this->_Meshes[i].IndexBuffer->GetGPUVirtualAddress(), this->_Meshes[i].IndexCount * this->GetMeshFormat().IndexSize, this->_Meshes[i].IndexFormat };
			this->_CommandList->IASetIndexBuffer(&indexBuffer);
			this->_CommandList->DrawIndexedInstanced(this->_Meshes[i].IndexCount, 1, 0, 0, 0);
		}

		D3D12_RESOURCE_BARRIER barriers[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(this->_LightmaskBuffers[(this->_LatestLightmaskBuffer + 1) % STARLIGHT_BUFFER_COUNT], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON),
			CD3DX12_RESOURCE_BARRIER::Transition(this->_LightmaskBuffers[(this->_LatestLightmaskBuffer + 2) % STARLIGHT_BUFFER_COUNT], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET),
		};
		this->_CommandList->ResourceBarrier(sizeof(barriers) / sizeof(barriers[0]), barriers);

		this->_LatestLightmaskBuffer = (this->_LatestLightmaskBuffer + 1) % STARLIGHT_BUFFER_COUNT;
	}

	this->ExecuteAndReset();
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

ID3D12Resource* Starlight::CreateUAV(size_t size, D3D12_RESOURCE_STATES initialState)
{
	D3D12_HEAP_PROPERTIES heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ID3D12Resource* result = nullptr;
	this->_Device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, initialState, nullptr, IID_PPV_ARGS(&result));
	return result;
}

ID3D12Resource* Starlight::CreateConstantBuffer(size_t size)
{
	D3D12_HEAP_PROPERTIES heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ID3D12Resource* result = nullptr;
	this->_Device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&result));
	return result;
}

ID3D12RootSignature* Starlight::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC* desc)
{
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* signatureErrors = nullptr;
	D3D12SerializeRootSignature(desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &signatureErrors);
	ID3D12RootSignature* rootSignature = nullptr;
	this->_Device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (signatureBlob != nullptr)
	{
		signatureBlob->Release();
	}
	if (signatureErrors != nullptr)
	{
		signatureErrors->Release();
	}
	return rootSignature;
}

D3D12_CPU_DESCRIPTOR_HANDLE Starlight::GetBakeResourceCPU(BakeResource resource) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(this->_ResourceDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (unsigned int)resource, this->_SRVDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE Starlight::GetBakeResourceGPU(BakeResource resource) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(this->_ResourceDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), (unsigned int)resource, this->_SRVDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Starlight::GetMeshResourceCPU(size_t mesh, MeshResource resource) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(this->_ResourceDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (unsigned int)BakeResource::VertexBuffers + (unsigned int)resource * STARLIGHT_MAX_MESHES + (unsigned int)mesh, this->_SRVDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE Starlight::GetMeshResourceGPU(size_t mesh, MeshResource resource) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(this->_ResourceDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), (unsigned int)BakeResource::VertexBuffers + (unsigned int)resource * STARLIGHT_MAX_MESHES + (unsigned int)mesh, this->_SRVDescriptorSize);
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
		this->_Meshes[i].AccelerationStructure->Release();
		this->_Meshes[i].AccelerationStructure = nullptr;
	}
	delete[] this->_Meshes;

	this->_DirectionalLightPipelineState->Release();
	this->_DirectionalLightPipelineState = nullptr;

	this->_DirectionalLightRootSignature->Release();
	this->_DirectionalLightRootSignature = nullptr;

	D3D12_RANGE emptyRange = CD3DX12_RANGE(0, 0);
	this->_DirectionalLightConstantBuffer->Unmap(0, &emptyRange);
	this->_DirectionalLightConstantBuffer->Release();
	this->_DirectionalLightConstantBuffer = nullptr;

	this->_PointLightPipelineState->Release();
	this->_PointLightPipelineState = nullptr;

	this->_PointLightRootSignature->Release();
	this->_PointLightRootSignature = nullptr;

	this->_PointLightConstantBuffer->Unmap(0, &emptyRange);
	this->_PointLightConstantBuffer->Release();
	this->_PointLightConstantBuffer = nullptr;

	this->_SceneInstanceBuffer->Unmap(0, &emptyRange);
	this->_SceneInstanceBuffer->Release();
	this->_SceneInstanceBuffer = nullptr;

	this->_SceneAccelerationStructure->Release();
	this->_SceneAccelerationStructure = nullptr;

	this->_SceneBuildScratchBuffer->Release();
	this->_SceneBuildScratchBuffer = nullptr;

	this->_SamplerDescriptorHeap->Release();
	this->_SamplerDescriptorHeap = nullptr;

	this->_ResourceDescriptorHeap->Release();
	this->_ResourceDescriptorHeap = nullptr;

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

#if _DEBUG
	PIXEndCapture(false);
#endif
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
	else*/ /*if (meshInfo->IndexSize == 2)
	{
		indexFormat = DXGI_FORMAT_R16_UINT;
	}
	else*/ if (meshInfo->IndexSize == 4)
	{
		indexFormat = DXGI_FORMAT_R32_UINT;
	}
	assert(indexFormat != DXGI_FORMAT_UNKNOWN);

	LoadedMesh* mesh = &this->_Meshes[this->_MeshCount];

	// Vertices
	mesh->VertexBuffer = this->CreateBuffer(meshInfo->VertexSize * meshInfo->VertexCount, (const char*)meshInfo->VertexData);
	mesh->VertexSize = meshInfo->VertexSize;
	mesh->VertexCount = meshInfo->VertexCount;
	D3D12_SHADER_RESOURCE_VIEW_DESC vbDesc = {
		DXGI_FORMAT_UNKNOWN,
		D3D12_SRV_DIMENSION_BUFFER,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	};
	vbDesc.Buffer = { 0, (unsigned int)meshInfo->VertexCount, (unsigned int)meshInfo->VertexSize, D3D12_BUFFER_SRV_FLAG_NONE };
	this->_Device->CreateShaderResourceView(mesh->VertexBuffer, &vbDesc, this->GetMeshResourceCPU(this->_MeshCount, MeshResource::VertexBuffer));
	
	// Indices
	mesh->IndexBuffer = this->CreateBuffer(meshInfo->IndexSize * meshInfo->IndexCount, (const char*)meshInfo->IndexData);
	mesh->IndexFormat = indexFormat;
	mesh->IndexCount = meshInfo->IndexCount;
	D3D12_SHADER_RESOURCE_VIEW_DESC ibDesc = {
		DXGI_FORMAT_UNKNOWN,
		D3D12_SRV_DIMENSION_BUFFER,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	};
	ibDesc.Buffer = { 0, (unsigned int)meshInfo->IndexCount, (unsigned int)meshInfo->IndexSize, D3D12_BUFFER_SRV_FLAG_NONE };
	this->_Device->CreateShaderResourceView(mesh->IndexBuffer, &ibDesc, this->GetMeshResourceCPU(this->_MeshCount, MeshResource::IndexBuffer));

	// Constants
	MeshConstants constants = {
		meshInfo->Transform,
		materialInfo->DiffuseTint,
	};
	mesh->ConstantBuffer = this->CreateBuffer(sizeof(constants), (const char*)&constants);

	// Textures
	mesh->DiffuseTexture = this->CreateTexture(materialInfo->DiffuseTexture.Width, materialInfo->DiffuseTexture.Height, materialInfo->DiffuseTexture.Stride, DXGI_FORMAT_R8G8B8A8_UNORM, (const char*)materialInfo->DiffuseTexture.PixelData);
	mesh->NormalRoughnessMetallicTexture = this->CreateTexture(materialInfo->NormalRoughnessMetallicTexture.Width, materialInfo->NormalRoughnessMetallicTexture.Height, materialInfo->NormalRoughnessMetallicTexture.Stride, DXGI_FORMAT_R8G8B8A8_UNORM, (const char*)materialInfo->NormalRoughnessMetallicTexture.PixelData);

	// Create bottom-level acceleration structure
	D3D12_RAYTRACING_GEOMETRY_DESC geoDesc = { };
	geoDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geoDesc.Triangles.IndexBuffer = mesh->IndexBuffer->GetGPUVirtualAddress();
	geoDesc.Triangles.IndexCount = mesh->IndexCount;
	geoDesc.Triangles.IndexFormat = mesh->IndexFormat;
	geoDesc.Triangles.Transform3x4 = 0;
	geoDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geoDesc.Triangles.VertexCount = mesh->VertexCount;
	geoDesc.Triangles.VertexBuffer.StartAddress = mesh->VertexBuffer->GetGPUVirtualAddress() + this->GetMeshFormat().PositionOffset;
	geoDesc.Triangles.VertexBuffer.StrideInBytes = mesh->VertexSize;

	// TEMP: Assume geometry is opaque
	geoDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = { };
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = { };
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.Flags = buildFlags;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geoDesc;
	// Determine the size of the acceleration structure and scratch buffers
	this->_Device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
	mesh->AccelerationStructure = this->CreateUAV(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	ID3D12Resource* scratchBuffer = this->CreateUAV(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = { };
	buildDesc.Inputs = bottomLevelInputs;
	buildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = mesh->AccelerationStructure->GetGPUVirtualAddress();
	this->_CommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	this->ExecuteAndReset();

	scratchBuffer->Release();

	// Update the top-level instance buffer and mark as needing rebuilding
	D3D12_RAYTRACING_INSTANCE_DESC* instance = &this->_SceneInstanceData[this->_MeshCount];
	// TODO: Use meshInfo transform
	instance->Transform[0][0] = instance->Transform[1][1] = instance->Transform[2][2] = 1.0f;
	instance->InstanceMask = 1;
	instance->InstanceID = (unsigned int)this->_MeshCount;
	instance->AccelerationStructure = mesh->AccelerationStructure->GetGPUVirtualAddress();
	this->_RebuildTopLevel = true;

	this->_MeshCount += 1;
}

void Starlight::BakePointLight(const PointLight* pointLight, const OutputBuffer* outputBuffer)
{
	this->RebuildTopLevelIfNeeded();

	this->PrepareOutputBuffer(outputBuffer);

	this->_LatestLightmaskBuffer = STARLIGHT_BUFFER_COUNT - 1;
	
	this->_PointLightConstantData->Light = *pointLight;

	this->BakeLight(this->_PointLightPipelineState, this->_PointLightRootSignature, BakeResource::PointLightConstantBuffer, outputBuffer);

	this->CleanupOutputBuffer(outputBuffer);
}

void Starlight::BakeDirectionalLight(const DirectionalLight* directionalLight, const OutputBuffer* outputBuffer)
{
	this->RebuildTopLevelIfNeeded();

	this->PrepareOutputBuffer(outputBuffer);

	this->_LatestLightmaskBuffer = STARLIGHT_BUFFER_COUNT - 1;

	this->_DirectionalLightConstantData->Light = *directionalLight;

	this->BakeLight(this->_DirectionalLightPipelineState, this->_DirectionalLightRootSignature, BakeResource::DirectionalLightConstantBuffer, outputBuffer);

	this->CleanupOutputBuffer(outputBuffer);
}

void Starlight::BakeSkyLight(const SkyLight* skyLight, const OutputBuffer* outputBuffer)
{
	this->RebuildTopLevelIfNeeded();
	// TODO: Implement!
}

Starlight* Starlight::CreateStarlight(ID3D12Device* device, const MeshFormat& meshFormat)
{
	// TODO: Implement!

	return new Starlight(device, meshFormat);
}

