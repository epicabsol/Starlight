#pragma once

#include "DirectXMath.h"

bool XM_CALLCONV IntersectRayCapsule(const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, const DirectX::XMVECTOR capsuleStart, const DirectX::XMVECTOR capsuleEnd, float radius, float& t);