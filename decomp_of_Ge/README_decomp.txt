Magic Polynomial Search Data (Digit Extraction in BGV/HElib)
=========================================================

[中文说明]
--------
本文件夹包含我们对 BGV/HElib 自举（bootstrapping）中 **digit extraction** 阶段所用的
**magic polynomial** 进行搜索与提升（Hensel lifting）得到的实验数据。

我们共整理并搜索了来自三篇文章的 25 组不同明文参数 (p, e, d)：

  - ChenHan2018
  - HaleviShoup2021
  - MaHuangWangWang2024

其中，ChenHan2018 使用的两组参数 (17,4,40) 与 (127,3,36) 与 HaleviShoup2021 中相同。
另外，文献中给出的 (5,6,-) 与 (31,3,-) 未给出槽次数/扩张度 d 的取值，因此我们自行选择了
共 18 个不同的 d 值进行搜索实验（以覆盖可能的 slot-degree 选择）。

每次成功搜索会输出：
  - g(X): 用于修正/搜索的 power-of-two 稀疏多项式（系数模 p）
  - h(X)=f(X)+g(X): 满足在 F_p 上含有足够多 degree-d 不可约因子的候选
  - 以及将 h(X) 提升到 Z/p^eZ 后的分解分组结果：a(X) 与一组 degree-d 因子 h_i(X)

实验程序已在 GitHub 中上传，文件名为：search_and_lift.cpp（编译后可执行文件 search_and_lift）。

----------------------------------------------------------------
[English overview]
----------------
This folder contains search results for **magic polynomials** used in the **digit extraction**
stage of BGV bootstrapping (HElib). We collected 25 parameter configurations (p, e, d) from:

  - ChenHan2018
  - HaleviShoup2021
  - MaHuangWangWang2024

ChenHan2018 shares (p,e,d) = (17,4,40) and (127,3,36) with HaleviShoup2021.
For the parameter sets (5,6,-) and (31,3,-), the papers do not specify the slot degree d.
We therefore tested 18 different choices of d.

For each successful hit, the program outputs g(X), h(X)=f(X)+g(X) (mod p), and the lifted
factor grouping modulo p^e: a(X) and the degree-d factors h_i(X).

Build & Run: search_and_lift
============================

Compilation
-----------
Requires: NTL, GMP, pthread.

  g++ -O3 -march=native -DNDEBUG -std=c++17 search_and_lift.cpp -lntl -lgmp -lpthread -o search_and_lift

Command-line usage
------------------
  ./search_and_lift p e d k max_exp include_const f_file start_n end_n batch_size progress_interval out_prefix

Example:
  ./search_and_lift 31 3 44 2 6 1 ./f_sparse.txt 0 8000000 1000 50000 ./out_ch_31_44

Parameter meanings
------------------
  p
    Prime modulus base (the program searches over F_p, then lifts to Z/p^eZ).

  e
    Lift precision exponent. The lifting target modulus is p^e.

  d
    Target slot degree (the degree of irreducible factors that we require inside h=f+g over F_p).

  k
    Required number of DISTINCT irreducible factors of degree d in h(X) over F_p.
    The scan accepts a candidate when h has at least k such factors (squarefree count).

  max_exp
    Controls the allowed power-of-two monomial exponents used in g(X).
    The exponent set is:
      { 1, 2, 4, ..., 2^{max_exp} }
    and additionally includes exponent 0 if include_const=1.

  include_const
    0/1 flag. If 1, g(X) may include a constant term (exponent 0). If 0, no constant term.

  f_file
    Input polynomial f(X) in sparse format (see below). Coefficients can be any integers;
    the program reduces them as needed mod p and mod p^e.

  start_n, end_n
    Scan range for enumerating g(X). Each integer n encodes the coefficients of g(X)
    in base p along the exponent list. The scan covers n in [start_n, end_n).

  batch_size
    Internal batching size for the scan loop (performance only).

  progress_interval
    Print a progress message every this many tested candidates (0 disables progress prints).

  out_prefix
    Output file prefix. If set to "-" the program does not write files (it still prints "[OK]" on success).

Search procedure (what is being tested)
---------------------------------------
1) Read sparse f(X) from f_file, build:
     - f_p in F_p[X] for scanning
     - fZ in Z[X] reduced mod p^e for lifting

2) Enumerate g(X) over F_p supported on power-of-two exponents (and optional constant term):
     g(X) = sum_i c_i X^{exp_i},   c_i in {0,...,p-1}
   Coefficients are derived from digits of n in base p.

3) For each candidate h(X) = f(X) + g(X) over F_p:
     - Count DISTINCT irreducible degree-d factors of h(X) over F_p
       (via the exact degree-d part using Frobenius/GCD).

4) If h(X) has at least k such factors, attempt to lift and factor/group modulo p^e:
     - Require h mod p to be square-free (for the pairwise Hensel-lift strategy used here).
     - Lift factors to Z/p^eZ and group them:
         h(X) ≡ a(X) * Π_i h_i(X)   (mod p^e),
       where each h_i has degree exactly d, and a(X) absorbs all remaining factors.

Output files
============
On the first successful hit (that also lifts/factors), the program writes:

  1) out_prefix_final.txt
     Human-readable dense format. Contains:
       p, e, d, mod (=p^e)
       g  (mod p^e, lifted from F_p coefficients)
       f  (mod p^e)
       a  (mod p^e)
       num_h
       h_1, h_2, ... (each degree d, mod p^e)
     Each polynomial is printed as a dense coefficient vector:
       [c0 c1 c2 ...] (deg=...)

  2) out_prefix_g.txt
     Sparse g(X) over F_p:
       one term per line:  <exp> <coef>
     (only non-zero terms are printed)

  3) out_prefix_h_modp.txt
     Sparse h(X)=f(X)+g(X) over F_p in the same "<exp> <coef>" format,
     and additionally prints the DISTINCT degree-d part J_d for debugging/repro:
       # DISTINCT degree-d part J_d:
       <exp> <coef>
       ...

Input file format: f_sparse.txt
===============================
f_file is a text file where each line is:
    <exp> <coef>

  - exp: non-negative integer exponent
  - coef: integer coefficient (can be negative or larger than p; it will be reduced)
  - Terms not listed are assumed to have coefficient 0.
  - The file must contain at least one term.

A sample input file is provided as: f_sparse.txt
