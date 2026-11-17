"""How this project records paths about itself.

Every record that lives under `data/` (or anywhere else inside the project) stores a path inside the project *relative*
to the project root, so the whole installation can be copied or moved as one folder: `data/build-root.txt`,
`data/<game>/languages.json`, `data/<game>/regional-builds.json` and the recompiler's sync state. A path outside the
project, or on another drive, stays absolute, because no relative value can describe it.

Write with `portable()`, read with `resolve()`: a reader always resolves against the root of the installation it is
reading, never against the process's working directory.
"""
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def portable(path, root=ROOT):
    """The value to record for `path`: relative to `root` with forward slashes when inside it, else absolute."""
    path = Path(path).resolve()
    try:
        return path.relative_to(Path(root).resolve()).as_posix()
    except ValueError:  # outside the project, or another drive on Windows
        return path.as_posix()


def resolve(value, root=ROOT):
    """The absolute path a recorded value names; a relative value is resolved against `root`."""
    path = Path(value)
    return path if path.is_absolute() else Path(root) / path


def source_belongs(cached, recorded, root=ROOT) -> bool:
    """True when a CMake build folder's source (`CMAKE_HOME_DIRECTORY`) is `recorded` — also under its previous name.

    CMake stores the source folder absolutely, so a renamed installation leaves the old path in the cache. That is not
    another copy: no project lives there any more, so the build inside the installation is still this installation's. A
    copy leaves the original in place, which is reported as another copy and rebuilt. The launcher applies the same rule
    in `launcher/src/build_cache.h`.
    """
    expected = resolve(recorded, root)
    if os.path.normcase(str(Path(cached))) == os.path.normcase(str(expected)):
        return True
    try:
        below = expected.resolve().relative_to(Path(root).resolve())
    except ValueError:  # recorded outside the installation: it cannot have travelled with the rename
        return False
    cached = Path(cached)
    if len(cached.parts) <= len(below.parts):
        return False
    if os.path.normcase(str(Path(*cached.parts[-len(below.parts):]))) != os.path.normcase(str(below)):
        return False
    previous = Path(*cached.parts[:-len(below.parts)])
    return not (previous / 'engine' / 'runtime.toml').exists()
