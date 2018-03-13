void   simple_vs(
        float4 position : POSITION,
        float2 uv       : TEXCOORD0,

        uniform float4x4 mvp,
        out float4 oPosition : POSITION,
        out float2 oUv       : TEXCOORD0)
{
    oPosition =mul (position, mvp);
    oUv = uv;
}
uniform sampler2D texture0 : register(s0);
float4 simple_ps(float2 texCoord0 : TEXCOORD0): COLOR
{
    return tex2D(texture0, texCoord0);
}
