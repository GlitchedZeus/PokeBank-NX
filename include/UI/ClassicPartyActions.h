#pragma once
namespace UI {
class TrainerViewScreen;
namespace ClassicPartyActions {
// Opens the native Gen I-III staged Party sheet. Unsupported source layouts
// return false so their existing read-only native View remains available.
bool open(TrainerViewScreen& screen,int slot);
}
}
