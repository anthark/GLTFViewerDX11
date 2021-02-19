#pragma once

struct WorldViewProjectionConstantBuffer
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Projection;
};

struct VertexPosition
{
	DirectX::XMFLOAT3 Pos;
};
