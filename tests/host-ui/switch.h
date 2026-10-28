#pragma once
// HOST UI HARNESS ONLY (never shipped): a scriptable libnx surface for tests that drive the real
// TrainerViewScreen::update / draw on the host (tests/host-ui/HostUi.h). The native build uses
// devkitPro/libnx. Pad and touch state come from HostUi::input, set by the test each frame.
#include <cstdint>
#include <cstddef>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef int64_t s64;
typedef u32 Result;
#define R_SUCCEEDED(r) ((r)==0)
#define R_FAILED(r) ((r)!=0)
#define BIT(n) (1ULL<<(n))
typedef enum : u64 {
 HidNpadButton_A=BIT(0),HidNpadButton_B=BIT(1),HidNpadButton_X=BIT(2),HidNpadButton_Y=BIT(3),
 HidNpadButton_StickL=BIT(4),HidNpadButton_StickR=BIT(5),HidNpadButton_L=BIT(6),HidNpadButton_R=BIT(7),
 HidNpadButton_ZL=BIT(8),HidNpadButton_ZR=BIT(9),HidNpadButton_Plus=BIT(10),HidNpadButton_Minus=BIT(11),
 HidNpadButton_Left=BIT(12),HidNpadButton_Up=BIT(13),HidNpadButton_Right=BIT(14),HidNpadButton_Down=BIT(15),
 HidNpadButton_StickLLeft=BIT(16),HidNpadButton_StickLUp=BIT(17),HidNpadButton_StickLRight=BIT(18),HidNpadButton_StickLDown=BIT(19),
 HidNpadButton_AnyLeft=BIT(12)|BIT(16),HidNpadButton_AnyUp=BIT(13)|BIT(17),HidNpadButton_AnyRight=BIT(14)|BIT(18),HidNpadButton_AnyDown=BIT(15)|BIT(19)
} HidNpadButton;
typedef enum { HidNpadStyleSet_NpadStandard = 0x1f } HidNpadStyleTag;
typedef struct { s32 x, y; } HidAnalogStickState;
typedef struct { u64 down, held, up; } PadState;
inline void padUpdate(PadState*) {}
inline u64 padGetButtons(const PadState* p) { return p->held; }
inline u64 padGetButtonsDown(const PadState* p) { return p->down; }
inline u64 padGetButtonsUp(const PadState* p) { return p->up; }
inline HidAnalogStickState padGetStickPos(const PadState*, int) { return {0,0}; }
inline void padInitializeDefault(PadState*) {}
inline void padConfigureInput(u32, u32) {}
typedef struct { u32 x, y, diameter_x, diameter_y, rotation_angle, finger_id; } HidTouchState;
typedef struct { u64 sampling_number; s32 count; u32 reserved; HidTouchState touches[16]; } HidTouchScreenState;
inline void hidInitializeTouchScreen() {}
namespace HostUi {
struct TouchFrame { bool down = false; u32 x = 0, y = 0; };
inline TouchFrame touch;   // the finger for the next TouchInput::update()
}
inline size_t hidGetTouchScreenStates(HidTouchScreenState* states, size_t) {
    *states = {};
    if (HostUi::touch.down) {
        states->count = 1;
        states->touches[0].x = HostUi::touch.x;
        states->touches[0].y = HostUi::touch.y;
    }
    return 1;
}
typedef struct { u8 d[0x38]; char nickname[0x21]; } AccountProfileBase;
typedef struct { u8 d[0x100]; } AccountUserData;
typedef struct { u8 d[16]; } AccountProfile;
inline bool appletMainLoop() { return false; }
inline u64 armGetSystemTick() { return 0; }
// Product Home (SaveSelectScreen) on the host (Round 7 C04, lane P): by default no account and no
// save data. Every call reports failure / nothing, so Product Home shows its empty state; it never
// reaches a real save. AccountUid comes from Utils/NXTypes.h, hence the deduced parameters.
// Round 8 automated QA: a test may script ONE account and the save-data entries the console would
// report (HostUi::consoleSaves), to drive the real title discovery with synthetic title ids. Nothing
// is mounted: fsdevMountSaveData still refuses, so no save content is ever read.
#define ACC_USER_LIST_SIZE 8
struct AccountUid;
namespace HostUi {
struct ConsoleSaves {
    bool active = false;            // false: the default empty console
    u64 user[2] = {0, 0};           // the one scripted account's uid
    bool allSpaceRefused = false;   // FsSaveDataSpaceId_All refused: discovery falls back to User/SdUser
    int opens = 0;                  // reader opens seen (diagnostic)
    int mountsRefused = 0;          // save mounts attempted (always refused on the host)
};
inline ConsoleSaves consoleSaves;
}
template <class Uid> inline Result accountListAllUsers(Uid* ids, s32 max, s32* count) {
    if (count) *count = 0;
    if (!HostUi::consoleSaves.active) return 1;
    if (max < 1 || !count) return 1;
    static_assert(sizeof(Uid) == sizeof(HostUi::consoleSaves.user), "AccountUid is 16 bytes");
    __builtin_memcpy(&ids[0], HostUi::consoleSaves.user, sizeof(Uid));
    *count = 1;
    return 0;
}
template <class Uid> inline Result accountGetProfile(AccountProfile*, Uid) { return HostUi::consoleSaves.active ? 0 : 1; }
inline Result accountProfileGet(AccountProfile*, AccountUserData*, AccountProfileBase* base) {
    if (!HostUi::consoleSaves.active || !base) return 1;
    *base = {};
    const char name[] = "QA Owner";
    __builtin_memcpy(base->nickname, name, sizeof(name));
    return 0;
}
inline void accountProfileClose(AccountProfile*) {}
typedef enum { FsSaveDataSpaceId_System = 0, FsSaveDataSpaceId_User = 1, FsSaveDataSpaceId_SdUser = 3,
               FsSaveDataSpaceId_All = -1 } FsSaveDataSpaceId;
