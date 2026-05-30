// Temporary stub — replaced by Phase 4+5 platform wiring
#include <core/Engine.h>

namespace pr32 = pixelroot32;

// Minimal Engine instance for linking (replaced by real platform config in Phase 4+5)
pr32::core::Engine engine(
    pr32::graphics::DisplayConfig(pr32::graphics::DisplayType::NONE, 0, 240, 240)
);

int main() {
    return 0;
}
