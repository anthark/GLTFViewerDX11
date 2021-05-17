#pragma once

class Camera 
{
public:
    Camera();
    ~Camera();

    DirectX::XMVECTOR GetPosition() const { return m_eye; };
    DirectX::XMVECTOR GetDirection() const { return m_viewDir; };
    DirectX::XMVECTOR GetUp() const { return m_up; };
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMVECTOR GetPerpendicular() const;

    void MoveDirection(float delta);
    void MovePerpendicular(float delta);
    void Rotate(float horisontalAngle, float verticalAngle);

private:
    DirectX::XMVECTOR m_eye;
    DirectX::XMVECTOR m_viewDir;
    DirectX::XMVECTOR m_up;
};
