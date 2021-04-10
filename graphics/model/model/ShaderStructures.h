#pragma once

#define NUM_LIGHTS 3

struct WorldViewProjectionConstantBuffer
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Projection;
	DirectX::XMFLOAT4 CameraPos;
};

struct VertexData
{
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 Tex;
};

struct VertexPosData
{
	DirectX::XMFLOAT3 Pos;
};

struct LightPositionConstantBuffer
{
	DirectX::XMFLOAT4 LightPosition[NUM_LIGHTS];
};

struct LightColorConstantBuffer
{
	DirectX::XMFLOAT4 LightColor[NUM_LIGHTS];
};

__declspec(align(16))
struct LuminanceConstantBuffer
{
	float AverageLuminance;
};

__declspec(align(16))
struct MaterialConstantBuffer
{
	DirectX::XMFLOAT4 Albedo;
	float Roughness;
	float Metalness;
};
