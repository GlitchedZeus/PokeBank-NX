from pathlib import Path

p = Path('src/UI/GameLauncher.cpp')
s = p.read_text(encoding='utf-8')
old = '''                result.contentPath = std::move(content);
                result.corePath = std::move(core);
                result.launcherPath = result.corePath;
                result.state = GameLaunchState::Ready;
                result.detail = "RetroArch playlist matched this save to its game file and core.";
                ::closedir(dir);
                return result;
'''
new = '''                matches.push_back({std::move(content), std::move(core)});
                pos += 6;
'''
count = s.count(old)
if count != 1:
    raise SystemExit(f'RetroArch immediate-return block changed; expected 1, found {count}')
p.write_text(s.replace(old, new, 1), encoding='utf-8')
print('Reconciled RetroArch ambiguity handling while preserving current core fallback behavior.')
