// search_and_lift.cpp
// g++ -O3 -march=native -DNDEBUG -std=c++17 search_and_lift.cpp -lntl -lgmp -lpthread -o search_and_lift

#include <bits/stdc++.h>
#include <NTL/ZZ.h>
#include <NTL/ZZX.h>

#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include <NTL/ZZ_pXFactoring.h>

using namespace std;
using namespace NTL;

/* ===================== Common utils ===================== */

static ZZX poly_mod_coeffs(const ZZX& f, const ZZ& mod)
{
  if (mod == 0) return f;
  ZZX g;
  long df = deg(f);
  if (df < 0) return g;
  g.SetLength(df + 1);
  for (long i = 0; i <= df; ++i) {
    ZZ c = coeff(f, i) % mod;
    if (c < 0) c += mod;
    SetCoeff(g, i, c);
  }
  g.normalize();
  return g;
}

static string ZZX_to_dense_string(const ZZX& f, const ZZ& mod)
{
  ZZX g = (mod==0 ? f : poly_mod_coeffs(f, mod));
  ostringstream out;
  out << "[";
  long dg = deg(g);
  if (dg < 0) { out << "]"; return out.str(); }
  for (long i = 0; i <= dg; ++i) {
    if (i) out << ' ';
    out << coeff(g, i);
  }
  out << "]";
  return out.str();
}

static void write_poly_line(ofstream& fout, const string& name, const ZZX& f, const ZZ& mod)
{
  ZZX g = (mod==0 ? f : poly_mod_coeffs(f, mod));
  fout << name << " = " << ZZX_to_dense_string(g, mod)
       << " (deg=" << deg(g) << ")\n";
}

static ZZX lift_from_mod_p(const ZZ_pX& fp)
{
  ZZX f; f.SetLength(max(0, deg(fp)) + 1);
  for (long i = 0; i <= deg(fp); ++i) SetCoeff(f, i, rep(coeff(fp, i)));
  f.normalize();
  return f;
}

/* ===================== Part A: more_than_k core ===================== */

// IO: read f(x) as sparse list: "<exp> <coef>" per line, coef can be in Z (we will reduce as needed)
struct SparsePoly {
  vector<pair<long, ZZ>> terms; // (exp, coef)
  long max_exp = -1;
};

static SparsePoly read_sparse_terms(const string& path)
{
  ifstream fin(path);
  if (!fin) throw runtime_error("Error: cannot open file " + path);

  SparsePoly sp;
  long e;
  ZZ c;
  while (fin >> e >> c) {
    sp.terms.push_back({e, c});
    sp.max_exp = max(sp.max_exp, e);
  }
  if (sp.terms.empty()) throw runtime_error("Error: empty f_file: " + path);
  return sp;
}

static ZZX build_ZZX_from_terms(const SparsePoly& sp)
{
  ZZX f; clear(f);
  for (auto &tc : sp.terms) {
    SetCoeff(f, tc.first, tc.second);
  }
  f.normalize();
  return f;
}

static ZZ_pX build_ZZ_pX_from_terms_mod_p(const SparsePoly& sp, long p)
{
  ZZ_p::init(to_ZZ(p));
  ZZ_pX f;
  for (auto &tc : sp.terms) {
    ZZ cc = tc.second % to_ZZ(p);
    if (cc < 0) cc += to_ZZ(p);
    ZZ_p coef = conv<ZZ_p>(cc);
    if (coef != 0) SetCoeff(f, tc.first, coef);
  }
  f.normalize();
  return f;
}

static vector<long> all_divisors(long d) {
  vector<long> res;
  for (long e = 1; e*e <= d; ++e) {
    if (d % e == 0) {
      res.push_back(e);
      if (e*e != d) res.push_back(d/e);
    }
  }
  sort(res.begin(), res.end());
  return res;
}

static ZZ_pX X_poly() { ZZ_pX X; SetX(X); return X; }

