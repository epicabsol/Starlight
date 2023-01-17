#pragma once

#include <stdint.h>

struct ID3D11Device;
struct ID3D11ShaderResourceView;

enum ShowcaseIcon
{
	ShowcaseIcon_ToolMove,
	ShowcaseIcon_ToolRotate,
	ShowcaseIcon_ToolSelect,

	ShowcaseIcon_COUNT,
};

class ShowcaseResources
{
private:
	ID3D11ShaderResourceView* _IconSRVs[ShowcaseIcon_COUNT];

	ID3D11ShaderResourceView* LoadTexture(ID3D11Device* device, const uint8_t* fileBytes, size_t fileLength);

public:
	ShowcaseResources(ID3D11Device* device);
	~ShowcaseResources();

	inline ID3D11ShaderResourceView* GetIconSRV(ShowcaseIcon icon) const { return this->_IconSRVs[icon]; }
};