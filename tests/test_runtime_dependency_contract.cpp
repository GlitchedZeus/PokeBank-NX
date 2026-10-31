#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    namespace fs = std::filesystem;
    for (const char* root : {"src", "include"}) {
        for (const auto& ent : fs::recursive_directory_iterator(root)) {
  if (!ent.is_regular_file()) continue;
  const auto ext = ent.path().extension().string();
  if (ext != ".cpp" && ext != ".h" && ext != ".inc") continue;
  std::ifstream f(ent.path(), std::ios::binary);
  std::ostringstream ss; ss << f.rdbuf();
  const std::string text = ss.str();
  if (text.find("sdmc:/PKSE") != std::string::npos) {
      std::cerr << "foreign runtime path remains: " << ent.path() << "\n";
      return 1;
  }
  if (text.find("PokeVault NX Generation II staged export") != std::string::npos ||
      text.find("PokeVault/Exports/Gen2") != std::string::npos) {
      std::cerr << "legacy user-visible staged-editor branding remains: " << ent.path() << "\n";
      return 1;
  }
        }
    }
    std::cout << "Runtime dependency/path textual contract passed\n";
}
