artifact-helib — Patch Set for Reproducing GN Experiments in HElib
=================================================================

This repository contains a set of Git patches and small runtime data files
to reproduce/test the GN modifications on top of a *clean* HElib source tree.
The patches are located in:

  patches/
    GN17.patch
    GN127.patch
    GN257.patch
    GN8191.patch
    GN65537.patch

Each patch may:
  - Modify HElib source files (CMakeLists.txt, headers, src/, tests/, etc.)
  - Add a benchmarking tool (tools/fatboot.cpp)
  - Add runtime data under data/params*/ (e.g., data/params257/, etc.)
  - Introduce a CMake cache variable PARAM_SET to select which data/params*
    directory to copy/use at build time.

Important Notes
---------------
1) Version matching matters:
   If you see many “patch does not apply” errors, your HElib version/commit
   likely does not match the one used to generate the patch. Switch your HElib
   checkout to the correct commit/tag and try again.

2) CMake caching matters:
   CMake stores configuration in build/CMakeCache.txt. If you switch patches
   or PARAM_SET values, you should wipe the build directory (recommended) to
   avoid using stale cached values.

3) WSL shell quoting matters:
   Do NOT write "~" inside quotes, e.g. "~/tmp/file.patch" (this will NOT
   expand). Use:
     ~/tmp/file.patch
   or:
     "$HOME/tmp/file.patch"

Quick Start (WSL/Linux)
-----------------------

A) Download this repository (for patches)
-----------------------------------------
Option A (HTTPS, force HTTP/1.1 to reduce TLS issues sometimes):
  mkdir -p ~/tmp
  cd ~/tmp
  git -c http.version=HTTP/1.1 clone --depth 1 https://github.com/Nobody673/artifact-helib.git

Option B (SSH, recommended if HTTPS/TLS errors occur):
  1) Create an SSH key:
       ssh-keygen -t ed25519 -C "wsl" -f ~/.ssh/id_ed25519
  2) Add public key to GitHub (Settings → SSH and GPG keys):
       cat ~/.ssh/id_ed25519.pub
  3) Clone via SSH:
       mkdir -p ~/tmp
       cd ~/tmp
       git clone git@github.com:Nobody673/artifact-helib.git

B) Prepare a clean HElib working tree
-------------------------------------
Assume your clean HElib repo is here:
  ~/HElib-clean

Create a dedicated test branch (recommended):
  cd ~/HElib-clean
  git checkout -b patch-test

C) Verify patch files are valid (not empty)
-------------------------------------------
  ls -lh ~/tmp/artifact-helib/patches
  wc -c  ~/tmp/artifact-helib/patches/*.patch

Each patch should be much larger than a few bytes.

D) Apply a patch to HElib
-------------------------
Always run a dry-run check first:

  cd ~/HElib-clean
  PATCH=~/tmp/artifact-helib/patches/GN17.patch

  git apply --check -p1 "$PATCH"
  git apply --3way  -p1 "$PATCH"

If apply succeeds, inspect changes:
  git status
  git diff --stat

Recommended workflow when testing multiple patches:
  - Apply ONE patch
  - Build & test
  - Then revert to clean state before applying the next patch

E) Revert HElib back to pre-patch state (when you did NOT commit)
-----------------------------------------------------------------
This restores the repo to the last commit (HEAD) and removes untracked files:

  cd ~/HElib-clean
  git reset --hard
  git clean -fd

(Preview what would be removed by clean, without deleting, using:
   git clean -fdn
)

Build Instructions (fatboot)
----------------------------

1) Clean the build directory to avoid stale cache (recommended)
--------------------------------------------------------------
  cd ~/HElib-clean
  rm -rf build
  mkdir build
  cd build

2) Configure with the correct PARAM_SET
---------------------------------------
The patch expects a data directory:
  ~/HElib-clean/data/<PARAM_SET>

If your patch created data/params257/, then configure with:
  cmake .. -DPARAM_SET=params257

If you are unsure which directories exist, list them:
  cd ~/HElib-clean
  ls -d data/* 2>/dev/null

Then pick one of those names (e.g., params1, params2, params257, params8191, ...).

3) Build
--------
  cd ~/HElib-clean/build
  cmake --build . -j32

If you see:
  "Data directory not found: .../data/paramsXYZ"
then either:
  - You chose the wrong PARAM_SET, or
  - You did not wipe build/ and CMake reused an old cached PARAM_SET.

Fix by wiping build/ and re-configuring with the correct PARAM_SET.

Running fatboot
---------------
After building, run the fatboot binary (location depends on the patch/CMake).
Common locations:
  - ~/HElib-clean/build/fatboot
  - or in build/tools/ depending on your CMake setup

Example:
  cd ~/HElib-clean/build
  ./fatboot i=3 h=24 t=1 newbts=1 newks=1 thick=1 repeat=1

The exact parameters depend on your experiment setup and the patch.

Common Problems & Fixes
-----------------------

1) “patch does not apply”
   - Your HElib version does not match the patch baseline.
   - Checkout the correct HElib commit/tag and re-apply.

2) “already exists in working directory”
   - You have untracked files that conflict with files introduced by the patch.
   - Clean them:
       git clean -fd
     (WARNING: this deletes untracked files/dirs.)

3) CMake uses the wrong params directory (e.g., looks for params17)
   - This is almost always a CMake cache issue.
   - Remove build/ and re-run cmake with -DPARAM_SET=...

4) WSL TLS errors with GitHub HTTPS (e.g., GnuTLS recv error)
   - Prefer SSH clone/push, or try:
       git -c http.version=HTTP/1.1 clone ...
   - Also ensure you are not using a broken proxy configuration.

License / Notes
---------------
This repository is intended as an experiment artifact container (patches + data)
to apply onto an existing HElib source tree. It does not replace the upstream
HElib repository.
