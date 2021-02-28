#pragma once

#define NUM_LIGHTS 3

struct WorldViewProjectionConstantBuffer
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Projection;
	DirectX::XMVECTOR Eye;
};

struct VertexData
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 Tex;
	DirectX::XMFLOAT3 Normal;
};

struct LightPositionConstantBuffer
{
	DirectX::XMFLOAT4 LightPosition[NUM_LIGHTS];
};

struct LightColorConstantBuffer
{
	DirectX::XMFLOAT4 LightColor[NUM_LIGHTS];
};