static void Frobenius_X_power(ZZ_pX& y, long p, long e, const ZZ_pXModulus& H) {
  PowerXMod(y, p, H);               // y = X^p (mod h)
  for (long i = 1; i < e; ++i) {
    PowerMod(y, y, p, H);           // y = y^p (mod h) => X^{p^e} (mod h)
  }
}

static ZZ_pX exact_div(const ZZ_pX& a, const ZZ_pX& b) {
  ZZ_pX q, r;
  DivRem(q, r, a, b);
  if (!IsZero(r)) throw runtime_error("[Error] exact_div failed: divisor does not divide.");
  return q;
}

// Returns J_d and (deg(J_d)/d): number of DISTINCT irreducible degree-d factors (squarefree count)
static pair<ZZ_pX,long> exact_degree_d_part(const ZZ_pX& h, long p, long d) {
  if (IsZero(h)) return {ZZ_pX(), 0};

  ZZ_pXModulus Hmod(h);
  ZZ_pX X; SetX(X);

  unordered_map<long, ZZ_pX> J;
  vector<long> divs = all_divisors(d);

  for (long e : divs) {
    ZZ_pX y; Frobenius_X_power(y, p, e, Hmod);
    sub(y, y, X);
    ZZ_pX He = GCD(h, y);

    ZZ_pX Je = He;
    for (long t : divs) {
      if (t >= e) break;
      if (e % t == 0) {
        auto it = J.find(t);
        if (it != J.end() && deg(it->second) >= 0) {
          if (!IsZero(it->second)) Je = exact_div(Je, it->second);
        }
      }
    }
    J[e] = Je;
  }

  ZZ_pX Jd = J[d];
  long cnt_distinct = (deg(Jd) >= 0 ? deg(Jd) / d : 0);
  return {Jd, cnt_distinct};
}

static vector<long> build_exps(long max_exp, bool include_const) {
  vector<long> exps;
  if (include_const) exps.push_back(0);
  long cur = 1;
  for (long i = 0; i <= max_exp; ++i) { exps.push_back(cur); cur <<= 1; }
  return exps;
}

static inline ZZ_pX n_to_g_poly(unsigned long long n, long p, const vector<long>& exps) {
  ZZ_pX g;
  for (size_t i = 0; i < exps.size(); ++i) {
    long ci = (long)(n % (unsigned long long)p); n /= (unsigned long long)p;
    if (ci != 0) SetCoeff(g, exps[i], conv<ZZ_p>(ci));
    if (n == 0) break;
  }
  return g;
}

static void dump_sparse_poly(ofstream& out, const ZZ_pX& a) {
  for (long i = 0; i <= deg(a); ++i) {
    ZZ_p c = coeff(a, i);
    if (c != 0) out << i << " " << rep(c) << "\n";
  }
}

/* ===================== Part B: factor + Hensel lift + group (from factor_lift_group) ===================== */

static vector<ZZ_pX> factor_over_fp_monic_squarefree(const ZZX& hZ, long p, ZZ& lcZZ_out)
{
  ZZ P = to_ZZ(p);
  ZZ_p::init(P);

  ZZ_pX hp = conv<ZZ_pX>(hZ);
  if (deg(hp) != deg(hZ))
    throw runtime_error("deg drop when reducing h mod p (p divides lc(h)?).");

  ZZ_p lc = LeadCoeff(hp);
  if (IsZero(lc)) throw runtime_error("unexpected: lc(h mod p)=0");
  lcZZ_out = rep(lc);

  ZZ_p inv_lc; inv(inv_lc, lc);
  ZZ_pX hp_monic = hp * inv_lc;

  // square-free check
  ZZ_pX dhp; diff(dhp, hp_monic);
  ZZ_pX g = GCD(hp_monic, dhp);
  if (deg(g) > 0) throw runtime_error("h mod p is NOT square-free (has repeated factors) -> pairwise Hensel lift needs square-free.");

  vec_pair_ZZ_pX_long facs;
  CanZass(facs, hp_monic);

  vector<ZZ_pX> res;
  for (long i = 0; i < facs.length(); ++i) {
    if (facs[i].b != 1) throw runtime_error("unexpected multiplicity>1 after square-free check.");
    res.push_back(facs[i].a);
  }
  return res;
}

