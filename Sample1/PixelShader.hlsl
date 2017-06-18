cbuffer cbPerObject : register(b0)
{
	float4		g_vObjectColor			: packoffset(c0);
};

cbuffer cbPerFrame : register(b1)
{
	float3		g_vLightDir				: packoffset(c0);

};


struct PS_INPUT
{
	float3 vNormal		: NORMAL;
	float2 vTexcoord	: TEXCOORD0;
	float4 vPosition	: SV_POSITION;

};


float4 main(PS_INPUT Input) : SV_TARGET
{
	float3 norm = normalize(Input.vNormal);
	float3 lightDir = normalize(g_vLightDir - Input.vPosition);
	float diffuseDot = saturate(dot(norm,lightDir));
	return diffuseDot * g_vObjectColor;
}