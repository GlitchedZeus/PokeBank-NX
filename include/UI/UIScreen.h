#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include <switch.h>

// Forward declaration
class PKSEFramebuffer;

// Base UI Screen class
class UIScreen {
public:
    virtual ~UIScreen() = default;
    virtual void update(const PadState& pad) = 0;
    virtual void draw(PKSEFramebuffer& fb) = 0;
    virtual bool shouldExit() const { return false; }
};

#endif
