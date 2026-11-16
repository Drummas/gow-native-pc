// gow-native-pc: test of the per-build boot profile (engine/patches/25-standalone-boot-profile.patch, header
// PS2Recomp/ps2xRuntime/include/runtime/gow_boot_profile.h). It needs the runtime fetched and patched
// (`build.cmd -Stages fetch`, or the launcher's Install tab); `verify` builds and runs it as gow_boot_profile_test.
//
//   clang++ -std=c++20 -I engine/runtime/PS2Recomp/ps2xRuntime/include tools/test/boot_profile_test.cpp -o boot_profile_test
//   ./boot_profile_test          (prints one line per case; exit code 1 and "FAIL" when one does not hold)
//
// What it checks: a path setting recorded relative to the runner's folder is resolved against that folder, an absolute
// one is used exactly as recorded, a setting that is not a path is untouched, and a legacy profile (the disc and flags
// only, as the pipeline wrote before path settings were recorded) restores what it recorded.
#include "runtime/gow_boot_profile.h"
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

int main()
{
    int failures = 0;
    auto check = [&](bool ok, const char *what)
    {
        if (ok) return;
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    };
    const auto folder = (std::filesystem::current_path() / "install" / "build-clang" / "ps2xRuntime").lexically_normal();
    // What a recorded value names, expressed relative to the runner's folder: independent of how the value was built.
    auto recordedBelow = [&](const std::string &value)
    {
        return gow_boot_profile::Utf8(value).lexically_normal().lexically_relative(folder).generic_string();
    };

    // 1. A path setting recorded relative to the runner's folder.
    check(gow_boot_profile::IsPathVariable("GOW_MC0"), "the card is a path setting");
    const auto card = gow_boot_profile::Resolve(folder, "GOW_MC0", "../../data/gow1/discs/usa/Mcd001.ps2");
    check(!gow_boot_profile::Utf8(card).is_relative(), "a relative path setting resolves against the runner's folder");
    check(recordedBelow(card) == "../../data/gow1/discs/usa/Mcd001.ps2", "the resolved card keeps its recorded location");
    for (const auto *key : {"GOW_SAVES_DIR", "GOW_SAVE_ASSETS", "GOW_VU1_CAPTURA", "GOW_TEXTURE_PACK", "GOW_TEX_PCSX2_NAMES",
                            "GOW_ISO", "GOW_MC1", "GOW_MOD_VFS_TABLE", "GOW_MOD_VFS_LOG", "TOBIICHI_ICONO",
                            "TOBIICHI_CAPTURA"})
        check(gow_boot_profile::IsPathVariable(key), key);
    const auto bare = gow_boot_profile::Resolve(folder, "GOW_SAVES_DIR", "saves");
    check(recordedBelow(bare) == "saves", "a bare relative name resolves inside the runner's folder");

    // 2. An absolute path setting is used exactly as recorded (another drive, or a folder outside the project).
    const auto outside = (folder / "../../elsewhere/gow.iso").lexically_normal().string();
    check(gow_boot_profile::Utf8(outside).is_absolute(), "the fixture outside path is absolute");
    check(gow_boot_profile::Resolve(folder, "GOW_ISO", outside) == outside, "an absolute path setting is not rewritten");

    // 3. Settings that are not paths are never touched.
    check(!gow_boot_profile::IsPathVariable("TOBIICHI_PANORAMICO"), "a video setting is not a path");
    check(gow_boot_profile::Resolve(folder, "TOBIICHI_PANORAMICO", "16:9") == "16:9", "a non-path setting is untouched");
    check(gow_boot_profile::Resolve(folder, "GOW_GS_FINISH_ASINCRONO", "1") == "1", "a switch reading like a number");
    check(gow_boot_profile::Resolve(folder, "PS2X_GS_HW_SCALE", "1.5") == "1.5", "a float setting");
    check(gow_boot_profile::Resolve(folder, "GOW_PAD_GUION", "5:start,12:x") == "5:start,12:x", "a value with its own colon");

    // 4. Reading: header, recorded disc, settings; values with '=' and CRLF line endings survive.
    {
        std::istringstream stream("GOW_NATIVE_BOOT_V1\r\n../../data/gow1/discs/usa/SCUS_973.99\r\n"
                                  "GOW_MC0=../../data/gow1/discs/usa/Mcd001.ps2\r\n"
                                  "GOW_PAD_GUION=5:start=1,12:x\r\n"
                                  "FOO_BAR=1\r\n"
                                  "GOW_ISO=/media/gow.iso\r\n");
        std::string elf;
        std::vector<gow_boot_profile::Setting> settings;
        check(gow_boot_profile::Read(stream, elf, settings), "a boot profile is read");
        check(elf == "../../data/gow1/discs/usa/SCUS_973.99", "the recorded disc is read without its CR");
        check(settings.size() == 3, "only runtime-prefixed settings are read");
        check(settings.size() >= 3 && settings[0].key == "GOW_MC0", "first setting keeps its key");
        check(settings.size() >= 3 && settings[1].value == "5:start=1,12:x", "a value may contain '='");
        check(settings.size() >= 3 && settings[2].value == "/media/gow.iso", "an absolute value is read as recorded");
    }

    // 5. A legacy profile: the disc and flags only, no path setting anywhere.
    {
        std::istringstream stream("GOW_NATIVE_BOOT_V1\n../data/gow1/discs/usa/SCUS_973.99\nGOW_VU1N_X64=1\n"
                                  "GOW_VU1_CAPTURE_MISSES=1\nPS2X_GS_WORKER=1\nGOW_GS_FINISH_ASINCRONO=1\n"
                                  "GOW_GS_EARLY_FINISH=1\nTOBIICHI_PANORAMICO=16:9\n");
        std::string elf;
        std::vector<gow_boot_profile::Setting> settings;
        check(gow_boot_profile::Read(stream, elf, settings), "a legacy profile is read");
        check(settings.size() == 6, "a legacy profile keeps its six flags");
        bool paths = false;
        for (const auto &setting : settings)
        {
            paths = paths || gow_boot_profile::IsPathVariable(setting.key);
            check(gow_boot_profile::Resolve(folder, setting.key, setting.value) == setting.value,
                  "a legacy profile restores exactly what it recorded");
        }
        check(!paths, "a legacy profile has no path setting");
    }

    // 6. A foreign or truncated file is not a boot profile: the caller keeps its other boot sources.
    {
        std::string elf;
        std::vector<gow_boot_profile::Setting> settings;
        std::istringstream foreign("SOMETHING_ELSE\nELF\nGOW_MC0=x\n");
        check(!gow_boot_profile::Read(foreign, elf, settings), "a foreign header is rejected");
        std::istringstream empty;
        check(!gow_boot_profile::Read(empty, elf, settings), "an empty file is rejected");
        std::istringstream header("GOW_NATIVE_BOOT_V1\n");
        check(!gow_boot_profile::Read(header, elf, settings), "a profile without its disc line is rejected");
    }

    std::printf(failures ? "FAIL: %d case(s)\n" : "PASS: boot profile reading and path resolution\n", failures);
    return failures ? 1 : 0;
}
