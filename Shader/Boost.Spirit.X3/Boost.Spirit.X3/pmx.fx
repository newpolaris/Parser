/*
Program source code are licensed under the zlib license, except source code of external library.

Zerogram Sample Program
http://zerogram.info/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.*
*/

#include "cbuff.h"
#include "shadow.h"

// 変数
Texture2D txDiffuse : register(t0);
Texture2D txToon : register(t1);
Texture2D txSphereMap : register(t2);

// サンプラーステート
SamplerState samLinear : register(s0)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};
SamplerState samToon : register(s1)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
};
SamplerState samSphereMap : register(s2)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

//-------------
struct VS_INPUT
{
	float4 Pos : POSITION;
	float3 Nor : NORMAL;
	float2 Tex : TEXCOORD0;
	uint4 Bidx : BONEINDEX;
	uint4 Bwgt : BONEWEIGHT;
};

//-------------
struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
	float3 Nor : NORMAL;
	
	float4 ShadowPos : SHADOW_POS;
};
// 関数

//-------------------------
PS_INPUT vsBasic( VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = BoneSkinning(input.Pos,input.Bidx,input.Bwgt);
	float3 nor = BoneSkinningNormal(input.Nor,input.Bidx,input.Bwgt);
	nor = normalize(nor);
	
	matrix world = mul(Object.mtxWorld, Model.mtxModel);
	pos = mul( pos, world );
	output.ShadowPos = mul(pos, Scene.mtxShadow);

	pos = mul( pos, Scene.mtxView );
	output.Pos = mul( pos, Scene.mtxProj );
	output.Tex = input.Tex;
	output.Nor = mul( nor, (float3x3)world );//とりあえずこれで
	return output;
}


//----------------------------
float4 psBasic( PS_INPUT input ) : SV_Target
{
	//return Material.aParam[0];
	float3 nor = normalize(input.Nor);
	float l = dot(nor, Light.vDirLight[0].xyz);
	float lighting = DepthShadow(input.ShadowPos,l);
	l = saturate(l)*lighting;

	float3 toon = float3(1,1,1);
	if( Material.aParam[0].y > 0){//トゥーンテクスチャあり
		toon = txToon.Sample( samToon, float2(0,0.5*(1-l))).xyz;
	}
	float4 diff = Material.vDiffuse;
	if( Material.aParam[0].x > 0){//テクスチャあり
		diff = diff*txDiffuse.Sample( samLinear, input.Tex);
	}
	float4 col = float4(toon,1)*diff;
	clip(col.w - 0.001);//Alpha Test

	return col;
}
//----------------------------
void psShadow( PS_INPUT input )
{
	float4 diff = Material.vDiffuse;
	if( Material.aParam[0].x > 0){//テクスチャあり
		diff = diff*txDiffuse.Sample( samLinear, input.Tex);
	}
	clip(diff.w - 0.001);//Alpha Test
}

// ステート定義　※大文字小文字の区別なし

BlendState NoBlend {
	BlendEnable[0] = False;
};
BlendState BlendOn {
	BlendEnable[0] = True;
	BlendOp[0] = ADD;
	SrcBlend[0] = SRC_ALPHA;
	DestBlend[0] = INV_SRC_ALPHA;
	SrcBlendAlpha[0] = ONE;
	DestBlendAlpha[0] = ZERO;
	BlendOpAlpha[0] = ADD;
};
DepthStencilState DepthTestOn {
	DepthEnable = True;	
	DepthWriteMask = All;
	DepthFunc = LESS_EQUAL;
};

RasterizerState RasterSolid {
	FillMode = Solid;
	CullMode = Back;
	FrontCounterClockwise = False;
};
BlendState BlendShadow {
	BlendEnable[0] = False;
	RenderTargetWriteMask[0] = 0;
};
RasterizerState RasterShadow {
	FillMode = Solid;
	CullMode = Back;
	FrontCounterClockwise = False;
};
// シェーダ
VertexShader vs_main = CompileShader( vs_5_0, vsBasic() );
PixelShader ps_main = CompileShader( ps_5_0, psBasic() );
PixelShader ps_shadow = CompileShader( ps_5_0, psShadow() );

// テクニック
technique11 t0 {
	//パス
	pass p0 {
		// ステート設定
		SetBlendState( BlendOn, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthTestOn, 0 );
		SetRasterizerState( RasterSolid );
		
		// シェーダ
		SetVertexShader( vs_main );
		SetPixelShader( ps_main );
	}
}

technique11 shadow_cast {
	//パス
	pass p0 {
		// ステート設定
		SetBlendState( BlendShadow, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthTestOn, 0 );
		SetRasterizerState( RasterShadow );

		// シェーダ
		SetVertexShader( vs_main );
		SetPixelShader( ps_shadow );
	}
}



