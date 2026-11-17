// What a CMake build folder says about the project it was configured from (CMAKE_HOME_DIRECTORY), shared by the
// launcher's runner check, its moved-installation report, the regional registry and tools/test/build-cache-test.cpp.
//
// The cache stores that source folder as an absolute path, so unlike data/build-root.txt it cannot follow a folder
// rename. That does not make the build useless: renaming an installation leaves no project at the old path, so the build
// inside it is still this installation's and can be played. A *copy* leaves the original in place, and there the cache
// belongs to the other copy: CMake refuses it, so the copy rebuilds (tools/pipeline/modules/context.ps1).
#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

namespace BuildCache
{
// build.ps1 configures the runtime from <project>/engine/runtime/PS2Recomp.
inline const QString &sourceTail()
{
    static const QString tail = QStringLiteral("/engine/runtime/PS2Recomp");
    return tail;
}

inline Qt::CaseSensitivity pathCase()
{
#ifdef Q_OS_WIN
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

inline QString clean(const QString &path)
{
    return QDir::cleanPath(QDir::fromNativeSeparators(path));
}

inline bool samePath(const QString &a, const QString &b)
{
    if (a.isEmpty() || b.isEmpty())
        return false;
    return clean(a).compare(clean(b), pathCase()) == 0;
}

// The absolute source folder the build folder was configured from; empty without a cache or without that entry.
inline QString sourceFolder(const QString &buildFolder)
{
    QFile cache(clean(buildFolder) + QStringLiteral("/CMakeCache.txt"));
    if (!cache.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QString source;
    const QByteArray prefix("CMAKE_HOME_DIRECTORY:INTERNAL=");
    while (!cache.atEnd())
    {
        const QByteArray line = cache.readLine().trimmed();
        if (line.startsWith(prefix))
            source = clean(QString::fromUtf8(line.mid(prefix.size())));
    }
    return source;
}

// True when a cache naming `cached` belongs with the source `recorded` of this installation: the same path, or the same
// path below the installation under the name the folder had before it was renamed. A source recorded outside the
// installation cannot have travelled with the rename; and a cached folder that is still a project means a copy left the
// original in place, which is not this installation's build.
inline bool sameSourceAcrossRename(const QString &cached, const QString &recorded, const QString &root)
{
    if (samePath(cached, recorded))
        return true;
    if (cached.isEmpty() || recorded.isEmpty())
        return false;
    const QString below = clean(QDir(root).relativeFilePath(clean(recorded)));
    if (below.isEmpty() || below.startsWith(QStringLiteral("..")) || QDir::isAbsolutePath(below))
        return false;
    const QString suffix = QLatin1Char('/') + below;
    if (!cached.endsWith(suffix, pathCase()))
        return false;
    const QString previous = cached.left(cached.size() - suffix.size());
    return !QFileInfo::exists(previous + QStringLiteral("/engine/runtime.toml"));
}

// True when a build folder was configured by this installation, whichever name its folder has now. A cache that is not
// this game's runtime (another tool's build, or another game) is nobody's and returns false.
inline bool belongsTo(const QString &cached, const QString &root)
{
    return sameSourceAcrossRename(cached, clean(root) + sourceTail(), root);
}
}
