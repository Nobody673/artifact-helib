Artifact: HElib patch + fatboot executable + runtime data (s1..s6)

Reproduce:
  git clone https://github.com/homenc/HElib.git
  cd HElib
  git checkout 3e337a6
  git apply /path/to/helib.patch

  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  cmake --build . --target fatboot -j

  # run from the fatboot directory so s1..s6 are found
  ./fatboot  (add your args here)