typedef enum { FsSaveDataType_System = 0, FsSaveDataType_Account = 1 } FsSaveDataType;
typedef struct { u8 d[8]; } FsSaveDataInfoReader;
typedef struct { u64 save_data_id; u8 save_data_space_id; u8 save_data_type; u8 pad[6];
                 u64 uid[2]; u64 system_save_data_id; u64 application_id; u64 size; } FsSaveDataInfo;
namespace HostUi {
// Entries per space id, in the order the console reports them (scripted by the test).
inline FsSaveDataInfo consoleSaveEntries[64];
inline s32 consoleSaveSpace[64];
inline size_t consoleSaveCount = 0;
}
inline Result fsOpenSaveDataInfoReader(FsSaveDataInfoReader* reader, FsSaveDataSpaceId space) {
    if (!HostUi::consoleSaves.active || !reader) return 1;
    if (space == FsSaveDataSpaceId_All && HostUi::consoleSaves.allSpaceRefused) return 1;
    ++HostUi::consoleSaves.opens;
    *reader = {};
    reader->d[0] = static_cast<u8>(static_cast<s32>(space) & 0xFF);   // space; d[1] = next entry
    return 0;
}
inline Result fsSaveDataInfoReaderRead(FsSaveDataInfoReader* reader, FsSaveDataInfo* out, size_t max, s64* n) {
    if (n) *n = 0;
    if (!HostUi::consoleSaves.active || !reader || !n) return 1;
    const s32 space = static_cast<s8>(reader->d[0]);
    size_t produced = 0;
    size_t next = reader->d[1];
    for (; next < HostUi::consoleSaveCount && produced < max; ++next) {
        const s32 entrySpace = HostUi::consoleSaveSpace[next];
        if (space != FsSaveDataSpaceId_All && entrySpace != space) continue;
        out[produced++] = HostUi::consoleSaveEntries[next];
    }
    reader->d[1] = static_cast<u8>(next);
    *n = static_cast<s64>(produced);
    return 0;
}
inline void fsSaveDataInfoReaderClose(FsSaveDataInfoReader*) {}
template <class Uid> inline Result fsdevMountSaveData(const char*, u64, Uid) { ++HostUi::consoleSaves.mountsRefused; return 1; }
inline Result fsdevUnmountDevice(const char*) { return 0; }
