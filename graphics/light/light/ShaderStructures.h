#pragma once

struct WorldViewProjectionConstantBuffer
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Projection;
};

struct VertexData
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 Tex;
};
