#include "Thing.h"


Thing::Thing(XMMATRIX& Transform, Thing* Parent)
{
	this->Transform = Transform;
	this->Parent = Parent;
}

Thing::~Thing()
{
}

XMMATRIX Thing::GetWorldPosition()
{
	return XMMATRIX();
}

XMMATRIX Thing::GetLocalPosition()
{
	return XMMATRIX();
}

XMMATRIX Thing::GetTransform()
{
	return this->Transform;
}

void Thing::SetTransform(XMMATRIX& Transform)
{
	this->Transform = Transform;
}