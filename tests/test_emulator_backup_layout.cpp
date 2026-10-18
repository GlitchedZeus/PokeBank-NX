// Round 2 (CG13): the emulator "Backups / Emulator save" list lays out through
// SharedMenuGeometry's BackupMenu rule, and every text it draws fits the rectangle it is drawn in,
// measured with the bundled font:
// - frame, viewport and rows come from layout(MenuType::BackupMenu) for 1..40 rows; the frame sits
//   above the screen footer and the viewport never shows more than the rule's maxVisibleRows;
// - the focus highlight IS the touch target (cellRect / hitCell), so a tap selects the drawn row;
// - heading + explanatory caption fit the header band; label + detail fit each cell;
// - every row label, tag, policy reason and refusal reason fits its column or the notice toast,
//   and the toast never covers the frame or the footer;
// - scroll clamping / reveal keep the focused row on screen;
// - both backup menus read the one rule: Save Backups (BackupSelectionScreen.cpp) takes its card,
//   rows and visible count from rule(BackupMenu), the emulator list lays out via layout(BackupMenu).
#include "UI/EmulatorBackupLayout.h"
#include "UI/SourceBackupClassification.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace E = PokeBank::UIModel::EmulatorBackupLayout;
namespace G = PokeBank::UIModel::SharedMenuGeometry;
namespace SB = PokeBank::UIModel::SourceBackups;

namespace {
std::string slurp(const char* path) {
    std::ifstream in(path);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    using G::Text;
    {
        const std::string saveBackups = slurp("src/UI/BackupSelectionScreen.cpp");
        assert(saveBackups.find("rule(BackupGeometry::MenuType::BackupMenu)") != std::string::npos);
        for (const char* k : {"CARD_W = kBackupRule.width", "LIST_ROW_H = kBackupRule.rowHeight",
                              "LIST_MAX_VISIBLE = kBackupRule.maxVisibleRows"})
            assert(saveBackups.find(k) != std::string::npos);
        const std::string home = slurp("src/UI/SaveSelectScreen.cpp");
        assert(home.find("EB::layout(rows, fb.getWidth(), fb.getHeight())") != std::string::npos);
        assert(home.find("G::cellRect(layout, i - emuBackupScroll)") != std::string::npos);
    }
    for (int rows = 1; rows <= 40; ++rows) {
        const G::Layout l = E::layout(rows);
        assert(l.type == G::MenuType::BackupMenu);
        assert(G::wellFormed(l));
        assert(l.visibleRows == (rows < l.rule.maxVisibleRows ? rows : l.rule.maxVisibleRows));
        assert(!l.frame.overlaps(G::kFooterBand));
        // Header text inside the header band.
        assert(E::kHeadingTop + E::lineH(Text::Heading) <= E::kExplainerTop);
        assert(E::kExplainerTop + E::lineH(Text::Caption) <= l.rule.headerH);
        assert(G::fitsWidth(E::kExplainer, Text::Caption, l.frame.w - 2 * l.rule.padX));
        for (int v = 0; v < l.visibleRows; ++v) {
            const G::Rect cell = G::cellRect(l, v);
            assert(cell.inside(l.list) && cell.inside(l.frame));
            assert(E::kLabelTop + E::lineH(Text::Body) <= E::kDetailTop);
            assert(E::kDetailTop + E::lineH(Text::Caption) <= cell.h);
            // A tap in the middle of a drawn row selects that row; the gap below it selects none.
            assert(G::hitCell(l, cell.x + cell.w / 2, cell.y + cell.h / 2) == v);
            if (l.rule.rowGap > 0) assert(G::hitCell(l, cell.x + 4, cell.bottom()) == -1);
        }
        const G::Rect toast = E::noticeRect(G::textWidth(
            SB::refusalReason(SB::Refusal::OnlyRemainingCopy), Text::Caption));
        assert(!toast.overlaps(l.frame) && !toast.overlaps(G::kFooterBand) && toast.x >= G::kMargin);
        // Scrolling never leaves the viewport and always reveals the focused row.
        for (int index = 0; index < rows; ++index) {
            for (int scroll = -2; scroll <= rows + 2; ++scroll) {
                const int s = E::revealRow(l, E::clampScroll(l, scroll), index);
                assert(s >= 0 && s + l.visibleRows >= index + 1 && s <= index);
                assert(s == E::clampScroll(l, s));
            }
        }
    }

    // Every label / tag drawn in a row fits its column.
    const G::Layout l = E::layout(3);
    const G::Rect label = E::labelRect(l, G::cellRect(l, 0));
    for (const SB::Kind k : {SB::Kind::OriginalSource, SB::Kind::AppBackup, SB::Kind::ExportedCopy,
                             SB::Kind::StagedWorkingCopy}) {
        const SB::Policy p = SB::policyFor(k);
        const std::string stamped = std::string(p.label) + "  ·  2026-10-18 08:30";
        assert(G::fitsWidth(stamped, Text::Body, label.w));
        // The reason is shown in the toast (A Details), which is as wide as its text.
        const G::Rect toast = E::noticeRect(G::textWidth(p.reason, Text::Caption));
        assert(toast.x >= G::kMargin && toast.right() <= G::kScreenW - G::kMargin);
    }
    for (const char* tag : {"READ ONLY", "APP-OWNED", "IN EDITOR"})
        assert(G::fitsWidth(tag, Text::Caption, E::kTagColumnW));
    for (const SB::Refusal r : {SB::Refusal::OriginalSource, SB::Refusal::StagedWorkingCopy,
                                SB::Refusal::NotAnExportRecord, SB::Refusal::UnexpectedFiles,
                                SB::Refusal::OnlyRemainingCopy, SB::Refusal::Missing}) {
        const G::Rect toast = E::noticeRect(G::textWidth(SB::refusalReason(r), Text::Caption));
        assert(toast.x >= G::kMargin && toast.right() <= G::kScreenW - G::kMargin);
    }
    std::cout << "CG13 emulator backups: SharedMenuGeometry BackupMenu frame / rows / hit targets, "
                 "header + rows + notice fit, scroll reveals focus PASS\n";
}
