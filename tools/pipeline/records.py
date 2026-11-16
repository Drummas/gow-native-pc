"""How this project records paths about itself.

Every record that lives under `data/` (or anywhere else inside the project) stores a path inside the project *relative*
to the project root, so the whole installation can be copied or moved as one folder: `data/build-root.txt`,
`data/<game>/languages.json`, `data/<game>/regional-builds.json` and the recompiler's sync state. A path outside the
project, or on another drive, stays absolute, because no relative value can describe it.

Write with `portable()`, read with `resolve()`: a reader always resolves against the root of the installation it is
reading, never against the process's working directory.
"""
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
