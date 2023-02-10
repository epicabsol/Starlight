#pragma once

#include "DirectXMath.h"

bool XM_CALLCONV IntersectRayCapsule(const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, const DirectX::XMVECTOR capsuleStart, const DirectX::XMVECTOR capsuleEnd, float radius, float& t);
bool XM_CALLCONV IntersectRayDisc(const DirectX::XMVECTOR rayOrigin, const DirectX::XMVECTOR rayDirection, const DirectX::XMVECTOR position, const DirectX::XMVECTOR normal, float radius, float thickness, float& t);

/**
 * Computes the part of `vector` that goes in the direction of `target`.
 */
inline DirectX::XMVECTOR XM_CALLCONV ProjectVectorOntoAxis(const DirectX::XMVECTOR vector, const DirectX::XMVECTOR axisDirection) { return DirectX::XMVectorScale(axisDirection, DirectX::XMVectorGetX(DirectX::XMVectorDivide(DirectX::XMVector3Dot(vector, axisDirection), DirectX::XMVector3Dot(axisDirection, axisDirection)))); }

/**
 * Computes the point on the given axis nearest to `point`.
 */
inline DirectX::XMVECTOR XM_CALLCONV ProjectPointOntoAxis(const DirectX::XMVECTOR point, const DirectX::XMVECTOR axisOrigin, const DirectX::XMVECTOR axisDirection) { return DirectX::XMVectorAdd(axisOrigin, ProjectVectorOntoAxis(DirectX::XMVectorSubtract(point, axisOrigin), axisDirection)); }


inline DirectX::XMVECTOR XM_CALLCONV RemoveVectorAxis(const DirectX::XMVECTOR vector, const DirectX::XMVECTOR axisToRemove) { return DirectX::XMVectorSubtract(vector, ProjectVectorOntoAxis(vector, axisToRemove)); }