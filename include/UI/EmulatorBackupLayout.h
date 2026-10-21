#pragma once

// Geometry of the emulator "Backups / Emulator save" list (Product Home, game workspace,
// non-Switch titles). The frame, rows, focus highlight and touch targets all come from
// SharedMenuGeometry's BackupMenu rule (the accepted Save Backups card: 1200 px, 62 px rows,
// 8 visible), so this list and Save Backups (BackupSelectionScreen) share one contract. This header only places the text inside those rectangles:
//   * header: the game heading and one explanatory caption, inside rule.headerH;
//   * row: label (Body) over detail (Caption), a right-aligned tag column, all inside the cell;
//   * notice: an action's reason (A Details / a refused Y Delete) as a toast centred above the
//     screen footer, outside the frame, sized to its text.
// Pure and host-tested (tests/test_emulator_backup_layout.cpp).

#include "UI/SharedMenuGeometry.h"
#include "UI/SourceBackupClassification.h"

#include <string>
#include <string_view>

namespace PokeBank::UIModel::EmulatorBackupLayout {

namespace G = SharedMenuGeometry;

// The source save named by its provider ("DraStic save", "RetroArch (DeSmuME) save"), exactly as
// Product Home names the provider. Only when discovery recorded no single known provider (an
// unknown label, or a classic card holding several providers) does it fall back to "emulator save".
[[nodiscard]] inline std::string sourceSaveName(std::string_view providerDisplay) {
    if (providerDisplay.empty() || providerDisplay == "Unknown provider" ||
        providerDisplay == "Multiple providers")
        return "emulator save";
    return std::string(providerDisplay) + " save";
}
[[nodiscard]] inline std::string originalSaveLabel(std::string_view providerDisplay) {
    return "Original " + sourceSaveName(providerDisplay);
}
[[nodiscard]] inline std::string explainer(std::string_view providerDisplay) {
    return "The " + sourceSaveName(providerDisplay) +
           " is read only; only PokeBank NX export records can be deleted.";
}
[[nodiscard]] inline std::string originalSaveReason(std::string_view providerDisplay) {
    return "Read only: PokeBank NX never changes, moves or deletes the " + sourceSaveName(providerDisplay) + ".";
}
[[nodiscard]] inline std::string onlyRemainingCopyReason(std::string_view providerDisplay) {
    return "Kept: the original " + sourceSaveName(providerDisplay) +
           " was not found, so this backup is the only remaining copy.";
}
[[nodiscard]] inline std::string untouchedLine(std::string_view providerDisplay) {
    return "The original " + sourceSaveName(providerDisplay) + " is never touched.";
}

// SourceBackupClassification's generic policy / refusal wording with the provider filled in.
[[nodiscard]] inline std::string rowLabel(SourceBackups::Kind kind, std::string_view providerDisplay) {
    return kind == SourceBackups::Kind::OriginalSource ? originalSaveLabel(providerDisplay)
                                                       : std::string(SourceBackups::policyFor(kind).label);
}
[[nodiscard]] inline std::string kindReason(SourceBackups::Kind kind, std::string_view providerDisplay) {
    return kind == SourceBackups::Kind::OriginalSource ? originalSaveReason(providerDisplay)
                                                       : std::string(SourceBackups::policyFor(kind).reason);
}
[[nodiscard]] inline std::string refusalText(SourceBackups::Refusal refusal, std::string_view providerDisplay) {
    if (refusal == SourceBackups::Refusal::OriginalSource) return originalSaveReason(providerDisplay);
    if (refusal == SourceBackups::Refusal::OnlyRemainingCopy) return onlyRemainingCopyReason(providerDisplay);
    return SourceBackups::refusalReason(refusal);
}
[[nodiscard]] inline std::string deletedNotice(std::string_view providerDisplay) {
    return "Backup deleted. The original " + sourceSaveName(providerDisplay) + " was not touched.";
}

// Round 4 (R2-EMUBACKUP / C07 / S05): the PKSE-style flow. Each save gets a "Create backup now"
// action row; A creates a copy only on that row. A on the original save browses it read-only (the
// normal open, which never makes a backup); A on a backup shows its details; Y deletes a backup
// after the confirmation. Nothing on this screen writes the emulator's own file.
inline constexpr int kCreateRowEntry = -1; // EmuBackupRow::entry of a "Create backup now" row

enum class RowAction : unsigned char { CreateBackup, BrowseSave, Details };

[[nodiscard]] constexpr RowAction rowAction(int entry, SourceBackups::Kind kind) noexcept {
    if (entry == kCreateRowEntry) return RowAction::CreateBackup;
    return kind == SourceBackups::Kind::OriginalSource ? RowAction::BrowseSave : RowAction::Details;
}
[[nodiscard]] constexpr const char* rowActionLabel(RowAction action) noexcept {
    switch (action) {
        case RowAction::CreateBackup: return "Create";
        case RowAction::BrowseSave:   return "Browse";
        case RowAction::Details:      return "Details";
    }
    return "Details";
}
[[nodiscard]] inline std::string createRowLabel() { return "Create backup now"; }
[[nodiscard]] inline std::string createRowDetail(std::string_view providerDisplay) {
    return "Copies the " + sourceSaveName(providerDisplay) + " into PokeBank NX backups. The save is not changed.";
}
[[nodiscard]] inline std::string createdNotice(std::string_view providerDisplay) {
    return "Backup created. The original " + sourceSaveName(providerDisplay) + " was not touched.";
}
[[nodiscard]] inline std::string explainerWithBackups(std::string_view providerDisplay) {
    return "The " + sourceSaveName(providerDisplay) +
           " is read only. Make a backup copy any time; only backups can be deleted.";
}

inline constexpr int kHeadingTop = 4;     // from frame.y (Heading, 28 px)
inline constexpr int kExplainerTop = 40;  // from frame.y (Caption, 17 px)
inline constexpr int kLabelTop = 2;       // from cell.y (Body, 20 px)
inline constexpr int kDetailTop = 27;     // from cell.y (Caption, 17 px)
inline constexpr int kTagColumnW = 112;   // "READ ONLY" / "APP-OWNED" / "IN EDITOR"
inline constexpr int kNoticePadX = 18;
inline constexpr int kNoticeGap = 8;      // toast bottom to the footer band (clears a full 8-row frame)

// Rendered line height of a text style (PKSEFramebuffer draws ~1.25 em from the text top).
constexpr int lineH(G::Text style) noexcept { return G::fontPx(style) * 5 / 4; }

[[nodiscard]] constexpr G::Layout layout(int rows, int screenW = G::kScreenW,
                                         int screenH = G::kScreenH) noexcept {
    return G::layout(G::MenuType::BackupMenu, rows, screenW, screenH);
}

// First row shown for a scroll position, clamped to the rule's viewport.
[[nodiscard]] constexpr int clampScroll(const G::Layout& l, int scroll) noexcept {
    const int maxFirst = l.totalRows > l.visibleRows ? l.totalRows - l.visibleRows : 0;
    return scroll < 0 ? 0 : scroll > maxFirst ? maxFirst : scroll;
}

// Scroll that keeps `index` visible after controller navigation.
[[nodiscard]] constexpr int revealRow(const G::Layout& l, int scroll, int index) noexcept {
    if (index < scroll) scroll = index;
    if (index >= scroll + l.visibleRows) scroll = index - l.visibleRows + 1;
    return clampScroll(l, scroll);
}

// Label column of a row (the tag column is trailing).
[[nodiscard]] constexpr G::Rect labelRect(const G::Layout& l, const G::Rect& cell) noexcept {
    return G::labelRect(l, cell, kTagColumnW);
}

// The notice toast for text of drawn width `textW`, centred above the footer band.
[[nodiscard]] constexpr G::Rect noticeRect(int textW, int screenW = G::kScreenW,
                                           int screenH = G::kScreenH) noexcept {
    const int w = textW + 2 * kNoticePadX;
    const int h = lineH(G::Text::Caption) + 14;
    return {(screenW - w) / 2, screenH - G::kFooterH - kNoticeGap - h, w, h};
}

} // namespace PokeBank::UIModel::EmulatorBackupLayout
