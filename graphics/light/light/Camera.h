#pragma once

class Camera 
{
public:
    Camera();
    ~Camera();

    DirectX::XMVECTOR GetPosition() const { return m_eye; };
    DirectX::XMMATRIX GetViewMatrix() const;

    void MoveDirection(float delta);
    void MovePerpendicular(float delta);
    void Rotate(float horisontalAngle, float verticalAngle);

private:
    DirectX::XMVECTOR GetPerpendicular() const;

    DirectX::XMVECTOR m_eye;
    DirectX::XMVECTOR m_viewDir;
    DirectX::XMVECTOR m_up;
};