static pair<ZZX, ZZX> hensel_lift_pair(const ZZX& f_in, ZZX g_in, ZZX h_in, long p, long e)
{
  ZZ P = to_ZZ(p);
  ZZ_p::init(P);

  ZZ_pX fp = conv<ZZ_pX>(f_in);
  ZZ_pX gp = conv<ZZ_pX>(poly_mod_coeffs(g_in, P));

  ZZ_pX hp, r;
  DivRem(hp, r, fp, gp);
  if (!IsZero(r)) throw runtime_error("[Hensel] g does not divide f mod p");
  if (!IsOne(GCD(gp, hp)))
    throw runtime_error("[Hensel] factors not coprime mod p (repeated factors?)");

  ZZX g = poly_mod_coeffs(lift_from_mod_p(gp), P);
  ZZX h = poly_mod_coeffs(lift_from_mod_p(hp), P);

  ZZ_pX d, s, t;
  XGCD(d, s, t, gp, hp);
  if (deg(d) != 0 || rep(ConstTerm(d)) != 1)
    throw runtime_error("[Hensel] XGCD failed");

  ZZ pk = P;
  for (long k = 1; k < e; ++k) {
    ZZX gh; mul(gh, g, h);
    ZZX res = f_in - gh;

    for (long i = 0; i <= deg(res); ++i) {
      if (!IsZero(coeff(res, i) % pk)) {
        ostringstream oss;
        oss << "[Hensel] residual not divisible by p^k at k=" << k;
        throw runtime_error(oss.str());
      }
    }

    ZZX eZ; eZ.SetLength(max(0, deg(res)) + 1);
    for (long i = 0; i <= deg(res); ++i) SetCoeff(eZ, i, coeff(res, i) / pk);

    ZZ_pX ep = conv<ZZ_pX>(poly_mod_coeffs(eZ, P));
    ZZ_pX up = (t * ep) % gp;
    ZZ_pX vp = (s * ep) % hp;

    ZZX uZ = lift_from_mod_p(up);
    ZZX vZ = lift_from_mod_p(vp);

    ZZX pku, pkv;
    mul(pku, uZ, pk);
    mul(pkv, vZ, pk);

    g += pku;
    h += pkv;

    pk *= P;
    g = poly_mod_coeffs(g, pk);
    h = poly_mod_coeffs(h, pk);
  }

  return {g, h};
}

static vector<ZZX> hensel_lift_all_factors_pairwise(ZZX Fcur,
                                                    vector<ZZ_pX> tilde_factors,
                                                    long p, long e)
{
  vector<ZZX> out;
  ZZ P = to_ZZ(p);
  ZZ_p::init(P);

  ZZ pk(1);
  for (long i = 0; i < e; ++i) pk *= P;

  ZZ_pX Fpcur = conv<ZZ_pX>(Fcur);

  for (size_t i = 0; i + 1 < tilde_factors.size(); ++i) {
    size_t pick = i;
    for (; pick < tilde_factors.size(); ++pick) {
      ZZ_pX q, r;
      DivRem(q, r, Fpcur, tilde_factors[pick]);
      if (IsZero(r)) break;
    }
    if (pick == tilde_factors.size())
      throw runtime_error("no remaining factor divides current remainder (mod p)");
    if (pick != i) swap(tilde_factors[i], tilde_factors[pick]);

    const ZZ_pX& gp = tilde_factors[i];

    ZZ_pX h0_p, r;
    DivRem(h0_p, r, Fpcur, gp);
    if (!IsZero(r)) throw runtime_error("division failed in Fp (unexpected)");

    auto GH = hensel_lift_pair(Fcur, conv<ZZX>(gp), conv<ZZX>(h0_p), p, e);

    ZZX G = poly_mod_coeffs(GH.first,  pk);
    ZZX H = poly_mod_coeffs(GH.second, pk);

    out.push_back(G);

    Fcur  = H;
    Fpcur = h0_p;
  }

  out.push_back(poly_mod_coeffs(Fcur, pk));
  return out;
}

