#pragma once

#include "ShaderStructures.h"
#include "../../tiny_gltf.h"

#ifdef ENV64
const std::string modelsPath = std::string("../../model/");
#else
const std::string modelsPath = std::string("../model/");
#endif

class Model
{
public:
    struct ShadersSlots
    {
        UINT baseColorTextureSlot;
        UINT metallicRoughnessTextureSlot;
        UINT normalTextureSlot;
        UINT samplerStateSlot;
        UINT transformationConstantBufferSlot;
        UINT materialConstantBufferSlot;
    };

    enum MODEL_PIXEL_SHADER_DEFINES
    {
        MATERIAL_HAS_COLOR_TEXTURE = 0x1,
        MATERIAL_HAS_METAL_ROUGH_TEXTURE = 0x2,
        MATERIAL_HAS_NORMAL_TEXTURE = 0x4,
        MATERIAL_HAS_OCCLUSION_TEXTURE = 0x8
    } MODEL_PIXEL_SHADER_DEFINES;

    Model(const char* modelPath);
    ~Model();

    HRESULT CreateDeviceDependentResources(ID3D11Device* device);    

    void Render(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots);
    void RenderTransparent(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots, DirectX::XMVECTOR cameraDir);

    void RenderEmissive(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots);
    void RenderEmissiveTransparent(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots, DirectX::XMVECTOR cameraDir);

private:
    struct Material
    {
        std::string name;
        bool blend;
        Microsoft::WRL::ComPtr<ID3D11BlendState> pBlendState;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> pRasterizerState;
        MaterialConstantBuffer materialBufferData;
        int baseColorTexture;
        int metallicRoughnessTexture;
        int normalTexture;
        int emissiveTexture;
        UINT pixelShaderDefinesFlags;
    };

    struct Attribute
    {
        UINT byteStride;
        Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
    };

    struct Primitive
    {
        std::vector<Attribute> attributes;
        UINT vertexCount;
        DirectX::XMVECTOR max;
        DirectX::XMVECTOR min;
        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
        DXGI_FORMAT indexFormat;
        Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;
        UINT indexCount;
        UINT material;
        UINT matrix;
    };

    HRESULT CreateTextures(ID3D11Device* device, tinygltf::Model& model);
    HRESULT CreateSamplerState(ID3D11Device* device, tinygltf::Model& model);
    HRESULT CreateMaterials(ID3D11Device* device, tinygltf::Model& model);
    HRESULT CreatePrimitives(ID3D11Device* device, tinygltf::Model& model);
    HRESULT ProcessNode(ID3D11Device* device, tinygltf::Model& model, int node, DirectX::XMMATRIX worldMatrix);
    HRESULT CreatePrimitive(ID3D11Device* device, tinygltf::Model& model, tinygltf::Primitive& gltfPrimitive, UINT matrix);
    HRESULT CreateShaders(ID3D11Device* device);
    HRESULT CreatePixelShader(ID3D11Device* device, UINT definesFlags);
    
    void RenderPrimitive(Primitive& primitive, ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer& transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots& slots, bool emissive=false);

    std::string m_modelPath;

    Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_pInputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pEmissivePixelShader;

    std::vector<Microsoft::WRL::ComPtr<ID3D11PixelShader>>        m_pPixelShaders;
    std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>>          m_pTextures;
    std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceViews;
    
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerState;

    std::vector<Material> m_materials;

    std::vector<DirectX::XMMATRIX> m_worldMatricies;
    
    std::vector<Primitive> m_primitives;
    std::vector<Primitive> m_transparentPrimitives;
    std::vector<Primitive> m_emissivePrimitives;
    std::vector<Primitive> m_emissiveTransparentPrimitives;
};
