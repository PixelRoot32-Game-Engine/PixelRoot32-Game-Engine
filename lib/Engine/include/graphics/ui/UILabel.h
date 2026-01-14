#pragma once
#include "Config.h"
#include "UIElement.h"
#include <string>

namespace UI {
    class UILabel : public UIElement {
    public:
        UILabel(std::string t, float x, float y, uint16_t col, uint8_t sz);

        void setText(const std::string& t);
        void setVisible(bool v) { isVisible = v; }
        void centerX(int screenWidth);

        void update(unsigned long deltaTime) override;
        void draw(Renderer& renderer) override;
    
    private:
        std::string text;
        uint16_t color;
        uint8_t size;
        bool dirty = false;

        inline void recalcSize() {
            this->width  = (float)(text.length() * (6 * size));
            this->height = (float)(8 * size);
        }
    };
}