// Core: factor/lift hZ mod p^e, then group by degree d_target -> a(X) and {h_i}
static void factor_lift_and_group(const ZZX& hZ_in, long p, long e, long d_target,
                                  ZZX& a_out, vector<ZZX>& hd_out, vector<ZZX>& lifted_all_out)
{
  ZZ P = to_ZZ(p);
  ZZ mod_pe(1);
  for (long i = 0; i < e; ++i) mod_pe *= P;

  ZZX hZ = poly_mod_coeffs(hZ_in, mod_pe);

  // Step1: factor monic(h mod p), require squarefree for this hensel strategy
  ZZ lcZZ;
  vector<ZZ_pX> tilde = factor_over_fp_monic_squarefree(hZ, p, lcZZ);

  // h_monic = h * inv(lc) mod p^e
  ZZ lcInv;
  InvMod(lcInv, lcZZ, mod_pe);
  ZZX h_monic = poly_mod_coeffs(hZ * lcInv, mod_pe);

  // Step3: lift factors of h_monic
  vector<ZZX> lifted = hensel_lift_all_factors_pairwise(h_monic, tilde, p, e);

  // multiply lc back into first factor so product matches h
  if (!lifted.empty()) {
    lifted[0] *= lcZZ;
    lifted[0] = poly_mod_coeffs(lifted[0], mod_pe);
  }

  // group
  ZZX a(1);
  a = poly_mod_coeffs(a, mod_pe);
  vector<ZZX> hd;

  for (auto &fi_raw : lifted) {
    ZZX fi = poly_mod_coeffs(fi_raw, mod_pe);
    if (deg(fi) == d_target) hd.push_back(fi);
    else {
      ZZX tmp; mul(tmp, a, fi);
      a = poly_mod_coeffs(tmp, mod_pe);
    }
  }

  // verify product
  ZZX check = a;
  for (auto &fi : hd) {
    ZZX tmp; mul(tmp, check, fi);
    check = poly_mod_coeffs(tmp, mod_pe);
  }
  if (check != poly_mod_coeffs(hZ, mod_pe))
    throw runtime_error("[LiftCheck] product mismatch mod p^e");

  a_out = a;
  hd_out = hd;
  lifted_all_out = lifted;
}

/* ===================== Main ===================== */

