#pragma once
#include "Config.h"
#include "UIElement.h"
#include <string>

namespace UI {
    class UILabel : public UIElement {
    private:
        std::string text;
        uint16_t color;
        uint8_t size;

    public:
        UILabel(std::string t, float x, float y, uint16_t col, uint8_t sz);

        void setText(std::string t) { text = t; }
        void setVisible(bool v) { isVisible = v; }
        void centerX(int screenWidth);

        void update(unsigned long deltaTime) override;
        void draw(Renderer& renderer) override;
    };
}