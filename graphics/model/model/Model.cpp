#include "pch.h"

#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "Model.h"
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#undef TINYGLTF_IMPLEMENTATION

#include "Utils.h"

Model::Model(const char* modelPath) :
    m_modelPath(modelsPath + modelPath)
{};

HRESULT Model::CreateDeviceDependentResources(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    tinygltf::TinyGLTF loader;

    tinygltf::Model model;

    bool ret = loader.LoadASCIIFromFile(&model, nullptr, nullptr, m_modelPath.c_str());
    if (!ret)
        return E_FAIL;

    hr = CreateShaders(device);
    if (FAILED(hr))
        return hr;

    m_pPixelShaders.resize(32);

    hr = CreateTextures(device, model);
    if (FAILED(hr))
        return hr;

    hr = CreateSamplerState(device, model);
    if (FAILED(hr))
        return hr;

    hr = CreateMaterials(device, model);
    if (FAILED(hr))
        return hr;

    hr = CreatePrimitives(device, model);

    return hr;
}

HRESULT Model::CreateTextures(ID3D11Device* device, tinygltf::Model& model)
{
    // All images have 8 bits per channel and 4 components
    HRESULT hr = S_OK;

    for (tinygltf::Image& gltfImage : model.images)
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        CD3D11_TEXTURE2D_DESC td(DXGI_FORMAT_R8G8B8A8_UNORM, gltfImage.width, gltfImage.height, 1, 1, D3D11_BIND_SHADER_RESOURCE);
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = gltfImage.image.data();
        initData.SysMemPitch = 4 * gltfImage.width;
        hr = device->CreateTexture2D(&td, &initData, &texture);
        if (FAILED(hr))
            return hr;
        m_pTextures.push_back(texture);

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResource;
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvd(D3D11_SRV_DIMENSION_TEXTURE2D, td.Format);
        hr = device->CreateShaderResourceView(texture.Get(), &srvd, &shaderResource);
        if (FAILED(hr))
            return hr;
        m_pShaderResourceViews.push_back(shaderResource);
    }

    return hr;
}

D3D11_TEXTURE_ADDRESS_MODE GetTextureAddressMode(int wrap)
{
    switch (wrap)
    {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return D3D11_TEXTURE_ADDRESS_MIRROR;
    default:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    }
}

HRESULT Model::CreateSamplerState(ID3D11Device* device, tinygltf::Model& model)
{
    // In model only one sampler is used
    HRESULT hr = S_OK;

    tinygltf::Sampler& gltfSampler = model.samplers[0];
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    
    switch (gltfSampler.minFilter)
    {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        else
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else
            sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        else
            sd.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else
            sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        else
            sd.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            sd.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        else
            sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    default:
        sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    }

    sd.AddressU = GetTextureAddressMode(gltfSampler.wrapS);
    sd.AddressV = GetTextureAddressMode(gltfSampler.wrapT);
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    hr = device->CreateSamplerState(&sd, &m_pSamplerState);

    return hr;
}

HRESULT Model::CreateMaterials(ID3D11Device* device, tinygltf::Model& model)
{
    HRESULT hr = S_OK;

    for (tinygltf::Material& gltfMaterial : model.materials)
    {
        Material material = {};
        material.name = gltfMaterial.name;
        material.blend = false;

        D3D11_BLEND_DESC bd = {};
        // All materials have alpha mode "BLEND" or "OPAQUE"
        if (gltfMaterial.alphaMode == "BLEND")
        {
            material.blend = true;
            bd.RenderTarget[0].BlendEnable = true;
            bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            hr = device->CreateBlendState(&bd, &material.pBlendState);
            if (FAILED(hr))
                return hr;
        }

        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        if (gltfMaterial.doubleSided)
            rd.CullMode = D3D11_CULL_NONE;
        else
            rd.CullMode = D3D11_CULL_BACK;
        rd.FrontCounterClockwise = true;
        rd.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
        rd.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
        rd.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        hr = device->CreateRasterizerState(&rd, &material.pRasterizerState);
        if (FAILED(hr))
            return hr;

        float albedo[4];
        for (UINT i = 0; i < 4; ++i)
            albedo[i] = static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[i]);
        material.materialBufferData.Albedo = DirectX::XMFLOAT4(albedo);
        material.materialBufferData.Metalness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
        material.materialBufferData.Roughness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

        material.pixelShaderDefinesFlags = 0;

        material.baseColorTexture = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
        if (material.baseColorTexture >= 0)
            material.pixelShaderDefinesFlags |= MATERIAL_HAS_COLOR_TEXTURE;

        material.metallicRoughnessTexture = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (material.metallicRoughnessTexture >= 0)
            material.pixelShaderDefinesFlags |= MATERIAL_HAS_METAL_ROUGH_TEXTURE;

        material.normalTexture = gltfMaterial.normalTexture.index;
        if (material.normalTexture >= 0)
            material.pixelShaderDefinesFlags |= MATERIAL_HAS_NORMAL_TEXTURE;

        if (gltfMaterial.occlusionTexture.index >= 0)
            material.pixelShaderDefinesFlags |= MATERIAL_HAS_OCCLUSION_TEXTURE;

        if (gltfMaterial.doubleSided)
            material.pixelShaderDefinesFlags |= MATERIAL_DOUBLE_SIDED;

        hr = CreatePixelShader(device, material.pixelShaderDefinesFlags);
        if (FAILED(hr))
            return hr;

        material.emissiveTexture = gltfMaterial.emissiveTexture.index;

        m_materials.push_back(material);
    }

    return hr;
}

