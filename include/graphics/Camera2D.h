/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once

namespace pixelroot32::graphics {

class Renderer;

class Camera2D {
public:
    Camera2D(int viewportWidth, int viewportHeight);

    void setBounds(float minX, float maxX);

    void setVerticalBounds(float minY, float maxY);

    void setPosition(float x, float y);

    void followTarget(float targetX);
    void followTarget(float targetX, float targetY);

    float getX() const;
    float getY() const;

    void apply(Renderer& renderer) const;

private:
    float x;
    float y;
    int viewportWidth;
    int viewportHeight;
    float minX;
    float maxX;
    float minY;
    float maxY;
};

} // namespace pixelroot32::graphics
