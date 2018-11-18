struct VS_VERTEX
{
	float4 localcoordinate : POSITION;
	float4 color : COLOR;
	float2 uv : UV;
	float4 normal : NORMAL;
};

struct PS_VERTEX
{
	float4 worldcoordinate : SV_POSITION;
	float4 localcoordinate : POSITION;
	float4 color : COLOR;
};

cbuffer CAMERA : register(b0)
{
	matrix cworld;
	matrix clocal;
	matrix cprojection;
}; 

cbuffer TRANSFORM : register(b1)
{
	matrix tworld;
	matrix tlocal;
};

PS_VERTEX main(VS_VERTEX fromVertexBuffer )
{
	PS_VERTEX sendToRasterizer = (PS_VERTEX)0;
	sendToRasterizer.worldcoordinate = mul(fromVertexBuffer.localcoordinate, mul(tlocal, tworld));
	sendToRasterizer.worldcoordinate = mul(sendToRasterizer.worldcoordinate, clocal);
	sendToRasterizer.worldcoordinate = mul(sendToRasterizer.worldcoordinate, cprojection);
	sendToRasterizer.localcoordinate = fromVertexBuffer.localcoordinate;
	sendToRasterizer.color = fromVertexBuffer.color;
	return sendToRasterizer;


}