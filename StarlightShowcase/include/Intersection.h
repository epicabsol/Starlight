#pragma once

#include "DirectXMath.h"

bool XM_CALLCONV IntersectRayCapsule(const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, const DirectX::XMVECTOR capsuleStart, const DirectX::XMVECTOR capsuleEnd, float radius, float& t);
bool XM_CALLCONV IntersectRayDisc(const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, const DirectX::XMVECTOR position, const DirectX::XMVECTOR normal, float radius, float thickness, float& t);
DirectX::XMVECTOR XM_CALLCONV ProjectPointOntoAxis(const DirectX::XMVECTOR point, const DirectX::XMVECTOR axisOrigin, const DirectX::XMVECTOR axisDirection);