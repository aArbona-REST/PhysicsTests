#pragma once
#include <DirectXMath.h>
using namespace DirectX;
class Thing
{
	XMMATRIX Transform;
	Thing* Parent = nullptr;
public:
	Thing(XMMATRIX& Transform, Thing* Parent);
	~Thing();
	XMMATRIX GetWorldPosition();
	XMMATRIX GetLocalPosition();
	XMMATRIX GetTransform();
	void SetTransform(XMMATRIX& Transform);
};
