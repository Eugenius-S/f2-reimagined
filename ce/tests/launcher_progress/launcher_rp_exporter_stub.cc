// A controllable exporter stand-in for the disposable RP integration fixture.
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char** argv)
{
    std::filesystem::path root;
    bool all = false;
    bool check = false;
    const char* log = std::getenv("F2R_RP_EXPORT_LOG");
    if (!log) return 90;
    std::ofstream out(log, std::ios::app);
    for (int i = 1; i < argc; ++i) {
        out << argv[i] << '\n';
        if (std::string(argv[i]) == "--game-root" && i + 1 < argc) root = argv[i + 1];
        if (std::string(argv[i]) == "--maps" && i + 1 < argc) all = std::string(argv[i + 1]) == "all";
        if (std::string(argv[i]) == "--check") check = true;
    }
    out << "END\n";
    out.close();
    if (!out || !all || check || root.empty()) return 91;
    const char* fail = std::getenv("F2R_RP_EXPORT_FAIL");
    if (fail && std::string(fail) == "1") return 17;
    const char* failFile = std::getenv("F2R_RP_EXPORT_FAIL_FILE");
    std::string flag;
    if (failFile && std::ifstream(failFile) >> flag && flag == "1") return 17;
    std::filesystem::create_directories(root / "mod-cache/scene");
    std::ofstream(root / "mod-cache/scene/fixture-scene.txt") << "vanilla";
    return 0;
}
