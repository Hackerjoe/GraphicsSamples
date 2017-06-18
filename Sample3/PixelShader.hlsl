cbuffer cbPerObject : register(b0)
{
	float4		g_vObjectColor			: packoffset(c0);
};

cbuffer cbPerFrame : register(b1)
{
	float4		g_vLightDir				: packoffset(c0);
	float4		g_vViewDir				: packoffset(c4);

};

Texture2D	g_txDiffuse : register(t0);
Texture2D	render : register(t1);
SamplerState g_samLinear : register(s0);

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
	float3 viewDir = normalize(g_vViewDir - Input.vPosition);

	float diffuseDot = saturate(dot(norm,lightDir));

	float specularIntensity = 0;

	if (diffuseDot > 0.0)
	{
		float reflectionVector = reflect(-lightDir, norm);
		float specTmp = max(dot(reflectionVector, viewDir), 0.0);
		specularIntensity = saturate(pow(specTmp, 8)*.05);
	}

	return (specularIntensity+ diffuseDot)*render.Sample(g_samLinear,Input.vTexcoord);
}