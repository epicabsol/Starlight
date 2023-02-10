#include "Intersection.h"

using namespace DirectX;

/**
 * Intersection implementations ported from https://iquilezles.org/articles/intersectors/
 */

bool XM_CALLCONV IntersectRayCapsule(const XMVECTOR rayOrigin, const XMVECTOR rayDirection, const XMVECTOR capsuleStart, const XMVECTOR capsuleEnd, float radius, float& t)
{
	XMVECTOR ba = XMVectorSubtract(capsuleEnd, capsuleStart);
	XMVECTOR oa = XMVectorSubtract(rayOrigin, capsuleStart);
	float baba = XMVectorGetX(XMVector3Dot(ba, ba));
	float bard = XMVectorGetX(XMVector3Dot(ba, rayDirection));
	float baoa = XMVectorGetX(XMVector3Dot(ba, oa));
	float rdoa = XMVectorGetX(XMVector3Dot(rayDirection, oa));
	float oaoa = XMVectorGetX(XMVector3Dot(oa, oa));
	float a = baba - bard * bard;
	float b = baba * rdoa - baoa * bard;
	float c = baba * oaoa - baoa * baoa - radius * radius * baba;
	float h = b * b - a * c;
	if (h >= 0.0f)
	{
		t = (-b - sqrtf(h)) / a;
		float y = baoa + t * bard;
		if (y > 0.0f && y < baba)
		{
			// body
			return true;
		}
		else
		{
			// caps
			XMVECTOR oc = (y <= 0.0f) ? oa : XMVectorSubtract(rayOrigin, capsuleEnd);
			b = XMVectorGetX(XMVector3Dot(rayDirection, oc));
			c = XMVectorGetX(XMVector3Dot(oc, oc)) - radius * radius;
			h = b * b - c;
			if (h > 0.0f)
			{
				t = -b - sqrtf(h);
				return true;
			}
			else
			{
				t = -1.0f;
				return false;
			}
		}
	}
	else
	{
		t = -1.0f;
		return false;
	}
}

bool XM_CALLCONV IntersectRayDisc(const XMVECTOR rayOrigin, const XMVECTOR rayDirection, const DirectX::XMVECTOR position, const DirectX::XMVECTOR normal, float radius, float thickness, float& t)
{
	XMVECTOR pointOnPlane = XMPlaneIntersectLine(XMPlaneFromPointNormal(position, normal), rayOrigin, XMVectorAdd(rayOrigin, rayDirection));
	float distanceFromRing = fabs(XMVectorGetX(XMVector3Length(XMVectorSubtract(pointOnPlane, position))) - radius);
	if (distanceFromRing <= thickness)
	{
		t = XMVectorGetX(XMVectorDivide(XMVector3Length(XMVectorSubtract(pointOnPlane, rayOrigin)), XMVector3Length(rayDirection)));
		return true;
	}
	else
	{
		t = -1.0f;
		return false;
	}
}