int main(int argc, char** argv) {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  if (argc < 13) {
    cerr << "Usage:\n"
         << "  " << argv[0] << " p e d k max_exp include_const f_file start_n end_n batch_size progress_interval out_prefix\n\n"
         << "Meaning:\n"
         << "  Scan g over F_p (power-of-two exps) to find h=f+g having >=k distinct degree-d irreducible factors over F_p;\n"
         << "  then lift/factor h in Z/p^e and output:\n"
         << "    out_prefix_final.txt  (contains p,e,d,a(X), and all degree-d h_i(X) modulo p^e)\n"
         << "    out_prefix_g.txt      (sparse g over F_p)\n"
         << "    out_prefix_h_modp.txt (sparse h over F_p)\n";
    return 1;
  }

  long p         = atol(argv[1]);
  long e_lift    = atol(argv[2]);
  long d         = atol(argv[3]);
  long k_req     = atol(argv[4]);
  long max_exp   = atol(argv[5]);
  bool include_c = (atoi(argv[6]) != 0);
  string f_file  = argv[7];
  unsigned long long start_n = strtoull(argv[8], nullptr, 10);
  unsigned long long end_n   = strtoull(argv[9], nullptr, 10);
  unsigned long long batch_size = strtoull(argv[10], nullptr, 10);
  unsigned long long progress_interval = strtoull(argv[11], nullptr, 10);
  string out_prefix = argv[12];

  ZZ P = to_ZZ(p);
  ZZ mod_pe(1);
  for (long i = 0; i < e_lift; ++i) mod_pe *= P;

  // Read f (sparse exp coef), build both mod p and mod p^e versions
  SparsePoly sp = read_sparse_terms(f_file);

  // f over ZZX (we will reduce mod p^e)
  ZZX fZ = build_ZZX_from_terms(sp);
  fZ = poly_mod_coeffs(fZ, mod_pe);

  // f over F_p for scanning
  ZZ_p::init(P);
  ZZ_pX f_p = build_ZZ_pX_from_terms_mod_p(sp, p);

  vector<long> exps = build_exps(max_exp, include_c);

  cerr << "[Info] p=" << p << " e=" << e_lift << " d=" << d << " k=" << k_req
       << " exps={";
  for (size_t i = 0; i < exps.size(); ++i) cerr << exps[i] << (i+1==exps.size()? "":",");
  cerr << "}\n[Info] Range: [" << start_n << "," << end_n << "), batch=" << batch_size << "\n";

  unsigned long long checked = 0;

  for (unsigned long long base = start_n; base < end_n; ) {
    unsigned long long lim = min(end_n, base + batch_size);

    for (unsigned long long n = base; n < lim; ++n) {
      ZZ_p::init(P);

      ZZ_pX g_p = n_to_g_poly(n, p, exps);
      ZZ_pX h_p = f_p + g_p;

      // core test: count distinct degree-d irreducibles
      auto [Jd, cnt_distinct] = exact_degree_d_part(h_p, p, d);

      if (cnt_distinct >= k_req) {
        // Candidate hit over F_p; now attempt lift+factor over Z/p^e
        try {
          // build gZ by lifting coefficients 0..p-1
          ZZX gZ = lift_from_mod_p(g_p);
          gZ = poly_mod_coeffs(gZ, mod_pe);

          // hZ = fZ + gZ mod p^e
          ZZX hZ = fZ + gZ;
          hZ = poly_mod_coeffs(hZ, mod_pe);

          // factor/lift/group
          ZZX a;
          vector<ZZX> hd, lifted_all;
          factor_lift_and_group(hZ, p, e_lift, d, a, hd, lifted_all);

          if (out_prefix != "-") {
            // write g and h (mod p) in sparse form for debugging/repro
            ofstream gout(out_prefix + "_g.txt");
            dump_sparse_poly(gout, g_p);
            gout.close();

            ofstream hout(out_prefix + "_h_modp.txt");
            dump_sparse_poly(hout, h_p);
            hout << "\n# DISTINCT degree-" << d << " part J_d:\n";
            dump_sparse_poly(hout, Jd);
            hout << "\n";
            hout.close();

            // final report (ONLY what you want)
ofstream fout(out_prefix + "_final.txt");
fout << "p = " << p << "\n";
fout << "e = " << e_lift << "\n";
fout << "d = " << d << "\n";
fout << "mod = " << mod_pe << "\n";
//fout << "n = " << n << "\n\n";

// add f and g (mod p^e)
write_poly_line(fout, "g", gZ, mod_pe);
write_poly_line(fout, "f", fZ, mod_pe);

// required outputs
write_poly_line(fout, "a", a, mod_pe);
fout << "num_h = " << hd.size() << "\n";
for (size_t i = 0; i < hd.size(); ++i) {
  ostringstream nm; nm << "h_" << (i+1);
  write_poly_line(fout, nm.str(), hd[i], mod_pe);
}

fout.close();

          }

          cout << "[OK] Found & lifted at n=" << n
               << " (distinct degree-" << d << " over F_p: " << cnt_distinct << ")\n";
          if (out_prefix != "-") cerr << "Wrote: " << out_prefix << "_final.txt\n";
          return 0;

        } catch (const exception& ex) {
          // hit over F_p but cannot lift/factor over Z/p^e with our strategy -> continue scanning
          cerr << "[HIT but SKIP] n=" << n << " because lift/factor failed: " << ex.what() << "\n";
        }
      }

      ++checked;
      if (progress_interval && (checked % progress_interval == 0)) {
        cerr << "[Progress] checked " << checked << " g's (up to n=" << n << ")\n";
      }
    }

    base = lim;
  }

  cerr << "[Done] No successful (hit+lift) in range. Total checked: " << checked << "\n";
  return 0;
}
