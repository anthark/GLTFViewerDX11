#include "pch.h"

#include "Camera.h"

Camera::Camera()
{
    m_eye = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    m_viewDir = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    m_up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
}

void Camera::MoveDirection(float delta)
{
    m_eye = DirectX::XMVectorAdd(m_eye, DirectX::XMVectorScale(m_viewDir, delta));
}

void Camera::MovePerpendicular(float delta)
{
    m_eye = DirectX::XMVectorAdd(m_eye, DirectX::XMVectorScale(GetPerpendicular(), delta));
}

void Camera::Rotate(float horisontalAngle, float verticalAngle)
{
    m_viewDir = DirectX::XMVector3Rotate(m_viewDir, DirectX::XMQuaternionRotationAxis(m_up, -horisontalAngle));
    DirectX::XMVECTOR perpendicular = DirectX::XMVectorScale(GetPerpendicular(), -1.0f);
    m_viewDir = DirectX::XMVector3Rotate(m_viewDir, DirectX::XMQuaternionRotationAxis(perpendicular, -verticalAngle));
    m_up = DirectX::XMVector3Rotate(m_up, DirectX::XMQuaternionRotationAxis(perpendicular, -verticalAngle));
}

DirectX::XMVECTOR Camera::GetPerpendicular() const
{
    return DirectX::XMVector4Normalize(DirectX::XMVector3Cross(m_viewDir, m_up));
}

DirectX::XMMATRIX Camera::GetViewMatrix() const
{
    return DirectX::XMMatrixLookAtLH(m_eye, DirectX::XMVectorAdd(m_eye, m_viewDir), m_up);
}

Camera::~Camera()
{}
