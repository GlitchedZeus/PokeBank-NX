// The accepted Generation II overlay implementation is preserved byte-for-byte in
// TrainerViewScreenGSCOverlay.inc and compiled exactly once by TrainerViewScreenCompositeOverlay.cpp.
//
// This translation unit intentionally defines nothing. Keeping the historical .cpp path as a shim
// avoids duplicate TrainerViewScreen::update/draw symbols while preserving the accepted source blob
// separately for the composite wrapper.