DirectX::XMMATRIX GetMatrixFromNode(tinygltf::Node& gltfNode)
{
    if (gltfNode.matrix.empty())
    {
        DirectX::XMMATRIX rotation;
        if (gltfNode.rotation.empty())
            rotation = DirectX::XMMatrixIdentity();
        else
        {
            float v[4] = {};
            float* p = v;
            for (double value : gltfNode.rotation)
            {
                *p = static_cast<float>(value);
                ++p;
            }
            DirectX::XMFLOAT4 vector(v);
            rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&vector));
        }
        return rotation;
    }
    else
    {
        float flat[16] = {};
        float* p = flat;
        for (double value : gltfNode.matrix)
        {
            *p = static_cast<float>(value);
            ++p;
        }
        DirectX::XMFLOAT4X4 matrix(flat);
        return DirectX::XMLoadFloat4x4(&matrix);
    }
}

HRESULT Model::CreatePrimitive(ID3D11Device* device, tinygltf::Model& model, tinygltf::Primitive& gltfPrimitive, UINT matrix)
{
    HRESULT hr = S_OK;

    Primitive primitive = {};
    primitive.matrix = matrix;

    for (std::pair<const std::string, int>& item : gltfPrimitive.attributes)
    {
        if (item.first == "TEXCOORD_1") // It isn't used in model
            continue;

        tinygltf::Accessor& gltfAccessor = model.accessors[item.second];
        tinygltf::BufferView& gltfBufferView = model.bufferViews[gltfAccessor.bufferView];
        tinygltf::Buffer& gltfBuffer = model.buffers[gltfBufferView.buffer];

        Attribute attribute = {};
        attribute.byteStride = static_cast<UINT>(gltfAccessor.ByteStride(gltfBufferView));

        CD3D11_BUFFER_DESC vbd(attribute.byteStride * static_cast<UINT>(gltfAccessor.count), D3D11_BIND_VERTEX_BUFFER);
        vbd.StructureByteStride = attribute.byteStride;
        D3D11_SUBRESOURCE_DATA initData;
        ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
        initData.pSysMem = &gltfBuffer.data[gltfBufferView.byteOffset + gltfAccessor.byteOffset];
        hr = device->CreateBuffer(&vbd, &initData, &attribute.pVertexBuffer);
        if (FAILED(hr))
            return hr;

        primitive.attributes.push_back(attribute);

        if (item.first == "POSITION")
        {
            primitive.vertexCount = static_cast<UINT>(gltfAccessor.count);
            
            DirectX::XMFLOAT3 maxPosition(static_cast<float>(gltfAccessor.maxValues[0]), static_cast<float>(gltfAccessor.maxValues[1]), static_cast<float>(gltfAccessor.maxValues[2]));
            DirectX::XMFLOAT3 minPosition(static_cast<float>(gltfAccessor.minValues[0]), static_cast<float>(gltfAccessor.minValues[1]), static_cast<float>(gltfAccessor.minValues[2]));

            primitive.max = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&maxPosition), m_worldMatricies[primitive.matrix]);
            primitive.min = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&minPosition), m_worldMatricies[primitive.matrix]);
        }
    }

    switch (gltfPrimitive.mode)
    {
    case TINYGLTF_MODE_POINTS:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        break;
    case TINYGLTF_MODE_LINE:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        break;
    case TINYGLTF_MODE_LINE_STRIP:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        break;
    case TINYGLTF_MODE_TRIANGLES:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        break;
    case TINYGLTF_MODE_TRIANGLE_STRIP:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        break;
    }

    tinygltf::Accessor& gltfAccessor = model.accessors[gltfPrimitive.indices];
    tinygltf::BufferView& gltfBufferView = model.bufferViews[gltfAccessor.bufferView];
    tinygltf::Buffer& gltfBuffer = model.buffers[gltfBufferView.buffer];

    primitive.indexCount = static_cast<uint32_t>(gltfAccessor.count);
    UINT stride = 2;
    switch (gltfAccessor.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        primitive.indexFormat = DXGI_FORMAT_R8_UINT;
        stride = 1;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        primitive.indexFormat = DXGI_FORMAT_R16_UINT;
        stride = 2;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        primitive.indexFormat = DXGI_FORMAT_R32_UINT;
        stride = 4;
        break;
    }

    CD3D11_BUFFER_DESC ibd(stride * primitive.indexCount, D3D11_BIND_INDEX_BUFFER);
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
    initData.pSysMem = &gltfBuffer.data[gltfBufferView.byteOffset + gltfAccessor.byteOffset];
    hr = device->CreateBuffer(&ibd, &initData, &primitive.pIndexBuffer);
    if (FAILED(hr))
        return hr;

    primitive.material = gltfPrimitive.material;
    if (m_materials[primitive.material].blend)
    {
        m_transparentPrimitives.push_back(primitive);
        if (m_materials[primitive.material].emissiveTexture >= 0)
            m_emissiveTransparentPrimitives.push_back(primitive);
    }
    else
    {
        m_primitives.push_back(primitive);
        if (m_materials[primitive.material].emissiveTexture >= 0)
            m_emissivePrimitives.push_back(primitive);
    }

    return hr;
}

