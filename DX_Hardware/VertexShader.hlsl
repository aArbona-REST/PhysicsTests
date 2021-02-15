struct INPUT_VERTEX
{
	float4 coordinate : POSITION;
	float4 colorin : COLOR;
	float2 uv : UV;
	float4 normal : NORMAL;
};

struct OUTPUT_VERTEX
{
	float4 colorout : COLOR;
	float4 projectedcoordinate : SV_POSITION;
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
	//matrix tlocal;
};

OUTPUT_VERTEX main( INPUT_VERTEX fromVertexBuffer )
{
	OUTPUT_VERTEX sendToRasterizer = (OUTPUT_VERTEX)0;

	//matrix local = mul(tlocal, tworld);

	//sendToRasterizer.projectedcoordinate = mul(fromVertexBuffer.coordinate, local);
	sendToRasterizer.projectedcoordinate = mul(fromVertexBuffer.coordinate, tworld);
	sendToRasterizer.projectedcoordinate = mul(sendToRasterizer.projectedcoordinate, clocal);
	sendToRasterizer.projectedcoordinate = mul(sendToRasterizer.projectedcoordinate, cprojection);

	sendToRasterizer.colorout = fromVertexBuffer.colorin;

	return sendToRasterizer;
}