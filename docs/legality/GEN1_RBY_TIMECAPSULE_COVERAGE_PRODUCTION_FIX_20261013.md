## Production Gen I Time Capsule coverage false-complete fix

The coordinator production `Legality::analyze` previously assigned
`CoverageLevel::Complete` to **transfer history** whenever an exact Gen I
source-game label was supplied. That was incorrect: a PK1 in a Red, Blue or
Yellow save may have travelled through the Generation II Time Capsule and
returned, while PK1 has no native retained original-trip proof.

The coordinator now conservatively marks **all native PK1 transfer checks as
Partial** regardless of Red/Blue/Yellow source hint. A current cartridge
label, checked save checksum, and unchanged native record do not prove all
earlier Gen II transfer methods were absent. Missing Gen II acquisition
histories remain `Incomplete`, never invented global Invalid.

Production code changed only the Gen I transfer-coverage branch; Gen III/IV
historical transfer status remains unchanged. The existing native L5 test
fixtures now enforce `CoverageLevel::Partial` and prohibit
`NoProblemsFound` for all 151 original Gen I species x six source labels x
two move-domain controls, plus 5,436 original-format synthetic save cases
when current-head exact PK1 production targets execute. Source tests are
registered, **not claimed passed** until the current-head CI does execute.
