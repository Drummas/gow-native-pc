// gow-native-pc: the build-folder ownership rule of launcher/src/build_cache.h (CMAKE_HOME_DIRECTORY of a CMake cache).
// It needs Qt 6 Core and the launcher sources:
//
//   cmake --build <launcher build folder> --target gow_build_cache_tests
//   gow_build_cache_tests        (prints one line per case; exit code 1 and "FAIL" when one does not hold)
//
// What it checks: a cache configured from this project belongs to it; one configured from another copy that still
// exists does not, so that copy rebuilds; one configured from a folder that is gone — the installation was renamed —
// still belongs to this installation and can be played; a cache that is not this game's runtime is nobody's build; and
// a regional build's own source snapshot follows the same rule.
#include "build_cache.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

namespace
{
int failures = 0;

void check(bool ok, const char *what)
{
    if (ok)
        return;
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++failures;
}

// A CMake build folder whose cache names `source`, as CMake writes it (with a CRLF pair, which must not matter).
QString writeCache(const QString &folder, const QString &source)
{
    QDir().mkpath(folder);
    QFile cache(folder + QStringLiteral("/CMakeCache.txt"));
    if (!cache.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return QString();
    cache.write("# For build in directory: " + folder.toUtf8() + "\r\n");
    cache.write("CMAKE_HOME_DIRECTORY:INTERNAL=" + source.toUtf8() + "\r\n");
    cache.close();
    return folder;
}

void writeProject(const QString &root)
{
    QDir().mkpath(root + QStringLiteral("/engine/runtime/PS2Recomp"));
    QFile toml(root + QStringLiteral("/engine/runtime.toml"));
    if (!toml.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;
    toml.write("repository = \"https://example.invalid/runtime\"\ncommit = \"0000000\"\n");
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir tmp;
    if (!tmp.isValid())
    {
        std::fprintf(stderr, "FAIL: no temporary folder\n");
        return 1;
    }
    const QString root = BuildCache::clean(tmp.filePath(QStringLiteral("gow-native-pc")));
    const QString tail = BuildCache::sourceTail();
    writeProject(root);

    // 1. A build folder configured from this project.
    const QString here = writeCache(root + QStringLiteral("/build-clang"), root + tail);
    check(BuildCache::sourceFolder(here) == root + tail, "the cache's source folder is read, whatever its line endings");
    check(BuildCache::belongsTo(BuildCache::sourceFolder(here), root), "a build configured here belongs to it");
    check(BuildCache::sourceFolder(root + QStringLiteral("/no-build-clang")).isEmpty(), "no cache means no source folder");

    // 2. Another copy of the project, still in place: its build is not this installation's (CMake refuses that cache).
    const QString other = BuildCache::clean(tmp.filePath(QStringLiteral("another-copy")));
    writeProject(other);
    const QString theirs = writeCache(other + QStringLiteral("/build-clang"), other + tail);
    check(!BuildCache::belongsTo(BuildCache::sourceFolder(theirs), root), "another copy's build is not ours");
    check(BuildCache::belongsTo(BuildCache::sourceFolder(theirs), other), "another copy's build is still its own");

    // 3. Renamed: the folder the cache names is not a project any more, so this installation was renamed, not copied.
    const QString oldName = BuildCache::clean(tmp.filePath(QStringLiteral("previous-name")));
    const QString renamed = writeCache(root + QStringLiteral("/build-renamed-clang"), oldName + tail);
    check(BuildCache::belongsTo(BuildCache::sourceFolder(renamed), root),
          "a build made before the folder was renamed still belongs to the installation");
    const QString stale = writeCache(root + QStringLiteral("/build-stale-clang"), oldName + QStringLiteral("/tools"));
    check(!BuildCache::belongsTo(BuildCache::sourceFolder(stale), root),
          "a cache without the runtime tail is not a build of this game");

    // 4. Regional builds: their source snapshot lives inside the installation too, so it follows the same rule.
    const QString snapshot = root + QStringLiteral("/data/gow1/regions-europe/PS2Recomp");
    const QString regional = writeCache(root + QStringLiteral("/build-europe-clang"),
                                        oldName + QStringLiteral("/data/gow1/regions-europe/PS2Recomp"));
    check(BuildCache::sameSourceAcrossRename(BuildCache::sourceFolder(regional), snapshot, root),
          "a regional snapshot registered before the rename still matches");
    check(!BuildCache::sameSourceAcrossRename(BuildCache::sourceFolder(regional), root + tail, root),
          "a regional build is not the USA build");
    check(!BuildCache::sameSourceAcrossRename(BuildCache::sourceFolder(regional), other + QStringLiteral("/data/x"), root),
          "a source outside the installation does not travel with the rename");

    std::printf(failures ? "FAIL: %d case(s)\n" : "PASS: build folder ownership across a folder rename\n", failures);
    return failures ? 1 : 0;
}
