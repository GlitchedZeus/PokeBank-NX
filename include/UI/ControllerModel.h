#ifndef UI_CONTROLLER_MODEL_H
#define UI_CONTROLLER_MODEL_H

#include <array>
#include <span>
#include <string_view>

namespace PokeBank::UIModel {
    enum class ControllerButton { DPad, A, B, X, Y, LR, ZLZR, Plus, Minus };
    enum class ControllerIntent {
        Navigate,
        Select,
        Back,
        SwitchUser,
        Options,
        Help,
        Choose,
        Cancel,
        TransferCompatibility,
        More
    };
    enum class ControllerContext {
        SelectGame,
        SelectGameMultipleUsers,
        PokemonActionSheet,
        PokemonSummary,
        CarryingPokemon
    };

    struct ControllerBinding {
        ControllerButton button;
        ControllerIntent intent;
        std::string_view label;
    };

    inline constexpr std::array SELECT_GAME_BINDINGS{
        ControllerBinding{ControllerButton::A, ControllerIntent::Select, "Select"},
        ControllerBinding{ControllerButton::Plus, ControllerIntent::Options, "Options"},
        ControllerBinding{ControllerButton::Minus, ControllerIntent::Help, "Help"}
    };
    inline constexpr std::array SELECT_GAME_MULTI_BINDINGS{
        ControllerBinding{ControllerButton::A, ControllerIntent::Select, "Select"},
        ControllerBinding{ControllerButton::LR, ControllerIntent::SwitchUser, "Switch User"},
        ControllerBinding{ControllerButton::Plus, ControllerIntent::Options, "Options"},
        ControllerBinding{ControllerButton::Minus, ControllerIntent::Help, "Help"}
    };
    inline constexpr std::array ACTION_SHEET_BINDINGS{
        ControllerBinding{ControllerButton::DPad, ControllerIntent::Choose, "Choose"},
        ControllerBinding{ControllerButton::A, ControllerIntent::Select, "Select"},
        ControllerBinding{ControllerButton::B, ControllerIntent::Cancel, "Cancel"}
    };
    inline constexpr std::array SUMMARY_BINDINGS{
        ControllerBinding{ControllerButton::DPad, ControllerIntent::Navigate, "Navigate"},
        ControllerBinding{ControllerButton::B, ControllerIntent::Back, "Back"},
        ControllerBinding{ControllerButton::Plus, ControllerIntent::TransferCompatibility, "Compatibility"},
        ControllerBinding{ControllerButton::Minus, ControllerIntent::Help, "Help"}
    };
    inline constexpr std::array CARRY_BINDINGS{
        ControllerBinding{ControllerButton::DPad, ControllerIntent::Navigate, "Move"},
        ControllerBinding{ControllerButton::A, ControllerIntent::Select, "Place"},
        ControllerBinding{ControllerButton::B, ControllerIntent::Back, "Put Back"},
        ControllerBinding{ControllerButton::Plus, ControllerIntent::More, "More"},
        ControllerBinding{ControllerButton::Minus, ControllerIntent::Help, "Help"}
    };

    constexpr std::span<const ControllerBinding> controllerBindings(ControllerContext context) {
        switch (context) {
            case ControllerContext::SelectGame:              return SELECT_GAME_BINDINGS;
            case ControllerContext::SelectGameMultipleUsers: return SELECT_GAME_MULTI_BINDINGS;
            case ControllerContext::PokemonActionSheet:      return ACTION_SHEET_BINDINGS;
            case ControllerContext::PokemonSummary:          return SUMMARY_BINDINGS;
            case ControllerContext::CarryingPokemon:         return CARRY_BINDINGS;
        }
        return {};
    }

    constexpr const ControllerBinding* findBinding(ControllerContext context,
                                                    ControllerButton button) {
        for (const ControllerBinding& binding : controllerBindings(context))
            if (binding.button == button) return &binding;
        return nullptr;
    }
}

#endif
