#include "ui/UIButton.h"

namespace UI {

    UIButton::UIButton(std::string t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback)
        : UIElement(x, y, w, h), 
            label(t), 
            index(index),
            onClick(callback) {
        
        textColor = COLOR_WHITE;
        backgroundColor = COLOR_BLACK;
        hasBackground = true;
    }

    void UIButton::setStyle(uint16_t textCol, uint16_t bgCol, bool drawBg) {
        textColor = textCol;
        backgroundColor = bgCol;
        hasBackground = drawBg;
    }

    void UIButton::setSelected(bool selected) {
        isSelected = selected;
    }

    bool UIButton::getSelected() const {
        return isSelected;
    }

    void UIButton::press() {
        if (isEnabled && onClick) {
            onClick();
        }
    }

    bool UIButton::isPointInside(int px, int py) const {
        return (px >= x && px <= x + width && 
                py >= y && py <= y + height);
    }

    void UIButton::handleInput(const InputManager& input) {
        if (!isEnabled || !isVisible) return;

        // 1. Accionamiento por Botón Físico (A / Enter)
        // Solo si el botón tiene el foco (isSelected)
        if (isSelected && input.isButtonPressed(index)) {
            this->press();
        }

        // 2. Accionamiento por Pantalla Táctil o Mouse
        // Si hay un evento de click/touch en las coordenadas del botón
        // if (input.isButtonClicked()) { 
        //     if (isPointInside(input.getMouseX(), input.getMouseY())) {
        //         this->press();
        //     }
        // }
    }

    void UIButton::update(unsigned long deltaTime) {
        (void)deltaTime;

        // Aquí podrías añadir un efecto de "latido" o parpadeo si isSelected es true
    }

    void UIButton::draw(Renderer& renderer) {
if (!isVisible) return;

        // 1. Dibujar el fondo y borde solo si tiene activado hasBackground
        if (hasBackground) {
            // Dibujamos fondo relleno
            renderer.drawFilledRectangle(x, y, width, height, backgroundColor);
        } else {
            // Si no tiene fondo, podemos indicar la selección con un pequeño marcador
            // o cambiando el color del texto
            if (isSelected) {
                renderer.drawText(">", x - 10, y + (height / 4), COLOR_YELLOW, 1);
            }
        }

        // 2. Dibujar el texto
        uint16_t currentTextCol = (isSelected && !hasBackground) ? COLOR_YELLOW : textColor;
        renderer.drawText(label.c_str(), x + 5, y + (height / 4), currentTextCol, 1);
    }
}