HRESULT Model::ProcessNode(ID3D11Device* device, tinygltf::Model& model, int node, DirectX::XMMATRIX worldMatrix)
{
    HRESULT hr = S_OK;

    tinygltf::Node& gltfNode = model.nodes[node];

    if (gltfNode.mesh >= 0)
    {
        tinygltf::Mesh& gltfMesh = model.meshes[gltfNode.mesh];
        UINT matrixIndex = static_cast<UINT>(m_worldMatricies.size() - 1);

        for (tinygltf::Primitive& gltfPrimitive : gltfMesh.primitives)
        {
            // Node with mesh can has matrix but it isn't our case
            hr = CreatePrimitive(device, model, gltfPrimitive, matrixIndex);
            if (FAILED(hr))
                return hr;
        }
    }
    
    if (gltfNode.children.size() > 0)
    {
        worldMatrix = DirectX::XMMatrixMultiplyTranspose(DirectX::XMMatrixTranspose(worldMatrix), DirectX::XMMatrixTranspose(GetMatrixFromNode(gltfNode)));
        m_worldMatricies.push_back(worldMatrix);
        for (int childNode : gltfNode.children)
        {
            hr = ProcessNode(device, model, childNode, worldMatrix);
            if (FAILED(hr))
                return hr;
        }
    }

    return hr;
}

HRESULT Model::CreatePrimitives(ID3D11Device* device, tinygltf::Model& model)
{
    HRESULT hr = S_OK;

    tinygltf::Scene& gltfScene = model.scenes[model.defaultScene];
    
    m_worldMatricies.push_back(DirectX::XMMatrixIdentity());
    
    for (int node : gltfScene.nodes)
    {
        hr = ProcessNode(device, model, node, m_worldMatricies[0]);
        if (FAILED(hr))
            return hr;
    }

    return hr;
}

HRESULT Model::CreateShaders(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    
    std::vector<D3D_SHADER_MACRO> defines;
    defines.push_back({ "HAS_TANGENT", "1" });
    defines.push_back({ nullptr, nullptr });

#ifdef ENV64
    hr = CompileShaderFromFile(L"../../model/PBRShaders.fx", "vs_main", "vs_5_0", &blob, defines.data());
#else
    hr = CompileShaderFromFile(L"../model/PBRShaders.fx", "vs_main", "vs_5_0", &blob, defines);
#endif
    if (FAILED(hr))
        return hr;

    hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_pVertexShader);
    if (FAILED(hr))
        return hr;

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD_", 0, DXGI_FORMAT_R32G32_FLOAT, 3, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), blob->GetBufferPointer(), blob->GetBufferSize(), &m_pInputLayout);
    if (FAILED(hr))
        return hr;

    defines.resize(1);
    defines.push_back({ "HAS_EMISSIVE", "1" });
    defines.push_back({ nullptr, nullptr });

