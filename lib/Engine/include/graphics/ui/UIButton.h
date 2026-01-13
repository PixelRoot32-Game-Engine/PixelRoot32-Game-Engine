#pragma once
#include "Config.h"
#include "graphics/ui/UIElement.h"
#include <string>
#include <functional>
#include <InputManager.h>

namespace UI {
    class UIButton : public UIElement {
    private:
        std::string label;
        uint16_t textColor;
        uint16_t backgroundColor;
        bool isSelected = false;
        bool hasBackground = true;
        uint8_t index;
        std::function<void()> onClick;

        // Helper interno para colisiones
        bool isPointInside(int px, int py) const;

    public:
        UIButton(std::string t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback);

        void setStyle(uint16_t textCol, uint16_t bgCol, bool drawBg);
        void setSelected(bool selected);
        bool getSelected() const;
        
        // Maneja la entrada tanto física como táctil
        void handleInput(const InputManager& input);
        void update(unsigned long deltaTime) override;
        void draw(Renderer& renderer) override;

        void press();
    };
}