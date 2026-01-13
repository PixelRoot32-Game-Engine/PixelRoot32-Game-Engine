#include "ui/UILabel.h"

namespace UI {
    UILabel::UILabel(std::string t, float x, float y, uint16_t col, uint8_t sz)
        : UIElement(x, y, 0, 0), text(t), color(col), size(sz) {
        
        this->width = (float)(t.length() * (6 * sz));
        this->height = (float)(8 * sz);
    }

    void UILabel::centerX(int screenWidth) {
        this->x = (float)((screenWidth - (text.length() * (6 * size))) / 2);
    }

    void UILabel::update(unsigned long deltaTime) {
        // Por ahora vacío
    }

    void UILabel::draw(Renderer& renderer) {
        if (!isVisible) return;
        renderer.drawText(text.c_str(), x, y, color, size);
    }
}