#ifdef ENV64
    hr = CompileShaderFromFile(L"../../model/PBRShaders.fx", "ps_main", "ps_5_0", &blob, defines.data());
#else
    hr = CompileShaderFromFile(L"../model/PBRShaders.fx", "ps_main", "ps_5_0", &blob, defines);
#endif
    if (FAILED(hr))
        return hr;

    hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_pEmissivePixelShader);

    return hr;
}
HRESULT Model::CreatePixelShader(ID3D11Device* device, UINT definesFlags)
{
    HRESULT hr = S_OK;

    if (m_pPixelShaders[definesFlags])
        return hr;

    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    std::vector<D3D_SHADER_MACRO> defines;
    defines.push_back({ "HAS_TANGENT", "1" });

    if (definesFlags & MATERIAL_HAS_COLOR_TEXTURE)
        defines.push_back({ "HAS_COLOR_TEXTURE", "1" });

    if (definesFlags & MATERIAL_HAS_METAL_ROUGH_TEXTURE)
        defines.push_back({ "HAS_METAL_ROUGH_TEXTURE", "1" });

    if (definesFlags & MATERIAL_HAS_NORMAL_TEXTURE)
        defines.push_back({ "HAS_NORMAL_TEXTURE", "1" });

    if (definesFlags & MATERIAL_HAS_OCCLUSION_TEXTURE)
        defines.push_back({ "HAS_OCCLUSION_TEXTURE", "1" });

    if (definesFlags & MATERIAL_DOUBLE_SIDED)
        defines.push_back({ "DOUBLE_SIDED", "1" });

    defines.push_back({ nullptr, nullptr });

#ifdef ENV64
    hr = CompileShaderFromFile(L"../../model/PBRShaders.fx", "ps_main", "ps_5_0", &blob, defines.data());
#else
    hr = CompileShaderFromFile(L"../model/PBRShaders.fx", "ps_main", "ps_5_0", &blob, defines);
#endif
    if (FAILED(hr))
        return hr;

    hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_pPixelShaders[definesFlags]);

    return hr;
}

void Model::Render(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots)
{
    context->VSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.materialConstantBufferSlot, 1, &materialConstantBuffer);
    context->PSSetSamplers(slots.samplerStateSlot, 1, m_pSamplerState.GetAddressOf());

    transformationData.World = DirectX::XMMatrixIdentity();
    for (Primitive& primitive : m_primitives)
        RenderPrimitive(primitive, context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots);
}

bool CompareDistancePairs(const std::pair<float, size_t>& p1, const std::pair<float, size_t>& p2)
{
    return p1.first < p2.first;
}

void Model::RenderTransparent(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots, DirectX::XMVECTOR cameraDir)
{
    context->VSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.materialConstantBufferSlot, 1, &materialConstantBuffer);
    context->PSSetSamplers(slots.samplerStateSlot, 1, m_pSamplerState.GetAddressOf());

    transformationData.World = DirectX::XMMatrixIdentity();
    
    std::vector<std::pair<float, size_t>> distances;
    float distance;
    DirectX::XMVECTOR center;
    DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat4(&transformationData.CameraPos);
    for (size_t i = 0; i < m_transparentPrimitives.size(); ++i)
    {
        center = DirectX::XMVectorDivide(DirectX::XMVectorAdd(m_transparentPrimitives[i].max, m_transparentPrimitives[i].min), DirectX::XMVectorReplicate(2));
        distance = DirectX::XMVector3Dot(DirectX::XMVectorSubtract(center, cameraPos), cameraDir).m128_f32[0];
        distances.push_back(std::pair<float, size_t>(distance, i));
    }

    std::sort(distances.begin(), distances.end(), CompareDistancePairs);

    for (auto iter = distances.rbegin(); iter != distances.rend(); ++iter)
        RenderPrimitive(m_transparentPrimitives[(*iter).second], context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots);
}

