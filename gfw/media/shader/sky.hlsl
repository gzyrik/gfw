void sky_box_vs(
        float4 position : POSITION,
        float3 uv       : TEXCOORD0,

        uniform float4x4 mvp,
        out float4 oPosition : POSITION,
        out float3 oUv       : TEXCOORD0)
{
    oPosition = mul (position, mvp);
    oUv = uv;
}
uniform samplerCUBE texture0 : register(s0);
float4 sky_box_ps(float3 texCoord0 : TEXCOORD0): COLOR
{
    float4 r= texCUBE(texture0, texCoord0);
    return r;
}
