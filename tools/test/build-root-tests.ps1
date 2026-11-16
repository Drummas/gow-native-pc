$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot)
$scratch = Join-Path $repo ('data/build-root-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $scratch | Out-Null
$source = [IO.File]::ReadAllText((Join-Path $repo 'tools/pipeline/modules/context.ps1'))
$begin = $source.IndexOf('$buildRootFile =')
$end = $source.IndexOf('$pgoDir =', $begin)
if ($begin -lt 0 -or $end -lt 0) { throw 'Build-root selection block missing' }
$selection = [scriptblock]::Create($source.Substring($begin, $end - $begin))
function Barra([string]$path) { return $path.Replace('\', '/') }
# The selection block records relative paths with the shared helpers.
. (Join-Path $repo 'tools/pipeline/modules/common.ps1')
# Budget 0 keeps the block's own value (the real Windows path limit); a test sets it to force one branch.
function Select-TestRoot([string]$explicit = '', [string]$install = '', [int]$budget = 0) {
    $root = if ($install) { $install } else { Join-Path $scratch 'install' }
    $rec = Join-Path $root 'engine/runtime/PS2Recomp'
    $Config = 'Release'; $Toolchain = 'clang'; $BuildRoot = $explicit
    if ($budget) { $portableBuildBudget = $budget }
    . $selection
    return $BuildRoot
}
# The same path the selection block writes (no Join-Path separator assumptions).
function RecordOf([string]$install) { return (Join-Path $install 'data\build-root.txt') }
function Recorded([string]$install) { return (Get-Content -LiteralPath (RecordOf $install) -Raw).Trim() }
function RecordedPath([string]$install) {
    $value = Recorded $install
    return $(if ([IO.Path]::IsPathRooted($value)) { $value } else { Join-Path $install $value })
}
try {
    $install = Join-Path $scratch 'install'
    # A fresh copy keeps its build folder inside itself and records it relative: the installation is portable.
    $inside = Select-TestRoot '' '' 10000
    if ($inside -ne (Join-Path $install 'build')) { throw "Default build folder is not inside the project: $inside" }
    if ((Recorded $install) -ne 'build') { throw "Default build folder was not recorded relative: $(Recorded $install)" }
    if ((Select-TestRoot '' '' 10000) -ne $inside) { throw 'Recorded relative build folder was not resolved and reused' }
    # A project folder too deep for the runtime's long dependency paths keeps the short drive-root folder, absolute.
    $deep = Join-Path $scratch ('deep-' + ('x' * 40))
    $shortRoot = Join-Path ([IO.Path]::GetPathRoot($deep)) 'gow-build'
    if ((Select-TestRoot '' $deep 1) -ne $shortRoot) { throw 'Deep project folder must fall back to the short build folder' }
    if (-not [IO.Path]::IsPathRooted((Recorded $deep))) { throw 'Short fallback folder must be recorded absolute' }
    # A chosen location outside the project is recorded absolute and must not fall back to the default folder.
    $choice = Join-Path $scratch 'chosen'
    $foreign = Join-Path $scratch 'chosen-clang'
    New-Item -ItemType Directory -Force $foreign | Out-Null
    [IO.File]::WriteAllText((Join-Path $foreign 'CMakeCache.txt'), 'CMAKE_HOME_DIRECTORY:INTERNAL=Z:/unrelated')
    $actual = Select-TestRoot $choice
    if (-not $actual.StartsWith($choice + '-')) { throw "Collision changed selected drive: $actual" }
    if (-not (Test-Path (Join-Path $foreign 'CMakeCache.txt'))) { throw 'Foreign build was touched' }
    if ((RecordedPath $install) -ne $actual) { throw 'Explicit effective root was not persisted' }
    if ((Select-TestRoot) -ne $actual) { throw 'Recorded root was not reused' }
    $unicode = Join-Path $scratch ('build-' + [char]0x00e1)
    if ((Select-TestRoot $unicode) -ne $unicode -or (Select-TestRoot) -ne $unicode) { throw 'UTF-8 root did not round trip' }
    # A -BuildRoot given relative to the project becomes a project path too.
    if ((Select-TestRoot 'build') -ne $inside) { throw 'Relative -BuildRoot must resolve inside the project' }
    Write-Output 'PASS: portable default build folder (inside the project, recorded relative), deep-folder fallback, selected-drive collision, ownership preservation, explicit persistence and UTF-8 root reuse'
} finally {
    $resolved = [IO.Path]::GetFullPath($scratch)
    $allowed = [IO.Path]::GetFullPath((Join-Path $repo 'data')) + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe test cleanup path' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