void Model::RenderPrimitive(Primitive& primitive, ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer& transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots& slots, bool emissive)
{
    std::vector<ID3D11Buffer*> combined;
    std::vector<UINT> offset;
    std::vector<UINT> stride;

    size_t attributesCount = primitive.attributes.size();
    combined.resize(attributesCount);
    offset.resize(attributesCount);
    stride.resize(attributesCount);
    for (size_t i = 0; i < attributesCount; ++i)
    {
        combined[i] = primitive.attributes[i].pVertexBuffer.Get();
        stride[i] = primitive.attributes[i].byteStride;
    }
    context->IASetVertexBuffers(0, static_cast<UINT>(attributesCount), combined.data(), stride.data(), offset.data());

    context->IASetIndexBuffer(primitive.pIndexBuffer.Get(), primitive.indexFormat, 0);
    context->IASetPrimitiveTopology(primitive.primitiveTopology);

    Material& material = m_materials[primitive.material];
    if (material.blend)
        context->OMSetBlendState(material.pBlendState.Get(), nullptr, 0xFFFFFFFF);
    context->RSSetState(material.pRasterizerState.Get());

    context->IASetInputLayout(m_pInputLayout.Get());
    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);

    transformationData.World = DirectX::XMMatrixTranspose(m_worldMatricies[primitive.matrix]);

    context->UpdateSubresource(transformationConstantBuffer, 0, NULL, &transformationData, 0, 0);

    MaterialConstantBuffer materialBufferData = material.materialBufferData;
    if (emissive)
    {
        materialBufferData.Albedo = DirectX::XMFLOAT4(1, 1, 1, materialBufferData.Albedo.w);
        context->PSSetShaderResources(slots.baseColorTextureSlot, 1, m_pShaderResourceViews[material.emissiveTexture].GetAddressOf());
        context->PSSetShader(m_pEmissivePixelShader.Get(), nullptr, 0);
    }
    else
    {
        context->PSSetShader(m_pPixelShaders[material.pixelShaderDefinesFlags].Get(), nullptr, 0);
        if (material.baseColorTexture >= 0)
            context->PSSetShaderResources(slots.baseColorTextureSlot, 1, m_pShaderResourceViews[material.baseColorTexture].GetAddressOf());
        if (material.metallicRoughnessTexture >= 0)
            context->PSSetShaderResources(slots.metallicRoughnessTextureSlot, 1, m_pShaderResourceViews[material.metallicRoughnessTexture].GetAddressOf());
        if (material.normalTexture >= 0)
            context->PSSetShaderResources(slots.normalTextureSlot, 1, m_pShaderResourceViews[material.normalTexture].GetAddressOf());
    }
    context->UpdateSubresource(materialConstantBuffer, 0, NULL, &material.materialBufferData, 0, 0);

    context->DrawIndexed(primitive.indexCount, 0, 0);

    if (material.blend)
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void Model::RenderEmissive(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots)
{
    context->VSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.materialConstantBufferSlot, 1, &materialConstantBuffer);
    context->PSSetSamplers(slots.samplerStateSlot, 1, m_pSamplerState.GetAddressOf());

    transformationData.World = DirectX::XMMatrixIdentity();
    for (Primitive& primitive : m_emissivePrimitives)
        RenderPrimitive(primitive, context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots, true);
}

void Model::RenderEmissiveTransparent(ID3D11DeviceContext* context, WorldViewProjectionConstantBuffer transformationData, ID3D11Buffer* transformationConstantBuffer, ID3D11Buffer* materialConstantBuffer, ShadersSlots slots, DirectX::XMVECTOR cameraDir)
{
    context->VSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.transformationConstantBufferSlot, 1, &transformationConstantBuffer);
    context->PSSetConstantBuffers(slots.materialConstantBufferSlot, 1, &materialConstantBuffer);
    context->PSSetSamplers(slots.samplerStateSlot, 1, m_pSamplerState.GetAddressOf());

    transformationData.World = DirectX::XMMatrixIdentity();

    std::vector<std::pair<float, size_t>> distances;
    float distance;
    DirectX::XMVECTOR center;
    DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat4(&transformationData.CameraPos);
    for (size_t i = 0; i < m_emissiveTransparentPrimitives.size(); ++i)
    {
        center = DirectX::XMVectorDivide(DirectX::XMVectorAdd(m_emissiveTransparentPrimitives[i].max, m_emissiveTransparentPrimitives[i].min), DirectX::XMVectorReplicate(2));
        distance = DirectX::XMVector3Dot(DirectX::XMVectorSubtract(center, cameraPos), cameraDir).m128_f32[0];
        distances.push_back(std::pair<float, size_t>(distance, i));
    }

    std::sort(distances.begin(), distances.end(), CompareDistancePairs);

    for (auto iter = distances.rbegin(); iter != distances.rend(); ++iter)
        RenderPrimitive(m_emissiveTransparentPrimitives[(*iter).second], context, transformationData, transformationConstantBuffer, materialConstantBuffer, slots, true);
}

Model::~Model()
{}
