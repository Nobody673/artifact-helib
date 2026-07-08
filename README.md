README (artifact-helib patches)
===========================

This file shows how to download the patch repository and apply a selected patch to a clean HElib tree at `~/HElib-clean`, then build and run `fatboot`.

1. Download
-----------

```bash
mkdir -p ~/tmp
cd ~/tmp
git -c http.version=HTTP/1.1 clone --depth 1 https://github.com/Nobody673/artifact-helib.git
```

2. Prepare a test branch in the clean HElib repo
------------------------------------------------

```bash
cd ~/HElib-clean
git status
git checkout -b patch-test
```

3. Apply a patch
----------------

```bash
PATCH=~/tmp/artifact-helib/patches/GNXXX.patch
git apply --check -p1 "$PATCH"
git apply --3way  -p1 "$PATCH"
```

4. Configure and build
----------------------

```bash
cd ~/HElib-clean
rm -rf build
mkdir build
cd build

cmake .. -DPARAM_SET=paramsXXX
cmake --build . -j32
```

5. Run
------

```bash
./fatboot i=3 h=24 t=1 newbts=1 newks=1 thick=1 repeat=1
```

Parameter presets:

```bash
# p = 17
./fatboot i=0 h=24 t=2 newbts=1 newks=1 thick=1 repeat=1

# p = 127
./fatboot i=1 h=22 t=1 newbts=1 newks=1 thick=1 repeat=1

# p = 257
./fatboot i=2 h=22 t=1 newbts=1 newks=1 thick=1 repeat=1

# p = 8191
./fatboot i=3 h=24 t=1 newbts=1 newks=1 thick=1 repeat=1

# p = 65537
./fatboot i=4 h=24 t=1 newbts=1 newks=1 thick=1 repeat=1
```

6. Revert to the clean version
------------------------------

```bash
cd ~/HElib-clean
git reset --hard
git clean -fd
```
