struct PS_VERTEX
{
	float4 worldcoordinate : SV_POSITION;
	float4 localcoordinate : POSITION;
	float4 color : COLOR;
};

cbuffer GALAXYPLANE : register(b0)
{
	float4 planespace;
	float4 planenormal;
};

float4 main(PS_VERTEX input) : SV_TARGET
{
	float4 v = input.localcoordinate - planespace;
	float4 d = dot(v, planenormal);

	return d * input.color;

}