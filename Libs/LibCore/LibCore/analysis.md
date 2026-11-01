# Audit LibCore — Rapport complet

**Date :** 2026-10-22 (ré-audit ; audit initial du 2026-10-11)
**Périmètre :** 131 headers + 242 fichiers source (C++17) + 1 shader GLSL
**Dépendances :** Qt6 (Core, Gui, Widgets, Charts, Svg), OpenGL / GLEW, STL, intrinsèques AVX, API Windows (`cpu.cpp`)
**Domaines :** M — Mathématiques · P — Polynômes, aléatoire, graphes · G — Géométrie 2D · K — Géométrie 3D de base · R — Primitives 3D volumiques · C — Courbes et surfaces paramétriques · S — Champs scalaires et grilles · V — Champs vectoriels et de couleur · N — Bruit et échantillonnage · T — Maillages et graphes géométriques · U — Utilitaires, couleur, GPU
**Auditeurs :** ré-audit du 2026-10-22 : 8 agents parallèles (M+P, G, K+V, R, C+T, S, N, U) vérifiant chaque entrée ouverte contre le code actuel et relisant les fichiers modifiés depuis le 2026-10-11 (3 entrées corrigées sans suivi : B-P06, B-G42, B-G73 ; 6 nouvelles entrées : B-P11, B-P12, D-P08, B-G106, B-R63, B-S66) ; audit initial : 10 agents parallèles (M, P, G, K, R, C, S, N, T, U+V), puis vérification de chaque entrée dans le code (directe pour M, P, C, T, U, V, N ; agents vérificateurs indépendants pour K, R, S, G, avec contre-vérification des principaux 🔴) ; 11 constats confirmés par exécution de tests compilés (domaine M)
**Hors périmètre :** `Include/Extern/renderdoc_app.h` (dépendance tierce) ; les grandes tables constantes de `Source/scalarfield.cpp` (l. 432-707 et 844-1124) ont été parcourues sans analyse entrée par entrée

---

## Légende

| Symbole | Criticité | Description |
|---------|-----------|-------------|
| 🔴 | **CRITIQUE** | Comportement indéfini, corruption mémoire, crash, boucle infinie, résultat entièrement faux |
| 🟠 | **HAUTE** | Bug significatif produisant des résultats incorrects dans des cas courants |
| 🟡 | **MOYENNE** | Bug de cas-limite, crash sur entrée dégénérée |
| 🔵 | **BASSE** | Imprécision mineure, sentinelle ambiguë, code mort |
| 📝 | **DOC** | Documentation manquante, incorrecte ou trompeuse |
| ⚡ | **OPT** | Opportunité d'optimisation |
| ⬜ / ✅ | **Suivi** | Ouvert / Corrigé |

---

## Bilan chiffré

| Catégorie | Critique 🔴 | Haute 🟠 | Moyenne 🟡 | Basse 🔵 | Total | Corrigés ✅ |
|-----------|:-----------:|:--------:|:----------:|:--------:|:-----:|:-----------:|
| M — Mathématiques | 3 | 10 | 16 | 12 | **41** | **9** |
| P — Polynômes, aléatoire, graphes | 1 | 0 | 4 | 7 | **12** | **2** |
| G — Géométrie 2D | 15 | 44 | 31 | 16 | **106** | **44** |
| K — Géométrie 3D de base | 9 | 19 | 15 | 16 | **59** | **21** |
| R — Primitives 3D volumiques | 9 | 33 | 12 | 9 | **63** | **16** |
| C — Courbes et surfaces paramétriques | 2 | 3 | 12 | 6 | **23** | **9** |
| S — Champs scalaires et grilles | 13 | 16 | 24 | 12 | **65** | **13** |
| V — Champs vectoriels et de couleur | 2 | 1 | 4 | 8 | **15** | **5** |
| N — Bruit et échantillonnage | 6 | 7 | 15 | 13 | **41** | **6** |
| T — Maillages et graphes géométriques | 2 | 7 | 6 | 5 | **20** | **2** |
| U — Utilitaires, couleur, GPU | 2 | 5 | 15 | 20 | **42** | **5** |
| **Total bugs** | **64** | **145** | **154** | **124** | **487** | **132** |
| Documentation 📝 | — | — | — | — | **142** | **80** |
| Optimisations ⚡ | — | — | — | — | **12** | **1** |

| Restants à corriger | 🔴 | 🟠 | 🟡 | 🔵 | Bugs | 📝 | ⚡ |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Ouverts** | **20** | **119** | **122** | **94** | **355** | **62** | **11** |

3 entrées écartées comme faux positifs à la vérification ne sont pas comptées ci-dessus ; leur ID est conservé, barré, dans le corps du rapport et dans les index.

---

## Bugs Critiques 🔴

### M — Mathématiques

#### 🔴 ✅ B-M01 — `Source/aa.cpp:1392` — `Aa_pow(const Aa&, const Aa&)` — Lecture hors limites de devs_b[dev_b]/devs_e[dev_e] dans un fprintf de débogage

Le `fprintf` de débogage lit `devs_b[dev_b]` et `devs_e[dev_e]` à chaque itération, avant les tests `dev_b < length_b` / `dev_e < length_e`. Dès que l'une des listes d'indices est épuisée (cas normal quand les symboles diffèrent), on lit une case après la fin d'un tableau du tas (UB). En plus, la sortie standard est polluée à chaque appel.
**Correctif :** supprimer le `fprintf` de la ligne 1392. Supprimer les traces de débogage `fprintf` (B-M31) suffit à corriger ce bug : aucune autre modification n'est nécessaire.

#### 🔴 ✅ B-M02 — `Source/aa.cpp:1695` — `Aa_pow(const Aa&, const double&), Aa_pow(const double&, const Aa&)` — Boucle de copie jusqu'à length+1 : lecture hors limites des indices/déviations source

`for (i = 0; i < length_b + 1; i++) { result.indexes[i] = ind_b[i]; result.deviations[i] = beta * devs_b[i]; }` lit `ind_b[length_b]` et `devs_b[length_b]`, hors du tableau (UB), avant que la dernière case soit réécrite. Même motif dans `Source/aa.cpp:1762` (`ind_e[length_e]`, `devs_e[length_e]`). Si l'affine source n'a aucun symbole (length 0, pointeurs nuls), c'est un déréférencement de nullptr.
**Correctif appliqué :** les deux boucles de copie s'arrêtent à `length_b` (resp. `length_e`) ; la dernière case est écrite ensuite par `inclast()` et `gamma`. Vérifié avec un allocateur à pages gardées (chaque `new[]` est placé en fin de page, la page suivante est inaccessible) : `Aa_pow(Aa, double)` et `Aa_pow(double, Aa)` sur une affine à un symbole de bruit provoquent une violation d'accès avec l'ancien code (lecture de `ind[length]`) et passent avec le nouveau, avec les mêmes résultats (centre 3,041829, rayon 2,615026 ; centre 2,913929, rayon 1,086071) ; sur une affine constante (tableaux nuls), l'ancien code déréférence un pointeur nul, le nouveau ne plante plus. Vérifié par compilation.

#### 🔴 B-M03 — `Source/matrix-algebra.cpp:236` — `Matrix::SingularValueDecomposition()` — SVD fausse : indexation ligne-major mêlée à operator() colonne-major, S non affecté

L'algorithme (Bidiagonalize/GolubKahanStep, repris de Wild Magic, où `m[i][j] = r[3i+j]`) manipule `r[1]`, `r[3]`… comme dans une matrice rangée par lignes, alors que `Matrix` est rangée par colonnes ; en même temps, les mises à jour de L et R passent par `L(row,1)`, `R(0,row)` (operator() colonne-major, l.114-117, 154-157, 177-180, 269-284…). Les deux conventions se contredisent et la décomposition est fausse. De plus, si la boucle de 16 itérations ne converge pas, `S` n'est jamais écrit (aucune affectation hors des `break`). Vérification numérique (g++, code de la bibliothèque) : pour M = Rz(0.3)·diag(3,2,1), on obtient S = (2.935, 2.044, 1) au lieu de (3,2,1) et L·S·R ≠ M ; pour M = [[2,-1,0.3],[1,3,-0.7],[0.5,0.2,1.5]] ou une matrice bidiagonale, S garde sa valeur d'entrée (non initialisée chez l'appelant). Seules les matrices diagonales sont correctement traitées.
**Correctif :** réécrire avec une seule convention, par exemple via `EigenSolveSymmetric` : MᵀM = V Λ Vᵀ, σᵢ = √λᵢ, uᵢ = M vᵢ / σᵢ (avec une orthonormalisation de Gram-Schmidt pour σᵢ ≈ 0), ou remplacer tous les `r[3*i+j]`/`L[...]` par `(*this)(i,j)` de façon cohérente ; et toujours affecter S en sortie de boucle (dernière diagonale A[0], A[4], A[8]).


### P — Polynômes, aléatoire, graphes

#### 🔴 ✅ B-P01 — `Source/polynomial.cpp:564` — `Polynomial::Compose(const Quartic&, const Quadric&)` — Composition quartique∘quadrique fausse (p[1] à la place de p[4], q20 au lieu de q21)

Le calcul de p(q(x)) = p4 q⁴ + p3 q³ + p2 q² + p1 q + p0 utilise `p[1]` comme coefficient de tous les termes en q⁴ (lignes 564, 565, 566, 567, 568, 569, 570 second terme, 571 dernier terme, 572 dernier terme) au lieu de `p[4]`. En outre, le coefficient de x⁷ (ligne 565) vaut 4 q2³ q0 au lieu de 4 q2³ q1 (`q20` utilisé au lieu de `q21`). Le résultat est faux pour toute quartique dont p4 ≠ p1 (en pratique toujours) : coefficient dominant faux, x⁷ faux, et termes en p1 comptés deux fois.
**Correctif :**
```cpp
return Polynomial(p[4] * (q22 * q22),
  p[4] * (4.0 * q22 * q21),
  p[3] * (q22 * q[2]) + p[4] * (6.0 * q22 * q11 + 4.0 * q22 * q20),
  p[3] * (3.0 * q22 * q[1]) + p[4] * (4.0 * q21 * q11 + 12.0 * q22 * q10),
  p[2] * q22 + p[3] * (3.0 * q22 * q[0] + 3.0 * q[2] * q11) + p[4] * (6.0 * q22 * q00 + q11 * q11 + 12.0 * q20 * q11),
  p[2] * 2.0 * q21 + p[3] * (q11 * q[1] + 6.0 * q21 * q[0]) + p[4] * (4.0 * q11 * q10 + 12.0 * q21 * q00),
  p[1] * q[2] + p[2] * (2.0 * q20 + q11) + p[3] * (3.0 * q11 * q[0] + 3.0 * q[2] * q00) + p[4] * (6.0 * q11 * q00 + 4.0 * q20 * q00),
  p[1] * q[1] + p[2] * 2.0 * q10 + p[3] * (3.0 * q[1] * q00) + p[4] * (4.0 * q10 * q00),
  p[0] + p[1] * q[0] + p[2] * q00 + p[3] * (q00 * q[0]) + p[4] * (q00 * q00));
```


### G — Géométrie 2D

#### 🔴 ✅ B-G01 — `Source/box2.cpp:715` — `Box2::RandomOn()` — Paramètre d'interpolation négatif : tous les points hors du périmètre

Pour chaque côté, le paramètre passé à `Lerp` vaut `(u - x) / x` (resp. `(u - y) / y`) alors que `u ∈ [0, x)` : il est dans [-1, 0) et `Lerp(a, b, t) = a + t(b - a)` extrapole le point sur le prolongement du côté, hors de la boîte. 100 % des points renvoyés sont hors du périmètre : résultat entièrement faux. Même motif aux lignes Source/box2.cpp:722, Source/box2.cpp:729 et Source/box2.cpp:734.
**Correctif :**
```cpp
if (u < x) return Vector2::Lerp(a, Vertex(1), u / x);
u -= x;
if (u < y) return Vector2::Lerp(Vertex(1), b, u / y);
u -= y;
if (u < x) return Vector2::Lerp(b, Vertex(2), u / x);
u -= x;
return Vector2::Lerp(Vertex(2), a, u / y);
```

#### 🔴 ✅ B-G02 — `Source/capsule2.cpp:160` — `Capsule2::Normal()` — Facteur (1.0 - r) au lieu de (d - r) : norme constante

Après projection, `n` est le vecteur axe→point de norme `d`. Le vecteur projection→point vaut `n·(d-r)/d`. Le code utilise `n *= (1.0 - r) / d`, ce qui donne un vecteur de norme constante |1-r| quel que soit le point : résultat entièrement faux (sauf coïncidence). Aucun `Vector2::Null` n'est renvoyé pour un point intérieur, et division par zéro si le point est sur l'axe.
**Correctif :**
```cpp
double d = Norm(n);
if (d <= r) return Vector2::Null;
n *= (d - r) / d;
return n;
```

#### 🔴 ✅ B-G03 — `Source/circle2.cpp:539` — `Circle2::Normal()` — Formule entièrement fausse

`y = sqrt(n*n)` est déjà la distance au centre ; le code prend ensuite `sqrt(y) - r` (racine d'une distance) puis multiplie par `(y - r)/y`, retranchant r une seconde fois. Le vecteur renvoyé n'a aucun rapport avec le vecteur projection→point `(d-r)/d·n`. Impacte aussi `CircleArc2::Normal()`.
**Correctif :**
```cpp
Vector2 n = p - c;
double d = Norm(n);
// Au centre (d == 0), la direction est indéfinie : renvoyer Vector2(-r, 0.0) pour éviter la division par zéro
n *= (d - r) / d;
return n;
```

#### 🔴 ✅ B-G04 — `Source/ellipse2.cpp:521` — `Ellipse2::Perimeter()` — Formule de Ramanujan mal recopiée : périmètre toujours négatif

La première approximation de Ramanujan est π[3(a+b) − √((3a+b)(a+3b))] = π[3(a+b) − √(10ab + 3(a²+b²))]. Le code utilise `(a + b)` au lieu de `3.0 * (a + b)` : pour un cercle unité il renvoie π(2 − 4) = −2π. Comme √(10ab+3a²+3b²) > a+b toujours, le résultat est négatif pour toute ellipse.
**Correctif :**
```cpp
return Math::Pi * (3.0 * (a + b) - sqrt(10.0 * a * b + 3.0 * (a * a + b * b)));
```

#### 🔴 ✅ B-G05 — `Source/hexagon2.cpp:350` — `Hexagon2::Intersect(const Circle2&)` — Test inversé et distance au carré comparée au rayon

`R()` renvoie une distance au carré ; le code teste `R(center) > radius`, ce qui (1) compare un carré à une longueur et (2) renvoie vrai quand le cercle est **loin** de l'hexagone. Le prédicat est inversé : résultat faux dans quasiment tous les cas.
**Correctif :**
```cpp
return R(circle.Center()) <= circle.Radius() * circle.Radius();
```

#### 🔴 ✅ B-G06 — `Source/irregularoctogon2.cpp:149` — `IrregularOctogon2::Signed()` — Ordre de tri inversé et formule de l'arête diagonale fausse

`Vector2::Sort()` trie par ordre **croissant** (`evector2.cpp:173`), alors que la suite du code (comme `Inside()`, qui échange explicitement pour avoir q[0] ≥ q[1]) suppose q[0] ≥ q[1]. Le décalage utilisé est (s+t, t) au lieu du sommet (s+t, s). De plus la distance à l'arête diagonale x + y = 2s + t vaut (x + y − 2s − t)/√2, alors que le code calcule `qst * Vector2(√2, √2)` = √2·(x + y − s − 2t). Exemples : p = c + (s+t+1, 0) renvoie √2(1 − t) au lieu de 1 ; au centre avec s = t = 1, renvoie −4,24 au lieu de −2. Faux presque partout.
**Correctif :**
```cpp
Vector2 q = Abs(p - c);
if (q[1] > q[0]) Math::Swap(q[0], q[1]);
const double h = s + t;
double d = sqrt(Math::Min(Segment2(Vector2(h, 0.0), Vector2(h, s)).R(q),
                          Segment2(Vector2(h, s), Vector2(s, h)).R(q)));
bool in = (q[0] <= h) && (q[0] + q[1] <= 2.0 * s + t);
return in ? -d : d;
```

#### 🔴 ✅ B-G07 — `Source/irregularoctogon2.cpp:193` — `IrregularOctogon2::R()` — Mêmes erreurs que Signed() et valeurs non élevées au carré

Même tri croissant erroné et même formule diagonale fausse que `IrregularOctogon2::Signed()` (B-G06, erreur liée mais distincte) ; en outre les branches Source/irregularoctogon2.cpp:204 (`return qst[0]`) et Source/irregularoctogon2.cpp:217 (`Math::Max(..., 0.0)`) renvoient des distances non carrées alors que la doc annonce « squared distance ». Faux presque partout.
**Correctif :**
```cpp
double d = Signed(p); // avec Signed() corrigé (B-G06)
return (d > 0.0) ? d * d : 0.0;
```

#### 🔴 ✅ B-G08 — `Source/octogon2.cpp:172` — `Octogon2::R(), Octogon2::Signed(), Octogon2::Normal()` — Normale intérieure et apothème faux : distance toujours nulle / négative

`normal[0] = Normalized(edge[0].Orthogonal())` pointe vers l'**intérieur** (edge[0] ∝ (−1,1), `Orthogonal()` = (−1,−1) ; `Inside()` reste correct car il teste `< 0`). Pour q dans le premier quadrant, `q * normal[0]` est ≤ 0, donc `d = q * normal[0] - r` est toujours ≤ −r : `R()` renvoie toujours 0, `Signed()` toujours une valeur ≤ −r, `Normal()` toujours `Vector2::Null`, pour tout point. En outre l'offset devrait être l'apothème r·cos(π/8) et non r, l'arête verticale (edge[7]) du domaine réduit q[0] ≥ q[1] est ignorée, et `l > r` compare une projection sur un vecteur `edge[0]` non unitaire (longueur ≈ 0,765) à r. Même motif Source/octogon2.cpp:213 et Source/octogon2.cpp:261.
**Correctif appliqué :** la table `normal[]` (intérieure, utilisée correctement par `Inside()`) est conservée ; une fonction locale `OctogonReducedNormal()` calcule, dans le domaine réduit q[0] ≥ q[1] ≥ 0, le vecteur distance exact à l'arête verticale située à l'apothème r·cos(π/8) et à la première moitié de l'arête diagonale, et le test d'appartenance ; `R()`, `Signed()` et `Normal()` s'appuient dessus. La symétrie en y de `Normal()` reste fausse (B-G72).

#### 🔴 ✅ B-G09 — `Source/pentagon2.cpp:35` — `Pentagon2::Inside(), Pentagon2::R(), Pentagon2::Normal()` — Normales d'arêtes intérieures utilisées comme normales extérieures

`normal[i] = Normalized(edge[i]).Orthogonal()` avec des sommets CCW pointe vers l'**intérieur** (ex. edge[0] = (−0,809 ; 0,588) → normal = (−0,588 ; −0,809)). Toutes les fonctions les utilisent comme normales extérieures :
- `Inside()` (l.197-199) ne teste pas le pentagone retourné mais une région non bornée (ex. p − c = (100, 0) : les 3 tests sont ≤ 0 → true) ;
- `R()` (l.216-225) obtient d < 0 pour tout point extérieur proche d'une arête et renvoie 0 ;
- `Normal()` (l.353-375) renvoie `Vector2::Null` à l'extérieur et un vecteur non nul à l'intérieur.
Fonctions entièrement fausses.
**Correctif :**
```cpp
const Vector2 Pentagon2::normal[5] = {
  -edge[0].Orthogonal(), -edge[1].Orthogonal(), -edge[2].Orthogonal(),
  -edge[3].Orthogonal(), -edge[4].Orthogonal() };
```

#### 🔴 ✅ B-G10 — `Source/pentagon2.cpp:277` — `Pentagon2::Signed()` — Adaptation du code d'I. Quilez entièrement fausse

Le code calcule `q` à partir de `p` mais (1) ignore le centre `c`, (2) utilise `p` au lieu de `q` dans les plis l.279-281, (3) renvoie `Norm(p) * Math::Sign(p[1])` au lieu de `Norm(q) * Math::Sign(q[1])` (q calculé puis ignoré), (4) omet l'inversion `q[1] = -q[1]` nécessaire à l'orientation de la classe (sommet en haut), (5) utilise r (rayon circonscrit) là où la formule attend l'apothème. Point mineur : (6) `abs()` sur un `double` (la surcharge globale existe sur MSVC/libstdc++). Le résultat n'a aucun rapport avec la distance signée.
**Correctif :**
```cpp
const double kx = 0.809016994, ky = 0.587785252, kz = 0.726542528; // cos, sin, tan(pi/5)
Vector2 q = p - c;
q[1] = -q[1];
q[0] = fabs(q[0]);
q -= 2.0 * Math::Min(Vector2(-kx, ky) * q, 0.0) * Vector2(-kx, ky);
q -= 2.0 * Math::Min(Vector2(kx, ky) * q, 0.0) * Vector2(kx, ky);
const double ri = r * kx; // apothème
q -= Vector2(Math::Clamp(q[0], -ri * kz, ri * kz), ri);
return Norm(q) * Math::Sign(q[1]);
```

#### 🔴 ✅ B-G11 — `Source/pentagon2.cpp:418` — `Pentagon2::Intersect(const Ray2&, double&, double&)` — tb jamais mis à jour, paramètre t de signe opposé

(1) La branche « sortie » écrit `ta = Math::Min(tb, t);` au lieu de `tb = ...` : `tb` reste +∞, `ta` est écrasé, et la fonction renvoie presque toujours true. (2) Pour (o + t·d − pᵢ)·n = 0, t = (pᵢ − o)·n / e ; le code calcule `(o − pᵢ)·n / e`, opposé de la vraie racine. (3) Les normales `Edge(i).Orthogonal()` sont intérieures, ce qui inverse en plus l'appariement entrée/sortie. (4) Un rayon parallèle à une arête et situé à l'extérieur n'est pas rejeté. Le résultat est faux pour quasiment tout rayon.
**Correctif :**
```cpp
for (int i = 0; i < 5; i++)
{
  const Vector2& ni = normal[i]; // normale extérieure (table corrigée par B-G09)
  double e = ray.Direction() * ni;
  double dist = (ray.Origin() - Vertex(i)) * ni;
  if (fabs(e) < epsilon) { if (dist > 0.0) return false; continue; }
  double t = -dist / e;
  if (e < 0.0) ta = Math::Max(ta, t); else tb = Math::Min(tb, t);
}
return ta < tb;
```

#### 🔴 ✅ B-G12 — `Source/quad2.cpp:170` — `Quadrangle2::Normal(), Quadrangle2::R()` — Normales d'arêtes intérieures : résultat inversé

`ne[i] = e[i].Orthogonal()` pointe vers l'intérieur pour un quadrangle CCW, et le test `neq > 0.0` sélectionne donc les points situés du côté **intérieur** de l'arête. Tout point intérieur obtient un vecteur non nul (cas « arête », `neq * ne[i]`), alors qu'un point extérieur face à une arête peut obtenir `Vector2::Null` ou une valeur fausse (ex. carré unité CCW, q = (0,5 ; −1) → renvoie (0 ; −2) au lieu de (0 ; −1)). `R()` (Source/quad2.cpp:142) hérite de l'erreur et est faux pour tout point. Les tests de secteurs de sommets sont, eux, corrects pour une normale extérieure. Résultat entièrement faux.
**Correctif :**
```cpp
ne[i] = -e[i].Orthogonal(); // normale extérieure
```

#### 🔴 ✅ B-G13 — `Source/triangle2.cpp:131` — `Triangle2::OrthoCenter()` — Coordonnées barycentriques homogènes non normalisées

Les poids 1/(b²+c²−a²), 1/(c²+a²−b²), 1/(a²+b²−c²) (∝ tan A, tan B, tan C) sont les coordonnées barycentriques **homogènes** de l'orthocentre ; `BaryCenter()` les utilise directement sans les diviser par leur somme. Pour un triangle équilatéral de côté 1, poids (1,1,1) → renvoie 3 fois le centre. Faux pour tout triangle.
**Correctif appliqué :** les poids 1/(b+c−a), 1/(c+a−b), 1/(a+b−c) sont multipliés par le produit des dénominateurs (pas de division par zéro pour un triangle rectangle, où l'orthocentre est le sommet de l'angle droit), puis normalisés.
```cpp
const double ka = b + c - a, kb = c + a - b, kc = a + b - c;
Vector w(kb * kc, kc * ka, ka * kb);
return BaryCenter(w / (w[0] + w[1] + w[2]));
```

#### 🔴 ✅ B-G14 — `Source/triangle2.cpp:248` — `Triangle2::Inside(const Circle2&)` — Test inversé : renvoie toujours faux

`n` est la normale intérieure (arêtes CCW, `Orthogonal()`, convention confirmée par `Inside(Vector2)` qui teste `Orthogonal()·vecp < 0`), donc `d = (c - p[i]) * n` est la distance positive du centre à l'arête pour un centre intérieur. Le cercle est inclus si d ≥ r pour les trois arêtes. Le code rejette dès que `d >= -r`, toujours vrai pour un centre intérieur ; et un centre extérieur ne peut être à plus de r à l'extérieur des trois arêtes à la fois : la fonction renvoie toujours false.
**Correctif :**
```cpp
if (d < r)
{
  return false;
}
```

#### 🔴 ✅ B-G15 — `Source/triangle2.cpp:349` — `Triangle2::Normal(), Triangle2::R()` — Test de côté inversé pour un triangle CCW

`e[i].Orthogonal()` est la normale intérieure pour un triangle CCW (convention de `Inside()`, `Signed()`, `Equilateral()`), mais le cas « arête » est sélectionné si `pa * e[i].Orthogonal() >= 0.0`, c'est-à-dire pour un point **intérieur**. Exemple : triangle (0,0),(1,0),(0,1) : q = (0,2 ; 0,2) renvoie (0 ; 0,2) au lieu de 0, et q = (0,5 ; −1) renvoie `Vector2::Null` au lieu de (0 ; −1). Seules les régions de sommets sont justes ; `R()` est > 0 à l'intérieur et nul face aux arêtes, et `Intersect(const Circle2&)` est faux en conséquence (même motif que B-G12).
**Correctif :**
```cpp
if (pa * e[i].Orthogonal() <= 0.0)
```


### K — Géométrie 3D de base

#### 🔴 ✅ B-K01 — `Source/camera.cpp:668` — `Camera::LeftRightFPS(), Camera::UpDownRoundFPS()` — Le point visé est réfléchi derrière l'œil à chaque appel

`LeftRightFPS` : `e = eye - at` pointe vers l'arrière, puis `SetAt(Eye() + e_tourné)` place la cible derrière l'œil : pour a = 0, at' = 2·eye − at, la direction de visée est inversée. `UpDownRoundFPS` (Source/camera.cpp:684) : `z` = direction de visée tournée, puis `SetAt(Eye() - z * length)` : même inversion. Chaque appel retourne la caméra de 180°. De plus `left` est calculé pour rien (l.666, l.680-681).
**Correctif :**
```cpp
  SetAt(Eye() - e);              // LeftRightFPS, l.668
  SetAt(Eye() + z * length);     // UpDownRoundFPS, l.684
```

#### 🔴 ✅ B-K02 — `Source/segment.cpp:404` — `Segment::Intersect(const Box&), RayStep::Intersect(const Box&)` — Test SAT segment/boîte : vecteur du segment utilisé au lieu des demi-dimensions de la boîte

Dans les six tests, le terme censé être la demi-dimension de la boîte est `ba[i]` (composante signée, éventuellement négative, du vecteur b − a du segment) au lieu de la demi-diagonale de la boîte ; la boîte n'intervient que par son centre. Le résultat ne dépend pas de la taille de la boîte : faux dans la quasi-totalité des cas (une petite boîte est « intersectée » par tout segment long ; un segment court qui la traverse est rejeté si ses composantes sont négatives). Occurrences : Source/segment.cpp:404, 408, 412, 415, 418, 421 et Source/raystep.cpp:158, 162, 166, 169, 172, 175.
**Correctif :** demi-dimensions de la boîte à la place de `ba`, et seuils multipliés par une tolérance relative `k = 1 + 1e-12` (sans elle, environ 10 % de faux négatifs quand le segment touche exactement la boîte : extrémité sur une face, segment dans le plan d'une face). Même correctif dans `RayStep::Intersect(const Box&)` (avec le centre `c` stocké).
```cpp
  constexpr double k = 1.0 + 1.0e-12;
  const Vector e = 0.5 * (box[1] - box[0]);      // Box half extents
  const Vector d = 0.5 * (b - a);                 // Segment half vector
  const Vector c = 0.5 * (a + b) - box.Center();  // Segment center relative to box center
  const double adx = fabs(d[0]), ady = fabs(d[1]), adz = fabs(d[2]);
  if (fabs(c[0]) > (e[0] + adx) * k) return false;
  if (fabs(c[1]) > (e[1] + ady) * k) return false;
  if (fabs(c[2]) > (e[2] + adz) * k) return false;
  if (fabs(d[1] * c[2] - d[2] * c[1]) > (e[1] * adz + e[2] * ady) * k) return false;
  if (fabs(d[2] * c[0] - d[0] * c[2]) > (e[0] * adz + e[2] * adx) * k) return false;
  if (fabs(d[0] * c[1] - d[1] * c[0]) > (e[0] * ady + e[1] * adx) * k) return false;
  return true;
```
**Vérification :** 3,7 millions de tests contre une référence par slabs (segments aléatoires, courts, parallèles à un axe, dans un plan de coordonnées, réduits à un point, extrémité sur une face, segment dans le plan d'une face, grandes coordonnées) : 0 erreur, contre 99 582 faux négatifs sur 111 377 intersections et 35 617 faux positifs pour l'ancienne version sur le seul jeu aléatoire. Coût ≈ 12 ns par test (référence par slabs : 23 ns). Effet mesuré dans LibBlob (AppBlobTree, scènes SmallTemple et Geode) : le segment tracing, qui ne trouvait aucun impact (`K(Segment)` nul à tort), retrouve 99,97 % et 100 % des pixels de la référence, avec 61 et 32 évaluations du champ par pixel contre 2 310 et 111 en sphere tracing.

#### 🔴 ✅ B-K03 — `Source/ktope.cpp:120` — `KTope::Intersect(const Ray&, double&, double&)` — Bornes jamais mises à jour et signe faux : renvoie toujours true

`if (t > tmin) { t = tmin; }` affecte `t` au lieu de `tmin` (idem Source/ktope.cpp:124, 132, 136) : `tmin`/`tmax` restent à ±1e18 et la fonction renvoie toujours `true`. De plus, avec la convention `n_i·p + c_i <= 0` (cf. `KTope(Vector)`, `operator+=`, `Inside`), le paramètre pour le plan opposé `n_{i+1} = -n_i` est `t = (c[i+1] - k)/s`, pas `(k - c[i+1])/s` (Source/ktope.cpp:129), et le cas parallèle (|s| < epsilon) ne rejette pas un rayon hors de la tranche.
**Correctif :**
```cpp
  for (int i = 0; i < 18; i += 2) {
    double s = normals[i] * ray.Direction(), k = normals[i] * ray.Origin();
    if (fabs(s) > epsilon) {
      double t0 = -(k + c[i]) / s;       // plan i
      double t1 = (c[i + 1] - k) / s;    // plan i+1
      if (t0 > t1) Math::Swap(t0, t1);
      tmin = Math::Max(tmin, t0); tmax = Math::Min(tmax, t1);
    }
    else if (k + c[i] > 0.0 || -k + c[i + 1] > 0.0) return false;
    if (tmin > tmax) return false;
  }
  return true;
```

#### 🔴 B-K04 — `Source/framebox.cpp:155` — `OrientedBox::Intersect(const OrientedBox&)` — Changement de repère faux dans le test OBB/OBB

`toMe = other.r * r.T()` et `centerOther = toMe * other.Center()` puis `separation = centerOther - Center()` : le centre de this n'est pas ramené dans le repère local (le centre de l'autre boîte est tourné puis comparé au centre monde de this), et la matrice relative n'est pas Rᵀ·R_other. Le reste de l'algorithme (lignes = axes de `other` exprimés dans le repère de this) suppose `toMe(j,i) = B_j·A_i` et `separation` exprimé dans le repère de this. Résultat faux dès que this est tourné et hors de l'origine. Lignes Source/framebox.cpp:155-159. En outre le test 2 sur X (Source/framebox.cpp:190) n'a pas de `fabs`.
**Correctif :**
```cpp
  Matrix toMe = other.r.T() * r;                    // toMe(j,i) = B_j . A_i
  Vector separation = r.T() * (other.Center() - Center());
  ...
  r = fabs(rotations.Row(0) * separation);          // l.190
```

#### 🔴 ✅ B-K05 — `Source/slab.cpp:34` — `Slab::Slab(const QVector<Vector>&, const Vector&)` — Arguments de Plane inversés : normale nulle

`Plane(Vector::Null, n)` appelle `Plane(normal, point)` (la normale vient en premier) avec normal = 0 et point = n. La normale du slab est donc nulle et n'est jamais corrigée ensuite : `Eval`, `R`, `Normal`, `Intersect` sont dégénérés pour tout slab construit ainsi.
**Correctif :**
```cpp
  :Plane(n, 0.0), e(0.0)
```

#### 🔴 ✅ B-K06 — `Source/slab.cpp:77` — `Slab::Intersect()` — Profondeurs d'intersection fausses

Le slab est `|n·p − cqc| <= e` (Eval = n·p − cqc). Les plans bornants donnent `t = (cqc ± e − k)/u`. Le code calcule `(e − cqc − k)/u` et `(k + cqc + e)/u` (Source/slab.cpp:77-78) : signe de `cqc` faux et `tmax` faux même pour cqc = 0 (dès que k ≠ 0).
**Correctif :**
```cpp
  tmin = (cqc - e - k) / u;
  tmax = (cqc + e - k) / u;
  Math::Sort(tmin, tmax);
```

#### 🔴 ✅ B-K07 — `Source/slab.cpp:92` — `Slab::Inside()` — Test inversé

`if (d < e) return false;` : un point à l'intérieur du slab (|Eval| < e) est déclaré extérieur et inversement.
**Correctif :**
```cpp
  return fabs(Eval(p)) <= e;
```

#### 🔴 B-K08 — `Source/ktope.cpp:176` — `KTope::Intersect(const KTope&)` — Test de recouvrement des intervalles faux

Avec la convention `n_i·p + c_i <= 0`, l'intervalle de la projection sur n_i est `[c[i+1], -c[i]]`. Deux k-topes sont disjoints si `c[i+1] > -k.c[i]` ou `k.c[i+1] > -c[i]`. Le test `c[i] > k.c[i+1] || c[i+1] < k.c[i]` n'a pas de sens géométrique : testé contre lui-même, tout k-tope dont xmin + xmax > 0 (centre de coordonnée positive) est déclaré disjoint. Le résultat est faux presque partout.
**Correctif :**
```cpp
  if ((c[i + 1] > -k.c[i]) || (k.c[i + 1] > -c[i])) return false;
```

#### 🔴 ✅ B-K09 — `Source/tetrahedra.cpp:102` — `Tetrahedra::Intersect(const Ray&, double&, double&) (et la surcharge avec normales)` — Compteur réinitialisé à chaque face, test sur la mauvaise face : renvoie toujours false

`int k = 0;` est déclaré à l'intérieur de la boucle sur les faces (Source/tetrahedra.cpp:102, 161) : k ne dépasse jamais 1 et la condition `k == 2` n'est jamais atteinte → la fonction renvoie toujours `false`. De plus le test contre les « trois autres plans » utilise `p[i]`/`n[i]` au lieu de `p[j]`/`n[j]` (Source/tetrahedra.cpp:120, 179) : q étant sur le plan i, le test donne toujours ≈ 0 et ne rejette rien.
**Correctif :**
```cpp
  int k = 0;                                   // hors de la boucle
  for (int i = 0; i < 4; i++) {
    double x = n[i] * ray.Direction();
    if (fabs(x) < epsilon) continue;
    double t = (n[i] * (p[i] - ray.Origin())) / x;
    Vector q = ray(t);
    int j;
    for (j = 0; j < 4; j++)
      if (j != i && (q - p[j]) * n[j] > epsilon) break;
    if (j == 4) { if (k == 0) ta = t; else tb = t; k++; }
    if (k == 2) { Math::Sort(ta, tb); return true; }
  }
  return false;
```

**Correction appliquée :** les deux surcharges sont réécrites en intersection des quatre demi-espaces (comme les autres polyèdres convexes) : pour chaque plan, entrée (`ta = max`) ou sortie (`tb = min`), rejet d'un rayon parallèle extérieur, normales `na`/`nb` des plans retenus. Plus robuste que le test face par face, qui compte deux fois un rayon passant par une arête. Vérifié sur 200 000 rayons contre un échantillonnage de `Inside` (orientation sortante ; l'orientation de `split` relève de B-K25).


### R — Primitives 3D volumiques

#### 🔴 B-R01 — `Include/icosidodecahedron.h:106` — `Icosidodecahedron::Vertex(int, int)` — Accès hors bornes au tableau triangle pour les faces triangulaires

Pour `i >= 12` (faces triangulaires, documentées comme numérotées après les 12 pentagones), le code lit `triangle[i][j]` alors que `triangle` est déclaré `[20][3]` : l'index doit être `i - 12`. Pour i ∈ [12, 31], on lit `triangle[12..31]`, soit hors bornes pour i ≥ 20 (UB) et un mauvais triangle pour 12 ≤ i < 20.
**Correctif :**
```cpp
return Vertex(triangle[i - 12][j]);
```

#### 🔴 ✅ B-R02 — `Source/boxsphere.cpp:69` — `BoxSphere::BoxSphere(const BoxSphere&, const Matrix&), BoxSphere::BoxSphere(const BoxSphere&, const Frame&)` — Lecture du membre r non initialisé dans le constructeur délégué

Dans `:BoxSphere(Box(box, matrix), r)` (ligne 69) et `:BoxSphere(Box(box, frame), r)` (Source/boxsphere.cpp:78), `r` désigne le membre `this->r` de l'objet en construction (aucun paramètre ne s'appelle `r`), encore non initialisé au moment de la délégation : UB, rayon arbitraire. Il faut utiliser le rayon de la boîte source.
**Correctif :**
```cpp
BoxSphere::BoxSphere(const BoxSphere& box, const Matrix& matrix) :BoxSphere(Box(box, matrix), box.r) {}
BoxSphere::BoxSphere(const BoxSphere& box, const Frame& frame) :BoxSphere(Box(box, frame), box.r) {}
```

#### 🔴 ✅ B-R03 — `Source/capsule.cpp:226` — `Capsule::R(const Capsule&, double&, double&)` — Distance segment-segment calculée comme si les deux segments étaient de longueur 1

**Corrigé :** le correctif ci-dessous (directions non normalisées avec l'algorithme d'Eberly) reste faux pour des segments presque parallèles (erreur ~1,2 fois l'angle, branche parallèle approchée) et donne NaN pour un segment de longueur nulle ; `Capsule::R(const Capsule&, double&, double&)` a été réécrite avec l'algorithme de Christer Ericson (*Real-Time Collision Detection*, 2005, §5.1.9), sans seuil, u et v dans [0,1] le long des segments entiers (erreur max 8e-15 sur 300 000 paires de capsules contre une minimisation de référence).

L'algorithme d'Eberly suppose des directions D0 = b − a, D1 = cs.b − cs.a non normalisées (a00 = D0·D0, a11 = D1·D1) et des paramètres u, v ∈ [0, 1]. Ici on utilise `axis` (normalisé par Axis::Axis) avec a00 = a11 = 1 : les paramètres sont bornés à [0, 1] le long d'un vecteur unitaire, ce qui revient à tronquer chaque capsule à une longueur 1. Le résultat (et `Intersect(const Capsule&…)`) est faux dès qu'une longueur diffère de 1.
**Correctif :**
```cpp
Vector d0 = b - a, d1 = cs.b - cs.a;
Vector aa = a - cs.a;
double a00 = d0 * d0;
double a01 = -(d0 * d1);
double a11 = d1 * d1;
double aaxa = aa * d0;
double aaxb = -(aa * d1);
double ra = aa * aa;
double fDet = fabs(a00 * a11 - a01 * a01);
// garder le reste de l'algorithme ; seuil de parallélisme relatif : fDet >= epsilon * a00 * a11
```

#### 🔴 B-R04 — `Source/cone.cpp:97` — `Cone::Intersect(), NewCone::Intersect()` — Signe faux dans la quadrique du rayon du cône : intersection rayon/cône fausse

Avec pa = a − o, la coordonnée axiale du point du rayon vaut s(t) = t·dx − pax, donc r(t)² = (ra + rlength·(t·dx − pax))². Le code développe (ra + rlength·(t·dx + pax))² (termes en `pax` de signe opposé), alors que la quadrique `d` utilise bien −pax. Le résultat n'est juste que si rlength = 0 ou pax = 0. Exemple : cône (0,0,0)-(0,0,1), ra=1, rb=0, rayon horizontal à z=0,5 : le code utilise un rayon 1,5 au lieu de 0,5. Même erreur dans `Cone::Intersect(…, na, nb)` (Source/cone.cpp:170) et dans `NewCone::Intersect` (Source/conet.cpp:98 et Source/conet.cpp:173).
**Correctif :**
```cpp
Quadric r(rlength * rlength * dx * dx,
          2.0 * (ra * rlength * dx - rlength * rlength * pax * dx),
          ra * ra + rlength * rlength * pax * pax - 2.0 * ra * rlength * pax);
```

#### 🔴 ✅ B-R05 — `Source/dodecahedron-rhombic.cpp:85` — `RhombicDodecahedron::Signed()` — Distance toujours nulle : Inside() toujours faux et boucle infinie dans RandomInside()

La fin du portage GLSL est commentée (lignes 87-89) : `Vector q = t;` (ligne 85), donc `d = Norm(t - q) = 0` (ligne 90) et la fonction renvoie ±0. `Inside()` (`Signed(p) < 0.0`) est toujours faux, et `RandomInside()` boucle indéfiniment. Par ailleurs les mises à jour « vectorielles » GLSL (lignes 61-62, 65-66, 76-77) sont traduites séquentiellement : la 2e composante utilise la 1re déjà modifiée, ce qui change le calcul.
**Correctif appliqué :** fin du portage GLSL décommentée et traduite, mises à jour vectorielles faites simultanément ; de plus `halfSize = 2r/√3` (au lieu de r/2) pour que les sommets d'ordre 3 soient à distance r, conformément à la doc de `r`. Vérifié exact contre la distance aux 12 faces (400 000 points). Le centre reste ignoré (B-R22).
```cpp
// q.yz -= 0.5*h ; q -= (2,1,-1)/3 * max(q.x - q.z, 0) ; q = clamp(q, 0, s)
Vector q = t;
q[1] -= 0.5 * h; q[2] -= 0.5 * h;
double m = Math::Max(q[0] - q[2], 0.0);
q -= Vector(2.0, 1.0, -1.0) / 3.0 * m;
q = Vector(Math::Clamp(q[0], 0.0, s), Math::Clamp(q[1], 0.0, s), Math::Clamp(q[2], 0.0, s));
// et calculer chaque min() une seule fois avant de mettre à jour les deux composantes
```

#### 🔴 ✅ B-R06 — `Source/dodecahedron.cpp:121` — `Dodecahedron::Intersect(), Icosahedron::Intersect(), Icosidodecahedron::Intersect()` — Affectations inversées : ta/tb jamais mis à jour, renvoie toujours true

Dans la boucle (lignes 121-133), `if (ta > t) { t = ta; ... }` et `if (tb < t) { t = tb; ... }` écrivent dans la variable locale `t` au lieu de `ta`/`tb`, et les comparaisons sont elles-mêmes inversées (il faut ta = max(ta, t) pour les faces d'entrée, tb = min(tb, t) pour les faces de sortie). Avec ta = −∞ et tb = +∞, les tests ne sont jamais vrais : `ta` et `tb` ne changent pas, la fonction renvoie toujours `true` et `fa`/`fb` restent −1. Même erreur dans `Icosahedron::Intersect()` (Source/icosahedron.cpp:143, lignes 143-155) et `Icosidodecahedron::Intersect()` (Source/icosidodecahedron.cpp:226, lignes 226-238).
**Correctif :**
```cpp
if ((Normal(i) * ray.Direction()) < 0.0)
{
  if (t > ta) { ta = t; fa = i; }
}
else
{
  if (t < tb) { tb = t; fb = i; }
}
```

#### 🔴 B-R07 — `Source/ellipsoid.cpp:47` — `Ellipsoid::Ellipsoid(const Spheroid&)` — Lecture du membre b non initialisé et rayons mal affectés

Dans le constructeur délégué, `b` désigne le membre `this->b`, pas encore initialisé (UB). De plus pour un Spheroid, `A()` est la longueur axiale et `B()` le rayon radial, et `Frame::Orthonormal` place l'axe sur z : les rayons doivent être (B, B, A) et non (A, A, ?).
**Correctif :**
```cpp
Ellipsoid::Ellipsoid(const Spheroid& e) :Ellipsoid(Frame::Orthonormal(e.Center(), e.GetAxis()), e.B(), e.B(), e.A()) {}
```

#### 🔴 ✅ B-R08 — `Source/hemisphere.cpp:263` — `HemiSphere::RandomInside()` — Boucle infinie : while (true) sans sortie

La boucle tire un point dans la sphère unité, le retourne si besoin, mais n'a ni `break` ni `return` : la fonction ne termine jamais, et `PoissonInside()` (qui l'appelle) non plus.
**Correctif :**
```cpp
Vector p = Sphere::Unit.RandomInside(random);
if (p * axis < 0.0) p = -p;
return c + r * p;
```

#### 🔴 ✅ B-R09 — `Source/icosidodecahedron.cpp:124` — `Icosidodecahedron::Signed(), Inside(), Intersect(), GetPlane()` — Tables triangle, pentagon, normal factices : tous les plans de faces sont faux

`triangle[20][3]` (ligne 124) ne contient que des zéros (les vraies faces sont en commentaire lignes 66-123, en triangulation de 60 triangles), `pentagon[12][5]` (ligne 61) est la table des faces du *dodécaèdre* (indices sans rapport avec les 30 sommets de l'icosidodécaèdre), et `normal[32]` (ligne 26) répète quatre fois les 8 mêmes normales du dodécaèdre : les normales des pentagones doivent être ∝ (0, ±1, ±φ) et permutations circulaires, celles des triangles ∝ (±1, ±1, ±1) et (0, ±1/φ, ±φ) et permutations. Tous les plans de faces sont donc faux : `Signed()`, `Inside()`, `Intersect()` et `GetPlane()` ne décrivent pas un icosidodécaèdre.
**Correctif appliqué :** tables générées par programme et ordonnées par symétrie centrale : `vertex[i+15] = -vertex[i]`, pentagone i+6 opposé au pentagone i, triangle i+10 opposé au triangle i (normales opposées) ; faces orientées dans le sens trigonométrique vu de l'extérieur, normales 0-11 pour les pentagones et 12-31 pour les triangles. `Signed()` et `Inside()` ne testent plus qu'une face par paire, avec |n·(p−c)| − d (16 plans au lieu de 32). Vérifié : faces planes et convexes, 120 demi-arêtes appariées, 2 pentagones et 2 triangles par sommet, `Signed()` identique au maximum sur 32 plans, `Intersect()` exact. L'ancienne triangulation commentée est supprimée.


### C — Courbes et surfaces paramétriques

#### 🔴 ✅ B-C01 — `Source/bezier-curve.cpp:172` — `BezierCurve::Second()` — Dérivée seconde fausse : base de Bernstein de mauvais degré, dernier terme omis

La dérivée seconde d'une Bézier de degré n vaut n(n-1) Σ_{i=0}^{n-2} (P_{i+2}-2P_{i+1}+P_i) B_i^{n-2}(u), soit n-1 termes. `Math::BernsteinSeries(u, k, b)` calcule k termes de degré k-1 (cf. `Point()` qui passe `Degree()+1`). Le code passe `n-2` (donc n-2 termes de degré n-3) et boucle sur `i < n-2` : base de mauvais degré et dernier terme manquant. Conséquences : pour une quadrique (n=2) le résultat est toujours le vecteur nul ; pour une cubique on obtient 6(P2-2P1+P0) constant au lieu de 6[(1-u)Δ²P0 + uΔ²P1]. Le résultat est faux pour tous les degrés et toutes les valeurs de u.
**Correctif :**
```cpp
const int n = Degree();
if (n < 2) return Vector::Null;
Math::BernsteinSeries(u, n - 1, b);   // n-1 termes de degré n-2
const double nm = double(n * (n - 1));
for (int i = 0; i < n - 1; i++)
  q += (Vertex(i + 2) - 2.0 * Vertex(i + 1) + Vertex(i)) * nm * b[i];
```

#### 🔴 ✅ B-C02 — `Source/bezier-curve2.cpp:189` — `BezierCurve2::Second()` — Même erreur que BezierCurve::Second() en 2D : dérivée seconde fausse

Même motif qu'en 3D : `BernsteinSeries(u, n - 2, b)` et la boucle `i < n - 2` calculent une base de degré n-3 et omettent le dernier terme. Le résultat est nul pour une quadrique et faux pour tous les autres degrés.
**Correctif :**
```cpp
if (n < 2) return Vector2::Null;
Math::BernsteinSeries(u, n - 1, b);
for (int i = 0; i < n - 1; i++) q += (Vertex(i + 2) - 2.0 * Vertex(i + 1) + Vertex(i)) * nm * b[i];
```


### S — Champs scalaires et grilles

#### 🔴 ✅ B-S01 — `Source/algebraic.cpp:415` — `Algebraic::IntersectHigh()` — Polynôme de degré 0 : aucune racine trouvée pour n ≥ 3

`Polynomial eqn;` est construit avec `n = 0` (degré stocké). Les coefficients sont écrits via `eqn[offset+...]` mais le degré n'est jamais mis à jour (aucun setter dans `Polynomial`). `eqn.Reversed()` (polynomial.cpp:424) ne copie que `c[0..n]` = `c[0]`, soit le seul coefficient de t^n ; `Solve` reçoit un polynôme constant et ne renvoie aucune racine. Toute intersection rayon/surface algébrique de degré ≥ 3 échoue systématiquement.
**Correctif :**
```cpp
double a[degree + 1] = { 0.0 };   // a[m] = coefficient de t^(n-m)
// ... accumuler dans a[offset + i1 + j1 + k1] au lieu de eqn[...]
Polynomial poly(0.0);
for (int m = 0; m <= n; m++)       // Horner : poly = poly * t + a[m]
  poly = poly * Polynomial(1.0, 0.0) + Polynomial(a[m]);
poly.Check();
return poly.Solve(Depths);
```

**Correction appliquée :** plus simple que le correctif ci-dessus : `eqn` est construit avec le constructeur à 9 coefficients (degré stocké 8 = `Algebraic::degree`), les coefficients sont indexés directement par la puissance de t `(i - i1) + (j - j1) + (k - k1)`, puis `eqn.Check()` réduit le degré ; `Reversed()` et `offset` disparaissent. Vérifié avec le vrai `Polynomial::Solve` sur 360 surfaces aléatoires de degré 3 à 8 : 589 racines de référence (changements de signe de `Value`), 589 retrouvées, aucune fausse (0 avec l'ancien code).

#### 🔴 ✅ B-S02 — `Source/analyticscalarfield.cpp:1056` — `AnalyticScalarField::StochasticVolume()` — Rapport inversé : volume faux

Renvoie `box.Volume() * n / m` au lieu de `box.Volume() * m / n` (m = nombre d'échantillons intérieurs). Le résultat est l'inverse de la fraction attendue (toujours ≥ volume de la boîte) et division par zéro si aucun échantillon n'est intérieur.
**Correctif :**
```cpp
return (n > 0) ? box.Volume() * double(m) / double(n) : 0.0;
```

#### 🔴 ✅ B-S03 — `Source/voxel.cpp:276` — `Voxel::Accessibility()` — Double décalage des indices : accès hors bornes

Les bornes `ax..bx` sont déjà des coordonnées absolues (`x−1 .. x+1`), puis la boucle lit `At(x + i, y + j, z + k)`, soit environ `2x` : pour toute cellule au-delà de la moitié de la grille, l'index sort du tableau (assert Qt / lecture hors bornes). Le résultat est faux partout ailleurs. Le compte inclut aussi la cellule centrale (27 et non 26 comme dit la doc).
**Correctif :**
```cpp
for (int i = ax; i < bx; i++)
  for (int j = ay; j < by; j++)
    for (int k = az; k < bz; k++)
      if ((i != x || j != y || k != z) && At(i, j, k) > 0)
        accessibility++;
```

**Correction appliquée :** correctif ci-dessus, plus les bornes supérieures `min(x + 2, nx)` (au lieu de `nx - 1`, qui en borne exclusive ignorait la dernière couche de la grille). Vérifié contre un comptage direct des 26 voisins sur une grille 7×5×6 : 0 écart, aucun accès hors bornes.

#### 🔴 ✅ B-S04 — `Source/analyticscalarfield2.cpp:42` — `AnalyticScalarField2::Local(), ScalarField2::Local()` — Voisinage 3×3 mal rempli

Dans `AnalyticScalarField2::Local` : `a(1,0)` est écrit deux fois (l.46 puis l.49), `a(2,0)` deux fois (l.47 puis l.53), `a(1,2)` reçoit `p − ε` en x au lieu de `p + ε` (l.51), `a(2,2)` reçoit `(x−ε, y+ε)` au lieu de `(x+ε, y+ε)` (l.55), et `a(0,1)`, `a(0,2)` ne sont jamais écrits. Le même copier-coller existe dans Source/scalarfield2.cpp:1555-1565 (`ScalarField2::Local(int,int)`) : la première ligne suit la convention `a(dx,dy)`, les deux suivantes `a(dy,dx)` ; `a(1,0)` et `a(2,0)` sont écrasés, `a(0,1)` et `a(0,2)` restent non initialisés. `AnalyticScalarField2::Hessian()`/`Curvature()` et `ScalarField2::Hessian(int,int)` qui en dépendent sont donc entièrement faux.
**Correctif :**
```cpp
Matrix a;   // a(1+di, 1+dj) = f(x + di*eps, y + dj*eps)
for (int di = -1; di <= 1; di++)
  for (int dj = -1; dj <= 1; dj++)
    a(1 + di, 1 + dj) = Value(Vector2(p[0] + di * epsilon, p[1] + dj * epsilon));
return a;
```
(même réécriture pour `ScalarField2::Local(int,int)` avec `at(x + di, y + dj)` borné à la grille).

**Correction appliquée :** sans boucle, un seul `return Matrix(...)` : `Matrix::operator()(i, j)` lit `r[i + 3j]` et le constructeur à 9 arguments remplit `r[0..8]` dans l'ordre, donc les valeurs sont listées par ligne de y avec x croissant. Même chose pour `ScalarField2::Local(int,int)` avec les indices bornés `ia, ib, ja, jb`. Vérifié avec le vrai `matrix.h` sur une fonction non symétrique et sur toute une grille 5×4 (bords compris) : 0 écart.

#### 🔴 ✅ B-S05 — `Source/analyticscalarfield2.cpp:71` — `AnalyticScalarField2::Hessian()` — Différences secondes divisées par ε au lieu de ε²

`(a(2,1) + a(0,1) − 2a(1,1)) / epsilon` et le terme croisé `/ (4.0 * epsilon)` : les dérivées secondes doivent être divisées par ε² (resp. 4ε²). Avec ε = 1e-6 le résultat est faux d'un facteur 1e6. De plus ε = 1e-6 est trop petit pour une différence seconde (erreur d'arrondi ~1e-4·|f| même une fois corrigé) : utiliser ~1e-4.
**Correctif :**
```cpp
const double e2 = epsilon * epsilon;
return Matrix2((a(2,1) + a(0,1) - 2.0 * a(1,1)) / e2,
               (a(1,2) + a(1,0) - 2.0 * a(1,1)) / e2,
               ((a(2,2) - a(0,2)) - (a(2,0) - a(0,0))) / (4.0 * e2));
```

**Correction appliquée :** division par ε² (4ε² pour le terme croisé), avec une constante séparée `epsilonhessian = 1e-4` pour `Local()` et `Hessian()` (`epsilon = 1e-6` reste utilisé par `Gradient()`). Vérifié sur f = 3x + 7y² + 0,5xy + x³ + sin(x+2y) : avec ε = 1e-6 l'erreur relative est d'environ 1e-3 (arrondi) ; avec ε = 1e-4 elle tombe à 1e-7. Avec `epsilonhessian`, erreur relative mesurée entre 8e-9 et 3e-8.

#### 🔴 B-S06 — `Source/diffusion.cpp:254` — `Diffusion::VcycleStep()` — Récursion infinie : la résolution des niveaux grossiers ne diminue pas

Le niveau inférieur est construit avec `epsilon.Resized(nx / 2, ny / 2)` où `nx, ny` sont les membres (résolution fine), et non `ltnx / 2, ltny / 2`. Au niveau 2, `ltnx = nx/2`, et le niveau 3 est encore redimensionné à `nx/2` : la taille ne décroît plus, `min_ltnxy < 40` n'est jamais atteint si `min(nx,ny) ≥ 80` → récursion infinie (dépassement de pile). Même erreur l.259 (`Resized(nx, ny)` au lieu de `ltnx, ltny`), qui donne une taille incompatible avec `result` dès le niveau 2.
**Correctif :**
```cpp
epsilon = epsilon.Resized(ltnx / 2, ltny / 2);
...
epsilon_result = epsilon_result.Resized(ltnx, ltny);
```

#### 🔴 B-S07 — `Source/diffusion.cpp:110` — `Diffusion::InitBuffersGL()` — Lecture hors bornes aux niveaux grossiers du V-cycle

`current_tbs = nx * ny` (taille fine) est utilisé pour toutes les `glBufferData`, alors que `start` et `lap_target` ont la taille `ltnx * ltny` aux niveaux grossiers : la copie lit au-delà des tableaux `GetAsFloats()` (UB). Les contraintes et le masque sont « redimensionnés » à `(nx, ny)`, c.-à-d. à leur propre taille (l.117-118), au lieu de `(ltnx, ltny)`, et les uniformes `buffer_size_x/y` reçoivent `nx, ny` (l.145-146).
**Correctif :**
```cpp
int current_tbs = ltnx * ltny;
if (ltnx != nx || ltny != ny)
{
  current_constraints = current_constraints.Resized(ltnx, ltny);
  current_mask = current_mask.Resized(ltnx, ltny);
}
...
glUniform1i(glGetUniformLocation(shader_program, "buffer_size_x"), ltnx);
glUniform1i(glGetUniformLocation(shader_program, "buffer_size_y"), ltny);
```

#### 🔴 B-S08 — `Source/integerfield2.cpp:105` — `Array2I::Array2I(const ScalarField2&)` — Écriture dans un QVector vide

`field` n'est jamais redimensionné : la boucle écrit `field[i]` pour `i < nx*ny` dans un vecteur de taille 0 (assert Qt / corruption mémoire), écriture hors bornes garantie à chaque appel.
**Correctif :**
```cpp
Array2I::Array2I(const ScalarField2& s) :Array2(s)
{
  const int size = nx * ny;
  field.resize(size);
  for (int i = 0; i < size; i++) field[i] = int(s.at(i));
}
```

#### 🔴 B-S09 — `Source/scalarfield.cpp:803` — `ScalarField::Hessian(int, int, int)` — Écriture hors bornes sur la pile, pas de grille ignoré

Le stencil est rempli avec `a[1 + i][1 + j][1 + k] = Value(i + ai, j + aj, k + ak)`, où i, j, k sont les coordonnées du point et non les décalages `ai, aj, ak` : dès que i, j ou k > 1, écriture hors du tableau `a[3][3][3]` (corruption de pile), et le reste du stencil n'est jamais initialisé. En plus, les différences sont divisées par `Epsilon²` (1e-12, constante du header) au lieu du pas de grille `celldiagonal²`, les termes croisés par `2ε²` au lieu de `4h²`, et `Value(i−1, …)` sort de la grille aux bords.
**Correctif :**
```cpp
for (int ai = -1; ai < 2; ai++)
  for (int aj = -1; aj < 2; aj++)
    for (int ak = -1; ak < 2; ak++)
    {
      int u = Math::Clamp(i + ai, 0, nx - 1), v = Math::Clamp(j + aj, 0, ny - 1), w = Math::Clamp(k + ak, 0, nz - 1);
      a[1 + ai][1 + aj][1 + ak] = Value(u, v, w);
    }
const Vector& h = celldiagonal;
H[0] = (a[2][1][1] - 2.0 * a[1][1][1] + a[0][1][1]) / (h[0] * h[0]);
H[1] = (a[2][2][1] - a[2][0][1] - a[0][2][1] + a[0][0][1]) / (4.0 * h[0] * h[1]);
// idem H[2] (h[0]*h[2]), H[4] (h[1]^2), H[5] (h[1]*h[2]), H[8] (h[2]^2)
```

#### 🔴 B-S10 — `Source/scalarfield.cpp:163` — `ScalarField::BicubicValue()` — Pas d'indice incompatibles avec la disposition mémoire

`Array::VertexIndex(i,j,k) = (i*ny + j)*nz + k` : k a le pas 1, j le pas nz, i le pas ny·nz. Le code suppose la disposition `i + nx*(j + ny*k)` : décalage initial `c -= nx*ny + nx + 1`, pas internes `+1`, `+nx − 4`, `+nx*ny − 4nx`. Les 64 échantillons lus ne forment pas le voisinage 4×4×4 de (i,j,k) dès que nx ≠ nz, et même pour une grille cubique les poids `x[]` sont appliqués à l'axe z (et `z[]` à x). Le test `c > 0 && c < size` n'empêche pas de passer sur une autre ligne aux bords (et exclut à tort `c == 0`). Le long commentaire l.168-177 qui justifie ce parcours est faux.
**Correctif :**
```cpp
double r = 0.0;
for (int di = 0; di < 4; di++)
  for (int dj = 0; dj < 4; dj++)
    for (int dk = 0; dk < 4; dk++)
    {
      int u = Math::Clamp(i + di - 1, 0, nx - 1);
      int v = Math::Clamp(j + dj - 1, 0, ny - 1);
      int w = Math::Clamp(k + dk - 1, 0, nz - 1);
      r += at(u, v, w) * x[di] * y[dj] * z[dk];
    }
return r;
```

#### 🔴 ✅ B-S11 — `Source/scalarfield2.cpp:1381` — `ScalarField2::Sample(const Segment2&, const double&, int, int)` — Boucle interne qui incrémente i au lieu de j

`for (int j = 0; j < ny; i++)` : j n'est jamais incrémenté (boucle infinie) et i croît sans borne, d'où des écritures `s(i, j)` hors du tableau (corruption mémoire) puis un crash. La boîte `Box2(segment.Length(), radius)` a de plus une hauteur `radius`, alors que les échantillons couvrent `[−radius, radius]` (hauteur 2·radius) ; la surcharge à 2 paramètres (l.1362) calcule ny avec le même facteur 2 manquant.
**Correctif :**
```cpp
ScalarField2 s(Box2(segment.Length(), 2.0 * radius), nx, ny);
...
for (int j = 0; j < ny; j++)
```

#### 🔴 ✅ B-S12 — `Source/scalarfield2.cpp:3246` — `ScalarField2::Op(const std::function<double(const ScalarField2&, int, int)>&)` — Bornes de boucle inversées : accès hors bornes

`for (j < nx) for (i < ny) ret(i, j) = function(*this, i, j)` : i (indice x) parcourt `[0, ny)` et j (indice y) `[0, nx)`. Pour toute grille non carrée, `ret(i, j)` et les accès de `function` sortent du tableau (écriture hors bornes), et une partie du champ n'est jamais calculée.
**Correctif :**
```cpp
for (int j = 0; j < ny; j++)
  for (int i = 0; i < nx; i++)
    ret(i, j) = function(*this, i, j);
```

#### 🔴 ✅ B-S13 — `Source/scalarfield2.cpp:2481` — `ScalarField2::Lerp(const ScalarField2&, const ScalarField2&, const ScalarField2&)` — Chemin AVX : calcule a + (1−t)·b au lieu de (1−t)·a + t·b

Dans la branche AVX (active par défaut sous MSVC avec AVX), `avxa = a * avxu` avec `avxu = 1.0` et `avxb = b * (1 − t)` : le résultat est `a + (1−t)·b`, alors que la doc et la branche scalaire (l.2489, 2497) calculent `(1−t)·a + t·b`. Le résultat dépend du CPU et est faux sur le chemin principal.
**Correctif :**
```cpp
__m256d avxt = _mm256_loadu_pd(tp + i * 4);
__m256d avxv = _mm256_sub_pd(avxu, avxt);      // 1 - t
avxa = _mm256_mul_pd(avxa, avxv);              // (1-t) a
avxb = _mm256_mul_pd(avxb, avxt);              // t b
_mm256_storeu_pd(p + i * 4, _mm256_add_pd(avxa, avxb));
```


### V — Champs vectoriels et de couleur

#### 🔴 B-V01 — `Source/vectorfield2.cpp:43` — `VectorField2::VectorField2(const Box2&, int, int, const AnalyticVectorField2&)` — Écriture dans un QVector vide (champ jamais dimensionné)

Le constructeur à partir d'un champ analytique ne dimensionne jamais `field` (vide par défaut) puis écrit `field[VertexIndex(i, j)]` : écriture hors limites (assert Qt en debug, corruption mémoire en release) à chaque appel. Même motif dans `VectorField::VectorField(const Box&, int, int, int, const AnalyticVectorField&)` (Source/vectorfield.cpp:37).
**Correctif :**
```cpp
VectorField2::VectorField2(const Box2& box, int x, int y, const AnalyticVectorField2& a) :Array2(box, x, y)
{
  field.resize(nx * ny);
  ...
}
// idem VectorField : field.resize(nx * ny * nz);
```

#### 🔴 B-V02 — `Source/vectorfield2.cpp:238` — `VectorField2::Clone()` — Indice source non borné : lecture hors du champ

La zone destination est clampée par `VertexIntegerArea`, mais l'indice source `VertexIndex(x0 + x - xb, y0 + y - yb)` n'est jamais testé : dès que la source (décalée de la destination) sort de la grille (pinceau de clonage près d'un bord), `field[...]` lit hors du tableau (UB). La variable `destxy` (Source/vectorfield2.cpp:220) est inutilisée.
**Correctif :**
```cpp
const int xs = x0 + x - xb, ys = y0 + y - yb;
if (!InsideVertexIndex(xs, ys)) continue;
field[VertexIndex(x, y)] = (1.0 - a) * field[VertexIndex(x, y)] + a * field[VertexIndex(xs, ys)];
```


### N — Bruit et échantillonnage

#### 🔴 ✅ B-N01 — `Source/noise-phasor.cpp:45` — `PhasorEric::Gabor_noise()` — Lecture hors bornes de directionmap / frequencymap

**Résolu par suppression :** PhasorEric et Source/noise-phasor.cpp ont été supprimés, remplacés par `Phasor2` (`Include/phasornoise.h`, `Source/phasornoise2.cpp`), qui lit son orientation et sa fréquence via les fonctions virtuelles `Orientation()` et `Frequency()` sans carte ni accès indexé.

`pos = Vector2(1.5 * rndi(i, 0), rndi(i, 1))` donne `pos[0]` dans [0, 1.5[. L'indice `ii = pos[0] * (directionmap.GetSizeX() - 1)` (l.45, idem l.54 pour frequencymap) peut donc atteindre 1.5*(nx-1), bien au-delà de la dernière colonne : `directionmap.at(ii, jj)` (Source/noise-phasor.cpp:48) et `frequencymap.at(ii, jj)` (Source/noise-phasor.cpp:57) lisent hors du tableau pour environ un tiers des noyaux.
**Correctif :**
```cpp
ii = Math::Clamp(int(pos[0] / 1.5 * (directionmap.GetSizeX() - 1)), 0, directionmap.GetSizeX() - 1);
jj = Math::Clamp(int(pos[1] * (directionmap.GetSizeY() - 1)), 0, directionmap.GetSizeY() - 1);
// idem pour frequencymap (l.54-55)
```

#### 🔴 B-N02 — `Source/eden.cpp:112` — `EdenGrowth::GrowthSites()` — Débordement du tableau local subset[37] en 3D

Tous les points à moins de 4r de q sont copiés dans `int subset[N]` (N = 37, déclaré Source/eden.cpp:104) sans contrôle de `num` (écriture `subset[num] = j` l.112). En 3D, avec un espacement minimal ≈2.15r (2r(1+0.075)), un amas compact peut contenir plus de 37 centres dans la boule de rayon 4r : sur un empilement CFC d'espacement 2.15r, les couches à 2.15r (12), 3.04r (6) et 3.72r (24) donnent, avec le centre, 1+12+6+24 = 43 centres dans la boule de rayon 4r, soit plus que 37. Écriture hors pile → corruption mémoire / crash dans les zones denses de l'amas.
**Correctif :**
```cpp
QVector<int> subset;
for (int j = 0; j < points.size(); j++)
  if (SquaredNorm(points.at(j) - q) < subhit) subset.append(j);
```

#### 🔴 B-N03 — `Source/eden2.cpp:342` — `EdenGrowth2::Strahler()` — Utilise l'ensemble frontière s au lieu du tableau strahler

L'algorithme lit `s.at(j)` (Source/eden2.cpp:342, :349, :354), c'est-à-dire la liste des indices de particules frontières, au lieu de `strahler.at(j)`. Le résultat est entièrement faux et, comme `s` est en général bien plus court que `points` (les particules mortes en sont retirées), `s.at(j)` est hors bornes (assertion Qt en debug, UB en release). `DrawStrahler()` (Source/eden2.cpp:511) hérite du problème.
**Correctif :**
```cpp
if (strahler.at(j) == 0) strahler[j] = p;
else if (p < strahler.at(j)) break;
else if (p == strahler.at(j)) { p++; strahler[j] = p; }
else strahler[j] = p;
```

#### 🔴 B-N04 — `Source/eden2.cpp:241` — `EdenGrowth2::Topology()` — Table d'enfants entièrement fausse

Pour une particule i ayant un ancêtre ai, le code lit le compteur dans `next[i*8]`, écrit `next[i*8 + n] = i` (la particule elle-même, dans sa propre ligne) et incrémente `next[i*8 + 7]`. L'ancêtre `ai` n'est jamais utilisé : le compteur lu (case 0) n'est pas celui incrémenté (case 7), et les enfants ne sont pas rangés sous leur parent. Le résultat ne représente aucune topologie. Pas de doc du format (8 cases, dont un compteur).
**Correctif :**
```cpp
if (ai != -1)
{
  int n = next.at(ai * 8 + 7);
  if (n < 7) { next[ai * 8 + n] = i; next[ai * 8 + 7]++; }
}
```

#### 🔴 B-N05 — `Source/relaxedarray.cpp:52` — `RelaxedArray::Add(const Vector&, int)` — Indice de cellule borné à nx-1 au lieu de nx-2 : écriture hors tableau

`indexes` a `CellSize() = (nx-1)(ny-1)(nz-1)` éléments, les indices de cellule valides vont donc de 0 à nx-2. Le code borne `x` à `nx - 1` (idem y, z ; clamps Source/relaxedarray.cpp:52-54) : pour tout point situé sur la face max de la boîte ou au-delà, `CellIndex(nx-1, …)` dépasse la taille et `indexes[entry].append(id)` écrit hors du QVector (pas de contrôle en release). Même problème dans `RelaxedArray::Add(const Box&, int)` (Source/relaxedarray.cpp:74), qui s'appuie sur `Array::CellIntegerVolume()` bornant aussi à `nx - 1` (Source/array.cpp:231-233), et donc dans `Add(const Sphere&, int)`.
**Correctif :**
```cpp
x = Math::Clamp(x, 0, nx - 2);
y = Math::Clamp(y, 0, ny - 2);
z = Math::Clamp(z, 0, nz - 2);
// et dans Add(const Box&) : bx = min(bx, nx - 2); by = min(by, ny - 2); bz = min(bz, nz - 2);
```

#### 🔴 B-N06 — `Source/sampling.cpp:107` — `SphereTile::Relaxation()` — Points ramenés modulo la taille d'une cellule au lieu de celle de la tuile

Après déplacement, `p[u] = Vector::Mod(p[u], Vector(s))` utilise la variable locale `s = Math::Sqrt2 * r` (taille de cellule de la grille), et non la taille de la tuile `a`. Tous les points sont repliés dans le cube [0, √2 r]³ : la distribution est entièrement détruite après une seule relaxation (la version 2D `DiscTile::Relaxation()` utilise bien `Vector2(s, s)` où s est la taille de la tuile).
**Correctif :**
```cpp
p[u] = Vector::Mod(p[u], Vector(a));
```


### T — Maillages et graphes géométriques

#### 🔴 B-T01 — `Source/mesh.cpp:251` — `Mesh::Mesh(const Quadrangle&, int, int)` — Grilles : indice linéaire j*nx+i avec i<ny (débordement si nx>ny)

La boucle externe parcourt `j ∈ [0,nx)` et l'interne `i ∈ [0,ny)`, mais l'indice linéaire est `j * nx + i` au lieu de `j * ny + i`. Dès que `nx != ny` les sommets se chevauchent ou laissent des trous, et si `nx > ny` l'indice max `(nx-1)*nx + ny-1` dépasse `nx*ny` : écriture hors bornes (UB). Les indices de triangles (`j*nx+i+1`, `(j+1)*nx+i`) reproduisent la même erreur. Les tests avec `nx == ny` masquent le bug. Même motif dans :
- Source/mesh.cpp:251-252 et :262-270 (`Mesh(const Quadrangle&, int, int)`)
- Source/mesh.cpp:300-301 et :311-319 (`Mesh(const Rectangles&, int, int)`)
- Source/mesh.cpp:3214-3238 (`Mesh(const QuadricSurface&, const Box2&, int, int)`) : `Array2::VertexIndex(i,j)` vaut `i + nx*j` avec `i<nx`, or ici `i` parcourt `[0,ny)` et `j` `[0,nx)` : indices permutés, même débordement.
- Source/mesh2.cpp:58 et :68-75 (`Mesh2(const Quadrangle2&, int, int)`)
- Source/mesh2.cpp:98 et :108-114 (`Mesh2(const Box2&, int, int)`) : de plus `a.ArrayVertex(i, j)` est appelé avec `i<y`, `j<x` (arguments permutés).
**Correctif :**
```cpp
// Remplacer partout j * nx + i par j * ny + i (et (j+1)*nx par (j+1)*ny)
vertices[j * ny + i] = quad.Vertex(u, v);
...
narray[n + 0] = varray[n + 0] = j * ny + i;
narray[n + 1] = varray[n + 1] = j * ny + i + 1;
narray[n + 2] = varray[n + 2] = (j + 1) * ny + i;
// QuadricSurface / Mesh2(Box2) : boucler i sur [0,nx) et j sur [0,ny)
for (int i = 0; i < nx; i++) for (int j = 0; j < ny; j++) { int ij = a.VertexIndex(i, j); ... }
```

#### 🔴 B-T02 — `Source/mesh.cpp:1628` — `Mesh::LoadTim()` — Indices de normales positifs non décrémentés (test inversé)

Pour les sommets : `if (vertexId >= 0) vertexId--; else ...`. Pour les normales le test est inversé : `if (normalId <= 0) normalId--; else normalId %= normals.size();`. Un indice OBJ positif (cas normal, base 1) n'est donc jamais converti en base 0 : toutes les normales sont décalées d'une position, et l'indice `normals.size()` (dernière normale) devient 0. Tout l'ombrage est faux pour tout fichier OBJ avec normales ; un indice négatif (relatif) devient encore plus négatif, d'où un accès hors bornes ultérieur.
**Correctif :**
```cpp
if (normalId > 0) normalId--;
else normalId += normals.size(); // indice relatif : -1 = dernière normale
```


### U — Utilitaires, couleur, GPU

#### 🔴 ✅ B-U01 — `Source/color.cpp:255` — `Color::Wheel()` — La roue chromatique renvoie toujours du blanc

`Hsl(a, 1.0, 1.0).ToColor()` utilise une luminance L=1. Dans `Hsl::ToColor()`, pour l=1 et s=1 : v = l+s-s*l = 1, u = 2l-v = 1, donc les trois composantes valent 1 quel que soit l'angle : la fonction renvoie blanc pour toute entrée. La teinte saturée correspond à L=0.5 (la doc mentionne en outre l'espace HSV, ce qui est faux). Conséquence : `VectorField2::CreateAngleImage()` (Source/vectorfield2.cpp:938) produit une image entièrement blanche pour tous les vecteurs non nuls.
**Correctif :**
```cpp
Color Color::Wheel(const double& a)
{
  return Hsl(a, 1.0, 0.5).ToColor();
}
```

#### 🔴 ✅ B-U02 — `Source/palette.cpp:253` — `Palette::Reverse()` — Ancres inversées en ordre décroissant : palette inversée entièrement fausse

`ra[i] = a[a.size() - 1 - i]` produit des ancres décroissantes (ex. {0,0.5,1} → {1,0.5,0}). `GetColor()` suppose des ancres croissantes : avec ra[0]=1, tout t<1 renvoie `c.at(0)` (Source/palette.cpp:180-181) : la palette inversée est constante.
**Correctif :**
```cpp
  const double a0 = a.first(), a1 = a.last();
  for (int i = 0; i < a.size(); i++)
    ra[i] = a0 + a1 - a[a.size() - 1 - i];
```


---

## Bugs Haute criticité 🟠

### M — Mathématiques

#### 🟠 ✅ B-M04 — `Source/mathematics.cpp:453` — `Math::Sort5(double*)` — Insertion de A[4] mal imbriquée : tri faux même sur une entrée déjà triée

Après les deux premières étapes on a A[0]<=A[2]<=A[3] et A[0]<=A[1]. L'insertion de A[4] dans {A[0],A[2],A[3]} est mal parenthésée : la branche « A[4] <= A[2] » (rotations vers la gauche, l.461-475) est placée dans le `else` de `if (A[4] < A[3])` au lieu du `else` de `if (A[4] > A[2])`. Résultat : (1) si A[4] <= A[2], A[4] n'est jamais inséré ; (2) si A[4] >= A[3], une rotation est appliquée à tort. Exemples : {1,2,3,4,5} -> {1,2,5,3,4} ; {1,2,3,4,0} -> {1,2,3,4,0}. La fonction rend un ordre faux pour une grande partie des entrées.
**Correctif :**
```cpp
  if (A[4] > A[2])
  {
    if (A[4] < A[3]) Math::Swap(A[3], A[4]); // sinon déjà en place
  }
  else
  {
    if (A[4] > A[0]) { double t = A[2]; A[2] = A[4]; A[4] = A[3]; A[3] = t; }
    else { double t = A[0]; A[0] = A[4]; A[4] = A[3]; A[3] = A[2]; A[2] = t; }
  }
```

#### 🟠 B-M05 — `Source/ia.cpp:371` — `cos(const Ia&)` — Test de parité faux pour les multiples négatifs de π : intervalle non englobant

`ua % 2 == 1` est faux pour un entier impair négatif (en C++, -1 % 2 == -1). Pour x = [-3.5,-2.5] (ua = -1, minimum -1 en -π), on renvoie `[min(ca,cb), 1]` = [-0.94, 1] au lieu de [-1, -0.80] : la borne inférieure n'englobe pas le vrai minimum, ce qui viole la garantie de l'arithmétique d'intervalles. `sin(const Ia&)` (implémenté via cos) est touché aussi.
**Correctif :**
```cpp
    if ((ua & 1) != 0) // ou abs(ua) % 2 == 1
```

#### 🟠 B-M06 — `Source/aa.cpp:1744` — `Aa_pow(const double&, const Aa&)` — Approximation Minimum : facteur 0.5 manquant sur alpha et gamma

Avec y_min = fmin - β·min_e et y_max = fmax - β·max_e, le décalage du centre vaut 0.5(y_min+y_max) et l'erreur 0.5(y_max-y_min). Le code calcule `alpha = -beta*(min_e+max_e)+fmin+fmax` et `gamma = beta*(min_e-max_e)-fmin+fmax` sans le facteur 0.5, alors que la surcharge `Aa_pow(Aa,double)` (l.1614-1615) l'applique. Le centre est faux (décalé de 0.5(y_min+y_max)) et le rayon doublé : en mode Aa::Minimum, le résultat est faux.
**Correctif :**
```cpp
      alpha = 0.5 * (-beta * (min_e + max_e) + fmin + fmax);
      gamma = 0.5 * (beta * (min_e - max_e) - fmin + fmax);
```

#### 🟠 B-M07 — `Source/aa.cpp:1174` — `Aa::Pow(int)` — Chebyshev, exposant pair : faux point de tangence, encadrement non garanti

Pour n pair, f'(x)=α n'a qu'une solution réelle x = sign(α)|α/n|^(1/(n-1)), mais le code prend ±t et les retient tous deux s'ils sont dans [a,b]. Exemple sqr sur [-1,2] : α=1, x_1=-0.5, x_2=0.5 ; y_1 = g(-0.5)=0.75, y_2 = g(0.5) = -0.25 donc δ=0.5, ζ=0.25. L'approximation en x=-1 donne [-1.25,-0.25] alors que f(-1)=1 : le résultat n'englobe pas la vraie valeur. Le max de g = f-αx est atteint aux extrémités (g(a)=g(b)=2), pas en -t. Cela touche `sqr(Aa)` (mode par défaut Chebyshev) dès que l'intervalle contient ±t.
**Correctif :**
```cpp
    if (exponent % 2 == 0 && exponent > 0)
    {
      double xt = Math::Clamp(Math::CopySign(pow(fabs(alpha / exponent), 1.0 / (exponent - 1.0)), alpha), a, b);
      double ymax = fa - alpha * a;              // = fb - alpha*b (sécante)
      double ymin = pow(xt, double(exponent)) - alpha * xt;
      delta = 0.5 * (ymax - ymin); dzeta = 0.5 * (ymax + ymin);
    }
    else { /* code actuel (exposants impairs) */ }
```

#### 🟠 B-M08 — `Source/aa.cpp:2814` — `sin(const Aa&)` — Rayon du résultat jamais calculé (radius = 0)

`Temp` est construit par `Aa(double)` (radius = 0), on remplit les déviations mais `Temp.radius` n'est jamais mis à jour (contrairement à toutes les autres fonctions, ex. l.2695). `GetIa()`, `Radius()`, les comparaisons et les fonctions suivantes (sqrt, inv, Pow… qui se basent sur `radius`) voient un intervalle réduit à un point. `cos` et `tan` (construits sur sin) sont touchés aussi.
**Correctif :**
```cpp
  Temp.radius = fabs(alpha) * P.radius + fabs(delta);
  return Temp;
```

#### 🟠 B-M09 — `Source/ivector2.cpp:34` — `Vec2I::pcg2d()` — Hachage PCG sur int signé : débordements signés (UB) et décalages arithmétiques

L'algorithme de référence pcg2d travaille sur `uvec2` (non signé). Ici `Vec2I` stocke des `int` : `(*this) * 1664525 + Vec2I(1013904223)` et les lignes 41-42 débordent en arithmétique signée pour quasiment toute entrée (UB : le compilateur peut supposer l'absence de débordement), et `v >> 16` sur un négatif propage le bit de signe (pas le résultat de référence). Le hachage obtenu n'est ni portable ni conforme à PCG.
**Correctif :**
```cpp
Vec2I Vec2I::pcg2d() const
{
  uint32_t x = uint32_t(c[0]) * 1664525u + 1013904223u;
  uint32_t y = uint32_t(c[1]) * 1664525u + 1013904223u;
  x += y * 1664525u; y += x * 1664525u;
  x ^= x >> 16; y ^= y >> 16;
  x += y * 1664525u; y += x * 1664525u;
  x ^= x >> 16; y ^= y >> 16;
  return Vec2I(int(x), int(y));
}
```

#### 🟠 ✅ B-M10 — `Source/quaternion.cpp:167` — `Quaternion::RotationMatrix(), Quaternion::Quaternion(const Matrix&)` — Convention ligne/colonne inversée : la matrice produite est la rotation inverse

**Corrigé :** `RotationMatrix()` passe les coefficients colonne par colonne et `Quaternion(const Matrix&)` lit `R(i, j)` (ligne i, colonne j) : `q.RotationMatrix() * p == q * p` et `Quaternion(Matrix::Rotation(u, a)) == ±Quaternion(u, a)` (erreurs ≤ 2e-15 sur 100 000 rotations). `Matrix::Lerp`, seul utilisateur, était déjà juste et le reste. Corrigé en même temps : `Quaternion::Slerp` renvoyait `p` pour des rotations distantes de moins de 2e-3 rad (erreur jusqu'à 9e-4, y compris en t = 1) ; il utilise désormais l'interpolation linéaire normalisée (erreur 1,5e-11).

`Matrix` stocke ses coefficients par colonne et son constructeur à 9 réels les prend « in column order » (matrix.h:420, cf. `Matrix::RotationZ` : `Matrix(c, s, 0, -s, c, 0, ...)`). `RotationMatrix()` passe les termes dans l'ordre ligne : la première colonne vaut (1-2(y²+z²), 2(xy-wz), 2(xz+wy)) alors que la matrice de rotation de q a pour première colonne (…, 2(xy+wz), 2(xz-wy)). On obtient Rᵀ = R(q⁻¹) : `q.RotationMatrix()*p` ≠ `q*p` (quaternion.cpp:333), et `Quaternion(u,a).RotationMatrix()` = `Matrix::Rotation(u,-a)`. Même convention inversée dans le constructeur `Quaternion(const Matrix&)` (`Source/quaternion.cpp:119-121` : `R[7]-R[5]` = R(1,2)-R(2,1) = -4wx, et `Source/quaternion.cpp:143-145`) qui renvoie le conjugué. L'aller-retour (Matrix::Lerp) reste cohérent, mais tout mélange avec Matrix::Rotation* ou Quaternion::operator*(Vector) donne la rotation inverse.
**Correctif :**
```cpp
  return Matrix(
    1.0 - (tyy + tzz), txy + twz, txz - twy,   // colonne 0
    txy - twz, 1.0 - (txx + tzz), tyz + twx,   // colonne 1
    txz + twy, tyz - twx, 1.0 - (txx + tyy));  // colonne 2
// et dans Quaternion(const Matrix&) : utiliser R(i,j) (operator()) au lieu de R[3*i+j],
// ex. x = (R(2,1) - R(1,2)) * root; y = (R(0,2) - R(2,0)) * root; z = (R(1,0) - R(0,1)) * root;
```

#### 🟠 B-M11 — `Source/matrix.cpp:185` — `Matrix::Symmetry(const Vector&)` — Composition dans le mauvais ordre : symétrie par rapport au mauvais plan

`r = Rotation(n̂, Z)` envoie n̂ sur Z (r n̂ = Z). La symétrie de plan normal n̂ vaut `rᵀ·diag(1,1,-1)·r`, alors que le code calcule `r·diag(1,1,-1)·rᵀ`, symétrie de plan normal `r·Z`, c'est-à-dire le symétrique de n̂ par rapport à Z. Le résultat n'est juste que si n̂ est parallèle à Z ou dans le plan xy (cas des usages actuels avec Vector::X) ; pour n = (1,0,1) on obtient la symétrie de normale (-1,0,1). L'exemple de la doc de classe (`Symmetry(Vector(2,-3,1))`) est faux.
**Correctif :**
```cpp
Matrix Matrix::Symmetry(const Vector& n)
{
  Vector u = Normalized(n);
  return Matrix::Identity - 2.0 * Matrix::External(u); // I - 2 n nᵀ
}
```

#### 🟠 B-M12 — `Source/matrix4.cpp:422` — `Matrix4::LookAt(const Vector&, const Vector&, const double&)` — Code écrit pour la convention vecteur-ligne : l'œil n'est pas envoyé à l'origine

`Matrix4::operator*(const Vector&)` utilise la convention vecteur-colonne (M·v, translation en r[12..14]). Le produit `((torg*roty)*rotx)*rotz` applique donc la translation en DERNIER (v ↦ torg(roty(rotx(rotz v)))), et les matrices `roty`/`rotx` sont remplies comme dans la version vecteur-ligne d'origine (Graphics Gems), soit transposées. Exemple : eye = (5,0,0), look = 0 : M·eye = (-5,0,-5) au lieu de 0, et look est envoyé en (-5,0,0) au lieu de l'axe -Z. Le résultat n'est juste que pour eye sur l'axe z regardant vers -z.
**Correctif :** utiliser la version (eye, look, up) corrigée : `Matrix4 v = LookAt(eye, look, Vector(0,1,0)); return Matrix4::Rotation(Vector(0,0,-twist)) * v;` ou transposer chaque rotation et inverser l'ordre : `rotz.T() * rotx.T() * roty.T() * torg`.

#### 🟠 B-M13 — `Source/matrix4.cpp:435` — `Matrix4::LookAt(const Vector&, const Vector&, const Vector&)` — right et up_ non normalisés : la vue contient une mise à l'échelle

`right = up / d` a pour norme |up|·sin(up,d) ; dès que `up` n'est pas unitaire et orthogonal à la direction de visée (cas usuel : caméra qui regarde vers le bas avec up = Y, cf. spheretracingwidget.cpp:1069), la base (right, up_, d) n'est pas orthonormée et `V_1.Inverse()` met x et y à l'échelle 1/|right|, ce qui déforme l'image.
**Correctif :**
```cpp
    Vector right = Normalized(up / d);
    Vector up_ = d / right; // unitaire puisque d et right sont unitaires et orthogonaux
```


### G — Géométrie 2D

#### 🟠 B-G16 — `Source/annulus2.cpp:58` — `Annulus2::Intersect(const Ray2&, double*)` — Profondeurs d'intersection mal réordonnées

Avec t[0]<t[1] (cercle extérieur) et t[2]<t[3] (cercle intérieur), l'ordre attendu le long du rayon est t0, t2, t3, t1. Le code fait `s=t[3]; t[3]=t[2]; t[2]=t[1]; t[1]=s;` et produit {t0, t3, t1, t2} : séquence non monotone (sortie intérieure, sortie extérieure, puis entrée intérieure). Tout appelant qui exploite les paires [t0,t1] et [t2,t3] comme intervalles pleins obtient des intervalles faux.
**Correctif :**
```cpp
double s = t[1];
t[1] = t[2];
t[2] = t[3];
t[3] = s;
return 4;
```

#### 🟠 B-G17 — `Source/annulus2.cpp:123` — `Annulus2::Normal()` — Condition inversée : vecteur nul à l'extérieur, non nul dans l'anneau

Le code renvoie `Vector2::Null` si le point est dans le trou ou à l'extérieur (`d <= ri*ri || d >= re*re`) et calcule un vecteur (de norme ≤ 0, donc orienté vers le centre) seulement pour les points dans l'anneau. La convention de la bibliothèque (cf. `Circle2::Normal`, `Convex2::Normal`, `Capsule2::Normal`, doc « distance vector between an annulus and a point ») est l'inverse : vecteur nul à l'intérieur de la forme, vecteur projection→point à l'extérieur.
**Correctif :**
```cpp
Vector2 n = p - c;
double d = Norm(n);
if (d >= ri && d <= re) return Vector2::Null;
n /= d;
return (d > re) ? (d - re) * n : (d - ri) * n; // (d-ri)<0 : vers l'extérieur du trou
```

#### 🟠 B-G18 — `Source/boxset2.cpp:48` — `BoxSet2::Area()` — Coordonnées réelles tronquées en int

La compression de coordonnées utilise `std::vector<int> ys` et des événements `{int x, ...}` construits avec `(int)r[0][0]` etc. Les coordonnées `double` des boîtes sont tronquées : pour des boîtes à coordonnées non entières (ex. dans [0,1]²), l'aire calculée est fausse (souvent 0). `A` et `area` sont en outre accumulés en entiers (`long long`). (Aucun accès hors bornes à `ys[n]` : un nœud [l,n) ne peut pas être entièrement couvert.)
**Correctif :** remplacer `int` par `double` pour `ys`, `event::x`, `A`, `area` et `prev` (garder `C` et les indices `l,h` en `int`).

#### 🟠 ✅ B-G19 — `Source/box2.cpp:479` — `Box2::Intersect(const Segment2&, double&, double&)` — Paramètres de droite en longueur comparés à [0,1]

**Corrigé :** les profondeurs sont divisées par la longueur du segment, et un segment de longueur nulle est traité comme un point (`Inside()`) au lieu de produire une direction NaN. Sur 400 000 segments de longueur 0,01 à 100, l'ancienne version donnait 33 531 réponses fausses (8 755 faux négatifs, 24 776 faux positifs) ; la nouvelle aucune, avec des paramètres exacts à 2e-16 près. Seuls utilisateurs : `Triangle2::Intersect(const Box2&)` et le pré-filtre de `ContourTerrainGen.cpp` (AppRiver), qui deviennent justes.

`Line2(s)` normalise la direction (`line2.cpp:32`) : les profondeurs `a,b` renvoyées sont des distances curvilignes. Elles sont pourtant bornées à [0,1] comme un paramètre de segment. Pour un segment de longueur L≠1, seule la portion [0, min(L,1)] est testée (faux négatifs si L>1, faux positifs si L<1) et `tmin, tmax` ne sont pas des paramètres de segment.
**Correctif :**
```cpp
const double l = s.Length();
if (Intersect(Line2(s), a, b))
{
  tmin = Math::Max(a / l, 0.0);
  tmax = Math::Min(b / l, 1.0);
  return tmin < tmax;
}
return false;
```

#### 🟠 B-G20 — `Source/line2.cpp:131` — `Line2::Symmetry(const Vector2&)` — Réflexion par rapport à la normale au lieu de la droite

`(p-c) - ((p-c)·v)v` avec `v ⊥ u` est la composante **parallèle** à la droite ; `p - 2·(composante parallèle)` donne c + (perp − par), c'est-à-dire la réflexion par rapport à la droite perpendiculaire passant par c. La réflexion par rapport à la droite doit retirer deux fois la composante orthogonale (cf. `Axis2::Symmetric`, correct). `Symmetry(Box2)` et `Symmetry(Circle2)` sont faux par conséquence.
**Correctif :**
```cpp
Vector2 v = Normalized(n).Orthogonal();
return p - 2.0 * ((p - c) * v) * v;
```

#### 🟠 B-G21 — `Source/capsule2.cpp:40` — `Capsule2::Area()` — Aire fausse : πr² + πrL au lieu de πr² + 2rL

L'aire d'une capsule 2D (stade) est πr² + 2rL (deux demi-disques + rectangle L×2r, L = longueur de l'axe). Le code renvoie `Math::Pi * r * (r + length)` = πr² + πrL, faux dès que L>0.
**Correctif :**
```cpp
return Math::Pi * r * r + 2.0 * r * length;
```

#### 🟠 B-G22 — `Source/circle2.cpp:439` — `Circle2::Intersect(const Circle2&, Vector2&, Vector2&)` — Milieu de la corde du mauvais côté

`u = c - circle.c` pointe de l'autre cercle vers ce cercle ; `l` est la distance de `c` à la corde commune **vers l'autre centre**. Le milieu de la corde est donc `c - l*u` (après normalisation), pas `c + l*u`. Exemple : c = (0,0), r = 1, autre cercle (1,0), r = 1 : u = (−1,0), l = 0,5, m = (−0,5 ; 0) au lieu de (0,5 ; 0). Les points d'intersection renvoyés sont faux dès que l ≠ 0. Par ailleurs, deux cercles concentriques de même rayon passent le test et provoquent une division par d = 0.
**Correctif :**
```cpp
if (d < Segment2::epsilon) return false; // avant le calcul de l
// ...
Vector2 m = c - l * u;
```

#### 🟠 ✅ B-G23 — `Source/circle2.cpp:294` — `Circle2::Vogel()` — Centre et rayon du cercle ignorés

La variable locale `double r` masque le membre `r`, et le centre `c` n'est pas ajouté (contrairement à la fonction voisine l.282) : la fonction renvoie un point du disque unité centré à l'origine, quel que soit le cercle.
**Correctif :**
```cpp
double s = Circle2::r * sqrt((i + 0.5) / n);
double a = i * goldenAngle;
return c + s * Vector2(cos(a), sin(a));
```

#### 🟠 B-G24 — `Source/circlearc2.cpp:74` — `CircleArc2::GetBox()` — Quadrants testés en coordonnées absolues au lieu de relatives au centre

Tous les tests (`a[0] > 0.0`, `a[1] > 0.0`, `b[0] > 0.0`, …) portent sur les coordonnées absolues des extrémités, alors que la classification en quadrants doit se faire sur `a - c` et `b - c`. Dès que le centre n'est pas à l'origine (ex. après `Translate`), le cas choisi est faux.
**Correctif :** calculer `Vector2 ca = a - c, cb = b - c;` et tester `ca`, `cb` dans toute la fonction.

#### 🟠 B-G25 — `Source/circlearc2.cpp:147` — `CircleArc2::GetBox()` — 7 cas sur 16 de la table de quadrants faux (sens horaire / boîte inversée)

Avec un arc parcouru dans le sens trigonométrique de a vers b (doc du constructeur), 7 des 16 cas (table refaite avec c = 0) renvoient la boîte d'un parcours horaire ou une boîte inversée ; `Box2(Vector2, Vector2)` ne réordonne pas min/max (box.h:463). Les 9 autres cas sont justes :
- l.147 (a∈Q4, b∈Q2, passe par 0 et 90°) : renvoie `Box2((b0,a1),(a0,b1))` au lieu de `Box2((b0,a1),(c0+r,c1+r))` ;
- l.152 (a∈Q4, b∈Q3, passe par 0, 90, 180°) : attendu `Box2((c0-r,min(a1,b1)),(c0+r,c1+r))` ;
- l.169 (a∈Q2, b∈Q1, passe par 180, 270, 0°) : attendu `Box2((c0-r,c1-r),(c0+r,max(a1,b1)))` ;
- l.174 (a∈Q2, b∈Q4, passe par 180, 270°) : ymin doit valoir `c1-r` et non `b1` ;
- l.186 (a,b∈Q2, b0<a0) : `Box2((b0,a1),(a0,b1))` est inversée en y (b1<a1) ; attendu `Box2((b0,b1),(a0,a1))` ;
- l.209 (a∈Q3, b∈Q1, passe par 270°) : `Box2(a,b)` omet `c1-r` ;
- l.223 (a∈Q3, b∈Q2, passe par 270, 0, 90°) : attendu `Box2((min(a0,b0),c1-r),(c0+r,c1+r))`.
**Correctif :** réécrire la fonction génériquement : boîte de {a, b} étendue par chacun des 4 points extrêmes `c ± r·X`, `c ± r·Y` dont l'angle est compris dans [angle(a), angle(b)] (modulo 2π) ; cela corrige aussi B-G24.

#### 🟠 B-G26 — `Source/circlearc2.cpp:290` — `CircleArc2::R()` — Distance au cercle complet conservée hors du secteur de l'arc

Hors du secteur angulaire de l'arc, la distance doit être celle à l'extrémité la plus proche. Le code prend `Math::Min(s, |p-a|², |p-b|²)` où `s` est la distance au cercle complet ; comme a et b sont sur le cercle, s ≤ |p-a|², |p-b|² et le `Min` ne change jamais rien : `R()` vaut toujours la distance au cercle complet. Incohérent avec `CircleArc2::Normal()` qui, elle, remplace le vecteur. Le test de secteur par deux produits scalaires n'est en outre valide que pour un arc d'angle < π.
**Correctif :**
```cpp
s = Math::Min(SquaredNorm(p - a), SquaredNorm(p - b));
```
(et tester l'appartenance angulaire via les angles pour gérer les arcs > π).

#### 🟠 ✅ B-G27 — `Source/convex2.cpp:436` — `Convex2::Signed()` — Distance intérieure positive et calculée avec max au lieu de min

Pour un polygone CCW, `Normalized(e).Orthogonal()` pointe vers l'intérieur (cf. `Inside()` l.351), donc chaque `di` est ≥ 0 pour un point intérieur. La distance signée intérieure vaut `-min(di)` ; le code renvoie `max(di)`, positif : un point intérieur a une distance positive (égale à la distance au bord le plus éloigné). Exemple : carré [0,1]², p = (0,5 ; 0,25) renvoie 0,75 au lieu de −0,25. Même calcul Source/convex2.cpp:440-441.
**Correctif :**
```cpp
double d = Normalized(q.at(0) - q.at(q.size() - 1)).Orthogonal() * (p - q.at(q.size() - 1));
for (int i = 0; i < q.size() - 1; i++)
  d = Math::Min(d, Normalized(q.at(i + 1) - q.at(i)).Orthogonal() * (p - q.at(i)));
return -d;
```

#### 🟠 B-G28 — `Source/convex2.cpp:496` — `Convex2::Normal()` — Cas « arête » : norme au carré et sens inversé

Dans la zone d'une arête, `r = pa*pa - (paeb*paeb)/(eb*eb)` est la distance **au carré** à la droite support, et `ebo` pointe vers l'intérieur (commentaire l.471) alors que le point est à l'extérieur. Le vecteur renvoyé `ebo * r / Norm(ebo)` a donc la mauvaise norme (d² au lieu de d) et le mauvais sens (vers le polygone au lieu de projection→point), contrairement au cas « sommet » qui renvoie correctement `pa`.
**Correctif :**
```cpp
double r = sqrt(Math::Max(0.0, pa * pa - (paeb * paeb) / (eb * eb)));
return -ebo * (r / Norm(ebo));
```

#### 🟠 B-G29 — `Source/convex2.cpp:623` — `Convex2::Intersect(const Circle2&)` — Cercle contenu dans le polygone déclaré disjoint

Seule la distance du centre au **bord** (`R()` = min des distances au carré aux arêtes, sans test `Inside`) est comparée au rayon. Un petit cercle entièrement à l'intérieur du polygone (centre à une distance du bord > r) est déclaré sans intersection.
**Correctif :**
```cpp
if (Inside(c.Center())) return true;
return R(c.Center()) <= c.Radius() * c.Radius();
```

#### 🟠 ✅ B-G30 — `Source/diamond2.cpp:112` — `Diamond2::R()` — Distance à l'arête non normalisée

La normale à l'arête `n = Vector2(d[1], d[0])` n'est pas unitaire : `s = qa * n` vaut distance × |n|. Le cas « arête » renvoie `s * s`, soit la distance au carré multipliée par `d[0]² + d[1]²`. Les tests sommets/intérieur (`s < 0`) restent justes, mais la distance est fausse dès que |n| ≠ 1.
**Correctif :**
```cpp
return s * s / (n * n);
```

#### 🟠 ✅ B-G31 — `Source/disctwins2.cpp:85` — `TwinDisc2::Signed()` — Distance intérieure fausse (positive ou issue du mauvais disque)

**Corrigé :** correctif ci-dessous, complété par le traitement des disques disjoints (l ≥ 2r, où h valait NaN) et des centres confondus (l = 0, division par zéro), qui renvoyaient NaN pour tout point intérieur. Erreur max 9e-16 sur 409 000 points intérieurs contre une référence analytique (distance aux arcs extérieurs).

Le test de secteur est correct, mais dans la zone où le point le plus proche du bord est sur l'arc du disque situé du côté x≥0 (repère symétrisé), la distance doit être celle au cercle de ce disque, soit min(sa, sb) : `Norm(q - (l/2,0)) - r`. Le code renvoie `Math::Max(sa, sb)` calculé sur le point **non symétrisé**, qui sélectionne le disque lointain :
- si le point n'est que dans un disque, l'autre distance est positive et `max` renvoie une valeur > 0 pour un point intérieur (ex. r = 1, l = 1, q = (1,0) : renvoie +0,5 au lieu de −0,5) ;
- si le point est dans les deux disques, `max` peut sélectionner la portion de cercle de l'autre disque, intérieure à l'union (ex. r>l, q=(r-l/2-ε,0) : renvoie ≈-ε au lieu de ≈-l).
**Correctif :**
```cpp
if ((q - Vector2(0.5 * l, 0.0)) * n > 0.0)
{
  return Norm(q - Vector2(0.5 * l, 0.0)) - r;
}
```

#### 🟠 ✅ B-G32 — `Source/ellipse2.cpp:344` — `Ellipse2::Signed()` — Point non exprimé dans le repère local de l'ellipse

Contrairement à `R()` (l.279-281), qui projette sur (u, u⊥), `Signed()` prend `Abs(p - c)` en coordonnées monde. Pour toute ellipse dont l'axe n'est pas `Vector2::X`, la distance signée est fausse (et `Intersect(Circle2)` qui repose dessus aussi).
**Correctif appliqué :** le point est exprimé dans le repère de l'ellipse avant la symétrie (`pp = p - c; pp = Abs(Vector2(u * pp, u.Orthogonal() * pp));`), comme dans `R()`. Vérifié contre une référence (recherche dense du point le plus proche puis affinage) sur 100 000 ellipses d'axe quelconque : 99 997 écarts > 1e-6 avant, 0 après (écart max 2e-10) ; axe = X inchangé (0 écart). `Intersect(Circle2)`, qui s'appuie sur `Signed()`, en profite. Vérifié par compilation (`g++ -fsyntax-only`).

#### 🟠 ✅ B-G33 — `Source/ellipse2.cpp:268` — `Ellipse2::Curvature()` — Renvoie le rayon de courbure au lieu de la courbure

κ(t) = ab / (a² sin²t + b² cos²t)^{3/2}. Le code calcule (a⁴·b²sin²t + b⁴·a²cos²t)^{3/2} / (a⁴b⁴) ; or a⁴b²sin² + b⁴a²cos² = a²b²(a²sin² + b²cos²), d'où (a² sin²t + b² cos²t)^{3/2} / (ab), c'est-à-dire 1/κ.
**Correctif appliqué :** `return (a4 * b4) / Math::Sqrt32(a4 * p[1] + b4 * p[0]);` (= ab / (a² sin²t + b² cos²t)^{3/2}) ; doc : paramètre t = angle paramétrique, courbure a/b² au bout du grand axe et b/a² au bout du petit. Vérifié sur 100 000 ellipses contre la courbure analytique du paramétrage : 100 000 écarts avant (ancien × vraie courbure = 1 à 1e-15 près : c'était bien 1/κ), 0 après (écart relatif max 1e-15) ; a=2, b=1 : 2,0000 en t=0 et 0,2500 en t=π/2 ; cercle de rayon 3 : 1/3.

#### 🟠 ✅ B-G34 — `Source/ellipse2.cpp:146` — `Ellipse2::Gradient()` — Terme q[0]²/bb au lieu de q[0]²/aa : norme du gradient fausse

Le gradient de f = √(x²/a² + y²/b²) − 1 vaut (x/a², y/b²)/√(x²/a² + y²/b²). Le dénominateur du code utilise `q[0] * q[0] / bb` : seul le facteur scalaire (la norme du gradient) est faux dès que a ≠ b ; la direction (q0/aa, q1/bb) est juste. Problème distinct de B-G35 (même fonction).
**Correctif appliqué :** dénominateur `q[0] * q[0] / aa + q[1] * q[1] / bb` dans `Ellipse2::Gradient()`. Vérifié contre la différence finie de `Value()` : sur 100 000 ellipses d'axe X (seule la norme était fausse) 99 545 écarts avant, 0 après (max 5e-9).

#### 🟠 ✅ B-G35 — `Source/ellipse2.cpp:141` — `Ellipse2::Gradient()` — Gradient renvoyé dans le repère local, non ramené en coordonnées monde

Le point est projeté dans le repère (u, u⊥) l.141, mais le vecteur résultat (composantes locales) est renvoyé tel quel l.146. Pour une ellipse non alignée sur X, la direction du gradient est fausse. Problème distinct de B-G34.
**Correctif appliqué :** le gradient local est ramené en coordonnées monde (`return g[0] * u + g[1] * u.Orthogonal();`), doc de `Gradient()` complétée (non normalisé, repère global, non défini au centre). Vérifié contre la différence finie de `Value()` sur 100 000 ellipses d'axe quelconque : 100 000 écarts avant, 0 après (max 1.6e-8). Aucun autre appelant de `Gradient()`/`Curvature()` d'`Ellipse2` dans le dépôt. Vérifié par compilation (`g++ -fsyntax-only`).

#### 🟠 ✅ B-G36 — `Source/framebox2.cpp:157` — `OrientedBox2::Intersect(const Ray2&, double&, double&), OrientedBox2::Intersect(const Ray2&)` — Boîte locale construite deux fois trop petite

`Box2(const double& x, const double& y)` construit la boîte [-x/2, x/2]×[-y/2, y/2] (dimensions complètes, cf. `box.h:522`). Lui passer `length[0], length[1]` (demi-diagonale, donc demi-dimensions) produit une boîte de demi-taille `length/2`. Même motif Source/framebox2.cpp:167 (`Intersect(const Ray2&)`). Impacte `Capsule2::Intersect(const Ray2&...)` qui s'appuie sur `GetOrientedBox()`.
**Correctif :**
```cpp
return Box2(-length, length).Intersect(ray.InverseTransform(*this), ta, tb);
```

#### 🟠 ✅ B-G37 — `Source/framebox2.cpp:70` — `OrientedBox2::Signed()` — Distance intérieure toujours nulle

`q` est écrasé par `Vector2::Max(Abs(q) - length, Vector2::Null)` avant le calcul de la partie intérieure : `q.Max()` est alors ≥ 0 et `Math::Min(q.Max(), 0.0)` vaut toujours 0. Tout point intérieur a une distance signée nulle au lieu d'être négative (comparer avec `Box2::Signed`, correct).
**Correctif :**
```cpp
Vector2 q = Abs(InverseTransform(p)) - length;
double r = Norm(Vector2::Max(q, Vector2::Null));
double i = Math::Min(q.Max(), 0.0);
return r + i;
```

#### 🟠 ✅ B-G38 — `Source/hexagon2.cpp:255` — `Hexagon2::R()` — Distance à l'arête non élevée au carré

Dans le cas « arête », la fonction renvoie `d` (distance) alors que les autres branches renvoient des distances au carré (`SquaredNorm`) et que la doc annonce « squared Euclidean distance ».
**Correctif :**
```cpp
return d * d;
```

#### 🟠 B-G39 — `Source/hexagonalpha2.cpp:118` — `HexagonAlpha2::GetTightBox()` — Rayon de l'hexagone ignoré

La boîte renvoyée vaut `Box2(Vector2(-x,-y), Vector2(x,y)).Translated(c)` avec x = cos(e), y = sin(e+π/3) ∈ [0,1] : elle n'est jamais multipliée par `r`, donc fausse pour tout hexagone de rayon ≠ 1.
**Correctif :**
```cpp
return Box2(-r * Vector2(x, y), r * Vector2(x, y)).Translated(c);
```

#### 🟠 B-G40 — `Source/hexagonalpha2.cpp:112` — `HexagonAlpha2::GetTightBox()` — Réduction de l'angle modulo π/6 incorrecte

La boîte d'un hexagone tourné est π/3-périodique, avec une symétrie miroir en π/6 : pour e ∈ [π/6, π/3) la demi-largeur vaut cos(π/3 − e), pas cos(e − π/6) comme le donne `fmod(a, Math::Pi / 6.0)`. De plus `fmod` renvoie une valeur négative pour un angle négatif. Exemples : a = π/6 donne x = 1, y = 0,866 au lieu de 0,866 et 1 ; a = −π/12 donne y = 0,707 au lieu de 0,966. La boîte est fausse pour environ la moitié des orientations (cas courant).
**Correctif :**
```cpp
Box2 box(Vertex(0));
for (int i = 1; i < 6; i++) box.Extend(Vertex(i));
return box;
```

#### 🟠 B-G41 — `Source/lens2.cpp:29` — `Lens2::Lens2(const Vector2&, const double&, const double&)` — Lentille incohérente avec l'épaisseur et la hauteur demandées

Les centres sont placés à distance l = t et le rayon vaut `sqrt(t*t + h*h)`. La lentille obtenue a une épaisseur 2r − t ≠ t et une demi-hauteur √(r² − t²/4) = √(3t²/4 + h²) ≠ h ; le membre `height` (= h) ne correspond donc plus à la géométrie et fausse `Signed()`/`R()`. Pour une épaisseur T et une demi-hauteur h, il faut r = (h² + T²/4)/T et l = 2r − T.
**Correctif :**
```cpp
Lens2::Lens2(const Vector2& c, const double& t, const double& h)
  :Lens2(c - (0.5 * ((h * h + t * t / 4.0) / t * 2.0 - t)) * Vector2::X,
         c + (0.5 * ((h * h + t * t / 4.0) / t * 2.0 - t)) * Vector2::X,
         (h * h + t * t / 4.0) / t)
{}
```

#### 🟠 ✅ B-G42 — `Source/lens2.cpp:153` — `Lens2::GetBox(), Lens2::GetOrientedBox()` — Hauteur de la boîte divisée par deux

`Height()` renvoie la **demi**-hauteur √(r² − l²/4), mais elle est passée à `Box2(const double& x, const double& y)` qui attend des dimensions complètes (boîte [-x/2,x/2]×[-y/2,y/2]). La boîte n'englobe que la moitié centrale de la lentille dans la direction orthogonale à l'axe. Même motif Source/lens2.cpp:161.
**Correctif :**
```cpp
return OrientedBox2(Box2(Thickness(), 2.0 * Height()), GetFrame()).GetBox();
```

#### 🟠 B-G43 — `Source/particle2.cpp:78` — `ParticleSet2::Intersect(const Circle2&)` — Rayon du cercle argument ignoré

Le seuil `h = Math::Sqr(2.0 * r)` suppose que le cercle a le même rayon que les particules ; `circle.Radius()` n'est pas utilisé. Le résultat est faux dès que le rayon du cercle diffère de celui des particules, ce qui est le cas courant et non un cas limite.
**Correctif :**
```cpp
double h = Math::Sqr(r + circle.Radius());
```

#### 🟠 ✅ B-G44 — `Source/pentagon2.cpp:197` — `Pentagon2::Inside()` — Seuil r (rayon circonscrit) au lieu de l'apothème

En plus de B-G09, même avec des normales extérieures, la distance du centre à une arête est l'apothème r·cos(π/5) ≈ 0,809 r et non r : le test `cp * normal[i] - r > 0` accepte des points jusqu'à 24 % au-delà des arêtes. Même motif Source/pentagon2.cpp:198 et Source/pentagon2.cpp:199.
**Correctif :**
```cpp
const double ri = r * cos(Math::Pi / 5.0);
if (cp * normal[0] > ri) return false; // idem pour normal[1], normal[2]
```

#### 🟠 B-G45 — `Source/pentagon2.cpp:143` — `Pentagon2::Sector()` — Partition angulaire décalée de 36°

Les tests `s * edge[0] >= 0` et `s * edge[4] >= 0` comparent la direction aux **directions d'arêtes** au lieu des rayons des sommets. Pour x ≥ 0, le code renvoie 1 pour les angles [54°, 90°], 0 pour [−18°, 54°) et 4 pour [−90°, −18°), alors que `R()` (vertex[s], edge[s]) attend 0 pour [18°, 90°], 4 pour [−54°, 18°] et 3 pour [−90°, −54°] ; le secteur 3 n'est jamais renvoyé pour x ≥ 0. Le côté x < 0 est également décalé (1 sur [90°, 126°] au lieu de [90°, 162°]). `R()` et `Normal()` travaillent donc sur la mauvaise arête.
**Correctif :** tester les rayons des sommets : `bool b = s * vertex[0].Orthogonal() >= 0.0; bool c = s * vertex[4].Orthogonal() >= 0.0;` → x≥0 : `b ? 0 : (c ? 4 : 3)` ; x<0 : `b' ? 1 : (c' ? 2 : 3)` avec les sommets miroirs.

#### 🟠 B-G46 — `Source/pentagon2.cpp:241` — `Pentagon2::R()` — Distance à l'arête non carrée et longueur d'arête fausse

Le cas « arête » renvoie `d` au lieu de `d * d` (la doc annonce une distance au carré). `edge[]` est unitaire et la projection `ap` n'est pas normalisée par r, donc le test `l > r` compare à r alors que la longueur d'arête vaut `Edge()` = 2r·sin(π/5) ≈ 1,176 r.
**Correctif :**
```cpp
else if (l > Edge()) { /* ... */ }
else { return d * d; }
```

#### 🟠 B-G47 — `Source/pentagon2.cpp:437` — `Pentagon2::RandomInside()` — Tirage limité au quadrant x ≥ 0, y ≥ 0

`random.Uniform()` renvoie une valeur dans [0,1] : les candidats `c + r * (u, v)` ne couvrent que le quart supérieur droit du carré englobant, donc seule la partie du pentagone dans ce quadrant est échantillonnée (distribution non uniforme ; `Poisson()` hérite du biais).
**Correctif :**
```cpp
p = c + r * Vector2(random.Uniform(-1.0, 1.0), random.Uniform(-1.0, 1.0));
```

#### 🟠 B-G48 — `Source/polygon2.cpp:553` — `Polygon2::Cut()` — Appariement sortie/entrée faux si le premier sommet est à gauche

Les intersections sont collectées dans l'ordre des arêtes et appariées `(c, c+1)` en supposant que la première est une « sortie ». Le code de fragment parcourt en fait les sommets **gauches** situés entre `crossings[c]` et `crossings[c+1]` : cela ne fonctionne que si `crossings[c]` est une transition droite→gauche. Lorsque `q[0]` est du côté gauche, `crossings[0]` est une transition gauche→droite, les sommets entre `crossings[0]` et `crossings[1]` sont à droite, les fragments n'ont que 2 points et sont rejetés, et la région gauche (qui fait le tour par l'indice 0) n'est jamais construite : la fonction renvoie `false` sans couper, alors que la droite traverse le polygone.
**Correctif :** si `status[crossings[0].edge]` est vrai (transition gauche→droite), apparier `(crossings[c+1], crossings[(c+2) % m])` au lieu de `(crossings[c], crossings[c+1])` (ou faire tourner `crossings` d'un cran avant la boucle).

#### 🟠 B-G49 — `Source/polygon2.cpp:764` — `Polygon2::Centroid()` — Centroïde de signe opposé pour un polygone horaire

Les sommes `x`, `y` sont pondérées par l'aire **signée** des triangles, mais divisées par `6 * |aire|` (Source/polygon2.cpp:764-766). Pour un polygone orienté dans le sens horaire (aire signée négative), le centroïde renvoyé est l'opposé du vrai centroïde.
**Correctif :**
```cpp
area *= 0.5; // aire signée
return Vector2(x, y) / (6.0 * area);
```

#### 🟠 B-G50 — `Source/quad2.cpp:45` — `Quadrangle2::Quadrangle2(const double&)` — Sommets donnés dans le sens horaire

La classe exige des sommets en ordre trigonométrique (`quadrangle.h:60`, doc du constructeur l.24), mais ce constructeur passe (−r,−r), (−r,r), (r,r), (r,−r), qui est l'ordre **horaire**. `Vector2::Clockwise` ((b−a)/(c−a) < 0) rejette alors tout point intérieur : `Inside()` renvoie toujours faux, et `Normal()` / `R()` sont faux. Pour ce quadrangle, `InverseBilinear()` renvoie aussi systématiquement la mauvaise racine (cf. B-G81).
**Correctif :**
```cpp
Quadrangle2::Quadrangle2(const double& r) :Quadrangle2(Vector2(-r, -r), Vector2(r, -r), Vector2(r, r), Vector2(-r, r))
{}
```

#### 🟠 B-G51 — `Source/rectangle2.cpp:103` — `Rectangles2::Normal()` — Vecteur renvoyé dans le repère local du rectangle

`n` est calculé dans le repère local (`q * x`, `q * x.Orthogonal()`) puis renvoyé tel quel : pour un rectangle tourné (x ≠ X), la direction du vecteur distance est fausse (sa norme, utilisée par `R()`, reste juste).
**Correctif :**
```cpp
return n[0] * x + n[1] * x.Orthogonal();
```

#### 🟠 B-G52 — `Source/revolution-quadric2.cpp:219` — `RevolutionQuadricVolume2::GetOrientedBox(), RevolutionQuadricVolume2::GetBox()` — Centre local ajouté en coordonnées monde et demi-tailles permutées

`box` est construite dans le repère local (x radial, y axial) mais son centre `c` (local) est ajouté directement à `a` (`c + a`) en coordonnées monde au lieu d'être exprimé le long de l'axe (`a + c[1]*axis`). De plus `OrientedBox2(…, h, axis)` (via `Frame2(c, axis)`) attribue `h[0]` (demi-extension radiale) à la direction `axis` et `h[1]` (axiale) à la direction orthogonale. La boîte (et `GetBox()`) est fausse même pour un axe vertical (demi-tailles permutées), pas seulement pour un axe non vertical.
**Correctif :**
```cpp
return OrientedBox2(a + c[1] * axis, Vector2(h[1], h[0]), axis);
```

#### 🟠 B-G53 — `Source/scales2.cpp:142` — `Lune2::Area()` — Aire de la lune du grand disque au lieu du disque a

La formule utilise `la = Max(ra, rb)` comme disque conservé ; elle calcule donc π·max(ra,rb)² − aire(intersection). Or la lune est le disque (a, ra) privé du disque (b, rb), et la doc du constructeur (l.17) impose rb > ra. Exemple ra = 1, rb = 2, d = 2 : renvoie 1,936 + 10,54 − 1,318 ≈ 11,16 = 4π − 1,403 (lune du grand disque) au lieu de π − 1,403 ≈ 1,739. La formule (Héron) renvoie aussi NaN si les disques sont disjoints ou inclus.
**Correctif :**
```cpp
return Math::Pi * ra * ra - Circle2(a, ra).Area(Circle2(b, rb));
```

#### 🟠 B-G54 — `Source/simplex2.cpp:66` — `Simplex2::Corners()` — Triangle renvoyé dans l'espace normalisé (taille ignorée)

Le point est normalisé (`p = q / size`) mais les sommets du triangle sont renvoyés sans être remultipliés par `size` : pour `size ≠ 1`, le triangle ne contient pas le point argument. Voir aussi B-G59 (`Vertex()`).
**Correctif :**
```cpp
return Triangle2(size * Vector2(X0, Y0), size * Vector2(X0 + i1 - G2, Y0 + j1 - G2), size * Vector2(X0 + 1.0 - 2.0 * G2, Y0 + 1.0 - 2.0 * G2));
```

#### 🟠 B-G55 — `Source/smoothbox2.cpp:27` — `SmoothBox2::Value()` — Distance passée là où une distance au carré est attendue

`Cubic::SmoothCompact(x, r)` attend x et r **au carré** (`cubic.h:312-318`). Le code passe `d = sqrt(Box2::R(p))` avec `re * re` (Source/smoothbox2.cpp:27-29) : le support effectif devient d < re² au lieu de d < re (ex. re = 0,5 → support 0,25 ; re = 2 → support 4), et le noyau (1 − d/re²)³ a une forme fausse.
**Correctif :**
```cpp
return Cubic::SmoothCompact(Box2::R(p), re * re);
```

#### 🟠 B-G56 — `Source/smoothdisc2.cpp:55` — `SmoothDisc2::Gradient()` — Rayon du disque utilisé à la place du rayon d'atténuation

`Value()` utilise le noyau (1 − d²/re²)³ (rayon d'atténuation `re`), dont le gradient vaut −3(1 − d²/re²)²·2u/re². `Gradient()` utilise `r * r` (rayon du disque) aux deux endroits : dans `Quadric::SmoothCompact(d, r * r)` et dans le facteur `3/(r*r)`. Le gradient n'est pas celui de `Value()` (support et amplitude faux dès que r ≠ re).
**Correctif :**
```cpp
return -(3.0 * Quadric::SmoothCompact(d, re * re) / (re * re)) * 2.0 * u;
```

#### 🟠 B-G57 — `Source/smoothellipsegaussian2.cpp:120` — `GaussianEllipse2::Transform(const FrameScaled2&)` — Mise à l'échelle sans effet

`e.Scaled(fs.S()[0]);` renvoie un nouvel objet qui est ignoré : l'ellipse renvoyée est seulement tournée et translatée, jamais mise à l'échelle. De plus seule la composante x de l'échelle est lue, et l'échelle devrait s'appliquer avant la translation. Le correctif doit aussi mettre `height` à l'échelle, comme le fait `GaussianEllipse2::Scaled()` (l.110 : `s * height`).
**Correctif :**
```cpp
const double s = fs.S()[0];
GaussianEllipse2 e(Ellipse2(s * c, s * a, s * b, u).Transformed(f), s * height);
return e;
```

#### 🟠 B-G58 — `Source/triangle2.cpp:488` — `Triangle2::Intersect(const Ray2&, double&, double&)` — Profondeurs renvoyées opposées et permutées

Pour (o + t·d − pᵢ)·n = 0, t = (pᵢ − o)·n / e, mais le code calcule `(o − pᵢ)·n / e` (t = −t*). Avec la normale intérieure `Orthogonal()`, l'appariement entrée/sortie (`e < 0` → `ta`) est en plus inversé. Les deux erreurs se compensent pour le booléen renvoyé, mais `ta` vaut −t_sortie et `tb` vaut −t_entrée.
**Correctif :**
```cpp
double t = (pi - ray.Origin()) * ni / e;
if (e > 0.0) ta = Math::Max(ta, t); // entrée (normale intérieure)
else tb = Math::Min(tb, t);
```

#### 🟠 B-G59 — `Include/simplex.h:64` — `Simplex2::Vertex(int, int)` — Taille du réseau ignorée

`Vertex()` renvoie `i * X2 + j * Y2` sans multiplier par `size`, alors que `Corners()` divise le point par `size` : pour `size ≠ 1`, les sommets renvoyés ne correspondent pas à la structure. Par ailleurs X2 = e1 + e2 et Y2 = e2, donc `Vertex(i,j)` désigne le sommet (i, i+j) de `Corners()` : cette indexation est à documenter (cf. B-G102).
**Correctif :**
```cpp
return size * (i * X2 + j * Y2);
```


### K — Géométrie 3D de base

#### 🟠 B-K10 — `Source/box.cpp:1418` — `Box::Intersect(const Vector&, const Vector&)` — Test SAT segment/boîte faux sur les axes principaux (direction non normalisée)

Le demi-rayon projeté du segment sur l'axe i doit valoir `0.5*|l_i|`, mais il est calculé `hl * fl[i]` avec `hl = 0.5*Norm(l)` et `fl = Abs(l)` non normalisé, soit `0.5*|l|*|l_i|`. Pour un segment de longueur < 1 le segment est sous-estimé (faux négatifs : intersections manquées), pour une longueur > 1 il est surestimé (faux positifs). Exemple : boîte [0,1]^3, segment (1.1,0.5,0.5)-(0.95,0.5,0.5) : |T0| = 0.525 > 0.51125 → retourne 0 alors que le segment entre dans la boîte. Lignes Source/box.cpp:1418, 1421, 1424. Les trois tests sur produits vectoriels (l.1430-1440) sont, eux, invariants d'échelle et corrects.
**Correctif :**
```cpp
  if (fabs(T[0]) > E[0] + 0.5 * fl[0]) return 0;
  if (fabs(T[1]) > E[1] + 0.5 * fl[1]) return 0;
  if (fabs(T[2]) > E[2] + 0.5 * fl[2]) return 0;
```

#### 🟠 B-K11 — `Source/camera.cpp:514` — `Camera::VectorToPixel()` — Facteurs hLength et vLength inversés

`u = horizontal * ep / vLength` et `v = vertical * ep / hLength` (Source/camera.cpp:514-515). D'après `PixelToRay` (direction = view + horizontal*hLength*x + vertical*vLength*y), l'inverse est `x = (horizontal·ep)/(z*hLength)` et `y = (vertical·ep)/(z*vLength)`. Les coordonnées et le test de frustum sont faux dès que w ≠ h.
**Correctif :**
```cpp
  u = horizontal * ep / hLength;
  v = vertical * ep / vLength;
```

#### 🟠 B-K12 — `Source/cameraframes.cpp:307` — `CameraAnimatedFrame::setNbFrames()` — start() relu pendant la modification des clés

La boucle modifie `maf.time` et appelle `start()` (= `mafs[0].time`) à chaque itération : après la première itération `mafs[0].time` vaut 0, donc les clés suivantes sont recalculées avec `time - 0` au lieu de `time - ancien_start`. Résultat faux dès que la première clé n'est pas à t = 0. Par ailleurs avec une seule clé (totalFrames == 0) : division 0/0 → NaN converti en int (UB). Traces `std::cout` de débogage laissées (Source/cameraframes.cpp:298, 305, 309).
**Correctif :**
```cpp
  const int s0 = start(), totalFrames = end() - s0;
  if (s0 < 0 || totalFrames <= 0 || totalFrames == nbFrames) return;
  for (CameraFrame& maf : mafs)
    maf.time = int(double(maf.time - s0) / double(totalFrames) * nbFrames);
```

#### 🟠 B-K13 — `Source/camera.cpp:217` — `Camera::UpDown(), Camera::UpDownRound(), Camera::UpDownRoundFPS()` — Rotation calculée avec un vecteur up non orthogonal à la visée

`z = z*cos(a) + up*sin(a)` n'est une rotation que si `up` est unitaire et orthogonal à `z`. Avec up = Z (valeur posée par le constructeur, `SetAt`, `Vertical`, `CircleAnim`) et une caméra inclinée, |z'|² = 1 + 2 sin a cos a (z·up) ≠ 1 : la distance œil/cible change à chaque appel (UpDownRound : rayon d'orbite qui dérive, ≈5 % pour a = 0.1 et une inclinaison de 30°) et l'angle effectif est faux. `up = z / left` (Source/camera.cpp:220, 313) n'est pas renormalisé non plus. Lignes Source/camera.cpp:217, 310, 682.
**Correctif :**
```cpp
  Vector u = Normalized(left / z);   // up orthogonal à z, unitaire
  z = z * cos(a) + u * sin(a);       // vraie rotation, |z| = 1
  up = Normalized(z / left);
```

#### 🟠 B-K14 — `Source/framebox.cpp:74` — `OrientedBox::R()` — Valeur absolue manquante : distance nulle sur la moitié de l'espace

`Vector::Max(q - length, Vector::Null)` sans `Abs(q)` : un point extérieur du côté des coordonnées locales négatives (q[i] < -length[i]) obtient une contribution nulle, donc une distance 0. (`Signed`, juste en dessous, utilise bien `Abs(q)`.)
**Correctif :**
```cpp
  Vector n = Vector::Max(Abs(q) - length, Vector::Null);
```

#### 🟠 B-K15 — `Source/ktope.cpp:52` — `KTope::KTope(const Box&)` — Les 12 plans diagonaux restent à 0

Seuls c[0..5] sont initialisés (Source/ktope.cpp:54-59) ; c[6..17] gardent la valeur 0, soit des demi-espaces `n_i·p <= 0` passant par l'origine. Pour une boîte ne contenant pas l'origine (ex. [1,2]^3), `Inside()` renvoie faux partout et les autres tests sont faux.
**Correctif :**
```cpp
  for (int i = 0; i < 18; i++) {
    double m = -Math::Infinity;
    for (int k = 0; k < 8; k++) m = Math::Max(m, box.Vertex(k) * normals[i]);
    c[i] = -m;
  }
```

#### 🟠 B-K16 — `Source/framescaled.cpp:83` — `FrameScaled::Inverse()` — Inverse fausse dès que l'échelle n'est pas 1

La transformation est `p → R(S p) + t`, dont l'inverse est `S⁻¹ Rᵀ (p − t)`. `FrameScaled(Frame::Inverse(), s.Inverse())` représente `p → Rᵀ(S⁻¹ p) − Rᵀ t` : l'ordre échelle/rotation est inversé (faux pour une échelle non uniforme avec rotation) et la translation n'est pas mise à l'échelle (faux dès que s ≠ 1 et t ≠ 0, même pour une échelle uniforme). La forme R·S·p + t ne peut pas représenter l'inverse exacte en général.
**Correctif :** pour une échelle uniforme : `FrameScaled(r.T(), -(r.T() * t) / s[0], s.Inverse())` ; sinon documenter que l'inverse n'est pas représentable et utiliser `InverseTransform()`.

#### 🟠 B-K17 — `Source/framescaled.cpp:179` — `FrameScaled::InverseTransformDirection()` — Échelle appliquée avant la rotation et avec le mauvais facteur

Pour `T = R·S`, une normale se transforme par `R S⁻¹` (ce que fait `TransformDirection`) ; l'inverse est donc `S Rᵀ n` (ou `S⁻¹ Rᵀ n` pour un vecteur direction). Le code calcule `Rᵀ (S⁻¹ n)` : la direction est fausse dès que l'échelle n'est pas uniforme, même sans rotation, et avec un facteur inversé pour les normales.
**Correctif :**
```cpp
  return Frame::InverseTransformDirection(n).Scaled(s);   // normales
```

#### 🟠 B-K18 — `Source/frame.cpp:113` — `Frame::Canonical()` — Axe de rotation non normalisé

`a = Vector(-axis[1], axis[0], 0.0)` a pour norme sin θ, et `Matrix::Rotation(const Vector&, const double&)` (Source/matrix.cpp:276) ne normalise pas son axe : la matrice obtenue n'est pas une rotation sauf pour un axe horizontal. Le test `fabs(axis[2]) == 1.0` (Source/frame.cpp:107) est par ailleurs une égalité exacte instable, et `acos` (l.116) est appelé sans clamp.
**Correctif :**
```cpp
  double s = sqrt(axis[0] * axis[0] + axis[1] * axis[1]);
  Vector a = (s < 1e-12) ? Vector(1.0, 0.0, 0.0) : Vector(-axis[1], axis[0], 0.0) / s;
  return Frame(Matrix::Rotation(a, acos(Math::Clamp(axis[2], -1.0, 1.0))), p);
```

#### 🟠 B-K19 — `Source/plane.cpp:332` — `Plane::Symmetry()` — Valeur absolue erronée : symétrique faux du côté négatif

`p - 2.0 * Math::Abs(Eval(p)) * n` : pour un point du côté négatif (Eval < 0), le point est éloigné encore plus du plan au lieu d'être réfléchi. Impacte `Symmetric(Box/Segment/Sphere)`.
**Correctif :**
```cpp
  return p - 2.0 * Eval(p) * n;
```

#### 🟠 B-K20 — `Source/plane.cpp:259` — `Plane::Intersect(const Plane&)` — Point de la droite d'intersection faux

`q = Vertex()` (sur A) puis `q += plane.Signed(q) * plane.n` : le signe est inversé (projeter sur B est `q -= Signed·n_B`, le code éloigne le point de B) et un déplacement le long de n_B fait quitter le plan A dès que les plans ne sont pas orthogonaux. L'axe renvoyé n'appartient en général à aucun des deux plans.
**Correctif :**
```cpp
  Vector u = n / plane.n;                    // n_A x n_B
  double uu = u * u;
  Vector q = (cqc * (plane.n / u) + plane.cqc * (u / n)) / uu;
  return Axis(q, q + u / sqrt(uu));
```

#### 🟠 B-K21 — `Source/raystep.cpp:68` — `RayStep::Step()` — Centre décalé de la demi-longueur précédente

Ancien centre c = a + 0.5·l_old·u ; nouveau centre attendu a + s·u + 0.5·l·u. `c += (s + 0.5 * l) * u` ajoute en trop 0.5·l_old·u : la sphère englobante (`GetSphere`) et `RayStep::Intersect(Box)` (qui utilise c) dérivent à chaque pas.
**Correctif :**
```cpp
  c = a + 0.5 * lu;   // après la mise à jour de a
```

#### 🟠 B-K22 — `Include/rectangle.h:60` — `Rectangles::Area()` — Aire 4 fois trop petite

`a` et `b` sont des demi-côtés (cf. `Vertex()` qui utilise (2u − 1)·a, `Width()` « half width », `R()` qui compare |n| à a, `GetBox()` avec ±a) : l'aire vaut 4ab et non ab.
**Correctif :**
```cpp
  return 4.0 * a * b;
```

#### 🟠 B-K23 — `Include/quadricsurface.h:34` — `QuadricSurface::Index(), QuadricSurface::operator()(int,int)` — Coefficients x²y et xy² intervertis

`Index(a,b)` doit renvoyer l'indice de xᵃyᵇ. Avec `terms = {8,6,3,7,5,2,4,1,0}` et `Index(a,b) = terms[a + 3b]` : `Index(2,1) = terms[5] = 2` renvoie c[2] (coefficient de xy²) au lieu de c[1] (x²y), et `Index(1,2) = terms[7] = 1` renvoie c[1]. Les deux termes sont intervertis ; les 7 autres indices sont corrects.
**Correctif :**
```cpp
  static constexpr int terms[9] = { 8, 6, 3, 7, 5, 1, 4, 2, 0 };
```

#### 🟠 B-K24 — `Source/tetrahedra-intersection.cpp:198` — `Tetrahedra::Intersect(const Tetrahedra&)` — Seconde phase : côté testé opposé à celui de la première phase

Phase 1 (FaceA_1/FaceA_2) : une face de this sépare si tous les sommets de T sont du côté positif de `e_i × e_j` (coord > 0, convention de Ganovelli et al.). Phase 2 : `T2.Inside(Plane(e_v2[0] / e_v2[1]))` utilise `Plane::Inside` (n·p − 0 < 0), donc teste que tous les sommets de this sont du côté négatif de normales construites de la même façon. Les deux phases sont incohérentes : séparations manquées et faux « disjoints ». Lignes Source/tetrahedra-intersection.cpp:198, 202, 204, 211.
**Correctif :** tester le côté positif, p. ex. `if (T2.Inside(Plane(-(e_v2[0] / e_v2[1])))) return false;` (idem pour les trois autres faces).

#### 🟠 B-K25 — `Include/tetrahedra.h:65` — `Tetrahedra::Tetrahedra(const Box&, int)` — Trois des six tétraèdres de découpe ont des normales rentrantes

Le constructeur exige un ordre de sommets donnant des normales sortantes (Source/tetrahedra.cpp:28-29), ce qui correspond à det(p1−p0, p2−p0, p3−p0) < 0. Calcul sur la boîte unité (Vertex k : bit0 = x, bit1 = y, bit2 = z) : `split[0]` {0,1,5,7} det = −1, `split[1]` {0,1,3,7} det = +1, `split[2]` {0,3,7,2} det = −1, `split[3]` {1,6,7,2} det = +1, `split[4]` {1,6,4,7} det = +1, `split[5]` {1,4,7,5} det = −1. Pour k = 1, 3, 4 les normales `n[]` pointent vers l'intérieur : `Inside`, `Signed`, `Normal`, `R` et `Intersect` sont faux pour ces tétraèdres. Lignes Include/tetrahedra.h:65-71.
**Correctif :** échanger deux sommets dans les lignes concernées : `{ 0, 3, 1, 7 }`, `{ 1, 7, 6, 2 }`, `{ 1, 4, 6, 7 }`.

#### 🟠 B-K26 — `Source/tetrahedra.cpp:259` — `Tetrahedra::Area()` — Formule sans rapport avec l'aire

La doc annonce la somme des aires des quatre faces ; le code renvoie `(u+v+w)(v-w+u)(w-u+v)(u-v+w)` avec u, v, w les longueurs des trois arêtes issues de p0 : c'est 16 fois le carré de l'aire d'un triangle de côtés u, v, w (Héron), triangle qui n'est pas une face du tétraèdre ; le résultat est en outre homogène à une longueur⁴. Faux pour toute entrée (🔴 défendable).
**Correctif :**
```cpp
  return GetTriangle(0).Area() + GetTriangle(1).Area() + GetTriangle(2).Area() + GetTriangle(3).Area();
```

#### 🟠 B-K27 — `Source/triangle.cpp:1059` — `Triangle::Intersect(const Sphere&)` — Vecteur TS issu du mauvais sommet

L'algorithme d'Eberly requiert `D = edge0·(p0 − C)`, `E = edge1·(p0 − C)`, `F = |p0 − C|²`. Or `TS` garde la valeur de la dernière itération de la boucle, soit `p[2] − C` : la distance minimale calculée est fausse et des intersections sont manquées ou inventées quand les trois sommets sont hors de la sphère.
**Correctif :**
```cpp
  TS = p[0] - sphere.Center();   // avant le calcul de D (l.1059)
```


#### 🟠 ✅ B-K34 — `Source/line.cpp:50` — `Line::R(const Line&)` — Cas parallèle : renvoie la longueur au carré du vecteur directeur

Le test `e < 1e-6` (avec `e = |u|²|v|² sin²θ`) est un seuil absolu : toute paire de droites définies par des points proches (|u|·|v|·sin θ < 1e-3) est traitée comme parallèle, même très éloignée, et la fonction renvoie alors `uu = |b-a|²`, qui n'est pas une distance. Mesuré : droites parallèles (|u| = 10, d = 2) → 100 au lieu de 4 ; droites gauches orthogonales (|u| = 0,01, d = 3) → 1e-4 au lieu de 9 ; sur des droites quasi parallèles, erreur relative jusqu'à 10²–10³. Criticité recalibrée de 🟡 à 🟠 : résultat faux dans des cas courants. Aucun appelant dans le dépôt.
**Correctif appliqué :**
```cpp
if (e < 1e-12 * uu * vv)   // seuil relatif : sin²θ < 1e-12
{
  return line.R(a);        // distance d'un point de cette droite à l'autre
}
```
Doc Doxygen de `Line`, `Line::R(const Vector&)` et `Line::R(const Line&)` complétée (formules, seuil, eturn, sa). Vérifié contre une référence `long double` : erreur relative maximale < 4e-12 pour θ de 1e-1 à 1e-9, exact pour les cas parallèles et à petite échelle.

### R — Primitives 3D volumiques

#### 🟠 B-R10 — `Source/boxsphere.cpp:90` — `BoxSphere::Volume()` — Volume faux : la correction des 12 arêtes est oubliée

Avec L = b − a, le volume de la somme de Minkowski boîte ⊕ boule vaut LxLyLz + 2r(LxLy+LyLz+LzLx) + πr²(Lx+Ly+Lz) + 4/3πr³. Le code calcule (Lx+2r)(Ly+2r)(Lz+2r) − (8 − 4π/3)r³, qui retranche bien le défaut des coins mais pas celui des arêtes : résultat surestimé de (4 − π)r²(Lx+Ly+Lz).
**Correctif :**
```cpp
Vector l = b - a;
return l[0]*l[1]*l[2] + 2.0*r*(l[0]*l[1] + l[1]*l[2] + l[2]*l[0])
     + Math::Pi*r*r*(l[0] + l[1] + l[2]) + (4.0/3.0)*Math::Pi*r*r*r;
```

#### 🟠 B-R11 — `Source/boxsphere.cpp:101` — `BoxSphere::Area()` — Aire fausse : la correction des arêtes est oubliée

L'aire exacte vaut 2(LxLy+LyLz+LzLx) + 2πr(Lx+Ly+Lz) + 4πr². Le code calcule 2(XY+YZ+ZX) − (24 − 4π)r² avec X = Lx+2r…, soit 2ΣLL + 8rΣL + 4πr² : surestimation de (8 − 2π)r(Lx+Ly+Lz).
**Correctif :**
```cpp
Vector l = b - a;
return 2.0*(l[0]*l[1] + l[1]*l[2] + l[2]*l[0]) + Math::TwoPi*r*(l[0] + l[1] + l[2]) + 4.0*Math::Pi*r*r;
```

#### 🟠 B-R12 — `Source/cone.cpp:103` — `Cone::Intersect(), NewCone::Intersect()` — L'intervalle [v0, v1] est supposé toujours intérieur au cône infini

Le coefficient dominant de q = d − r vaut 1 − dx²(1 + rlength²), négatif quand l'angle entre le rayon et l'axe est inférieur au demi-angle du cône (cas courant, p. ex. un rayon visant le long de l'axe). L'intérieur du cône le long du rayon est alors (−∞, v0] ∪ [v1, +∞) (une seule nappe utile), et l'intersection avec [u0, u1] calculée par max/min est fausse (rayon le long de l'axe : racine double à l'apex, retour 0 alors que le rayon traverse le cône). Même défaut en Source/cone.cpp:176, Source/conet.cpp:104 et Source/conet.cpp:179.
**Correctif :** traiter le cas `q[2] < 0` : choisir la nappe du côté du tronc (s(t) tel que ra + rlength·s ≥ 0) et intersecter la demi-droite correspondante avec [u0, u1] ; traiter aussi dx ≈ 0 (u = ±inf/NaN).

#### 🟠 B-R13 — `Source/cone.cpp:759` — `Cone::Signed()` — Distance signée positive à l'intérieur des cônes avec ra ≤ rb

Dans la branche `ra <= rb`, `y` a été décalé (`y -= length`) avant `Math::Max(-y, y - length, rx)` : −y vaut alors length − y_orig ≥ 0 pour tout point intérieur, et le résultat est positif (point classé extérieur). Même erreur Source/cone.cpp:802. Le cas ra ≤ rb (apex en a) est courant.
**Correctif :**
```cpp
// lignes 759 et 802 : y est décalé de -length
return Math::Max(-(y + length), y, rx);
```

#### 🟠 B-R14 — `Source/conesphere.cpp:216` — `ConeSphere::Normal()` — Vecteur distance faux sur la partie conique

Après translation au point de tangence, `i` et `j` portent tx (radial) et ty (axial) ; le vecteur distance à la génératrice doit être rx·(side[1]·î − side[0]·axis), avec rx = tx·side[1] − ty·side[0]. Le code renvoie `i*side[1] - j*side[0]` = tx·side[1]·î − ty·side[0]·axis, qui n'est ni de la bonne direction ni de la bonne norme (sauf si tx ou ty est nul).
**Correctif :**
```cpp
Vector ir = (n - y0 * axis) / x; // vecteur radial unitaire, y0 = coordonnée axiale initiale
e = rx * (side[1] * ir - side[0] * axis);
```

#### 🟠 B-R15 — `Source/conesphere.cpp:197` — `ConeSphere::Normal()` — Vecteur distance faux vers la sphère de l'extrémité b

Aux lignes 173-174, i et j ont déjà été remplacés par (x − sc_hra)·î et (y − sc_ha)·axis (translation au point de tangence). Dans la branche `ry > s`, `j *= 1.0 - length / y` donne (y0 − sc_ha)(y0 − 2·length)/(y0 − length)·axis au lieu de (y0 − length)·axis, et i reste translaté au lieu de x·î : le vecteur `(i + j) * (1 - rb/sqrt(nn))` (ligne 204) est faux, même si le test `nn > rb*rb` est juste.
**Correctif :**
```cpp
Vector ir = (x > 0.0) ? (n - y0 * axis) / x : Vector::Null; // y0 = coordonnée axiale initiale
Vector w = x * ir + (y0 - length) * axis;                   // p - b
e = w * (1.0 - rb / Norm(w));
```

#### 🟠 B-R16 — `Source/cuboctahedron.cpp:27` — `Cuboctahedron::normal[]` — Normales des faces triangulaires fausses (7 sur 8) et carrés dans le mauvais ordre

Le tableau des normales triangulaires contient des doublons ((−,−,−), (+,+,−), (−,−,+), (+,+,+) deux fois chacun) et omet les octants (+,−,·) et (−,+,·). D'après `face[]`, les normales attendues sont : f0 (−,−,−), f1 (+,−,−), f2 (+,−,+), f3 (−,−,+), f4 (+,+,+), f5 (−,+,+), f6 (−,+,−), f7 (+,+,−). Seule `normal[0]` est juste. L'ordre des normales des carrés (−x,+x,−y,+y,−z,+z) ne correspond pas non plus à celui des carrés de `face[]` (−y,+x,+z,−x,−z,+y).
**Correctif :**
```cpp
const Vector Cuboctahedron::normal[14] = {
  Vector(-1,-1,-1) / Math::Sqrt3, Vector( 1,-1,-1) / Math::Sqrt3,
  Vector( 1,-1, 1) / Math::Sqrt3, Vector(-1,-1, 1) / Math::Sqrt3,
  Vector( 1, 1, 1) / Math::Sqrt3, Vector(-1, 1, 1) / Math::Sqrt3,
  Vector(-1, 1,-1) / Math::Sqrt3, Vector( 1, 1,-1) / Math::Sqrt3,
  Vector(0,-1,0), Vector(1,0,0), Vector(0,0,1), Vector(-1,0,0), Vector(0,0,-1), Vector(0,1,0)
};
```

#### 🟠 B-R17 — `Source/cuboid.cpp:28` — `Cuboid::quadrangle[], Cuboid::GetQuadrangle(), Cuboid::Area()` — Quatre faces sur six ont des sommets dans un ordre non cyclique (quadrilatère croisé)

Avec la numérotation de `Box::Vertex` (bit0 = x, bit1 = y, bit2 = z), les faces `4,5,6,7`, `1,3,5,7`, `0,1,4,5` et `3,2,7,6` ont une diagonale en guise d'arête (5→6, 3→5, 1→4, 2→7). `GetQuadrangle()` (utilisé par `Mesh`) renvoie des quadrilatères croisés et `Area()` est fausse dès que les faces ne sont pas des parallélogrammes (les deux triangles (p0,p1,p2) et (p0,p2,p3) se recouvrent).
**Correctif :**
```cpp
const int Cuboid::quadrangle[24] =
{
  0, 2, 3, 1,
  4, 5, 7, 6,
  0, 4, 6, 2,
  1, 3, 7, 5,
  0, 1, 5, 4,
  3, 2, 6, 7
};
```

#### 🟠 B-R18 — `Include/octahedron.h:52` — `Octahedron::Volume()` — Volume divisé par 8

Un octaèdre de demi-diagonales (hx, hy, hz) est formé de 8 tétraèdres trirectangles de volume hx·hy·hz/6, soit un volume de 4/3·hx·hy·hz. Le code renvoie hx·hy·hz/6 (volume d'un seul tétraèdre), contrairement au commentaire (« 8 times the volume of a tetrahedron », qui dit aussi à tort « 1/48 the volume of the box » au lieu de 1/6).
**Correctif :**
```cpp
return (4.0 / 3.0) * half[0] * half[1] * half[2];
```

#### 🟠 B-R19 — `Include/octahedron.h:66` — `Octahedron::Area()` — Aire doublée

L'aire d'une face (hx,0,0),(0,hy,0),(0,0,hz) vaut ½‖(hy·hz, hx·hz, hx·hy)‖ ; 8 faces donnent 4·‖…‖. Le code renvoie 8·‖…‖ (oubli du facteur ½ du produit vectoriel).
**Correctif :**
```cpp
return 4.0 * Norm(Vector(half[1] * half[2], half[0] * half[2], half[0] * half[1]));
```

#### 🟠 B-R20 — `Source/cylinder.cpp:820` — `Cylinder::Transformed(const FrameScaled&)` — Le rayon transformé ignore le rayon d'origine

Le rayon du cylindre transformé est `Abs(frame.S()).Max()`, c.-à-d. le seul facteur d'échelle : le rayon `r` d'origine est perdu (un cylindre de rayon 0,1 transformé par une échelle unité aura un rayon 1).
**Correctif :**
```cpp
return Cylinder(frame.Transform(a), frame.Transform(b), r * Abs(frame.S()).Max());
```

#### 🟠 B-R21 — `Source/cylinder.cpp:435` — `Cylinder::SignedNormal()` — Test paroi/disques incohérent : les disques ne sont jamais choisis

Pour un point intérieur, `u` est la distance radiale à l'axe (u > 0) et `s` la distance au disque le plus proche (s ≥ 0). Le test `if (-u < s)` est donc toujours vrai et la branche des disques (lignes 440-443) est du code mort : les points intérieurs proches d'un disque (cas fréquent) reçoivent toujours le vecteur radial, résultat faux dans un cas courant. Il faudrait comparer la distance à la paroi `r - u` à `s`. De plus `nc` (lignes 424 et 429) est orienté vers l'intérieur (+axis·sa au disque a, axis·sb < 0 au disque b), à l'inverse de la convention de la branche radiale.
**Correctif :**
```cpp
if (r - u < s) { n *= r / u - 1.0; }
else { n = (sa > -sb) ? (-sb) * axis : -sa * axis; }
```

#### 🟠 B-R22 — `Source/dodecahedron-rhombic.cpp:58` — `RhombicDodecahedron::Signed()` — Le centre c est ignoré

`Vector t = Abs(p);` au lieu de `Abs(p - c)` : la distance est calculée pour un dodécaèdre centré à l'origine, quel que soit `c`.
**Correctif :**
```cpp
Vector t = Abs(p - c);
```

#### 🟠 B-R23 — `Include/dodecahedron.h:118` — `RhombicDodecahedron::GetBox()` — Boîte englobante trop petite

Avec r = longueur d'arête = distance aux 8 sommets d'ordre 3 (cohérent avec `Volume()` = 16√3/9·r³ et `Area()` = 8√2·r²), les 6 sommets d'ordre 4 sont à 2r/√3 ≈ 1,155 r du centre le long des axes. `Box(c, r)` ne contient donc pas le solide, et `RandomInside()` ne tire jamais près des sommets d'ordre 4.
**Correctif :**
```cpp
return Box(c, 2.0 * r / Math::Sqrt3);
```

#### 🟠 B-R24 — `Source/dodecahedron.cpp:80` — `Dodecahedron::Volume(), Dodecahedron::Area(), Icosahedron::Volume()/Area(), Icosidodecahedron::Volume()/Area()` — Longueur d'arête fausse et rayon r incohérent avec les sommets

Les sommets ne sont pas normalisés (rayon circonscrit √3) : `Vertex(i) = c + r·vertex[i]` donne un rayon circonscrit r√3 et une arête 2r/φ, alors que `Radius()` est documenté comme rayon circonscrit. `Volume()` et `Area()` (Source/dodecahedron.cpp:89) prennent comme arête u = r/√3, qui n'est l'arête dans aucune des deux interprétations (arête = 2r/φ avec les sommets actuels, 2r/(φ√3) si r était le rayon circonscrit).
Même défaut dans `Icosahedron::Volume()`/`Area()` (Source/icosahedron.cpp:102 et :111) : sommets d'arête 2 non normalisés (rayon circonscrit r·R2, arête 2r), mais u = r/R2 = demi-arête si r était le rayon circonscrit (volume divisé par 8, aire par 4). Et dans `Icosidodecahedron::Volume()`/`Area()` (Source/icosidodecahedron.cpp:187 et :196) : les sommets sont ceux d'arête 1 et de rayon circonscrit φ (et non √3 : `R2 = sqrt(3.0)` ligne 131 est faux) ; avec les sommets actuels l'arête vaut r, si r est le rayon circonscrit elle vaut r/φ, jamais r/√3.
**Correctif :** dodécaèdre : normaliser la table des sommets par R2 (et `GetBox()` en `Box(c, r * Phi / R2)`), puis utiliser `double u = 2.0 * r / (Phi * R2);` ; icosaèdre : u = 2r/R2 après normalisation des sommets ; icosidodécaèdre : corriger R2 en φ, normaliser les sommets et prendre u = r/φ.

#### 🟠 B-R25 — `Source/ellipsoid.cpp:98` — `Ellipsoid::Value(), Ellipsoid::Signed(), Ellipsoid::Ellipsoid(const Matrix&, const Vector&)` — Conventions de repère incohérentes entre constructeurs et requêtes

`Frame::Transform` passe du repère local au repère monde (r·p + t). Les constructeurs (lignes 20, 29, 38, 47) stockent un repère local→monde, alors que `Ellipsoid(const Matrix&, const Vector&)` (Source/ellipsoid.cpp:65) stocke son inverse. `Value()`/`Signed()` appliquent `frame.Transform(p)` à un point monde, ce qui n'est juste que pour le repère inversé ; à l'inverse `Center()`, `GetBox()`, `GetSphere()` (frame.T()) et `GetPrincipalAxis()` (colonnes de R) ne sont justes que pour le repère direct. Résultat : pour un ellipsoïde construit par axes et centre non nul, `Signed()` est faux ; pour un ellipsoïde issu de `FromPoints()`, centre, boîte et axe principal sont faux (ce qui fausse `Capsule::FromPoints()`).
**Correctif :** adopter une seule convention (repère local→monde) : supprimer `.Inverse()` ligne 65 et écrire, ligne 98 :
```cpp
Vector q = frame.InverseTransform(p);
```

#### 🟠 B-R26 — `Source/spheroid.cpp:162` — `Spheroid::Area()` — Division par e² au lieu de l'excentricité e

Avec ε l'excentricité, l'aire vaut 2πb²(1 + a²/(b²ε)·atanh ε) (aplati) et 2πb²(1 + a/(bε)·arcsin ε) (allongé). Le code nomme `e` le carré ε² et divise par `e` (ligne 162 et Source/spheroid.cpp:169) au lieu de `se` = ε : le second terme est surestimé d'un facteur 1/ε.
**Correctif :**
```cpp
return s * (1.0 + a2 * atanh(se) / (se * b2)); // aplati
return s * (1.0 + a * asin(se) / (se * b));    // allongé
```

#### 🟠 B-R27 — `Source/hemisphere.cpp:173` — `HemiSphere::Inside()` — Teste la mauvaise moitié de la sphère

`q = c - p` puis `s = q * axis` : s < 0 correspond à (p − c)·axis > 0, c.-à-d. au côté où se trouve la demi-sphère (cf. `Signed()`, `R()` et `Normal()` qui prennent `(p - c) * axis > 0` comme côté sphère). La fonction renvoie donc `false` pour les points de la demi-sphère et `true` pour ceux de la moitié opposée.
**Correctif :**
```cpp
Vector q = p - c;
if (q * q > r * r) return false;
return (q * axis >= 0.0);
```

#### 🟠 B-R28 — `Source/hemisphere.cpp:333` — `HemiSphere::Relaxation()` — Utilise le rayon de la demi-sphère au lieu du rayon de Poisson

Le déplacement `r * e` et le support `16.0 * r * r` utilisent le membre `r` (rayon de la demi-sphère), alors que la doc définit `e` comme une fraction du rayon de Poisson, qui n'est pas passé en paramètre. Pour une demi-sphère de rayon 10 échantillonnée avec un rayon de Poisson de 0,1, les points sont déplacés de 1,25 et tous interagissent : la relaxation est fausse dans tout usage normal (rayon de Poisson ≪ r). `Normalized(u)` donne aussi NaN pour deux points confondus. Voir aussi B-R29 (signe du déplacement).
**Correctif :**
```cpp
void HemiSphere::Relaxation(QVector<Vector>& p, const double& rp, const double& e, int n) const
// ...
Vector v = (nu > 0.0) ? rp * e * Normalized(u) * Cubic::SmoothCompact(nu, 16.0 * rp * rp) : Vector::Null;
```

#### 🟠 B-R29 — `Source/hemisphere.cpp:334` — `HemiSphere::Relaxation()` — Signe du déplacement inversé : les points s'attirent au lieu de se repousser

u = p[j] − p[i] et `d[i] += v` avec v colinéaire à +u : i se rapproche de j (et j de i via `d[j] += -v`). C'est le contraire d'une relaxation de Poisson : comparer avec `SphereTile::Relaxation` (Source/sampling.cpp:90), qui ajoute `-r * c * Normalized(dis)`. Distinct de B-R28 (rayon utilisé).
**Correctif :**
```cpp
d[i] -= v;
d[j] += v;
```

#### 🟠 B-R30 — `Source/hemispheredisc.cpp:197` — `HemiSphereDisc::Signed()` — Renvoie la distance au carré à l'extérieur

Copiée de `R()`, la fonction renvoie `y * y` (ligne 197), `z * z` (Source/hemispheredisc.cpp:216), `y * y` (Source/hemispheredisc.cpp:227) et `x * x + y * y` (Source/hemispheredisc.cpp:232) pour les points extérieurs, au lieu de distances non élevées au carré, alors que l'intérieur renvoie une vraie distance signée. Le champ n'est ni continu ni 1-lipschitzien.
**Correctif :**
```cpp
return y;              // ligne 197 (après y -= s)
return z;              // ligne 216 (z = sqrt(z) - s)
return -y;             // ligne 227 (y <= 0)
return sqrt(x * x + y * y); // ligne 232
```

#### 🟠 B-R31 — `Source/lens.cpp:26` — `Lens::Lens(const Vector&, const double&, const double&)` — Lentille construite incohérente avec l'épaisseur et la hauteur demandées

Le constructeur place les centres des sphères à distance l = t et prend r = √(t² + h²). Or pour deux sphères de rayon r dont les centres sont à distance l, la hauteur vaut √(r² − l²/4) = √(h² + 3t²/4) ≠ h, et l'épaisseur vaut 2r − l ≠ t. Le membre `height` (= h) ne correspond donc pas à la géométrie, et `Signed()`, qui l'utilise, est faux. Pour une épaisseur T et une hauteur h : r = (h² + T²/4)/T, l = 2r − T.
**Correctif :**
```cpp
Lens::Lens(const Vector& c, const double& t, const double& h)
  :Lens(c - (((h * h + t * t / 4.0) / t) - t / 2.0) * Vector::Z,
        c + (((h * h + t * t / 4.0) / t) - t / 2.0) * Vector::Z,
        (h * h + t * t / 4.0) / t)
{}
```

#### 🟠 B-R32 — `Source/particle.cpp:64` — `ParticleSet::Intersect(const Sphere&)` — Le rayon de la sphère testée est ignoré

Le seuil est `(2r)²` : il suppose que la sphère a le même rayon que les particules. Pour une sphère de rayon R ≠ r, l'intersection est fausse (il faut (r + R)²).
**Correctif :**
```cpp
double h = Math::Sqr(r + sphere.Radius());
```

#### 🟠 ✅ B-R33 — `Source/pyramid.cpp:65` — `Pyramid::Volume()` — Volume divisé par 8

`a` est la demi-arête de la base (cf. `vertex[]` en (±1, ±1, 0) mis à l'échelle par a) : la base a pour aire 4a² et le volume vaut 4a²h/3. Le code renvoie a²h/6.
**Correctif :**
```cpp
return 4.0 * a * a * height / 3.0;
```

#### 🟠 ✅ B-R34 — `Source/pyramid.cpp:75` — `Pyramid::Area()` — Aire latérale doublée et base oubliée

Les 4 faces latérales ont pour base 2a et pour hauteur d'apothème √(a² + h²) : aire latérale 4a√(a² + h²), plus la base 4a². Le code renvoie 8a√(a² + h²), sans la base, contrairement à sa propre doc (« eight times the area of the small triangle, plus surface of the base square »).
**Correctif :**
```cpp
return 4.0 * a * sqrt(a * a + height * height) + 4.0 * a * a;
```

#### 🟠 B-R35 — `Source/revolution-quadric.cpp:91` — `RevolutionQuadricTube::Normal()` — Cubique construite avec le décalage de la mauvaise extrémité

La cubique des points critiques de ‖C(t) − P‖² doit être formée avec E = C(0) − P (comme dans `Signed()`, Source/revolution-quadric.cpp:136). Ici `e` vaut P − C(0) ou P − C(1) selon l'extrémité la plus proche (lignes 79-88) : dès que l'extrémité t = 1 est la plus proche (environ la moitié des points), les coefficients sont calculés avec P − C(1), les racines sont fausses et le point le plus proche de la courbe peut être manqué.
**Correctif :** calculer la cubique avec `px - x[0]`, `py - y[0]` (décalage par rapport à C(0), cohérent avec `Signed()`) avant la comparaison des extrémités, puis garder `e` uniquement pour le choix de l'extrémité.

#### 🟠 B-R36 — `Source/scales.cpp:123` — `Lune::Normal()` — Fonction inachevée renvoyant des vecteurs sans rapport avec la distance

Marquée « GUNDY Finish this » : elle renvoie `-nb` (vecteur vers le centre de la sphère de coupe) ou `na` (vecteur depuis le centre de la grande sphère) quand le point est dans la sphère de coupe, et `Vector::Null` dans tous les autres cas, y compris pour les points extérieurs à la grande sphère. Ce n'est le vecteur distance dans aucun cas. Les variables `da`, `ra`, `ea`, `eb`, `q` sont inutilisées.
**Correctif :** implémenter le vecteur distance à la différence de sphères (sphère, sphère de coupe, cercle d'intersection selon la région) ou calculer le gradient numérique de `Signed()`.

#### 🟠 B-R37 — `Source/sphere.cpp:781` — `Sphere::Volume(const Sphere&)` — Volume de la lentille d'intersection : d² utilisé à la place de d

Le volume de l'intersection de deux boules de rayons R, r à distance d vaut π(R + r − d)²(d² + 2dr − 3r² + 2dR + 6rR − 3R²)/(12d). Le code utilise `cc` (= d²) là où il faut `scc` (= d) : dans `Sqr(ra + rb - cc)`, dans le premier terme `scc` (au lieu de d²), dans `2.0 * cc * rb` et `2.0 * cc * ra` (au lieu de 2d·r, 2d·R) et au dénominateur `12.0 * cc`. Le résultat est faux (non homogène à un volume).
**Correctif :**
```cpp
return Math::Pi * Math::Sqr(ra + rb - scc)
  * (cc + 2.0 * scc * rb - 3.0 * rb * rb + 2.0 * scc * ra + 6.0 * ra * rb - 3.0 * ra * ra) / (12.0 * scc);
```

#### 🟠 B-R38 — `Source/sphere.cpp:654` — `Sphere::Intersect(const Sphere&, Circle&)` — Centre du cercle d'intersection du mauvais côté

`n = c - sphere.c` pointe de l'autre sphère vers celle-ci ; le plan d'intersection est à la distance t·d de `c` *en direction de* `sphere.c`, soit `c - t * n`. Le code place le cercle en `c + t * n`, symétrique du bon par rapport à `c`. `Lune::Lune(const Sphere&, const Sphere&)` en dépend.
**Correctif :**
```cpp
circle = Circle(c - t * n, n / sqrt(length), rc);
```

#### 🟠 B-R39 — `Source/sphere.cpp:888` — `Sphere::Intersection(const Sphere&, const Sphere&, const Vector&, Vector&, Vector&)` — Points renvoyés relatifs à l'origine au lieu du centre de sa

`u = da * ab + x * bi` et `v = da * ab - x * bi` (ligne 888) oublient d'ajouter `sa.Center()` : les points ne sont justes que si `sa` est centrée à l'origine (les assert qui suivent échouent sinon). De plus `x = sqrt(ra2 - da * da)` (Source/sphere.cpp:884) vaut NaN si une sphère contient l'autre, cas non testé.
**Correctif :**
```cpp
u = sa.Center() + da * ab + x * bi;
v = sa.Center() + da * ab - x * bi;
```

#### 🟠 B-R40 — `Source/sphereset.cpp:92` — `SphereSet::Signed()` — Initialisation avec la distance au carré au lieu de la distance signée

Le minimum est initialisé avec `spheres.at(0).R(p)` (distance au carré, nulle à l'intérieur) puis comparé aux `Signed()` des autres sphères : si le point est dans la première sphère, le résultat vaut au plus 0 au lieu d'être négatif ; hors de celle-ci, on compare une distance au carré à des distances. Le résultat est faux dès que la première sphère est la plus proche.
**Correctif :**
```cpp
double a = spheres.at(0).Signed(p);
```

#### 🟠 B-R41 — `Source/taper.cpp:86` — `Taper::GetBox()` — Le facteur de tapering est ignoré

La boîte est l'union des boîtes des cercles de révolution passant par les 8 sommets, avec le rayon non déformé `Norm(p - o)` (lignes 86 et 101). Or `Transform()` multiplie la composante radiale par `c(t/length)` : dès que le facteur dépasse 1 quelque part, l'objet déformé sort de la boîte. Il faut multiplier chaque rayon par le maximum de |c| sur l'intervalle [tmin, tmax] des coordonnées axiales des sommets (la cubique peut dépasser ses valeurs aux extrémités).
**Correctif :**
```cpp
// tmin, tmax : coordonnées axiales extrêmes des sommets, bornées à [0, length]
double cmax = Math::Max(fabs(c(tmin / length)), fabs(c(tmax / length)));
// + valeurs aux racines de c' dans [tmin/length, tmax/length]
r *= cmax;
```

#### 🟠 B-R42 — `Source/twist.cpp:125` — `Twist::K()` — Constante de Lipschitz sous-estimée

Avec k = ω·r, la jacobienne du twist s'écrit (dans un repère adapté) [[I₂, w], [0, 1]] avec ‖w‖ = k ; sa plus grande valeur singulière vaut (k + √(k² + 4))/2, soit environ 1 + k/2 pour k petit. Le code renvoie √(1 + 4k²/π²), toujours inférieur (k = π/2 : 1,414 au lieu de 2,058 ; k petit : 1 + 2k²/π² au lieu de 1 + k/2). Un sphere tracing utilisant cette constante dépasse la surface. Le paramètre `r` n'est pas documenté.
**Correctif :**
```cpp
double k = fabs(omega) * r;
return 0.5 * (k + sqrt(k * k + 4.0));
```


### C — Courbes et surfaces paramétriques

#### 🟠 B-C03 — `Source/bezier-patch-cubic.cpp:68` — `BezierCubicPatch::Vertex(const double&, const double&)` — Paramétrage transposé vs BezierPatch : normales du maillage évaluées en (v,u)

`Vertex(j,k)` vaut `c[4k+j]` (indice x = j), et le code le pondère par B_j(v)·B_k(u) : l'indice x est donc associé à v. La classe de base `BezierPatch::Vertex(u,v)` associe l'indice x à u (`Vertex(i,j)*B_j(v)*B_i(u)`), et `BezierCubicPatch` n'a pas de `Tangents()` propre : il hérite de `BezierPatch::Tangents()`, qui suit la convention de base. `BezierCubicPatch::Normal(u,v)` (via `Tangents`) donne donc la normale de la surface au point S_c(v,u), alors que `Vertex(u,v)` donne S_c(u,v). Dans `BezierPatch::GetMesh()` (appels virtuels), sommets et normales ne correspondent pas dès que u≠v (théière `Teapot()` comprise).
**Correctif :**
```cpp
for (int i = 0; i < 4; i++)
  for (int j = 0; j < 4; j++)
    q += Vertex(i, j) * Math::binomials[3][j] * vj[j] * vmj[3 - j] * Math::binomials[3][i] * uk[i] * umk[3 - i];
```

#### 🟠 ✅ B-C04 — `Source/clothoid.cpp:59` — `Clothoid2::Clothoid2(a, ca, b, cb)` — Pente B incohérente avec Curvature() : courbures demandées non respectées

`Curvature(t)` vaut `t/B` (clothoid.h:50), et le constructeur à 2 paramètres pose `B = a/ca`, ce qui donne bien Curvature(a) = ca. Le constructeur à 4 paramètres pose en revanche `B = sqrt((b-a)/(π(cb-ca)))`, une formule d'abscisse curviligne réelle (s = πB·t). Exemple : a=0, ca=0, b=1, cb=1 donne B = 1/√π, donc Curvature(1) = √π ≠ 1. Il y a en outre NaN si (b-a)/(cb-ca) < 0 et division par zéro si cb == ca.
**Correctif :**
```cpp
B = (b - a) / (cb - ca); // cohérent avec Curvature(t)=t/B ; assert(cb != ca)
```
(ou bien redéfinir Curvature/Vertex en abscisse curviligne réelle, à condition de le faire partout de la même façon).

#### 🟠 ✅ B-C05 — `Source/curvecubic2.cpp:535` — `CubicCurve2::Curvature(a, b)` — Maximum de courbure : extrema intérieurs pris sans valeur absolue

Aux bornes, le code prend `fabs(Curvature(a))` et `fabs(Curvature(b))`, mais aux racines intérieures il utilise la courbure signée `n(t)/d^{3/2}`. Un extremum de courbure négative (courbe tournant dans le sens horaire) est ignoré, et le maximum renvoyé est sous-estimé. Le polynôme dérivé 2n'd - 3nd' a été vérifié et est correct.
**Correctif :**
```cpp
c = Math::Max(c, fabs(n(t[i])) / Math::Sqrt32(d(t[i])));
```


### S — Champs scalaires et grilles

#### 🟠 B-S14 — `Source/algebraic.cpp:59` — `Algebraic::Algebraic(), Algebraic::K()` — Constante de Lipschitz toujours nulle

Les constructeurs mettent `k = 0.0` (l.34) ou `k = K(b)` (l.59), or `K(const Box&)` (l.76-79, cause réelle l.78) renvoie simplement le membre `k`, initialisé à 0 dans le header (l.14). `K()` vaut donc toujours 0 pour une surface non constante. Les algorithmes Lipschitz (`AnalyticScalarField::Roots`, `Volume`, `Center`, sphere tracing) divisent par k (`ff3 / k`, `fabs(s) / k`) ou testent `ff3 > dt * k` : avec k = 0, le test élimine tout intervalle (aucune racine) et les divisions donnent inf/NaN.
**Correctif :** calculer réellement une borne sur la boîte, par exemple \f$ k = \sum_t |c_t|\,(i+j+k)\,M^{i+j+k-1} \f$ avec \f$ M = \max(|a|,|b|) \f$ sur les coordonnées de la boîte, et l'implémenter dans `K(const Box&)` (sans lire `k`).

#### 🟠 B-S15 — `Source/analyticscalarfield.cpp:1077` — `AnalyticScalarField::Center(int)` — Centre de gravité non normalisé

`Center(const Box&, int)` accumule `Sphere(c*v, v)` (somme des centres pondérés et des volumes, `operator+` additionnant centres et rayons). `Center(int)` renvoie cette somme brute alors que sa doc annonce « the center is the center of gravity » : le centre retourné est \f$ \sum c_i v_i \f$, pas \f$ \sum c_i v_i / \sum v_i \f$.
**Correctif :**
```cpp
Sphere s = Center(box, depth);
double v = s.Radius();
return (v > 0.0) ? Sphere(s.Center() / v, v) : s;
```

#### 🟠 B-S16 — `Source/voxel.cpp:324` — `Voxel::OctantSigned(), Voxel::OctantCells()` — Configuration non permutée : distance signée fausse

`OctantSigned` symétrise `e`, mais permute les bits de `o` (inutilisé ensuite) au lieu de ceux de la configuration `c`. De plus `OctantCells` code la cellule propre sur le bit 0 (u = 0), alors que `OctantSigned` l'attend sur le bit 7 (`c & 0b10000000`). Exemple : voxel isolé, point dans l'octant 0 → `c = 1`, `o = 7` → branche « extérieur », `d = Norm(e) > 0` pour un point à l'intérieur d'un voxel plein. Dans le repère symétrisé, la cellule u se trouve toujours à l'octant `7 − u`. Le correctif porte sur Source/voxel.cpp:156.
**Correctif :**
```cpp
// OctantCells, l.156
configuration |= 1 << (7 - u);
// OctantSigned : garder la symétrie de e, supprimer les trois lignes « o = ... » (inutiles)
```

#### 🟠 ✅ B-S17 — `Include/scalaralpha.h:106` — `ScalarAlpha::operator+=(), ScalarAlpha::operator*=()` — Opérateurs composés sans effet

`operator+=` et `operator*=` renvoient un nouvel objet sans modifier `*this` : `a += b;` ne fait rien. Tout code utilisant ces opérateurs obtient silencieusement un résultat faux. (`operator==`/`!=` ignorent aussi `alpha` sans que ce soit documenté, et aucun opérateur n'est `const`.)
**Correctif :**
```cpp
inline ScalarAlpha& ScalarAlpha::operator+=(const ScalarAlpha& msa)
{
  value += msa.value; alpha += msa.alpha; return *this;
}
inline ScalarAlpha& ScalarAlpha::operator*=(const double& a)
{
  value *= a; alpha *= a; return *this;
}
```

#### 🟠 ✅ B-S18 — `Source/analyticscalarfield2.cpp:116` — `AnalyticScalarField2::Curvature()` — Courbure moyenne jamais calculée, gaussienne écrasée

L.116 : la parenthèse est mal placée (seul `H01²` est divisé par `(1+|g|²)²`). L.120 : la formule de la courbure *moyenne* est affectée à `gaussian`, qui est donc écrasée, et `mean` reste à 0.
**Correctif :**
```cpp
double w = 1.0 + g * g;
gaussian = (H(0,0) * H(1,1) - Math::Sqr(H(0,1))) / Math::Sqr(w);
mean = ((1.0 + g[1]*g[1]) * H(0,0) - 2.0 * H(0,1) * g[0] * g[1] + (1.0 + g[0]*g[0]) * H(1,1)) / (2.0 * Math::Sqrt32(w));
```

**Correction appliquée :** correctif ci-dessus, et suppression du retour anticipé quand le gradient est nul : pour une surface z = f(x,y) les formules ne divisent pas par |g|, et un sommet (g = 0) a une courbure non nulle. Vérifié sur une calotte sphérique de rayon 2, sommet compris : K = 0,25 et H = −0,5.

#### 🟠 B-S19 — `Source/scalarfield2-smooth.cpp:96` — `ScalarField2::SmoothPoint(), ScalarHexagonField2::SmoothPoint()` — Voisins verticaux indexés avec ±ny au lieu de ±nx

`VertexIndex(i,j) = i + nx*j` : le voisin (i, j±1) est `k ± nx`. Les branches `x == 0` et `x == nx−1` et le cas central utilisent `k ± ny`. Dès que nx ≠ ny (terrain rectangulaire), `Smooth(const Vector2&, double)` mélange des échantillons arbitraires, voire lit hors bornes (`k + ny` au coin). Les branches `y == 0` / `y == ny−1` sont correctes, comme `Smooth()`. Le même code est dupliqué dans les deux fichiers :
- Source/scalarfield2-smooth.cpp:96, 101, 105, 113, 118, 122, 139
- Source/hexagonscalarfield2.cpp:292, 297, 301, 309, 314, 318, 335
**Correctif :** remplacer tous les `ny` utilisés comme pas d'indice par `nx` dans les deux `SmoothPoint`.

#### 🟠 B-S20 — `Source/hexagonscalarfield2.cpp:265` — `ScalarHexagonField2::Smooth(const Vector2&, const double&)` — Distance calculée au point central fixe

`u = SquaredNorm(center − ArrayVertex(x, y))` utilise `(x, y)`, le sommet le plus proche du centre, constant dans la boucle, au lieu du sommet courant `(i, j)`. Le poids est le même pour tout le rectangle : le lissage n'est pas radial et ne décroît pas avec la distance.
**Correctif :** `double u = SquaredNorm(center - ArrayVertex(i, j));`

#### 🟠 B-S21 — `Source/hexagonscalarfield2.cpp:494` — `ScalarHexagonField2::Gradient(), ScalarHexagonField2::K()` — Pas de grille hexagonale ignoré : gradient et Lipschitz faux

Les différences sont divisées par `r` (ou `2r`) dans les deux directions, alors que `HexagonArray2::ArrayVertex` place les colonnes à `3/4·Width()` en x et les lignes à `Height()` en y, avec un décalage de `Height()/2` une colonne sur deux : (i±1, j) n'est pas aligné horizontalement avec (i, j). Le gradient est faux d'un facteur constant par axe (≈1,5 en x et √3 en y pour Width = 2r), et donc `K()` (constante de Lipschitz) et `GradientNorm()` aussi.
**Correctif :** diviser par les vrais espacements (`2 * 0.75 * Width()` en x, `2 * Height()` en y en centré), et pour x utiliser la moyenne des deux voisins de la colonne adjacente pour compenser le décalage vertical.

#### 🟠 B-S22 — `Source/hexagonscalarfield2.cpp:184` — `ScalarHexagonField2::Value(const Vector2&), ScalarHexagonField2::BiCubic()` — Indices de stockage faux : conversion axial → offset erronée (cause : B-N11)

`ScalarHexagonField2::Value(const Vector2&)` (Source/hexagonscalarfield2.cpp:184) et `BiCubic()` lisent `at(i, j)` avec les indices renvoyés par `HexagonArray2::TriangleInteger()`. Le stockage suit `ArrayVertex`, en coordonnées *décalées* (offset, colonne impaire décalée de H/2) : pour l'échantillon (i, j), la coordonnée axiale vaut `v = j − (i − (i&1))/2`. `TriangleInteger` tente bien la conversion axial → offset, mais avec le mauvais signe (`j = j − i/2` au lieu de `j = v + (i − (i&1))/2`, Source/hexagonarray2.cpp:38, et de même l.58 dans `TriangleInteger(const Vector2&)`). L'indice lu est décalé d'environ i lignes et les coordonnées locales (u,v) axiales ne correspondent pas aux voisins offset (i+1,j), (i,j+1) : l'interpolation est fausse sur la majeure partie du domaine. `BiCubic` applique de plus un schéma bicubique carré à une maille triangulaire. La cause, `HexagonArray2::TriangleInteger()`, est déjà recensée dans le domaine N ; cette entrée en décrit l'effet sur les champs scalaires hexagonaux.
**Correctif :** corriger le bug de `HexagonArray2::TriangleInteger()` recensé dans le domaine N (`j = j + (i - (i & 1)) / 2;` dans les deux surcharges, l.38 et l.59 (B-N11), et revoir le choix des voisins du triangle en coordonnées offset) ; aucune modification propre à `ScalarHexagonField2::Value()` n'est nécessaire au-delà, hormis remplacer le schéma bicubique de `BiCubic()` par une interpolation adaptée à la maille triangulaire.

#### 🟠 B-S23 — `Source/hexagonscalarfield2.cpp:45` — `ScalarHexagonField2::ScalarHexagonField2(const ScalarField2&)` — Pas d'échantillonnage égal à la taille du domaine

Le pas transmis au constructeur délégué est `Norm(s.GetBox().Diagonal()) / √2`, de l'ordre du côté de la boîte entière et non d'une cellule. Pour une boîte carrée de côté L, r = L donne `Width() = 2L`, d'où nx = ny = 1 : le champ hexagonal ne contient qu'un seul échantillon. Le résultat est entièrement faux à chaque usage de ce constructeur. L'intention probable est la diagonale d'une *cellule*.
**Correctif :**
```cpp
ScalarHexagonField2::ScalarHexagonField2(const ScalarField2& s) :ScalarHexagonField2(s, Norm(s.CellDiagonal()) / sqrt(2.0))
{
}
```

#### 🟠 B-S24 — `Source/scalarfield.cpp:772` — `ScalarField::Hessian(const Vector&)` — Dérivées croisées deux fois trop grandes

La différence croisée `f(+,+) − f(+,−) − f(−,+) + f(−,−)` vaut \f$ 4\varepsilon^2 f_{xy} \f$ ; elle est divisée par `2ε²` (l.772, 773, 777), d'où des termes hors diagonale multipliés par 2 (la version analytique, analyticscalarfield.cpp:162, divise bien par 4ε²). De plus, avec `Epsilon = 1e-6` sur un champ trilinéaire, les dérivées secondes pures sont nulles à l'intérieur d'une cellule ou énormes à la traversée d'une face (division par 1e-12) : il faudrait un pas de l'ordre de la cellule.
**Correctif :** diviser H[1], H[2], H[5] par `4.0 * Epsilon * Epsilon`, et utiliser un pas `h = celldiagonal` (ou dériver le gradient discret).

#### 🟠 B-S25 — `Source/scalarfield.cpp:1691` — `ScalarField::Read()` — Le champ n'est pas redimensionné après lecture de l'en-tête

`Array::InStream` met à jour nx, ny, nz, mais la boucle lit `field.size()` valeurs, c'est-à-dire la taille *précédente* : sur un `ScalarField` vide (cas normal de chargement), aucune valeur n'est lue ; sinon le champ est tronqué ou contient des valeurs résiduelles. Les erreurs d'ouverture sont signalées sur `std::cout` sans code de retour.
**Correctif :**
```cpp
Array::InStream(in);
field.resize(nx * ny * nz);
for (int i = 0; i < field.size(); i++) { unsigned char c; in >> c; field[i] = 2.0 * (c / 255.0) - 1.0; }
```

#### 🟠 ✅ B-S26 — `Source/scalarfield2.cpp:1583` — `ScalarField2::Hessian(int, int)` — Pas de grille faux pour ∂²/∂y² et pour le terme croisé

`∂²f/∂y²` est divisé par `d[0] = dx²` au lieu de `d[1] = dy²`, et le terme croisé par `4·dx²` au lieu de `4·dx·dy`. Le résultat est faux pour des cellules non carrées (cas courant mais pas systématique), même une fois le voisinage `Local()` corrigé (voir B-S04).
**Correctif :**
```cpp
const double dx = celldiagonal[0], dy = celldiagonal[1];
return Matrix2((a(2,1) + a(0,1) - 2.0 * a(1,1)) / (dx * dx),
               (a(1,2) + a(1,0) - 2.0 * a(1,1)) / (dy * dy),
               ((a(2,2) - a(0,2)) - (a(2,0) - a(0,0))) / (4.0 * dx * dy));
```

#### 🟠 B-S27 — `Source/scalarfield2.cpp:2733` — `ScalarField2::MedianFilter() const` — Fenêtre transposée : lecture hors bornes sur grille non carrée

La fenêtre est lue par `for (j = x−1..x+1) for (i = y−1..y+1) window[k++] = at(i, j)` : l'indice x de `at` reçoit une coordonnée y et inversement. Le filtre médian est appliqué au voisinage de (y, x) au lieu de (x, y) (faux même sur grille carrée), et sort de la grille dès que nx > ny.
**Correctif :**
```cpp
for (int dj = -1; dj <= 1; dj++)
  for (int di = -1; di <= 1; di++)
    window[k++] = at(x + di, y + dj);
```

#### 🟠 B-S28 — `Source/scalarfield2.cpp:404` — `ScalarField2::Rotate()` — Grille non carrée : boîte doublée et transposition au lieu de rotation

Dans le cas `nx != ny`, `d = Box2::Diagonal()` (diagonale complète) puis `a = c − d; b = c + d` : la nouvelle boîte est deux fois plus grande. De plus `rotated(y, x) = (*this)(x, y)` est une transposition (rotation + symétrie), incohérente avec le cas carré (l.374-395) qui fait une vraie rotation anti-horaire.
**Correctif :**
```cpp
a = c - 0.5 * d;
b = c + 0.5 * d;
ScalarField2 rotated(Box2(a, b), ny, nx);
for (int x = 0; x < nx; x++)
  for (int y = 0; y < ny; y++)
    rotated(ny - 1 - y, x) = at(x, y);   // rotation anti-horaire de 90°
```

#### 🟠 B-S29 — `Source/scalarfield2.cpp:3629` — `ScalarField2::Polygonize(const double&)` — Marching squares inachevé : seuls les cas 14 et 15 produisent des triangles

Les cas 1 à 13 du `switch` sont vides : toutes les cellules traversées par l'iso-contour (sauf le cas 14) ne produisent aucun triangle, d'où un maillage troué le long du contour. Le cas 14 est lui-même faux : les triangles `(evax[i+1], evbx[i+1], ecy[i])` et `(evax[i+1], evbx[i+1], evbx[i])` se recouvrent et ne couvrent pas le pentagone `eax, BR, TR, TL, ecy` (le triangle `(eax, TL, ecy)` reste non couvert). `evax`, `evbx`, `ecy` ne sont pas initialisés et peuvent être lus pour des sommets non créés.
**Correctif :** implémenter les 16 configurations en triangulant le polygone « intérieur » de chaque cellule (en éventail à partir du premier sommet : `eax, BR, TR, TL, ecy` → `(eax,BR,TR), (eax,TR,TL), (eax,TL,ecy)` pour le cas 14), et initialiser les tableaux à −1 (`std::vector<int>(nx, -1)`).


### V — Champs vectoriels et de couleur

#### 🟠 B-V03 — `Source/vectorfield2.cpp:804` — `VectorField2::Normalize(), VectorField2::CreateBlueRedImage()` — La norme résultante n'est pas ramenée dans [0,1]

Le code multiplie chaque vecteur par `Linear::Affine(n, a, b) = (n-a)/(b-a)` : la nouvelle norme vaut `n·(n-a)/(b-a)` (ex. pour n=b la norme reste b), alors que la doc annonce une plage [0,1]. Il faut diviser par n. Même erreur dans `CreateBlueRedImage()` (Source/vectorfield2.cpp:973), où `v` est censé être normalisé pour le mapping couleur.
**Correctif :**
```cpp
for (int i = 0; i < nx * ny; i++)
{
  const double ni = n.at(i);
  if (ni > 0.0)
    field[i] *= Linear::Affine(ni, a, b) / ni;
}
```


### N — Bruit et échantillonnage

#### 🟠 ✅ B-N07 — `Source/phasor.cpp:64` — `PhasorGundy::CellPhasor()` — Impulsions dans [-0.5,0.5] au lieu de [0,1] : discontinuités aux bords de cellules

**Résolu par suppression :** PhasorGundy et Source/phasor.cpp ont été supprimés, remplacés par `Phasor2` (`Include/phasornoise.h`, `Source/phasornoise2.cpp`), qui calcule la cellule avec `Vector2::Floor()` et tire les impulsions dans [0,1[ avec `RandomFast`.

`Vec2I::pcg2d()` renvoie des composantes `int` (signées) ; `Vector2(V2[0], V2[1]) / double(0xffffffffu)` donne donc des valeurs dans [-0.5, 0.5] et non [0,1]. Les impulsions d'une cellule sont décalées d'une demi-cellule vers le bas/la gauche ; l'enveloppe gaussienne (écart type cellsize/3, portée ~1 cellule) déborde alors sur des cellules qui ne sont plus visitées par le voisinage 3x3 de `phasor_noise()` → contributions manquantes et discontinuités de la phase aux bords des cellules.
**Correctif :**
```cpp
Vector2 impulse_position = Vector2(unsigned(V2[0]), unsigned(V2[1])) / double(0xffffffffu);
```

#### 🟠 ✅ B-N08 — `Source/phasor.cpp:16` — `PhasorGundy::phasor_noise()` — Indice de cellule par troncature au lieu de floor

**Résolu par suppression :** PhasorGundy et Source/phasor.cpp ont été supprimés, remplacés par `Phasor2` (`Include/phasornoise.h`, `Source/phasornoise2.cpp`), qui calcule la cellule avec `Vector2::Floor()` et tire les impulsions dans [0,1[ avec `RandomFast`.

`Vec2I uvi(uv)` utilise le constructeur `Vec2I(const Vector2&)` qui fait `int(v[0])` (troncature vers zéro). Pour uv négatif, la cellule est fausse : les points de ]-1,1[ tombent tous dans la cellule 0 avec q dans ]-1,1[, si bien que le voisinage 3x3 ne couvre plus toutes les impulsions à portée (cellule -2 ignorée pour q proche de -1) et la cellule 0 est « doublée ». Discontinuités sur les axes x=0 et y=0 et motif non stationnaire dans les quadrants négatifs.
**Correctif :**
```cpp
Vec2I uvi(int(floor(uv[0])), int(floor(uv[1])));
```

#### 🟠 B-N09 — `Source/noise-worm.cpp:119` — `TurbulenceWormNoise::Value()` — Les octaves deviennent plus grossières au lieu de plus fines

Le point est multiplié par `l` (`q * l`) avec `l = lambda, lambda², ...`. Avec la valeur documentée lambda ~0.5 (< 1, Source/noise-worm.cpp:95), la fréquence est divisée par 2 à chaque octave : on ajoute des composantes de plus en plus basses fréquences d'amplitude décroissante, à l'inverse de toutes les autres turbulences de la bibliothèque (`q / l`) et de la doc « Wavelength attenuation factor ».
**Correctif :**
```cpp
tu += a * WormNoise::Value(q / l + offsets[i % 8]);
```

#### 🟠 B-N10 — `Source/hexagonarray2.cpp:115` — `HexagonArray2::CellArea()` — Aire de cellule 4 fois trop grande

Avec `Width() = 2r` et `Height() = √3 r` (Include/hexagon.h:106-117), `3.0 * Width() * Height()` vaut 6√3 r² alors que l'aire d'un hexagone régulier de côté r est (3√3/2) r² (= `Hexagon2::Area()`, cf. doc « same as UnitCell().Area() »). Facteur 4 d'erreur sur toute densité ou intégrale calculée par cellule.
**Correctif :**
```cpp
return 0.75 * Width() * Height();   // = Hexagon2::Area()
```

#### 🟠 B-N11 — `Source/hexagonarray2.cpp:38` — `HexagonArray2::TriangleInteger()` — Conversion axiale vers décalée de signe inversé, et troncature au lieu de floor

Les centres de cellule sont en disposition « odd-q » (`CellCenter` : x = 1.5 r i, y = H (j + (i&1)/2)). Pour cette disposition, la ligne vaut `row = v + (q - (q&1))/2` ; le code fait `j = j - i / 2`. Exemple : centre de la cellule (2,0), soit p = c + (3r, 0) : u = 2, v = -1, le code renvoie (2, -2) au lieu de (2, 0). L'erreur croît avec i (2 lignes pour i = 2, 4 pour i = 4…). De plus `int(u)`/`int(v)` (Source/hexagonarray2.cpp:29-30) tronquent vers zéro : pour des points à gauche/en dessous de l'origine les indices sont décalés d'une unité. Même code dans la surcharge `TriangleInteger(const Vector2&)` renvoyant un QPoint (Source/hexagonarray2.cpp:59, troncature l.55-56), donc `VertexIntegerArea()` renvoie de mauvaises plages.
**Correctif :**
```cpp
i = int(floor(u));
j = int(floor(v));
u -= i; v -= j;
j = j + (i - (i & 1)) / 2;
```

#### 🟠 B-N12 — `Source/sampling.cpp:122` — `SphereTile::Generate()` — Grille d'accélération trop grossière : propriété de Poisson non garantie

La grille suppose au plus un échantillon par cellule (`grid[...] = p.size()` écrase l'occupant précédent). Avec une taille de cellule a/mod < √3 r, la diagonale d'une cellule peut atteindre 3r > 2r : deux échantillons distants de plus de 2r peuvent tomber dans la même cellule, le premier est alors effacé de la grille et les tirages suivants ne sont plus testés contre lui → paires de sphères qui se chevauchent. Il faut une cellule de côté ≤ 2r/√3. Même défaut dans `SphereTile::Relaxation()` (Source/sampling.cpp:38, côté √2 r, diagonale jusqu'à √6 r ≈ 2.45 r) qui ignore alors des voisins.
**Correctif :**
```cpp
double s = 2.0 * r / Math::Sqrt3;   // Generate() et Relaxation()
```

#### 🟠 B-N13 — `Source/sampling2.cpp:271` — `DiscTile::Intersect(const Vector2&, bool)` — Mauvais décalage torique près des bords haut et bas

Dans la branche où le point n'est proche ni du bord gauche ni du bord droit, il est proche du bord bas ou haut ; il faut donc tester les copies décalées verticalement `q + (0, s)` (bas) et `q + (0, -s)` (haut). Le code teste `q + (s, 0)` et `q + (-s, 0)` (décalages horizontaux) : les collisions à travers les bords haut/bas de la tuile ne sont jamais détectées et la tuile n'est pas périodique en y.
**Correctif :**
```cpp
if (fabs(q[1]) < 2.0 * r)
  c |= Check(q + Vector2(0.0, s));   // Bottom
else
  c |= Check(q + Vector2(0.0, -s));  // Top
```


### T — Maillages et graphes géométriques

#### 🟠 B-T03 — `Source/mesh.cpp:1596` — `Mesh::LoadTim()` — Indexation relative (indices négatifs) fausse

La doc annonce « supports relative indexing », mais `vertexId %= vertices.size()` laisse un indice négatif négatif en C++ (`-1 % n == -1`) : l'indice stocké dans `varray` est négatif, d'où un accès hors bornes dans toutes les méthodes. L'indice relatif `-k` désigne `vertices.size() - k`.
**Correctif :**
```cpp
if (vertexId > 0) vertexId--;
else vertexId += vertices.size();
```

#### 🟠 B-T04 — `Source/mesh.cpp:1578` — `Mesh::LoadTim()` — Espace en fin de ligne f : chargement abandonné ; narray désynchronisé sans normales

Dans la boucle de face, un espace final (ex. `"f 1//1 2//2 3//3 \n"`) relance l'itération, `readStart == cursor`, `parseLong` échoue sur un champ vide et la fonction retourne en laissant un maillage partiel, sans code d'erreur. Or `Mesh::SaveObj()` (mesh.cpp:2185) écrit précisément cet espace final : un fichier produit par `SaveObj` n'est rechargé que jusqu'à la première face (aller-retour export/import cassé). Par ailleurs une face sans normale (`f 1 2 3` ou `f 1/1 2/2 3/3`) alimente `varray` mais pas `narray`, d'où des tableaux de tailles différentes et des accès hors bornes dans `NormalIndex()`, `SaveObj()`, `Append()`...
**Correctif :** sauter les blancs puis `if (cursor >= end || *cursor == '\r' || *cursor == '\n') break;` avant de lire un champ ; si une face n'a pas de normale, calculer une normale de face et l'ajouter à `normals`/`narray` (ou appeler `SmoothNormals()` en fin de chargement si `narray.size() != varray.size()`).

#### 🟠 B-T05 — `Source/mesh.cpp:1410` — `Mesh::Load()` — Parseur OBJ regex : sommets ignorés (indices décalés), faces sans normale, quads perdus

- `rexv`/`rexn` exigent un point décimal (`\d*\.\d+`) et n'acceptent pas la notation exponentielle : `v 0 0 1` ou `v 1.5e-05 0.2 0.3` ne sont pas reconnus, la ligne est ignorée silencieusement et tous les indices de faces suivants pointent sur le mauvais sommet.
- `rext` impose le format `v/t/n` (ou `v//n`) : `f 1 2 3` et `f 1/1 2/2 3/3` ne correspondent pas, faces ignorées ; de plus la ligne 1437 rejette toute face dont les normales manquent.
- Seuls les 3 premiers sommets d'une face sont lus : un quad/n-gone perd sa seconde moitié.
- Indices relatifs (négatifs) non supportés (rejetés).
La même regex `rexv` est utilisée dans Source/vectorset.cpp:456 (`VectorSet::Load()`) : les sommets entiers ou en notation exponentielle y sont aussi perdus silencieusement.
**Correctif :** remplacer par un tokeniseur (`line.split(' ', Qt::SkipEmptyParts)`, `toDouble()` gère l'exposant), trianguler en éventail les n-gones, accepter `v`, `v/t`, `v//n`, `v/t/n`, et calculer des normales de face quand elles manquent (ou réutiliser `LoadTim` une fois corrigé).

#### 🟠 B-T06 — `Source/mesh.cpp:645` — `Mesh::EdgeCollapse(int)` — Normales indexées comme les sommets

`normals[ic] = Normalized(normals[ia] + normals[ib])`, `normals[id] = normals.last()` et `narray[j] = ic / id` supposent que `normals` et `narray` sont indexés exactement comme `vertices`/`varray`. Pour un maillage à normales par face (constructeur `Mesh(vertices, indices)` avec `flat=true` par défaut, primitives `Box`, `Dodecahedron`...) `normals.size() != vertices.size()` : lecture/écriture hors bornes et `narray` corrompu (indices de normales remplacés par des indices de sommets).
**Correctif :** soit appeler `SmoothNormals()` au début de `EdgeCollapse(const double&)` (garantit `narray == varray`), soit ne mettre à jour que `varray` puis recalculer les normales à la fin.

#### 🟠 B-T07 — `Source/mesh.cpp:2081` — `Mesh::Symmetry()` — Inversion de l'orientation sans permuter narray

Pour retourner l'orientation, `varray[i]` et `varray[i+1]` sont échangés, mais pas `narray[i]` et `narray[i+1]`. Pour tout triangle lisse (normales par sommet : sphère, cylindre, tore...) les normales sont désormais attachées au mauvais coin.
**Correctif :**
```cpp
for (int i = 0; i < varray.size(); i += 3)
{
  std::swap(varray[i], varray[i + 1]);
  std::swap(narray[i], narray[i + 1]);
}
```

#### 🟠 B-T08 — `Source/mesh.cpp:2862` — `Mesh::Mesh(const Icosidodecahedron&)` — Les 20 faces triangulaires sont commentées : maillage troué

Seuls les 12 pentagones sont générés ; la boucle des 20 triangles (normales 12..31 pourtant allouées et calculées) est en commentaire alors que `Icosidodecahedron::VertexTriangleIndex()` existe (icosidodecahedron.h:69). Le maillage obtenu est ouvert (20 trous) : `Inside()`, `Signed()` et le rendu sont faux.
**Correctif :** décommenter la boucle `AddTriangle(VertexTriangleIndex(i,0..2), 12 + i)` (vérifier l'orientation des triangles de la table).

#### 🟠 B-T09 — `Include/geometricgraph.h:17` — `WeightEdge::WeightEdge(int, int, int)` — Poids tronqué en entier

Le constructeur prend `int w` alors que `weight` est un `double`. Tous les appels de geometricgraph2.cpp passent des longueurs euclidiennes `double` (`AddEdge(int,int,double)` ligne 106, `AddEdge(int,int)` ligne 117) : le poids est tronqué. Pour des points dans [0,1]² toutes les arêtes ont un poids 0 : `Kruskal()` (ligne 146, tri par poids) renvoie un arbre couvrant arbitraire, `Weight()` renvoie une somme fausse, `Reverse()` perd les poids.
**Correctif :**
```cpp
WeightEdge(int a, int b, double w) :a(a), b(b), weight(w) {}
```


### U — Utilitaires, couleur, GPU

#### 🟠 B-U03 — `Include/color.h:142` — `Color::Color(unsigned long), Color::Cast()` — Encodage compact incohérent (RGBA vs ARGB), sans clamp

Le constructeur décode R=bits 24-31, G=16-23, B=8-15, A=0-7 (RGBA), alors que `Cast()` (Source/color.cpp:104-111) encode A<<24, R<<16, G<<8, B (ARGB). `Color(c.Cast())` ne restitue donc pas la couleur (canaux décalés). De plus `Cast()` ne clampe pas : une composante >1 (ex. 1.2 → 306) déborde sur le canal voisin, une composante négative donne une conversion double→unsigned indéfinie.
**Correctif :**
```cpp
unsigned long Color::Cast() const
{
  auto q = [](double x) { return (unsigned long)(255.0 * Math::Clamp(x) + 0.5); };
  return (q(c[0]) << 24) | (q(c[1]) << 16) | (q(c[2]) << 8) | q(c[3]); // même ordre RGBA que le constructeur
}
```

#### 🟠 B-U04 — `Source/color-lab.cpp:52` — `Lab::ToXyz()` — Échelle XYZ ×100 incohérente avec Lab(const Xyz&)

`Lab(const Xyz&)` attend un XYZ normalisé Y=1 (il calcule `100*xyz.x/95.047`, soit x/Xn avec Xn=0.95047), conformément à la doc de `Xyz`. Mais `ToXyz()` multiplie par 95.047 / 100.0 / 108.883, donc renvoie Y∈[0,100]. L'aller-retour `Lab(Xyz(c)).ToXyz().ToColor()` donne une couleur 100× trop lumineuse (clampée au blanc).
**Correctif :**
```cpp
  x = ((x3 > 0.008856) ? x3 : ((x - 16.0 / 116.0) / 7.787)) * 0.95047;
  y = ((y3 > 0.008856) ? y3 : ((y - 16.0 / 116.0) / 7.787)) * 1.0;
  z = ((z3 > 0.008856) ? z3 : ((z - 16.0 / 116.0) / 7.787)) * 1.08883;
```

#### 🟠 B-U05 — `Source/chart.cpp:194` — `DrawChart::UpdateRanges()` — Plages d'axes fausses : branche initiale morte et min au lieu de max

`UpdateRanges` est appelé après `series_list.push_back(series)` (Source/chart.cpp:68-70, 121-123) : la branche `series_list.size() == 0` n'est jamais prise et la première série est fusionnée avec la plage par défaut (0,0). De plus les bornes supérieures sont combinées par `std::min` (Source/chart.cpp:202, 204). Pour des données positives sans `SetRangeX/Y`, l'axe vaut [0,0] (ex. `Histogram::ExportPNG`), et avec plusieurs séries la borne haute est tronquée.
**Correctif :**
```cpp
void DrawChart::UpdateRanges(Vector2 nx, Vector2 ny) {
  if (series_list.size() <= 1) { range_X = nx; range_Y = ny; return; } // appelé après push_back
  range_X[0] = std::min(range_X[0], nx[0]);
  range_X[1] = std::max(range_X[1], nx[1]);
  range_Y[0] = std::min(range_Y[0], ny[0]);
  range_Y[1] = std::max(range_Y[1], ny[1]);
}
```

#### 🟠 B-U06 — `Include/chart.h:9` — `DrawChart (pas de destructeur)` — Fuite de QChart/QChartView et fenêtre ouverte à chaque export

`chart` (alloué Source/chart.cpp:5) et `chartView` (Source/chart.cpp:220) ne sont jamais libérés. `ExportPDF/ExportPNG` passent par `SetUpChartView()`, qui crée et affiche (`show()`) une fenêtre : chaque appel de `Histogram::ExportPDF/ExportPNG` ouvre une fenêtre et fuit un graphe complet. Si aucune série n'a été ajoutée, `chart` fuit aussi.
**Correctif :** ajouter `~DrawChart()` qui supprime `chartView` (ou `chart` s'il n'est pas possédé par une vue) ; pour les exports, rendre via un `QChartView` local non affiché (ou `chart->scene()`), avec `Qt::WA_DeleteOnClose` pour `Display()`.

#### 🟠 B-U07 — `Source/cpu.cpp:302` — `System::Memory(uint64_t)` — Modulo 2014 au lieu de 1024

`kilo = (m / 1024) % 2014` : le nombre de Kio affiché est faux dès que m ≥ 1 Mio (ex. 2 Mio + 1 Kio → 1025 au lieu de 1) et peut dépasser 1023.
**Correctif :**
```cpp
const uint64_t kilo = (m / 1024) % 1024;
```


---

## Bugs Moyenne criticité 🟡

### M — Mathématiques

- **B-M14** — `Source/mathematics.cpp:151` — `Math::Binomial(int,int)` — Binomial(n>=15, 0) renvoie n au lieu de 1. Pour n >= 15 et r = 0, `b = n` et la boucle `for (i = 1; i < r; ...)` ne s'exécute pas : renvoie n au lieu de 1 (de plus `long` est 32 bits sous Windows, troncature des grands coefficients). **Correctif :** ajouter `else if (r == 0) return 1;` et retourner `int64_t`/`long long`.
- **B-M15** — `Include/mathematics.h:1098` — `Math::CompactGaussian()` — Valeurs énormes pour x < -3σ. `y = x/(3s)` et seul `y > 1` est testé : pour x < -3s, `(1 - y*y)^8` est positif et croissant (x=-6s -> 6561) au lieu de 0 ; support compact respecté seulement pour x >= 0. **Correctif :** `double y = fabs(x) / (3.0 * s);`
- **B-M16** — `Source/ia.cpp:293` — `operator/(const double&, const Ia&)` — Pas de traitement d'un dénominateur contenant 0. Si x contient 0, renvoie `Ia(a/x.b, a/x.a)`, intervalle inversé et faux (±inf/NaN si une borne vaut 0), alors que `operator/(Ia,Ia)` renvoie Ia::Infinity. **Correctif :** `if (x.a <= 0.0 && x.b >= 0.0) return Ia::Infinity;` en tête.
- **B-M17** — `Source/ia.cpp:456` — `pow(const Ia&, const double&)` — Intervalle faux pour les puissances paires (limitation seulement documentée). Chaque borne est élevée en `sign(v)|v|^n` : pour n pair et une borne négative, pow(Ia(-2,1),2) = [-4,1] au lieu de [0,4], pas un vrai encadrement malgré l'avertissement de la doc. **Correctif :** pour n entier pair, réutiliser la logique de `sqr` (`abs(x)` puis `Ia(pow(lo,n), pow(hi,n))`) ; pour n non entier, exiger x.a >= 0.
- **B-M18** — `Source/aa.cpp:2795` — `sin(const Aa&)` — Erreur δ estimée sur 8 échantillons : borne non garantie. δ est le max des résidus aux 8 seuls points d'échantillonnage ; entre deux échantillons l'erreur peut être plus grande, l'encadrement affine n'est pas garanti. **Correctif :** majorer l'erreur entre échantillons (ajouter `pas*pas/8` plus `pas*(1+|α|)/2`) ou utiliser une approximation Chebyshev/min-range avec majorant analytique.
- **B-M19** — `Source/aa.cpp:1798` — `sqrt(const Aa&), inv(const Aa&)` — Retour silencieux de Aa() (=0) hors domaine. `sqrt` renvoie `Aa()` dès que a < 1e-20, y compris pour un intervalle touchant 0 (sqrt de [0,4] -> 0), rendant mort le bloc dNeg (l.1806-1811) ; même motif pour `inv` en Source/aa.cpp:2006 (renvoie 0 si l'intervalle contient 0). **Correctif :** sqrt : ne rejeter que b < 0 et traiter a < 0 par la branche dNeg (clamp a=0) ; inv : renvoyer un Aa centré 0 de très grand rayon ou signaler l'erreur.
- **B-M20** — `Source/aa.cpp:61` — `Aa::Aa(double, const double*, const unsigned*, unsigned)` — Accès indexes[-1] si T == 0. `indexes[length - 1]` avec length = 0 (non signé) lit `indexes[UINT_MAX]` ; de plus `last` suppose t2 trié (requis aussi par `std::set_union`) sans le documenter. **Correctif :** `if (length > 0 && indexes[length - 1] > last) ...` et documenter que t2 doit être strictement croissant.
- **B-M21** — `Source/evector.cpp:383` — `Vector::SetMinMax(), Vector2::SetMinMax()` — else if : la borne max n'est pas mise à jour quand la min l'est. `if (x[i] < a[i]) ... else if (x[i] > b[i]) ...` : avec une initialisation vide (a=+∞, b=-∞) le premier point ne met à jour que `a` ; faux dès que a > b au départ (même motif Source/evector2.cpp:90 ; Math::SetMinMax a déjà été corrigé). **Correctif :** remplacer `else if` par `if` (ou `a[i] = Math::Min(a[i], x[i]); b[i] = Math::Max(b[i], x[i]);`).
- **B-M22** — `Source/evector.cpp:287` — `Vector::FromString(), Vector2::FromString()` — Aucune vérification du nombre de composantes. `cmps[0..2]` indexé sans tester `cmps.size()` : une chaîne mal formée (ex. "Vector(1,2)") provoque un accès hors limites de QStringList (même motif Source/evector2.cpp:167, `cmps[1]`). **Correctif :** `if (cmps.size() < 3) return Vector::Null;` (resp. `< 2` pour Vector2).
- **B-M23** — `Source/quaternion.cpp:289` — `Quaternion::Exp(), Quaternion::Log()` — Test sur sin(angle) au lieu de l'angle : résultat faux près de kπ. `Exp()` utilise `sin(a)/a ≈ 1` quand `|sin(angle)| < 1e-3`, donc aussi pour angle ≈ π (partie vectorielle de norme ≈ π au lieu de ≈ 0) ; même motif dans `Log()` (Source/quaternion.cpp:311 : pour w ≈ -1, renvoie (0,x,y,z) de norme ≈ 0 au lieu de π·v). **Correctif :** Exp : tester `angle < Epsilon` ; Log : `sn = |v|` puis `c = (sn < Epsilon && w > 0) ? 1.0 : atan2(sn, w) / sn`.
- **B-M24** — `Source/matrix2.cpp:306` — `Matrix2::EigenSolveSymmetric()` — Matrice diagonale : vecteurs propres associés aux mauvaises valeurs propres. `Quadric::Solve` renvoie des racines triées ; pour r[1] == 0 le code associe toujours X à a[0] et Y à a[1], donc couples permutés si r[0] > r[3] (et le commentaire « thus : identity » est faux). **Correctif :** `a[0] = r[0]; a[1] = r[3]; e[0] = Vector2::X; e[1] = Vector2::Y;`
- **B-M25** — `Source/matrix2.cpp:276` — `Matrix2::Eigen(), Matrix2::EigenSolveSymmetric()` — Code de retour de Quadric::Solve ignoré. Discriminant négatif (valeurs propres complexes, ex. rotation) : `Solve` renvoie 0 sans écrire les racines, `Eigen()` renvoie (0,0) et `EigenSolveSymmetric()` (Source/matrix2.cpp:292) laisse `a[]` non initialisé puis calcule des vecteurs avec. **Correctif :** tester `if (q.Solve(a[0], a[1]) == 0) { /* pas de valeurs propres réelles */ }` et documenter le cas.
- **B-M26** — `Source/matrix-algebra.cpp:362` — `Matrix::QDU()` — Gram-Schmidt appliqué aux lignes (indexation ligne-major) : M ≠ Q·D·U. Le code orthonormalise `(r[0], r[3], r[6])` (une ligne de la matrice colonne-major) et écrit Q par lignes : il décompose Mᵀ (vérifié : pour le cisaillement M(0,1)=0.5, Q·D·U ≠ M). **Correctif :** Gram-Schmidt sur `C(0)`, `C(1)`, `C(2)`, `a = Matrix(q0, q1, q2)` et `R(i,j) = q_i · C(j)`.
- **B-M27** — `Source/matrix-mn.cpp:110` — `MatrixMN::Inverse()` — Matrice singulière : renvoie silencieusement l'identité. Sans pivot non nul, renvoie `MatrixMN::Identity(m)` (et `*this` pour une matrice non carrée, l.99, avec un simple message cerr), résultat plausible qui fausse les calculs en aval (ex. Mahalanobis). **Correctif :** renvoyer une matrice vide/NaN ou un booléen de succès (`bool Inverse(MatrixMN&) const`) et documenter le cas singulier.
- **B-M28** — `Source/matrix-mn.cpp:37` — `MatrixMN::MatrixMN(int, int, const std::vector<double>&)` — Taille des données non vérifiée. `data.size() == m*n` n'est pas vérifié : un vecteur trop court provoque des accès hors limites dans `operator()`, `operator+`, etc. **Correctif :** `assert(int(data.size()) == m * n);` (ou redimensionner / lever une exception).

- **B-M41** — `Source/aa.cpp:1613, 1634, 1656, 1737` — `Aa_pow(const Aa&, const double&), Aa_pow(const double&, const Aa&)` — Résultat NaN pour une affine de largeur nulle. Quand l'intervalle de l'argument est réduit à un point (affine constante), l'approximation de Tchebychev divise 0 par 0 (`beta = (fmax - fmin) / (max_b - min_b)`, idem avec `max_e - min_e`) : centre et rayon valent NaN (mesuré : `Aa_pow(2.0, Aa(0.5))` et `Aa_pow(Aa(1.5), 2.5)` donnent NaN). **Correctif :** si `max - min` est nul (ou inférieur à un epsilon), renvoyer `Aa(pow(centre, exp))` (resp. `Aa(pow(base, centre))`). Découvert lors de la vérification de B-M02.

### P — Polynômes, aléatoire, graphes

- **B-P02** — `Source/polynomial-solver.cpp:25` — `Polynomial::Sturm(), Polynomial::Solve(double*)` — Division par zéro si le coefficient dominant est nul (degré non vérifié). `f = fabs(s[0][n] * n)` puis division par `f` : Solve() ne protège que n==5 && c[5]==0, donc pour n ≥ 6 avec c[n]==0 (degré non vérifié par les constructeurs ni par operator+=/-=) ou n == 0, la suite de Sturm devient inf/NaN et les racines sont fausses. **Correctif :** dans `Polynomial::Solve(double*)`, travailler sur une copie `Polynomial p = *this; p.Check();`, dispatcher sur `p.n` et supprimer le cas spécial n==5.
- **B-P03** — `Source/cubic.cpp:128` — `Cubic::Solve(double*) const, Cubic::SolveNormalized(double*) const` — NaN pour une racine triple (Q = R = 0) et pour |R/√Q³| > 1 par arrondi. `d = R / sqrt(Q3)` donne 0/0 = NaN pour une racine triple (ex. (x-1)³) et |R/√Q³| peut dépasser 1 par arrondi, d'où trois racines NaN via `acos` (même motif Source/cubic.cpp:300 dans SolveNormalized). **Correctif :** `if (Q3 <= 0.0) { y[0] = -an; return 1; }` puis `d = Math::Clamp(R / sqrt(Q3), -1.0, 1.0);`
- **B-P04** — `Source/cubic.cpp:133` — `Cubic::Solve(double*) const, Cubic::SolveNormalized(double*) const` — Les trois racines ne sont pas triées contrairement à la documentation. Avec θ ∈ [0, π/3] et sQ = -2√Q < 0, l'ordre renvoyé est (min, max, milieu) alors que la doc affirme « stores the sorted roots » (lignes 133-135, même motif Source/cubic.cpp:305-307). **Correctif :** échanger les deux dernières affectations : `y[1] = sQ*cos(theta + 4π/3) - an; y[2] = sQ*cos(theta + 2π/3) - an;`
- **B-P05** — `Source/quartic.cpp:270` — `Quartic::Solve(double*) const` — Signe du décalage inversé dans les branches racine double (p == 0). Le retour à x se fait par x = y + q2 (comme dans les branches p > 0), mais les branches p == 0 écrivent `-0.5*d1 - q2` (l.270) et `0.5*d1 - q2` (Source/quartic.cpp:282) : racine double décalée de c1/2 dès que c1 ≠ 0 et que le discriminant vaut exactement 0.0 (cas rare en flottant). **Correctif :** `results[i++] = -0.5 * d1 + q2;` (l.270) et `results[i++] = 0.5 * d1 + q2;` (l.282).

### G — Géométrie 2D

- **B-G60** — `Source/axis2.cpp:101` — `Axis2::Symmetric(const Box2&)` — Le 4e sommet de la boîte est ignoré. La boucle `for (int i = 0; i < 3; i++)` repart de `Vertex(0)` (déjà utilisé pour initialiser `s`) et `Vertex(3)` n'est jamais réfléchi : boîte trop petite pour un axe ni aligné ni diagonal. **Correctif :** `for (int i = 1; i < 4; i++)`.
- ✅ **B-G61** — `Source/boxset2.cpp:142` — `BoxSet2::RandomInside()` — Boucle infinie si l'ensemble est vide. Si `boxes` est vide, `GetBox()` renvoie `Box2::Null`, `Inside()` est toujours faux et `while(true)` ne termine jamais (entrée dégénérée) ; `GetBox().Cube()` agrandit en outre inutilement le domaine de tirage. **Correctif appliqué :** `BoxSet2::RandomInside` tire une boîte au prorata de son aire, un point uniforme dedans, et n'accepte le point que si cette boîte est la première de la liste à le contenir (distribution uniforme sur l'union, acceptation = aire de l'union / somme des aires ≥ 1/n) ; ensemble vide ou sans aire : origine, ou centre de la première boîte ; plus de `Cube()`. Doc Doxygen détaillée (méthode, complexité, cas dégénérés). Mesures sur 4 scénarios (V2 vs ancien tirage dans le carré englobant) : jusqu'à 14× plus rapide pour les ensembles épars (11,6 µs → 0,86 µs pour 1 000 boîtes éparses ; 3,3 µs → 0,25 µs pour une diagonale de boîtes fines), 11× pour une bande allongée (988 → 88 ns) ; légèrement plus lent (45 → 50 ns) sur 5 grosses boîtes recouvrantes. Vérifié sur la fonction extraite dans un programme séparé : ensemble vide et boîtes sans aire terminent, 4·10⁶ tirages sur 4 boîtes recouvrantes : 0 point hors de l'union, χ²/ddl = 0,92. Vérifié par compilation (`g++ -fsyntax-only -Wall`).
- **B-G62** — `Source/box2.cpp:291` — `Box2::Intersect(const Ray2&, double&, double&)` — Rayon traité comme une droite. Aucune contrainte t ≥ 0 : une boîte entièrement derrière l'origine du rayon est déclarée intersectée avec des profondeurs négatives (convention « droite » peut-être voulue) ; `Box2::Intersect(const Ray2&)` hérite du problème. **Correctif :** avant `return true;`, ajouter `if (tmax < 0.0) return false;` (et éventuellement `tmin = Math::Max(tmin, 0.0);`).
- **B-G63** — `Source/capsule2.cpp:352` — `Capsule2::Draw()` — Sens de balayage des arcs dépendant de l'orientation absolue. Le balayage `v - u` (angles `QLineF::angle()` dans [0,360)) vaut +180 si u<180 et −180 sinon : l'arc en a passe par θ+90 ou θ−90 selon l'orientation, et la moitié des orientations sont mal dessinées. **Correctif :** balayage constant, ex. `path.arcTo(ra, u, 180.0); ... path.arcTo(rb, v, 180.0);` (signe à choisir pour que l'arc en a passe par `a - r*axis`, à confirmer visuellement).
- **B-G64** — `Source/convex2.cpp:375` — `Convex2::Hull()` — Tri non lexicographique pour la chaîne monotone d'Andrew. Le comparateur ne compare que x : avec des points de même abscisse l'enveloppe peut être fausse (ex. tri (0,1),(0,0),(0,2),(1,1) → enveloppe (0,1),(1,1),(0,2), (0,0) manquant), et pour n = 1 `k` retombe à 0 (enveloppe vide). **Correctif :** comparer `(a[0] < b[0]) || (a[0] == b[0] && a[1] < b[1])` et ajouter `if (n < 3) return Convex2(p);` en tête.
- **B-G65** — `Source/disctwins2.cpp:77` — `TwinDisc2::Signed(), TwinDisc2::Draw()` — NaN pour des disques disjoints. `h = sqrt(r*r - l*l/4)` est NaN dès que l > 2r, le test `NaN > 0` est faux et la fonction renvoie `-Norm(q-(0,NaN))` = NaN (même calcul dans `TwinDisc2::Draw()` Source/disctwins2.cpp:148). **Correctif :** `if (l >= 2.0 * r) return d;` avant le calcul de `h` (`min(sa,sb)` est alors exact).
- **B-G66** — `Source/disctwins2.cpp:101` — `TwinDisc2::Intersect()` — Rayon de l'autre paire ignoré. Le seuil `s = 4.0 * r * r` ignore `disc.r` alors que la classe n'impose l'égalité des rayons qu'au sein d'une paire : faux dès que les rayons des deux paires diffèrent. **Correctif :** `double s = Math::Sqr(r + disc.r);`.
- ✅ **B-G67** — `Source/ellipse2.cpp:293` — `Ellipse2::R(), Ellipse2::Signed()` — Division par zéro pour une ellipse circulaire (a == b). `l = bb*bb - aa*aa` est nul quand a == b (cas de l'ellipse par défaut a = b = 1, le constructeur ne fait que min/max) : `pax` infini et résultat NaN (même motif Source/ellipse2.cpp:356). **Correctif :** `if (fabs(a - b) < 1e-12 * a) return Math::Sqr(Norm(p - c) - a);` (resp. `Norm(p - c) - a` pour `Signed`).
- **B-G68** — `Source/ellipse2.cpp:534` — `Ellipse2::Lerp()` — Axe interpolé non normalisé. L'axe `(1-t)*ea.u + t*eb.u` n'est pas renormalisé (le constructeur (c,a,b,u) ne normalise pas u, l.22) alors que tout le code suppose `u` unitaire : norme < 1, voire nulle pour des axes opposés. **Correctif :** passer `Normalized((1.0 - t) * ea.u + t * eb.u)` et traiter le cas de norme nulle.
- **B-G69** — `Source/framescaled2.cpp:80` — `FrameScaled2::Inverse(), FrameScaled2::Composed(), FrameScaled2::Compose()` — Inverse et composition faux pour une échelle non uniforme avec rotation. L'inverse de p → R·S·p + t est S⁻¹·Rᵀ·(p − t) mais le code renvoie Rᵀ·S⁻¹·p − Rᵀ·t (`InverseTransform` l.156 est, lui, correct), et `Composed()` (Source/framescaled2.cpp:175) / `Compose()` (Source/framescaled2.cpp:184) supposent que S_f commute avec R ; limitation non documentée. **Correctif :** documenter la restriction (échelle uniforme ou rotations alignées) avec un `assert`, ou représenter la partie linéaire par une `Matrix2` générale.
- **B-G70** — `Source/framescaled2.cpp:166` — `FrameScaled2::InverseTransformDirection()` — Inverse de la transformation des normales incorrect. `TransformDirection(n)` = R·S⁻¹·n, dont l'inverse est S·Rᵀ·n ; le code calcule Rᵀ·S⁻¹·n (direction fausse pour une échelle non uniforme, facteur s² sinon). **Correctif :** `return Frame2::InverseTransformDirection(n).Scaled(s);`.
- **B-G71** — `Source/hexagon2.cpp:210` — `Hexagon2::Intersect(const Hexagon2&)` — Test de séparation avec le rayon au lieu de l'apothème. Les deux hexagones partagent les normales et la demi-largeur projetée est l'apothème α·r : le seuil `hexagon.r + r` déclare en intersection des hexagones séparés d'une distance comprise entre α(r+r') et r+r' (faux positifs). **Correctif :** `if (fabs(a * normal[i]) > Alpha * (hexagon.r + r))`.
- **B-G72** — `Source/octogon2.cpp:295` — `Octogon2::Normal()` — Symétrie en y mal appliquée. Le code fait `n[0] = -n[1];` au lieu de `n[1] = -n[1];` : la composante x est écrasée et y garde le mauvais signe (actuellement masqué par B-G08, `Normal()` renvoyant toujours `Vector2::Null`). **Correctif :** `if (sy == true) { n[1] = -n[1]; }`.
- ✅ **B-G73** — `Source/lens2.cpp:131` — `Lens2::Draw()` — Segment parasite depuis l'origine. `arcTo` est appelé sur un `QPainterPath` vide (position courante (0,0)) : Qt trace un segment de l'origine jusqu'au début de l'arc (et `closeSubpath` revient à l'origine). **Correctif :** appeler `path.arcMoveTo(Disc2(a, r).GetBox().GetQtRect(), Math::RadianToDegree(theta - alpha));` avant le premier `arcTo`.
- **B-G74** — `Source/disctwins2.cpp:117` — `TwinDisc2::Area()` — NaN pour des disques disjoints. `Lens2(a, b, r).Area()` calcule `acos(length / (2r))` et `sqrt(4r² − length²)`, NaN pour l > 2r alors que l'aire de l'union vaut 2πr². **Correctif :** `if (SquaredNorm(b - a) >= 4.0 * r * r) return Math::TwoPi * r * r;` avant l'appel.
- **B-G75** — `Source/lines2.cpp:305` — `SegmentSet2::SimplifyPolyline()` — Dernier segment perdu. La boucle externe `while (seg_id < nb_seg - 1)` sort sans émettre le dernier segment quand il n'est pas fusionné (seg_id atteint nb_seg − 1) ; un ensemble d'un seul segment donne un résultat vide. **Correctif :** réécrire avec un indice qui avance sur les segments fusionnés et émettre systématiquement le segment courant (boucle `while (seg_id < nb_seg)`).
- **B-G76** — `Source/lines2.cpp:343` — `SegmentSet2::Order()` — Accès hors bornes sur un ensemble vide. `GetSegment(0)` est appelé sans vérifier la taille : `indices.at(0)` sur un `QVector` vide déclenche une assertion en debug et un comportement indéfini en release (crash sur entrée dégénérée). **Correctif :** `if (indices.size() < 2) return;` en tête de fonction.
- **B-G77** — `Source/polygon2.cpp:298` — `Polygon2::IsConvex()` — Un triplet de sommets jamais testé. Après la boucle `do … while`, la vérification démarre à `j = i + 1` : si le triplet 0 fixe l'orientation, i vaut 1 et le triplet 1 (sommet central 2) n'est jamais testé (ex. quadrangle concave dont le sommet réflexe est le n°2, déclaré convexe). **Correctif :** `for (int j = i; j < q.size(); j++)`.
- **B-G78** — `Source/polygon2.cpp:614` — `Polygon2::Intersect(const Triangle2&)` — Inclusion non détectée. Seules les intersections arête/arête sont testées : un triangle contenu dans le polygone (ou l'inverse) est déclaré sans intersection. **Correctif :** après le test des trois arêtes, `return Inside(t[0]) || t.Inside(q.at(0));`.
- **B-G79** — `Source/polygon2.cpp:1313` — `Polygon2::Hausdorff()` — Somme au lieu du maximum des distances dirigées. Le code renvoie la somme `Hausdorff(polygon, true) + polygon.Hausdorff(*this, true)` alors que le \param `directed` annonce le maximum (la note décrit la somme, contradictoire), et seules les distances sommet-sommet sont prises en compte, sans le signaler. **Correctif :** `return Math::Max(Hausdorff(polygon, true), polygon.Hausdorff(*this, true));` et corriger la note.
- **B-G80** — `Source/polygons2.cpp:107` — `Polygons2::Signed(), Polygons2::R()` — Trous ignorés, incohérence avec Inside(). `Signed()` et `R()` (Source/polygons2.cpp:94) prennent le min des distances par polygone, ignorant la règle de parité de `Inside()` (un point dans un trou obtient une distance négative / nulle), et accèdent à `poly.at(0)` sans tester un ensemble vide ; `Area()` (l.135) additionne les trous (point secondaire, la doc l.133 exige déjà des polygones disjoints). **Correctif :** d = min des distances non signées aux contours, puis `return Inside(p) ? -d : d;` (et `R = Inside(p) ? 0 : d²`), avec test de vide.
- ~~**B-G81**~~ — `Source/quad2.cpp:250` — `Quadrangle2::InverseBilinear()` — Une seule racine du trinôme testée. ~~Seule la racine (−k1 + √Δ)/(2k2) est retenue, la seconde (en commentaire) serait la bonne pour de nombreux quadrangles~~ — faux positif : testé sur 20000 quadrangles convexes CCW aléatoires (td2/ib.cpp), la racine (−k1 + √Δ)/(2k2) est toujours la bonne et l'autre jamais ; seul un quadrangle horaire (contraire au contrat, cf. B-G50) ou non convexe pose problème.
- **B-G82** — `Source/revolution-quadric2.cpp:76` — `RevolutionQuadricVolume2::Normal()` — Coefficients de la cubique faux si l'extrémité t=1 est la plus proche. La cubique est correcte pour e = P − C(0) (comme dans `Signed()`), mais fausse quand `e` a été remplacé par f = P − C(1) (l.69-73) : les racines ne sont plus les points critiques de la distance. **Correctif :** calculer `double ex = x[0] - px, ey = y[0] - py;` avant le test des extrémités et construire `Cubic c(xy[2], xy[1], xy[0] + 2.0 * (x[2] * ex + y[2] * ey), ex * x[1] + ey * y[1]);`.
- ✅ **B-G83** — `Source/revolution-quadric2.cpp:198` — `RevolutionQuadricVolume2::Scale()` — Intervalle ya, yb non mis à l'échelle. `xa` et `xb` sont multipliés par |s| mais pas `ya` et `yb`, utilisés par `GetOrientedBox()` (l.215) : extension axiale de la boîte fausse après `Scale()`. **Correctif appliqué :** `ya *= fabs(s); yb *= fabs(s);` dans `Scale()` (pour s < 0, l'axe est retourné par `Axis2::Scale` et la coordonnée axiale du même point vaut |s|·y : le facteur |s| est le bon). Vérifié par compilation (`g++ -fsyntax-only -Wall`) ; non exécuté.
- ✅ **B-G84** — `Source/scales2.cpp:98` — `Lune2::Normal()` — Fonction non implémentée renvoyant silencieusement le vecteur nul. Le corps se réduit à `return Vector2::Null;` sans avertissement ni mention dans la doc. **Correctif appliqué :** `Lune2::Normal()` renvoie le vecteur du point le plus proche de la lune vers `p` (nul à l'intérieur et si la lune est vide). Le point le plus proche est celui des candidats suivants le plus proche de `p` : projection radiale sur le cercle A si elle est hors du disque B, projection radiale sur le cercle B si elle est dans le disque A, et les deux coins ; cas sans coins (disques disjoints : disque A ; B dans A : deux cercles complets ; A dans B : lune vide). Fonction en O(1) (≈ 35 ns). Vérifié avec le code réel extrait dans un programme séparé contre un échantillonnage dense de la frontière, 5 000 configurations par cas pour 7 cas (cercles sécants, B dans A, disjoints, A dans B, concentriques, tangents intérieurs et extérieurs) : 0 écart > 1e-3 (max 3,7e-4, résolution de la référence), 0 point intérieur non nul, points confondus avec un centre corrects. Doc Doxygen détaillée. Vérifié par compilation (`g++ -fsyntax-only -Wall`).
- ✅ **B-G85** — `Source/sector2.cpp:38` — `Sector2::GetOrientedBox(), Sector2::GetBox()` — Boîte fausse pour une demi-ouverture supérieure à π/2. La boîte [0,r] × [±r·sin a] ne couvre ni la partie arrière (jusqu'à r·cos a < 0) ni la demi-largeur r quand a > π/2 ; `GetBox()` hérite de l'erreur. **Correctif appliqué :** quand cos(a) < 0 (`sc[1] < 0`), boîte de centre `c + axis * 0.5 r (1 + cos a)` et de demi-tailles `(0.5 r (1 - cos a), r)` ; sinon boîte inchangée. Doc complétée. Vérifié sur 200 000 secteurs aléatoires (arc échantillonné) : l'ancienne boîte ne couvrait pas le secteur dans 99 731 cas (a > π/2), la nouvelle le couvre toujours et est exacte ; `GetBox()` en hérite.
- ✅ **B-G86** — `Source/taper2.cpp:47` — `Taper2::Warp()` — Coefficient de tapering a ignoré. Le membre `a` (« Tapering coefficient », passé au constructeur) n'est jamais lu : le déplacement vaut `cp * v` quel que soit `a` (voir aussi B-G90). **Correctif appliqué :** `return p + a * v * cp;` ; doc de `Warp()` complétée (déplacement radial `a·v(d)·(p−c)`, nul pour a = 0). Le défaut par défaut `a = 1` donne le comportement précédent. `Taper2` n'est utilisée nulle part ailleurs dans le dépôt. La discontinuité au bord (B-G90) reste ouverte. Vérifié par compilation (`g++ -fsyntax-only -Wall`).
- ✅ **B-G87** — `Source/triangle2.cpp:58` — `Triangle2::SignedArea()` — Signe opposé à la convention trigonométrique. `(p[0] - p[1]) / (p[2] - p[0])` = −(p₁−p₀)×(p₂−p₀) : aire négative pour un triangle CCW, contrairement à `Polygon2::Area(true)` et à `Trigonometric()` (l.754) ; aucun appelant dans le dépôt. **Correctif appliqué :** `return 0.5 * ((p[1] - p[0]) / (p[2] - p[0]));` (l'opérateur `/` de `Vector2` est le produit vectoriel 2D : le triangle (0,0), (1,0), (0,1) donne +0,5), doc corrigée (« signed area », convention trigonométrique), cohérent avec `Trigonometric()`.
- **B-G88** — `Source/twistsector2.cpp:59` — `CircleWarp2::Warp()` — Identité dans la bande d'extension e : déformation discontinue. Pour d < e² la fonction renvoie `p` inchangé, puis juste au-delà (e⁺) d' = 0 et v = 1 (déformation maximale) : discontinuité dès que e > 0, et la zone « Constant warping extent » (Source/twistsector2.cpp:59-62) n'est pas déformée. **Correctif :** `d = (d < e * e) ? 0.0 : Math::Sqr(sqrt(d) - e);` au lieu du `return p;`.
- ✅ **B-G89** — `Source/sector2.cpp:94` — `Sector2::Signed()` — NaN possible pour un point sur l'axe. `x = sqrt(n * n - y * y)` : pour un point sur l'axe (ou presque), l'arrondi (axe de norme 1+ε) rend l'argument légèrement négatif et `Signed()` renvoie NaN. **Correctif appliqué :** `double x = sqrt(Math::Max(n * n - y * y, 0.0));`. Vérifié sur 2·10⁶ points placés sur l'axe (avec arrondi) : 575 467 NaN avant, 0 après.
- **B-G90** — `Source/taper2.cpp:39` — `Taper2::Warp()` — Déformation discontinue au bord du disque. Pour d < 0 le point est inchangé, mais pour d → 0⁺ le poids `v = SmoothCompact(0, r) = 1` et le déplacement vaut `cp` : saut sur le cercle ; de plus `Cubic::SmoothCompact(x, r)` attend une distance et un rayon au carré. **Correctif :** définir un profil nul en d = 0 et en d = r (ou appliquer le déplacement aussi à l'intérieur), et passer `d * d, r * r`.

- ✅ **B-G107** — `Source/scales2.cpp:79` — `Lune2::R()` — Distance au carré inexacte. `R()` était calculée à partir de `Signed() = max(dA, −dB)`, qui n'est pas la distance euclidienne près des coins ni dans le trou : mesuré sur 5 463 points extérieurs (cercles sécants), 1 206 écarts relatifs > 1e-3, jusqu'à 88 % (B dans A : 8 écarts, max 42 % ; disques disjoints : 4, max 88 %). **Correctif appliqué :** `R()` vaut `SquaredNorm(Normal(p))` ; la doc de `Signed()` précise que ce n'est qu'une borne. Vérifié : R = |Normal|² sur 35 000 points, 0 écart.

### K — Géométrie 3D de base

- **B-K28** — `Source/camera.cpp:33` — `Camera::Camera(...) (les deux constructeurs)` — Paramètres width/height/field ignorés, constructeur à champ de vision inappelable. Le constructeur principal ignore `width`/`height` (cah/cav/fl gardent leurs valeurs par défaut) et le constructeur à champ de vision (l.46) passe `sin(field/2)` sans effet ; avec 4 à 6 arguments les deux constructeurs (Include/camera.h:24, 25) sont ambigus, donc le constructeur à champ de vision n'est jamais appelable (paramètres sans effet, aucun résultat faux à l'exécution). **Correctif :** supprimer width/height ou les convertir en ouverture (cah, cav), calculer `fl = cah*25.4*0.5/tan(field/2)` et retirer les valeurs par défaut ambiguës.
- ✅ **B-K29** — `Source/axis.cpp:17` — `Axis::X, Axis::Y, Axis::Z` — Risque de fiasco d'ordre d'initialisation statique. `Axis::X/Y/Z` sont initialisés depuis `Vector::Null/X/Y` d'une autre TU (evector.cpp, constructeur non constexpr) : si axis.cpp passe avant, l'axe vaut 0/0 = NaN (risque plutôt que bug systématique, le compilateur faisant souvent l'initialisation statique ; commentaire l.19 « Check if Vector::Z works » ; même risque pour les initialiseurs `= Vector::Z` d'Axis, Camera, Circle). **Correctif appliqué :** `Axis::X/Y/Z` sont construits à partir de vecteurs littéraux `Vector(0.0, 0.0, 0.0)` et `Vector(1.0, 0.0, 0.0)` (idem Y, Z) : plus de dépendance à `Vector::Null/X/Y` d'une autre unité de compilation, commentaire ajouté. Les initialiseurs de membres par défaut (`= Vector::Z` dans `Axis`, `Camera`, `Circle`…) ne sont utilisés que par les objets construits sans ces valeurs ; ils ne sont dangereux que pour un objet global créé dans une autre unité de compilation, cas non traité.
- **B-K30** — `Source/cameraframes.cpp:61` — `CameraAnimatedFrame::Append()` — Clé de même temps que la dernière dupliquée. La boucle de remplacement exclut la dernière clé et l'insertion utilise des inégalités strictes, donc une clé de même temps que la dernière est ajoutée en double en fin de liste (le cas size()==1, l.51, ignore la clé au lieu de la remplacer). **Correctif :** `for (int i = 0; i < mafs.size(); i++) if (mafs[i].time == t) { mafs[i] = mm; return; }` (et remplacer aussi dans le cas size()==1).
- ✅ **B-K31** — `Source/box.cpp:233` — `Box::Box(const Vector*, int), Box::Box(const QVector<Vector>&)` — Accès hors bornes sur ensemble vide. `v[0]` / `v.at(0)` lus sans tester si le tableau est vide (Source/box.cpp:233, 257) : UB ou assertion Qt. **Correctif appliqué :** `if (n <= 0)` (resp. `v.isEmpty()`) : `a` et `b` valent `Vector(0.0)` (boîte réduite à l'origine) et le constructeur retourne ; comportement documenté. La boîte par défaut du membre (`[0,1]³`) n'est donc pas conservée pour un ensemble vide.
- **B-K32** — `Source/axis.cpp:28` — `Axis::Axis(const Vector&, const Vector&), Axis::Scale(const Vector&)` — Division par zéro pour un axe dégénéré. `axis /= length` sans test de longueur nulle (Source/axis.cpp:28, 95) : axe NaN qui contamine R, Normal, GetFrame. **Correctif :** `if (length > 0.0) axis /= length; else axis = Vector::Z;`
- **B-K33** — `Source/circle.cpp:152` — `Circle::Normal(), CircleArc::Normal()` — Division par zéro pour un point sur l'axe du cercle. `r / sqrt(y)` avec y = 0 pour un point situé sur l'axe → vecteur infini/NaN (aussi Source/circlearc.cpp:110 ; Disc::Normal est protégé). **Correctif :** `if (y < 1e-20) { Vector u, v; axis.Orthonormal(u, v); return z - r * u; }`
- ✅ **B-K35** — `Source/tetrahedra-intersection.cpp:8` — `Tetrahedra::Intersect(const Tetrahedra&)` — État global statique : non réentrant. `masks`, `T0`, `Coord_1` (globales statiques) et `e_v1`, `e_v2` (statiques locales) sont partagés : data race en cas d'appels concurrents (Source/tetrahedra-intersection.cpp:8, 10, 11, 157, 193). **Correctif appliqué :** `masks`, `Coord_1`, `T0`, `e_v1` et `e_v2` sont des variables locales de `Tetrahedra::Intersect` ; `T0` est passé à `FaceA_1` et `Coord_1`/`masks` à `EdgeA` par paramètre (`EdgeA` lit `Coord_1` en `const`), et la doc précise que la fonction est réentrante. Vérifié par compilation (`g++ -fsyntax-only -Wall` contre les en-têtes Qt, sans avertissement) ; non exécuté, la logique de l'algorithme est inchangée.
- **B-K36** — `Source/plane.cpp:289` — `Plane::ConvexPoints(), Plane::Intersection()` — Sentinelle Vector::Null : le sommet à l'origine est perdu. `Intersection` renvoie `Vector::Null` en sentinelle, donc `ConvexPoints` rejette tout vrai sommet situé à l'origine ; le seuil `fabs(e) < epsilon` (Source/plane.cpp:228) sur le déterminant dépend de l'échelle des normales. **Correctif :** `static bool Intersection(const Plane&, const Plane&, const Plane&, Vector&)`.
- ✅ **B-K37** — `Source/plane.cpp:167` — `Plane::Transform(const Matrix4&)` — Normale transformée comme une différence de points. La normale est transformée par M (via x et x+n) : faux en cas d'échelle non uniforme ou de cisaillement. **Correctif :** transformer n par l'inverse transposée de la partie 3×3 de M, puis `cqc = (M * x) * n`.
- ✅ **B-K38** — `Source/framescaled.cpp:205` — `FrameScaled::Composed(), FrameScaled::Compose()` — Composition inexacte pour une échelle non uniforme. Partie linéaire calculée `Rf·R·S·Sf` au lieu de `Rf·Sf·R·S` : exact seulement si Sf commute avec R, non documenté (Source/framescaled.cpp:205, 214-216). **Correctif :** documenter la restriction (ou assert sur `frame.s` uniforme quand `r` n'est pas l'identité).
- **B-K39** — `Source/quadricsurface.cpp:60` — `QuadricSurface::Normal()` — Normale 3D tronquée en Vector2. `Normalized(Vector(-g[0], -g[1], 1.0))` est converti implicitement en `Vector2` (constructeur `Vector2(const Vector&)` non explicit) : z est perdu, le résultat n'est ni unitaire ni une normale exploitable (déclaration Include/quadricsurface.h:18, définition Source/quadricsurface.cpp:56). **Correctif :** changer le type de retour en `Vector`.
- **B-K40** — `Source/quadricsurface.cpp:69` — `QuadricSurface::Translated()` — Translation de signe opposé. Le code calcule `g(p) = f(p + t)` (vérifié sur le coefficient x²y : c1 + 2·c0·ty), soit une surface translatée de −t, alors que `Scaled(s)` implémente la transformation inverse `f(p/s)` (Source/quadricsurface.cpp:69-70). **Correctif :** `const double x = -t[0]; const double y = -t[1];` (en valeurs et non en références).
- **B-K41** — `Source/tetrahedra.cpp:510` — `Tetrahedra::Normal(const Vector&)` — Comparaison du sommet 3 avec un produit scalaire erroné. `pa2 * pa3` est comparé au lieu de `pa3 * pa3` : le sommet le plus proche peut être mal choisi, donc `R()` faux. **Correctif :** `if (pa3 * pa3 < npetit) { petit = pa3; }`
- **B-K43** — `Source/triangle-intersection.cpp:106` — `coplanar_tri_tri() (macro POINT_IN_TRI)` — Paramètre V0 de la macro ignoré : le test « U contenu dans V » ne fonctionne pas. Le corps de la macro utilise `V[0]` (Source/triangle-intersection.cpp:106, 111, 116) au lieu du paramètre `V0` : pour `POINT_IN_TRI(U[0], V)` (l.166), V[0] est testé dans V, d0 = d2 = 0 et la macro ne renvoie jamais 1, donc un triangle U strictement contenu dans V est déclaré disjoint. **Correctif :** remplacer `V[0][i0]`, `V[0][i1]` par `V0[i0]`, `V0[i1]` dans la macro.
- ✅ **B-K44** — `Source/triangle.cpp:131` — `Triangle::Intersect(const Ray&, double&, double&, double&), Triangle::Intersect(const Ray&, double&)` — Intersections derrière l'origine du rayon acceptées. Aucun test `t >= 0` (comme dans Möller-Trumbore d'origine), incohérent avec les autres Intersect(Ray) (Circle::Intersect, Box::Intersect) ; lignes Source/triangle.cpp:131, 179. **Correctif :** `t = (e[1] * qvec) * det; return t >= 0.0;` (ou documenter que t peut être négatif).

### R — Primitives 3D volumiques

- ✅ **B-R43** — `Source/bubble.cpp:112` — `Bubble::Normal()` — Vecteur distance de sens inversé dans la cavité intérieure. Hors de la bulle le code renvoie p − q (gradient, comme Sphere::Normal), mais dans la cavité (‖p−c‖ < ri) il renvoie n·(ri/t − 1) = q − p, de sens opposé ; division par zéro si p = c. **Correctif :** écrire `n *= (1.0 - ri / t);` et traiter t = 0.
- ✅ **B-R44** — `Source/capsule.cpp:23` — `Capsule::Unit` — Membre statique jamais défini : variable globale Unit définie à la place. `const Capsule Unit(...)` définit `::Unit` et non `Capsule::Unit` (déclaré Include/capsule.h:49) : erreur d'édition de liens à la première utilisation et pollution de l'espace de noms global. **Correctif appliqué :** `const Capsule Capsule::Unit(Vector(0.0, 0.0, -1.0), Vector(0.0, 0.0, 1.0), 1.0);` : définition du membre statique, avec des vecteurs littéraux plutôt que `Vector::Z` pour ne pas dépendre de l'ordre d'initialisation statique (comme B-K29). Le global `::Unit` n'était utilisé nulle part. Vérifié par compilation (`g++ -fsyntax-only`).
- **B-R45** — `Source/conesphere.cpp:37` — `ConeSphere::ConeSphere()` — Division par zéro si ra = 0, NaN si abs(ra − rb) > length. `side = Vector2(-sc_ha / ra, sc_hra / ra)` vaut 0/0 pour ra = 0, et `s = sqrt(length² − (ra−rb)²)` est NaN si une sphère contient l'autre (constructeur, `R`, `Normal`, `Inside`). **Correctif :** `side = Vector2(-rab, s) / length;` et borner/traiter le cas dégénéré |ra − rb| > length.
- ✅ **B-R46** — `Source/conesphere.cpp:73` — `ConeSphere::Scale()` — sc_ha et sc_hra non mis à jour. `Scale()` met à jour ra, rb, rlength, conelength mais pas `sc_ha`/`sc_hra`, utilisés par `Normal()` pour translater au point de tangence : normale fausse après mise à l'échelle. **Correctif appliqué :** `sc_ha *= fabs(s); sc_hra *= fabs(s);` dans `ConeSphere::Scale()` (`side` est sans dimension et inchangé). Vérifié en copiant constructeur, `Scale` et `Normal` dans un programme séparé, contre un objet construit directement à l'échelle (20 000 cônes-sphères, facteurs de 0,3 à 2,8, négatifs compris) : 7 183 écarts avant, 0 après (écart max 1e-12). Vérifié par compilation (`g++ -fsyntax-only`).
- **B-R47** — `Source/conesphere.cpp:173` — `ConeSphere::Normal()` — NaN pour les points sur l'axe ou dans les plans des extrémités. `i *= (1.0 - sc_hra / x)` (x = 0 sur l'axe), `j *= (1.0 - sc_ha / y)` (y = 0, Source/conesphere.cpp:174) et `j *= 1.0 - length / y` (Source/conesphere.cpp:197) produisent 0·inf = NaN. **Correctif :** calculer directement `j = (y - sc_ha) * axis` et `i = (x - sc_hra) * ir` avec un vecteur radial arbitraire si x ≈ 0.
- ✅ **B-R48** — `Include/sector.h:13` — `Sector::s, Sector2::sc` — Initialisation par défaut incohérente (sinus/cosinus faux). `Vector2(Math::Sqrt2, Math::Sqrt2)` n'est le sinus/cosinus d'aucun angle (même valeur Include/sector.h:43 pour `Sector2::sc`) : un secteur construit par défaut donne une distance signée fausse. **Correctif :** `Vector2(Math::Sqrt2 / 2.0, Math::Sqrt2 / 2.0)`.
- ✅ **B-R49** — `Source/spheroid.cpp:160` — `Spheroid::Area()` — NaN pour une sphère (a = b). Pour a = b, la branche aplatie est prise avec e = 0 et `atanh(0)/0` = NaN. **Correctif :** ajouter `if (fabs(a - b) < epsilon) return 4.0 * Math::Pi * a * a;` (ou un développement limité pour ε petit).
- **B-R50** — `Source/lens.cpp:139` — `Lens::Scale()` — length et height non mis à jour. `Scale()` multiplie a, b et r mais laisse `Axis::length` et `height` inchangés, alors que `Signed()` les utilise (lignes 78-85) : distance fausse après mise à l'échelle. **Correctif :** ajouter `length *= fabs(s); height *= fabs(s);` (sans `Axis::Scale(s)`, qui rend la longueur négative pour s < 0).
- **B-R51** — `Source/sector.cpp:38` — `Sector::GetBox()` — Boîte trop petite pour un angle supérieur à π/2. La boîte du cylindre (c, c + axis·r, r) ne couvre que (p − c)·axis ≥ 0, alors que le secteur déborde sous le plan pour a > π/2. **Correctif :** utiliser `Box(c, r)` quand a > π/2.
- **B-R52** — `Source/sphere.cpp:645` — `Sphere::Intersect(const Sphere&, Circle&)` — Faux négatifs quand le plan d'intersection n'est pas entre les centres. Le test `t < 0.0 || t > 1.0` rejette des sphères sécantes (r = 10, rb = 1, d = 9,5 : t ≈ 1,05, la fonction renvoie false). **Correctif :** supprimer le test sur t (garder `length > 0` et `rc < 0.0`, qui suffit à détecter l'inclusion).
- **B-R53** — `Source/sphere.cpp:694` — `Sphere::Sphere(const QVector<Vector>&)` — Accès hors bornes pour un ensemble vide. `c = p[0]` sans vérifier `p.size()` : UB si le vecteur est vide (cas de `ParticleSet::GetSphere()` sur un ensemble vide). **Correctif :** renvoyer une sphère nulle si `p.isEmpty()`.
- **B-R63** — `Source/dodecahedron.cpp:112` — `Dodecahedron::Intersect(const Ray&, double&, double&, int&, int&)` (idem `Source/icosahedron.cpp:138`, `Source/icosidodecahedron.cpp:197`) — Le test par demi-espaces ignore les plans parallèles au rayon (`Plane::Intersect` renvoie `false` si |n·d| < epsilon, plane.cpp:102) : un rayon parallèle à une face et hors de son demi-espace n'est jamais rejeté, la fonction renvoie `true` avec un intervalle issu des autres plans. Défaut mis au jour par la correction de B-R06. **Correctif :** `if (fabs(n*d) < eps) { if (n*(o - Vertex(i,0)) > 0.0) return false; continue; }` dans chacune des trois boucles (`GetPlane(i)` pour l'icosidodécaèdre).

### C — Courbes et surfaces paramétriques

- **B-C06** — `Source/bezier-patch.cpp:131` — `BezierPatch::Normal(), BezierCubicPatch::Normal()` — Normale NaN sur les bords dégénérés (patches de la théière). `-Normalized(tu / tv)` divise par une norme nulle quand une rangée de points de contrôle est confondue (couvercle et fond de `Teapot()`, 4 points identiques : à v=0, tu=0), et `GetMesh()` échantillonne v=0, d'où des normales NaN (même motif Source/bezier-patch-cubic.cpp:82). **Correctif :** si `SquaredNorm(tu/tv)` est quasi nul, évaluer la normale en un (u,v) légèrement décalé vers l'intérieur (ou via la dérivée seconde) avant de normaliser.
- **B-C07** — `Source/bezier-patch.cpp:105` — `BezierPatch::Vertex(), BezierPatch::Tangents()` — Aucune vérification nx,ny ≤ 16 : débordement des tableaux sur la pile. `vn[16]`, `un[16]` sont remplis par `Math::Powers(v, ny, vn)` et `Math::binomials` est en [16][16], mais les constructeurs `BezierPatch(int,int)` et `BezierPatch(QVector,int,int)` acceptent toute taille (doc « n<16 » sans contrôle) ni ne vérifient `c.size() == nx*ny` : écriture hors pile (UB) si nx ou ny > 16. **Correctif :** `assert(nx <= 16 && ny <= 16 && c.size() == nx * ny);` dans les constructeurs.
- **B-C08** — `Source/bezier-quadric-curve.cpp:21` — `BezierQuadricCurve::GetQuadric(), BezierQuadricCurve2::GetQuadric()` — Avec 3 points de contrôle, la courbe s'arrête au milieu de [c1,c2]. Quand `n == 3`, la branche `i == 0` s'exécute et renvoie `Bezier(c0, c1, (c1+c2)/2)` au lieu de `Bezier(c0, c1, c2)` : la courbe n'atteint pas le dernier point ; `n < 3` provoque en outre un accès hors limites `c.at(2)` (même motif Source/bezier-quadric-curve2.cpp:20). **Correctif :** `if (n == 3) return QuadricCurve::Bezier(c.at(0), c.at(1), c.at(2));` (et garde sur n < 3).
- ✅ **B-C09** — `Source/clothoid.cpp:75` — `Clothoid2::Vertex()` — Approximation de Heald valable seulement pour t ≥ 0. L'approximation rationnelle R(t), A(t) n'est valide que pour t ≥ 0 ; pour t < 0 les dénominateurs peuvent s'annuler (0.803t³+…+2 = 0 vers t≈-1.5) et le résultat est faux, alors que C et S de Fresnel sont impaires. **Correctif :** calculer avec `at = fabs(t)` puis `return Math::Pi * B * (t < 0 ? -Vector2(c, s) : Vector2(c, s));`.
- **B-C10** — `Source/curvecubic.cpp:104` — `CubicCurve::Length(a,b,n)` — Boucle infinie quand a == b (entrée dégénérée, motif répété). `epsilon = (b-a)/n` vaut 0 si a == b, `t <= b + epsilon/2` reste vrai et `t += epsilon` n'avance plus ; l'accumulation flottante de t peut aussi sauter ou doubler le dernier pas (même motif Source/curvecubic2.cpp:97 `CubicCurve2::Length`, Source/curvequadric.cpp:332 `QuadricCurve::Length`, Source/curvequadric2.cpp:314 `QuadricCurve2::Length`). **Correctif :** `if (b == a) return 0.0;` puis boucle entière `for (int i = 1; i <= n; i++)` sur `a + i * epsilon` (règle des trapèzes, `s * epsilon`).
- ✅ **B-C11** — `Source/curvecubic.cpp:137` — `CubicCurve::InverseArcLength(), CubicCurve2::InverseArcLength()` — Pas de borne : boucle infinie sur courbe dégénérée, t > 1 si s > longueur. `while (length < s)` sans limite boucle indéfiniment si la courbe est réduite à un point et renvoie t > 1 si s dépasse la longueur ; la version 2D (Source/curvecubic2.cpp:127) n'a pas le garde `s <= 0` (renvoie epsilon/2), et `sqrt(quartic)` sans `fabs` peut donner NaN. **Correctif :** `while (length < s && t <= 1.0)` puis `return Math::Min(t - epsilon / 2.0, 1.0);`, avec `if (s <= 0.0) return 0.0;` en 2D.
- ✅ **B-C12** — `Source/curvecubic2.cpp:291` — `CubicCurve2::S()` — Abscisse curviligne sous-estimée : dernier segment [u_last, t] ignoré (motif répété). Avec `epsilon = 1/n` et `for (u = epsilon; u < t; …)`, la somme s'arrête au dernier multiple de 1/n < t : S(t) = 0 pour t ≤ 1/n et S(1) peut perdre le dernier segment, alors que `CubicCurve::S()` utilise `epsilon = t/n` (même motif Source/curvequadric.cpp:272 `QuadricCurve::S`, Source/curvequadric2.cpp:254 `QuadricCurve2::S`). **Correctif :** `epsilon = t / n; for (int i = 1; i <= n; i++) { b = Eval(i * epsilon); s += Norm(b - a); a = b; }`.
- ✅ **B-C13** — `Source/curvecubic2.cpp:462` — `CubicCurve2::GetFrames()` — Repère initial dupliqué (motif répété). `GetFrame(0.0)` est ajouté avant la boucle qui démarre à `i = 0` et ajoute le même repère une seconde fois (n+1 repères dont les deux premiers identiques), alors que `CubicCurve::GetFrames()` démarre à i=1 (même motif Source/curvequadric.cpp:438 `QuadricCurve::GetFrames`, Source/curvequadric2.cpp:635 `QuadricCurve2::GetFrames`). **Correctif :** `for (int i = 1; i < n - 1; i++)`.
- **B-C14** — `Source/curvecubic.cpp:394` — `CubicCurve::GetMatrix(), QuadricCurve::GetMatrix()` — Repère de Frenet NaN sur portion rectiligne ou tangente verticale. En mode non vertical, `b = t / n` est nul si la dérivée seconde est nulle ou colinéaire à la tangente (segment droit) et `Normalize` produit des NaN ; en mode `vertical`, `Vector::Z / t` est nul si la tangente est verticale ; les NaN se propagent à `GetFrame()`/`GetFrames()` (même motif Source/curvequadric.cpp:394). **Correctif :** si `SquaredNorm(b) < eps`, basculer sur le repère vertical (ou un axe quelconque orthogonal à t) avant de normaliser.
- **B-C15** — `Source/curvequadric.cpp:202` — `QuadricCurve::R(), QuadricCurve2::R(), QuadricCurve2::Normal(p,u)` — Quadrique dégénérée en segment : distance réduite aux extrémités. Si a2 = 0 (cas fréquent : `QuadricCurve::Bezier(QVector)` avec points colinéaires équidistants), `xyz[2] = 0` et les normalisations divisent par 0 : racines NaN rejetées, seules les extrémités sont retenues et la distance est fausse près du milieu (même motif Source/curvequadric2.cpp:184 et Source/curvequadric2.cpp:395). **Correctif :** `if (xyz[2] < 1e-12)` projeter sur le segment linéaire : `u = clamp(-(a1·(a0-p))/(a1·a1), 0, 1)`.
- **B-C16** — `Source/curvequadric.cpp:364` — `QuadricCurve::LengthIntegral(), QuadricCurve2::LengthIntegral()` — NaN si la quadrique est rectiligne (k = 0) ou linéaire (aa = 0). La formule fermée est correcte en général, mais si a1 ∥ a2 alors k = 0 et pour u < 0 `k*log(u+v) = 0*(-inf) = NaN`, et si aa = 0 on divise par 0 (même motif Source/curvequadric2.cpp:335). **Correctif :** `if (aa < eps) return Norm(a1) * (b - a);` et dans `Mu` `return u * v + (k > 0.0 ? k * log(u + v) : 0.0);` (avec traitement de |u| pour u < 0, k = 0).
- **B-C17** — `Source/curvequadric.cpp:124` — `QuadricCurve::Bezier(const QVector<Vector>&)` — Accès hors limites si moins de 3 points. `c[1]` est lu sans vérification puis `QVector(n - 2)` reçoit une taille négative pour n < 3 : assertion Qt ou crash (même motif Source/curvequadric2.cpp:501). **Correctif :** `if (n < 3) return QVector<QuadricCurve>();`.

### S — Champs scalaires et grilles

- **B-S30** — `Source/algebraic.cpp:131` — `Algebraic::Value(), Algebraic::HighGradient(), Algebraic::IntersectHigh()` — Débordement de pile si n > 8, doc annonçant degré 15. Tableaux locaux `x/y/z[degree+1]`, `t[3][degree+1]` et statiques `equation`/`equationt` de 9 cases (`degree = 8`) sans contrôle de `n` dans les constructeurs : écriture hors pile pour n > 8 (l.139-144, 193-198, 431-438) ; la doc de classe (l.15) annonce « maximum degree 15 » et `Polynomial::MaxDegree = 11`. **Correctif :** `assert(n >= 0 && n <= degree)` dans `Algebraic(const Box&, int)` et `Algebraic(const Box&, int, const QVector<double>&)`, et doc « maximum degree 8 ».
- **B-S31** — `Source/algebraic.cpp:387` — `Algebraic::Intersect()` — Filtrage des racines incorrect après permutation. Après `t[i] = t[cnt-1]` le `continue` incrémente i : la racine déplacée n'est jamais retestée (même défaut dans la boucle des doublons l.397-401), et les racines ne sont pas triées (`t[0]` pas forcément la plus proche). **Correctif :** boucle `for (int i = 0; i < cnt; )` qui n'incrémente i que si la racine est gardée (`t[i] = t[--cnt]` sinon, test hors intervalle et doublon fusionnés), puis `std::sort(t, t + cnt)`.
- **B-S32** — `Source/algebraic.cpp:171` — `Algebraic::Gradient(), Algebraic::LowGradient(), Algebraic::Term()` — Accès hors bornes pour n = 0 (constructeur par défaut). Pour n = 0 (`c` de taille 1), `Gradient` appelle `LowGradient` dont la branche quartique lit `c.at(1)` … `c.at(33)` ; `Term()` renvoie `size` (hors bornes) si aucun terme ne correspond, et `operator()(i,j,k)` indexe alors `c[size]`. **Correctif :** `if (n == 0) return Vector::Null;` en tête de `LowGradient`, tester explicitement `n == 4`, et renvoyer -1 / assert dans `Term()` si non trouvé.
- **B-S33** — `Source/algebraic.cpp:10` — `Algebraic::IntersectHigh()` — Tableaux statiques partagés : course de données en rendu parallèle. `equation` et `equationt` sont des membres statiques écrits à chaque appel de `IntersectHigh` (méthode `const`) : deux threads lançant des rayons simultanément corrompent les coefficients l'un de l'autre. **Correctif :** déclarer `double equation[3][degree+1], equationt[3][degree+1];` en variables locales de `IntersectHigh`.
- **B-S34** — `Source/analyticscalarfield.cpp:672` — `AnalyticScalarField::SphereTrace()` — Pas limité à epsilon (Min au lieu de Max) et convention de signe ignorée. `t += Math::Min(fabs(i) / k, epsilon)` plafonne le pas à epsilon (marche en O((b−a)/ε)), et l'arrêt sur `i > 0.0` ignore `sign` (par défaut `Inside` = v < 0, retour immédiat à t = a pour un rayon partant de l'extérieur) et la doc ; fonction non utilisée dans LibCore. **Correctif :** `if (Inside(i) || fabs(i) < epsilon) return true; t += Math::Max(fabs(i) / k, epsilon);`.
- **B-S35** — `Source/analyticscalarfield.cpp:738` — `AnalyticScalarField::Polygonize(const Box&, ...), AnalyticScalarField::Polygonize(const Tetrahedra&, ...)` — Paramètre epsilon passé comme longueur à Dichotomy. `Dichotomy(a, b, va, vb, length, epsilon = 1e-4)` est appelé avec 5 arguments : `epsilon` est reçu comme `length` et la précision vaut 1e-4 par défaut, donc pour epsilon ≤ 1e-4 aucune itération de bissection (l.738-760 et 817-837). **Correctif :** `Dichotomy(box.Vertex(0), box.Vertex(1), x[0], x[1], Norm(box.Vertex(1) - box.Vertex(0)), epsilon)` pour les 12 arêtes du cube et les 6 du tétraèdre.
- **B-S36** — `Include/array.h:921` — `Array::CubicCells(), AnalyticScalarField::Sample(const Box&, int), AnalyticScalarField::Voxelize(const Box&, int, Box&, QVector<Vector>&)` — Nombre de cellules utilisé comme nombre de sommets. `Box::SetParallelepipedic` renvoie des nombres de cellules alors que `Array(box, nx, ny, nz)` attend des sommets : cellules non cubiques dans `CubicCells` (array.h:921), `Sample` utilise `box` au lieu de `clipped` (analyticscalarfield.cpp:602), `Voxelize` appelle `CellCenter(nx−1, …)` hors boîte (analyticscalarfield.cpp:911-921) et `Voxel(box, …)` au lieu de `clipped` (analyticscalarfield.cpp:946). **Correctif :** `Array(clipped, nx + 1, ny + 1, nz + 1)` dans `CubicCells`, `Sample` et `Voxelize`, et `Voxel(clipped, nx, ny, nz)` l.946.
- **B-S37** — `Source/analyticscalarfield.cpp:375` — `AnalyticScalarField::Sample(const Quadrangle&, int)` — Champ transposé. `f(i, j)` reçoit `quad.Vertex(u, v)` avec `u = Unit(j)`, `v = Unit(i)` alors que `Vector::Bilinear` fait varier u de p0 à p1 : l'axe x de la grille suit v, champ transposé par rapport à `Sample(const Rectangles&, ...)` (l.346-351). **Correctif :** `double u = Math::Unit(i, n); double v = Math::Unit(j, n);`.
- **B-S38** — `Source/analyticscalarfield.cpp:486` — `AnalyticScalarField::Roots()` — Pile statique globale : non réentrant. `stack_t1`, `stack_t2`, `stack_s1`, `stack_s2`, `stack_n` sont des statiques de fichier modifiées par une méthode `const` : deux lancers de rayon concurrents se corrompent. **Correctif :** déclarer les quatre tableaux et `stack_n` en variables locales de `Roots()`.
- **B-S39** — `Source/array.cpp:231` — `Array::CellIntegerVolume()` — Borne maximale des cellules = nx−1 au lieu de nx−2. `bx = min(bx, nx − 1)` (idem y, z) alors que les cellules vont de 0 à nx−2 : `RelaxedArray::Add(const Box&)` boucle `x <= bx` et `CellIndex(nx−1, …)` dépasse `CellSize()`, écriture hors bornes si la boîte touche la face max. **Correctif :** `bx = min(bx, nx - 2); by = min(by, ny - 2); bz = min(bz, nz - 2);`.
- **B-S40** — `Source/arrayindex.cpp:66` — `DynamicArrayIndex::Insert(), DynamicArrayIndex::Update()` — Clamp au nombre de sommets et taille fixe des tables. Position clampée à `nx − 1` (l.66-68, 155-157) alors que les cellules vont de 0 à nx−2 : `CellIndex(n−1, …)` aliasse une autre cellule ; `spat_x/y/z` de taille fixe `SizeIndex = 250000` sans contrôle de `id`, et `Delete` appelle `remove(-1)` si l'id est absent. **Correctif :** clamp à `nx - 2` (resp. ny, nz), `table.resize(CellSize())`, et redimensionner `spat_*` si `id >= spat_x.size()`.
- **B-S41** — `Source/diffusion.cpp:46` — `Diffusion::~Diffusion()` — Buffer GL in_laplacian_buffer jamais libéré. Le destructeur libère 4 buffers sur 5 : `in_laplacian_buffer`, créé dans `BuildGL()` (l.96-97), fuit à chaque objet `Diffusion`. **Correctif :** ajouter `glDeleteBuffers(1, &in_laplacian_buffer);`.
- **B-S42** — `Source/scalarfield-mesh.cpp:133` — `AnalyticScalarField::Polygonize(int, Mesh&, const Box&, const double&)` — Une couche de cubes de trop hors de la boîte. nx, ny, nz sont des nombres de sommets (`d[2] = taille / (nz − 1)`) mais la boucle `for (k = naz; k < nbz; k++)` traite nz couches : la dernière sort de `clipped` et génère des triangles hors domaine (`ncubes`, l.59, compte bien nz−1 couches). **Correctif :** `for (int k = naz; k < nbz - 1; k++)`.
- **B-S43** — `Source/scalarfield-wyvill.cpp:466` — `AnalyticScalarField::PolygonizeLucie(const Box&, ...)` — Collisions de clés de cubes pour des coordonnées négatives ou grandes. La clé `id = (1024*x + y)*1024 + z` (idem `getCubeId`, l.293-303) n'est injective que pour y, z dans [−512, 511] : (0,0,−1) et (0,−1,1023) donnent −1, et `1024³·x` déborde un `int` dès |x| ≥ 2048 (trous ou triangles faux pour des propagations de plus de ~512 cubes par axe). **Correctif :** clé 64 bits décalée `((int64_t(x + (1 << 20)) << 42) | (int64_t(y + (1 << 20)) << 21) | int64_t(z + (1 << 20)))` et `std::unordered_map<int64_t, Cube>`.
- **B-S44** — `Source/scalarfield.cpp:287` — `ScalarField::Sum() et 14 méthodes AVX de ScalarField2` — Chargements/écritures AVX alignés sur des données non alignées sur 32 octets (à confirmer). `_mm256_load_pd`/`_mm256_store_pd` sur `QVector::data()` (alignement 16 octets) et sur des `double e[4]` sans `alignas(32)` : UB formel, exception matérielle si `vmovapd` est émis (MSVC émet souvent `vmovupd`) ; occurrences scalarfield.cpp:287-300, scalarfield2.cpp:509-523, 582-596, 1764-1766, 1809-1812, 1855-1857, 1897-1899, 1979-1983, 2036-2039, 2091-2094, 2162-2165, 2205-2208, 2349-2356, 2420-2425, 2472-2484. **Correctif :** `_mm256_loadu_pd`/`_mm256_storeu_pd` partout et `alignas(32) double e[4];`.
- **B-S45** — `Source/scalarfield.cpp:1790` — `ScalarField2::Texture()` — Axes de l'image inversés : accès hors bornes si l'image n'est pas carrée. `nx = height()`, `ny = width()` puis `pixelColor(i, j)` : x borné par la hauteur, lecture/écriture hors image si elle n'est pas carrée et `at(i, j)` hors du champ si la texture est plus grande que la grille (les locales masquent aussi les membres). **Correctif :** `const int w = texture.width(), h = texture.height();`, boucles `x < w`, `y < h`, `pixelColor(x, y)` et conversion pixel → monde avant `Value(...)` (ou exiger `w == nx && h == ny`).
- **B-S46** — `Source/scalarfield2.cpp:3339` — `ScalarField2::DistanceTransform(), ScalarField2::Polygonize(const double&)` — Fuites mémoire à chaque appel. `double* f = new double[max(nx, ny)]` (l.3339) jamais libéré dans `DistanceTransform` ; `evax` et `evbx` (l.3548-3549) jamais libérés dans `Polygonize`. **Correctif :** `std::vector<double> f(std::max(nx, ny));` et `std::vector<int>` pour les tableaux de `Polygonize`.
- **B-S47** — `Source/scalarfield2.cpp:3346` — `ScalarField2::DistanceTransform()` — Points intérieurs initialisés avec la valeur du champ au lieu de 0. Les points sources reçoivent `at(x, y)` au lieu de 0 dans l'entrée de la transformée de Felzenszwalb (distance biaisée, voire négative) ; le résultat est une distance au carré en cellules, non documentée (bloc l.3331 en `/*`). **Correctif :** `dist(x, y) = at(x, y) < threshold ? 0.0 : Math::Large;` et documenter l'unité (ou multiplier par `celldiagonal[0]²`).
- **B-S48** — `Source/scalarfield2.cpp:2957` — `ScalarField2::LineSegments()` — Orientation du cas 1 incohérente avec les autres cas. Tous les cas laissent la région au-dessus du seuil à gauche, sauf le cas 1 (gauche → bas, région haute à droite) : les polylignes chaînées changent de sens et les contours fermés ne sont pas orientés de façon cohérente. **Correctif :** `case 1: edges.append(eax[VertexIndex(i, j)]); edges.append(eay[VertexIndex(i, j)]); break;`.
- ✅ **B-S49** — `Source/scalarfield2.cpp:729` — `ScalarField2::GetPercentiles()` — Accès hors bornes pour le percentile 1.0. `i = int(p * n)` vaut n pour p = 1.0 : `nth_element(..., vals.begin() + n, ...)` puis `vals[n]` lit hors du vecteur. **Correctif appliqué :** `int i = std::clamp(int(p * n), 0, n - 1);`.
- **B-S50** — `Source/scalarfield2.cpp:662` — `ScalarField2::OldHistogram()` — Seuils des classes sans décalage par le minimum. Bornes `(i+1)·range/n` sans ajouter `min` : valeurs mal classées dès que min ≠ 0 (défaut hérité par `CumulativeHistogram` et `CumulativeNormedHistogram`). **Correctif :** `histogram.append({ min + (i + 1.0) * range / n, 0 });`.
- **B-S51** — `Source/scalarfield2.cpp:3451` — `ScalarField2::GaussianBlur(const double&), ScalarField2::AdaptiveBilateralFiltering(double, double)` — Noyau de largeur nulle : champ rempli de NaN. Pour un rayon inférieur à une cellule, r = 0 → `sigma = 0`, `norm = ∞`, `exp(−0/0)` = NaN ; même chose dans `AdaptiveBilateralFiltering` (l.3805 : `spatial_sigma = 0` → `is = ∞`, `0·∞` = NaN l.3754, 3775), le test `norm == 0.0` ne protège pas. **Correctif :** `if (r < 1) return *this;` en tête de `GaussianBlur(int)` et `if (spatial_sigma < 1) return;` dans `AdaptiveBilateralFiltering(int, double)`.
- **B-S52** — `Source/scalarfield2-morph.cpp:51` — `ScalarField2::MorphSkeleton()` — Écriture de fichiers PNG de débogage à chaque itération. Quatre `CreateImage(...).save(...)` (`init.png`, `erode.png`, `redilate.png`, `diff.png`) par itération dans le répertoire courant (l.51-63) : écrasement de fichiers et coût d'E/S. **Correctif :** supprimer les huit lignes `CreateImage`/`save`.
- **B-S66** — `Source/analyticscalarfield.cpp:166` — `AnalyticScalarField::K(const Box&, int, int, int)` — Écrit `voxel(i, j, k) = K(voxel.Cell(i, j, k))` dans un `Voxel` d'`int` (voxel.h:110) : la constante de Lipschitz est tronquée (toute valeur < 1 devient 0), la carte renvoyée est inexploitable. **Correctif :** renvoyer un champ de réels (`ScalarField` ou `QVector<double>` indexé par `CellIndex`).

### V — Champs vectoriels et de couleur

- ✅ **B-V04** — `Source/vectorfield2.cpp:1084` — `VectorField2::Euler(), VectorField::Euler()` — Schéma « prédiction-correction » seulement d'ordre 1. Le prédicteur fait un demi-pas (`q = p + 0.5*t*vp`) puis le correcteur moyenne `vp` et `v(q)` sur un pas complet : terme `t²/4·(∇v)v` au lieu de `t²/2·(∇v)v`, erreur locale en O(t²) comme Euler simple et non O(t³) comme documenté (même code Source/vectorfield.cpp:102-103). **Correctif appliqué :** méthode de Heun, prédicteur à pas entier (`Vector2 q = p + t * vp; return p + 0.5 * t * (vp + Value(q));`) dans `VectorField2::Euler` et `VectorField::Euler` ; doc mise à jour (erreur locale O(t³), globale O(t²)). Vérifié numériquement : ordre global 2 (rotation, 400 pas : erreur 2,6e-4 contre 2,5e-2 avant).
- **B-V05** — `Source/vectorfield2.cpp:608` — `VectorField2::Add(), VectorField2::Sub(), VectorField2::Mul()` — _mm256_load_pd/_mm256_store_pd sur données non garanties alignées (à confirmer). Les intrinsèques alignés exigent 32 octets alors que les données d'un `QVector<Vector2>` ne sont alignées que sur 16 et le tableau local `ps[4]` de `Mul` sur 8 : UB, plantage (#GP) si `vmovapd` est émis (à confirmer en pratique) ; Source/vectorfield2.cpp:608-611, 650-653, 691-696 ; aucune vérification que `v` a la même taille que `*this`. **Correctif :** `_mm256_loadu_pd` / `_mm256_storeu_pd`, `_mm256_set1_pd(s)` et `assert(v.field.size() == field.size())`.
- **B-V06** — `Source/vectorfield2.cpp:563` — `VectorField2::Divergence(int,int), VectorField2::Curl(int,int)` — Accès hors limites pour une grille de largeur ou hauteur 1. Pour nx==1 (ou ny==1), la branche `i == 0` lit `at(i + 1, j)` hors du champ (Source/vectorfield2.cpp:563, 577, 748, 762). **Correctif :** renvoyer 0 pour la composante concernée si `nx < 2` (resp. `ny < 2`).
- **B-V15** — `Source/colorfield.cpp:230` — `ColorField::BiCubic()` — Dérivées d'alpha égales à 1. `Color::Black` vaut (0, 0, 0, 1) : passé comme dérivées partielles et croisées, il donne des dérivées nulles en RVB mais égales à 1 sur l'alpha, si bien que l'opacité interpolée s'écarte des valeurs aux coins à l'intérieur de chaque cellule, même quand les quatre coins sont opaques. **Correctif :** passer `Color(0.0, 0.0, 0.0, 0.0)` comme dérivées (ou de vraies différences centrées).

### N — Bruit et échantillonnage

- ✅ **B-N14** — `Source/noise-phasor.cpp:157` — `PhasorEric::Value()` — Accès hors bornes de amplitudemap si le point sort du carré unité. `amplitudemap.at(v[0] * (nx - 1), v[1] * (ny - 1))` suppose v dans [0,1]² (non documenté) : tout point hors du carré unité donne un indice (conversion implicite double→int) hors bornes ou négatif. **Correctif :** borner les indices avec `Math::Clamp(int(...), 0, n-1)` et documenter le domaine [0,1]². **Résolu par suppression :** PhasorEric et Source/noise-phasor.cpp ont été supprimés, remplacés par `Phasor2` (`Include/phasornoise.h`, `Source/phasornoise2.cpp`), qui lit son orientation et sa fréquence via les fonctions virtuelles `Orientation()` et `Frequency()` sans carte ni accès indexé.
- ✅ **B-N15** — `Source/noise-voronoi.cpp:58` — `CellularNoise2::Value(), CellularNoise::Value()` — Plage [-2,0] au lieu de [-1,1] annoncé. `noise = 2.0 * (sqrt(noise) / Math::Sqrt2 - 1.0)` donne [-2, 0] (commentaire « maximum should be 1 » faux, décalage -1 propagé dans `CellularTurbulence2`) ; même formule en 3D Source/noise-voronoi.cpp:101 où la distance max est √3 (plage [-2, 0.45]). **Correctif appliqué :** `noise = 2.0 * sqrt(noise) / Math::Sqrt2 - 1.0;` en 2D et `2.0 * sqrt(noise) / Math::Sqrt3 - 1.0` en 3D, commentaires corrigés. Vérifié par Monte-Carlo (2·10⁷ points par dimension) : la graine de la cellule de p est à distance ≤ √dim, donc la plage est garantie dans [-1, 1] ; observé [-1,0 ; 0,67] en 2D et [-1,0 ; 0,42] en 3D (avant : [-2,0 ; -0,33] et [-2,0 ; -0,26]). Plage garantie [-1, 1] (borne 1 atteinte seulement à la limite, probabilité nulle) et plage pratique documentée dans `Value()`. Les valeurs de `CellularTurbulence2` augmentent de 1 par octave pondérée.
- **B-N16** — `Source/noise.cpp:436` — `Noise::K(), Noise2::K()` — Constante de Lipschitz sous-estimée (2 au lieu de ≥3). Avec gradients/valeurs de `rtable` dans [-1,1], la dérivée 1D en cubique atteint déjà -3 au milieu de cellule (plus encore en quintique et avec y, z), mais `K()` renvoie 2 : sphere tracing/projection non conservatifs ; même valeur Source/noise2.cpp:123, propagée par `NoiseTurbulence::K()`/`NoiseTurbulence2::K()`. **Correctif :** borner analytiquement et renvoyer une valeur sûre dépendant de `quintic` (p. ex. 4.0 en cubique, plus en quintique).
- ✅ **B-N17** — `Source/discset.cpp:261` — `DiscSet2::RemoveDuplicates()` — Disque inclus dans un disque d'indice inférieur conservé. Le disque i n'est comparé qu'aux j > i : un petit disque inclus dans un grand disque placé avant lui n'est jamais supprimé, résultat dépendant de l'ordre et contraire à la doc. **Correctif appliqué :** boucle sur tous les j ≠ i, avec `e = |ci-cj| + ri - rj` ; le disque i est supprimé si `e < t` pour j > i (le dernier des disques proches est conservé) ou `e < -t` pour j < i (inclusion nette), doc complétée. Vérifié sur 20 000 ensembles aléatoires (t = 0) : plus aucun disque inclus dans un autre (8 948 ensembles avant) et nombre de disques conservés indépendant de l'ordre. Même défaut dans `SphereSet::RemoveDuplicates` : corrigé avec B-R61.
- **B-N18** — `Source/discsystem.cpp:150` — `DiscSystem::Closest()` — Cache de voisins mélangeant distances et distances au carré. Au remplissage `ll[k] = sqrt(dst)` mais `cut = dst`, puis au remplacement (l.162) `ll[cut_id] = dst` et `cut` = max des `ll` : comparaisons incohérentes, le cache ne contient pas les plus proches voisins et des répulsions sont ignorées. **Correctif :** stocker partout la distance au carré (`ll[total_checked] = dst;` l.150).
- **B-N19** — `Source/discsystem.cpp:284` — `DiscSystem::SplitParticle()` — La particule fille reçoit un rayon r/2 au lieu de r/√2. La l.281 réduit déjà la particule i à r/√2, puis la l.284 relit `particles[i].Radius()` et redivise par √2 : la fille a un rayon r/2, l'aire du schéma de Witkin-Heckbert n'est pas conservée. **Correctif :** calculer `rs = Radius() / Math::Sqrt2` une fois et l'utiliser pour la particule i et pour `AddParticle(..., rs)`.
- **B-N20** — `Source/disctilelink.cpp:106` — `DiscTileLinked::DiscLink::AddD()` — Écriture hors tableau quand maxi ≥ size. `maxi` (= l du constructeur) n'est pas borné par `size` (32) : pour l ≥ 32 la boucle de décalage écrit `index[32]` et au-delà (débordement sur `cell[]`, `distanceQXQ[]` puis hors de l'objet). **Correctif :** `maxi = std::min(maxi, size);` et démarrer le décalage à `std::min(n, maxi - 1) - 1`.
- **B-N21** — `Source/dla.cpp:54` — `Dla::Add()` — Boucle infinie si l'ensemble de germes est vide. Avec `Dla(const QVector<Vector>&, r)` sur un vecteur vide, `Try()` ne détecte jamais de collision et `while (!Try()) {}` ne termine pas. **Correctif :** `if (points.isEmpty()) return;` en tête de `Add()` (ou borner le nombre d'essais).
- **B-N22** — `Source/eden2.cpp:143` — `EdenGrowth2::GrowthSites()` — Débordement possible de subset[16] en 2D. Même motif que B-N02 avec `int subset[16]` : jusqu'à 19 centres espacés de 2r peuvent se trouver à moins de 3.86r de q (zones compactes, après `Multi()` ou avec des germes utilisateur), écriture hors pile. **Correctif :** utiliser un `QVector<int>` ou tester `num < 16` avant d'écrire.
- **B-N23** — `Source/eden2.cpp:719` — `EdenAnisotropicGrowth2::Step()` — Indice -1 quand aucune croissance n'est possible. Si `ntab == 0` (amas saturé, obstacles, `s` vide), `Xoshiro256::Pick(tab, 0)` renvoie -1 : lecture de `can[-1]`/`anc[-1]` et ajout d'une particule fantôme. **Correctif :** `if (ntab == 0) { delete[] anc; delete[] can; delete[] tab; return; }`.
- **B-N24** — `Source/eden.cpp:177` — `EdenGrowth::AddCell()` — La cellule ajoutée n'est pas vivante. Contrairement à la doc (« added as a living particle »), la cellule n'est pas ajoutée à `s` et ne pourra jamais croître ; même code Source/eden2.cpp:213 (`EdenGrowth2::AddCell()`, qui prend en outre un `Vector` 3D). **Correctif :** `s.append(points.size());` avant `points.append(c);`.
- **B-N25** — `Source/eden2.cpp:593` — `EdenGrowth2::Multi()` — Corruption de l'arbre pour m = 1 et division par zéro pour m = 0. Pour m = 1, `v[i] = points.size() - 1` prend pour ancêtre la dernière particule (éventuellement i elle-même), d'où un cycle et une boucle infinie possible dans `Ancestor()` ; pour m ≤ 0, `r /= double(m)` donne un rayon infini/négatif. **Correctif :** `if (m <= 1) return;` en tête de fonction.
- **B-N26** — `Source/hexagonarray2.cpp:80` — `HexagonArray2::VertexIntegerArea(const Box2&)` — Dernière colonne et dernière ligne exclues du masque. `QRect mask(0, 0, nx - 1, ny - 1)` (Source/hexagonarray2.cpp:79) couvre les indices 0..nx-2 : les sommets valides d'indice nx-1 et ny-1 ne sont jamais renvoyés. **Correctif :** `QRect mask(0, 0, nx, ny);`.
- **B-N27** — `Include/relaxedarray.h:51` — `RelaxedArray2::At()` — Accès hors bornes pour un point hors de la grille. `indexes.at(CellIndex(CellInteger(p)))` n'est ni borné ni testé : un point hors domaine ou sur le bord max donne un indice invalide (assertion en debug, UB en release). **Correctif :** borner les coordonnées de cellule à [0, nx-2]×[0, ny-2] (ou renvoyer un vecteur vide statique hors domaine).
- **B-N28** — `Source/sampling.cpp:168` — `SphereTile::Generate()` — Indice de grille négatif si la tuile a moins de 3 cellules par axe. Voisinage ±3 avec `(i + xk + mod) % mod` négatif si mod < 3 → `grid.at()` hors bornes ; même code Source/sampling.cpp:82-84 (`SphereTile::Relaxation()`) et Source/sampling2.cpp:151 (`DiscTile::Relaxation()`), alors que `DiscTile::Generate()` (Source/sampling2.cpp:339) ajoute `2 * mod`. **Correctif :** `int gi = ((i + xk) % mod + mod) % mod;` (idem gj, gk).

### T — Maillages et graphes géométriques

- **B-T10** — `Source/mesh.cpp:2019` — `Mesh::Mesh(const Cone&, int)` — Cône pointu avec apex en a : repère non recalculé après l'échange. `z = Normalized(b - a)` et `x, y` sont calculés avant `Swap(a, b)` : quand `ra == 0`, z pointe de l'apex vers la base, la normale du disque `-z` pointe vers l'intérieur, la normale latérale est inclinée du mauvais côté et l'enroulement de tous les triangles est inversé. **Correctif :** après `Swap(a, b); Math::Swap(ra, rb);`, recalculer `z = Normalized(b - a); z.Orthonormal(x, y);`.
- **B-T11** — `Source/mesh.cpp:1893` — `Mesh::Mesh(const Disc&, int)` — Enroulement opposé à la normale z. Les triangles `(n, (i+1)%n, i)` ont pour normale géométrique `u_{i+1} × u_i = -z` (comme le disque inférieur du cylindre ligne 1837, associé à `-z`) alors que la normale stockée est `+z` : orientation incohérente (backface culling, `Signed`, export). **Correctif :** `AddTriangle(n, i, (i + 1) % n, 0);`.
- **B-T12** — `Source/mesh.cpp:3291` — `Mesh::Mesh(const Voxel&)` — Enroulement des faces indépendant du sens de la normale. L'ordre des sommets de chaque famille de faces est identique quel que soit le côté plein (faces xy géométrique +Z avec normale 5 ou 2 lignes 3291/3322/3326 ; faces yz géométrique +X avec normale 3 ou 0 lignes 3343/3347 ; faces xz géométrique -Y avec normale 4 ou 1 lignes 3364/3368) : environ la moitié des faces a un enroulement contraire à sa normale. **Correctif :** inverser l'ordre des sommets dans la branche où la normale est opposée à la normale géométrique (ex. `AddQuadrangle(a, d, c, b, n)`).
- ✅ **B-T13** — `Source/mesh.cpp:2132` — `Mesh::Scale(const double&)` — Facteur négatif : normales inversées mais pas l'enroulement. Avec `s < 0`, `(b-a)×(c-a)` est multiplié par `s² > 0`, donc l'enroulement donne une normale intérieure alors que les normales stockées sont inversées (extérieures) : orientation incohérente. **Correctif appliqué :** dans la branche `s < 0`, échange de `varray[i]/varray[i+1]` et de `narray[i]/narray[i+1]` pour chaque triangle (avec garde sur la taille de `narray`), doc de `param s` complétée. Vérifié sur un tétraèdre convexe avec s = -2 : avant, 0/4 faces à enroulement sortant (normales stockées sortantes, donc incohérentes) ; après, 4/4 et enroulement cohérent avec les normales.
- **B-T14** — `Source/mesh2.cpp:703` — `Mesh2::EdgeCollapse(const double&)` — Itération avant : triangles déplacés jamais examinés. Contrairement à `Mesh::EdgeCollapse` (itération inverse), `Mesh2` itère de 0 à size : un collapse déplace les derniers triangles aux positions déjà dépassées, qui ne sont plus examinées, et des arêtes courtes subsistent ; `count` est inutilisé. **Correctif :** `for (int i = indices.size() - 1; i >= 0; i--) { if (i >= indices.size()) continue; ... }` comme dans `Mesh`.
- **B-T15** — `Source/mesh2.cpp:300` — `Mesh2::Delaunay()` — Tolérance absolue eps = 1e-4 sur des distances au carré. `(dist - r²) <= eps` compare des grandeurs au carré à une constante absolue : pour un nuage de taille ~1e-2 presque tous les cercles « contiennent » le point (triangulation fausse), pour de grandes coordonnées la tolérance est nulle. **Correctif :** `const double eps = 1e-10 * d * d;` (relative à la diagonale de la boîte).

### U — Utilitaires, couleur, GPU

- ✅ **B-U08** — `Source/color.cpp:137` — `Color::Get()` — Recherche du segment par soustractions : dépassement d'indice possible, coût O(n). La boucle `while (tt >= u) { tt -= u; i++; }` accumule l'erreur d'arrondi : pour t juste sous 1 (ex. 1-1.1e-16 avec 4 couleurs) on obtient i = n-1 et `c.at(i + 1)` lit hors du tableau ; boucle linéaire en n, commentaire « If t == 1 » obsolète. **Correctif appliqué :** calcul direct `x = t * segments; i = Math::Clamp(int(x), 0, segments - 1); return Color::Lerp(x - i, c.at(i), c.at(i + 1));` avec `segments = c.size() - 1` ; `t` NaN renvoie la première couleur (`!(t > 0.0)`) ; doc complétée. Vérifié sur le code réel extrait dans un programme séparé (couleurs scalaires) : pour t = 1 − k ulp (k = 1..40) et des tailles de 3 à 64, l'ancien algorithme sortait du tableau dans 120 cas sur 2 480, le nouveau dans 0 ; 2·10⁶ tirages contre une référence en `long double` : écart max 3,3e-13 avant, 3,6e-14 après ; cas limites (t = 0, 1, < 0, > 1, NaN, tableau vide ou à un élément) corrects ; coût pour 256 couleurs : 59 ns avant, 2,4 ns après. Vérifié par compilation (`g++ -fsyntax-only -Wall`).
- **B-U09** — `Source/palette.cpp:161` — `Palette::GetColor()` — Raccourci « diverging » (type==1) : ignore les ancres et ne borne pas t. Avec `type == 1` les ancres `a` sont ignorées (après `ScaleTo()`, qui modifie `a` sans remettre `type` à 0, le résultat ne correspond plus à la plage demandée) et t hors [0,1] n'est pas clampé, contrairement au cas général. **Correctif :** `type = 0;` dans `ScaleTo()` et `t = Math::Clamp(t);` dans la branche type==1.
- **B-U10** — `Source/palette.cpp:144` — `Palette::ScaleTo(), Palette::GetColor()` — Pas de garde sur palette vide / tailles incohérentes. `ScaleTo()` appelle `a.at(0)` sans tester la taille (crash sur palette vide) et donne NaN (0/0 dans `Linear::Step`) pour une seule ancre ; le constructeur `Palette(c, a)` (Source/palette.cpp:24) n'impose pas `c.size() == a.size()` alors que `GetColor()` (Source/palette.cpp:186-191) indexe `a.at(i + 1)`. **Correctif :** `if (a.size() < 2) return;` dans `ScaleTo` et `assert(c.size() == a.size())` dans le constructeur.
- **B-U11** — `Source/histogram.cpp:51` — `Histogram::Histogram(int, const QVector<double>&)` — Crash sur ensemble vide, UB si toutes les valeurs sont égales. `values.at(0)` sans test de vacuité ; si toutes les valeurs sont égales, b==a donne `e = size/0 = inf` et `Index()` calcule `int(NaN)` (conversion indéfinie). **Correctif :** `if (size == 0 || values.isEmpty()) return;` puis `if (b == a) b = a + 1.0;` avant `e = size / (b - a);`.
- **B-U12** — `Source/histogram.cpp:227` — `Histogram::CreateImage()` — Division par zéro si l'histogramme est vide. `m = MaxCount()` vaut 0 sans échantillon : `double(v[k]) / m` = NaN, `int(NaN)` est indéfini et `height - 1 - y` déborde, la boucle d'écriture de pixels part d'un indice aberrant. **Correctif :** `if (m == 0.0) return image;` après le tracé des séparateurs.
- **B-U13** — `Source/chart.cpp:316` — `DrawChart::ExportPDF(), DrawChart::ExportPNG()` — Déréférencement de pointeur nul sans série. Si `series_list` est vide, `SetUpChartView()` retourne avant de créer `chartView`, puis `chartView->render(...)` (Source/chart.cpp:316) / `chartView->grab()` (Source/chart.cpp:322) déréférencent nullptr ; le retour de `file.open` (Source/chart.cpp:324) est ignoré. **Correctif :** `SetUpChartView(); if (!chartView) return;` et tester `file.open(...)`.
- **B-U14** — `Source/chart.cpp:59` — `DrawChart::AddLine(), DrawChart::AddScatter()` — Accès x[0] sur vecteur vide et fuite de la série en cas d'erreur. `x[0]`/`y[0]` (Source/chart.cpp:59-60, 112-113) sont lus sans test de vacuité (UB) et la série allouée (Source/chart.cpp:43, 103) avant le contrôle `#x != #y` fuit sur `return`. **Correctif :** tester `x.isEmpty()` et la cohérence des tailles avant `new QLineSeries/QScatterSeries`.
- **B-U15** — `Source/cpu.cpp:341` — `System::Memory()` — dwMemoryLoad (pourcentage) divisé par 1024. `dwMemoryLoad` est un pourcentage 0-100 : `/ k` donne toujours 0. **Correctif :** `"Memory   " + QString::number(statex.dwMemoryLoad) + "%"`.
- **B-U16** — `Source/cpu.cpp:342` — `System::Memory()` — Valeurs 64 bits tronquées en int par LongInteger(int). `statex.ullTotalVirtual / k` vaut ~1.37e11 Kio sur Windows x64 et la conversion en `int` de `LongInteger(int)` tronque (Source/cpu.cpp:342-346 ; idem Linux Source/cpu.cpp:372-376, `long long` → `int`). **Correctif :** utiliser `VeryLongInteger((long long)(... / k))`.
- **B-U17** — `Include/core-qt.h:6` — Casse incorrecte QPainterpath : échec de compilation sous Linux. `#include <QtGui/QPainterpath>` alors que le fichier Qt est `QPainterPath` : échec sur système de fichiers sensible à la casse ; pas de `#pragma once` et commentaire « Forward declare… » faux (l'en-tête inclut). **Correctif :** `#include <QtGui/QPainterPath>`, ajouter `#pragma once`, corriger le commentaire.
- **B-U18** — `Source/system.cpp:166` — `System::FlipVertical(), System::FlipHorizontal()` — Les items enfants sont retournés deux fois. `scene.items()` renvoie aussi les enfants, dont `pos()` est relatif au parent : ils subissent la symétrie en plus de celle héritée (Source/system.cpp:166-175 et 193-202), les groupes sont mal placés. **Correctif :** ignorer les items ayant un parent (`if (item->parentItem()) continue;`).
- **B-U19** — `Source/gpu-shader.cpp:261` — `read_program(), read_program_includes()` — Échec de compilation/linkage ignoré. Le code de retour de `reload_program` est ignoré (Source/gpu-shader.cpp:261, 269) et le programme non linké est renvoyé comme valide : `GLShader::Initialize` ne peut détecter l'échec (GL_INVALID_OPERATION à chaque `glUseProgram`). **Correctif :** si `reload_program(...) < 0`, appeler `program_print_errors(program)`, `release_program(program)` et renvoyer 0.
- **B-U20** — `Include/gpu-wrapper.h:135` — `GLShader::Destroy(), GLTexture2D::Destroy(), GLTexture2DArray::Destroy()` — Le handle GL n'est pas remis à 0 après suppression. `GLShader::Destroy()` (Include/gpu-wrapper.h:135-137), `GLTexture2D::Destroy()` (Source/gpu-texture.cpp:92-95) et `GLTexture2DArray::Destroy()` (Source/gpu-texture.cpp:184-188) conservent l'identifiant supprimé : un second `Destroy()` peut supprimer un nom réattribué par le driver (contrairement à `GLBuffer::Destroy()`). **Correctif :** `program = 0;` / `texture = 0;` après suppression et `if (texture == 0) return;` en tête.
- **B-U21** — `Source/gpu-texture.cpp:87` — `GLTexture2D::Generate(), GLTexture2DArray::Generate()` — Fuite de texture si Generate est rappelé. `glCreateTextures` écrase `texture` sans supprimer l'ancienne (Source/gpu-texture.cpp:87 et 179) : rappeler `Generate()` pour redimensionner fuit la texture précédente en mémoire GPU. **Correctif :** `if (texture != 0) glDeleteTextures(1, &texture);` avant `glCreateTextures`.
- **B-U22** — `Source/gpu-texture.cpp:112` — `GLTexture2D::SetData(…, const ScalarField2&), GLTexture2DArray::SetData(…, const ScalarField2&)` — Pas de contrôle de taille, poursuite après erreur de format. Format non mono-canal seulement journalisé puis envoi poursuivi (Source/gpu-texture.cpp:112-114, 212-214, 225-227) : lecture GL hors limites ; aucune vérification des dimensions de `data` ; les variantes `GLTexture2DArray` convertissent toujours en `float` quel que soit `type`. **Correctif :** `return;` après l'erreur, `if (data.VertexSize() != int(width * height)) return;`, respecter `type` comme `GLTexture2D::SetData`.

---

## Bugs Basse criticité 🔵

### M — Mathématiques

- **B-M29** — `Include/mathematics.h:614` — `Math::Integer()` — Décalage de 1 pour les entiers négatifs ou nuls. `(x > 0.0 ? int(x) : int(x) - 1)` donne Integer(0.0) = -1, Integer(-2.0) = -3 alors que la fonction sert de `floor` (noise*.cpp, simplex*.cpp) ; sans effet visible pour un bruit continu. **Correctif :** `return int(floor(x));` ou `int i = int(x); return (x < i) ? i - 1 : i;`
- **B-M30** — `Source/complex.cpp:53` — `Complex::Arg()` — Seuil absolu 1e-12 : argument nul pour les petits complexes. Pour |z| < 1e-12 (ex. 1e-13·i) l'argument vaut 0 au lieu de π/2 ; `atan2` gère toutes les échelles, la normalisation est inutile. **Correctif :** `return atan2(y, x);`
- ✅ **B-M31** — `Source/aa.cpp:1500` — `Aa_pow(const Aa&, const Aa&), Aa_pow(const double&, const Aa&)` — Traces de débogage fprintf(stdout, ...) laissées dans le code. Sorties console systématiques qui polluent stdout et coûtent cher en boucle (autres occurrences Source/aa.cpp:1530, 1557, 1572, et Source/aa.cpp:1736 « und gleich 1! »). **Correctif appliqué :** les six `fprintf` actifs supprimés (l. 1392, 1500, 1530, 1557, 1572, 1736), ainsi que les six `//fprintf` commentés ; plus aucune ligne `printf` dans le fichier. Corrige aussi B-M01.
- **B-M32** — `Source/aa.cpp:2149` — `exp(const Aa&)` — chebyshevFailed calculé mais jamais utilisé. L'indicateur d'échec Chebyshev (fa - 2δ < 0) est positionné puis ignoré : aucun repli Minimum n'est appliqué. **Correctif :** si `chebyshevFailed`, recalculer avec la formule Minimum (α = fa, l.2155-2159).
- **B-M33** — `Source/evector.cpp:180` — `Vector::Angle(const Vector&)` — acos sans écrêtage : NaN pour des vecteurs colinéaires. Par arrondi, le cosinus de vecteurs (anti)colinéaires peut valoir ±(1+ε) et `acos` renvoie NaN. **Correctif :** `acos(Math::Clamp(Cosine(*this, b), -1.0, 1.0))` (ou `atan2(Norm(u/v), u*v)`).
- ✅ **B-M34** — `Source/quaternion.cpp:187` — `Quaternion::AngleAxis()` — acos(w) sans écrêtage. Des arrondis peuvent donner |w| > 1 et `acos(w)` = NaN ; la fonction suppose aussi un quaternion unitaire sans le documenter. **Correctif appliqué :** `angle = 2.0 * atan2(sqrt(n), w);` (norme de la partie vectorielle : indépendante de la norme du quaternion et précise aux petits angles), commentaire ajouté. Vérifié avec la vraie bibliothèque (quaternion lié avec matrix, evector, mathematics, cubic, quadric, linear, random et des remplaçants Qt) : 100 000 rotations quelconques, erreur max 3,9e-12 avant et 4,4e-16 après ; petits angles : θ = 1e-8, 1e-9 et 1e-12 donnaient un angle nul avec `acos(w)` (w arrondi à 1), exacts après (θ = 1e-6 : rapport 1,000044 avant, 1,000000 après) ; w = 1 + 2,2e-16, x = 1e-9 et w = 1,0000001, x = 0,1 : NaN avant, 2e-9 et 0,199337 après ; quaternion non unitaire de facteur 0,5 / 2 / 10 : 2,233 / NaN / NaN avant, 1,000000 pour tous après. Vérifié par compilation (`g++ -fsyntax-only -Wall`).
- **B-M35** — `Source/matrix.cpp:536` — `Matrix::External(const Vector&, const Vector&)` — Renvoie b·aᵀ au lieu de a·bᵀ (à confirmer selon l'intention). Termes écrits en ordre ligne mais pris par colonne par le constructeur : M(i,j) = a_j·b_i (sans effet pour `External(u)` ; la doc ne précise pas l'ordre). **Correctif :** construire par colonnes `Matrix(a * b[0], a * b[1], a * b[2])` et documenter `\f$ a b^T \f$`.
- **B-M36** — `Source/matrix-algebra.cpp:648` — `Matrix::EigenSolveSymmetric()` — Échec de QLAlgorithm ignoré. `QLAlgorithm` renvoie 0 sans convergence en 32 itérations ; ignoré, des valeurs/vecteurs propres partiellement réduits sont renvoyés sans signalement. **Correctif :** propager le code (`bool EigenSolveSymmetric(...)`) ou au minimum le documenter.
- **B-M37** — `Source/matrix-algebra.cpp:678` — `Matrix::Eigen()` — Une seule racine réelle renvoyée comme valeur propre triple. Avec deux valeurs propres complexes (ex. rotation), renvoie `Vector(e0,e0,e0)`, indiscernable d'une valeur propre triple. **Correctif :** renvoyer aussi le nombre de racines réelles (`int Eigen(Vector&) const`) ou documenter ce comportement.
- ✅ **B-M38** — `Source/matrix-algebra.cpp:707` — `Matrix::SpectralNorm()` — Division par zéro pour la matrice nulle. `n = A.InfinityNorm()` vaut 0 si M = 0, `A /= n` produit des NaN et la fonction renvoie NaN au lieu de 0. **Correctif appliqué :** `if (n == 0.0) return 0.0;` dans `Matrix::SpectralNorm()`, doc complétée (plus grande valeur singulière, nulle pour la matrice nulle). Vérifié avec la vraie bibliothèque (matrix-algebra, matrix, evector, mathematics, cubic, quadric, linear, quaternion, random liés avec des remplaçants pour les fonctions Qt) : matrice nulle → NaN avant, 0 après ; 20 000 matrices aléatoires d'échelles 1e-6 à 1e6 : erreur relative maximale 1,2e-14 contre une itération de puissance sur MᵀM.
- ✅ **B-M39** — `Source/matrix-mn.cpp:109` — `MatrixMN::Inverse()` — Gauss-Jordan sans pivot partiel : instabilité numérique. Le pivot est la première valeur non nulle et non la plus grande en valeur absolue : un pivot minuscule amplifie les erreurs (alors que `LogDet()` pivote correctement). **Correctif appliqué :** pivot partiel (`j = argmax_{j>=i} |copy(j,i)|`), matrice déclarée singulière si ce pivot est ≤ 1e-12 × plus grand coefficient en valeur absolue, auquel cas l'identité est renvoyée (comportement existant, désormais documenté) ; doc complétée. Vérifié sur le code réel extrait dans un programme séparé : `[[1e-17, 1], [1, 1]]` → résidu ‖AX − I‖ de 1,0 avant, 0 après ; matrices à pivot diagonal minuscule (n = 8, diagonale × 1e-14) → résidu max 775 avant, 4,7e-13 après ; matrices aléatoires n = 10 et 20 → résidu moyen divisé par 22 et 42 (1,1e-13 → 5,2e-15 et 5,8e-13 → 1,4e-14), résidu max 1,1e-10 → 1,3e-12 et 2,4e-10 → 3,5e-12 ; n = 3 et 6 : aucune dégradation ; matrice de rang 1 : identité dans les deux cas. Limite : une matrice très mal conditionnée (pivot ≤ 1e-12 × max) renvoie maintenant l'identité au lieu d'un inverse inexact. Vérifié par compilation (`g++ -fsyntax-only -Wall`).
- ✅ **B-M40** — `Source/matrix-mn.cpp:164` — `MatrixMN::LogDet()` — Seuil de singularité absolu 1e-12. Le test `|d| < 1e-12` dépend de l'échelle : une covariance bien conditionnée à petits coefficients (~1e-7) est déclarée singulière et renvoie -∞. **Correctif appliqué :** seuil relatif `|d| <= 1e-12 * maxAbsCoeff` (plus grand coefficient en valeur absolue calculé au début ; matrice nulle → -∞), doc complétée, `#include <algorithm>` ajouté. Vérifié sur le code réel extrait dans un programme séparé, avec une référence de Cholesky en `long double` : covariances bien conditionnées d'échelle 1e6 à 1e-12, aucune différence avec l'ancien code (erreur ≤ 4e-14) ; à l'échelle 1e-15, l'ancien code renvoyait -∞ pour 2000 matrices sur 2000, le nouveau les calcule (erreur 5,6e-14). Le seuil absolu produisait aussi l'erreur inverse : sur des matrices réellement singulières (rang n-1) d'échelle 1e9, l'ancien code ne détectait la singularité que dans 15 cas sur 1000, le nouveau dans 1000 sur 1000 ; échelles 1e-9 et 1 : 1000 sur 1000 pour les deux. (L'exemple de l'entrée, des coefficients ~1e-7, n'était pas déclaré singulier : l'effet apparaît sous 1e-12.) Vérifié par compilation (`g++ -fsyntax-only -Wall`).

### P — Polynômes, aléatoire, graphes

- ✅ **B-P06** — `Source/polynomial.cpp:102` — `Polynomial::modp()` — Copie du dividende par arithmétique de pointeurs sur Polynomial (correcte par accident). `r[i] = u[i]` copie des polynômes entiers au lieu des coefficients (Graphics Gems : `r->c[i] = u->c[i]`) ; correct seulement dans l'usage actuel depuis Sturm() (r = u+2, écritures parasites restant dans `sseq[16]`), reste faux ou écriture hors bornes pour tout autre appel. **Correctif :** `for (int i = 0; i <= u->n; i++) r->c[i] = u->c[i]; r->n = u->n;`
- **B-P11** — `Include/polynomial.h:261` — `Polynomial::IsNull()` — Teste `Degree() == 0 && fabs(c[0]) < epsilon` : `Degree()` est le degré stocké `n`, jamais ajusté, donc un polynôme dont tous les coefficients sont nuls mais avec `n > 0` (`Polynomial(0.0, 0.0, 0.0)`, différence qui annule tout, `Integral()` d'un polynôme nul) renvoie `false`, contrairement à la doc. **Correctif :** `for (int i = 0; i <= n; i++) if (fabs(c[i]) >= epsilon) return false; return true;`
- **B-P12** — `Source/polynomial.cpp:90` — `Polynomial::Check()` — Le test `c[n] != 0.0` est devenu `fabs(c[n]) < epsilon` (1e-10 absolu) et le coefficient est écrasé par 0 : `1e-11·x + 1` devient la constante 1 et la racine −1e11 disparaît (même famille que B-P09). **Correctif :** seuil relatif au plus grand |coefficient|, ou documenter le seuil absolu.
- **B-P07** — `Source/polynomial-solver.cpp:53` — `Polynomial::VisibleRoots()` — Fonction privée jamais appelée (code mort). `VisibleRoots` n'est utilisée nulle part (SturmSolve utilise Changes()), code mort hérité de Graphics Gems. **Correctif :** supprimer la déclaration et la définition.
- **B-P08** — `Source/polynomial.cpp:59` — `Polynomial::operator()(const double&) const` — Branche n == -1 inatteignable. Aucun chemin ne met `n` à -1 (Check() s'arrête à 0, Prime() et modp mettent 0) : code mort. **Correctif :** supprimer la branche.
- **B-P09** — `Source/cubic.cpp:96` — `Cubic::Solve(double*) const` — Seuil absolu sur le coefficient dominant. `fabs(a0) < epsilon` (1e-10) déclasse toute cubique à petit coefficient dominant quelle que soit l'échelle (ex. 1e-11 x³ - 1e-11 x : racines ±1 perdues). **Correctif :** seuil relatif, p.ex. `fabs(a0) < epsilon * max(|c[2]|,|c[1]|,|c[0]|)`.
- **B-P10** — `Source/quadric.cpp:159` — `Quadric::Solve(double*) const, Quadric::Solve(double&, double&) const` — Seuil absolu sur le discriminant et formule non stable numériquement. `fabs(d) < epsilon` avec d non normalisé fusionne des racines distinctes à petits coefficients (1e-6 x² - 1e-11 -> racine double 0) et `(b ± d)/(2a)` subit une annulation catastrophique quand b² ≫ 4ac (même motif Source/quadric.cpp:204). **Correctif :** tester `fabs(d) < epsilon * b * b` (ou relatif à max(b², |4ac|)) et utiliser `q = -0.5*(B + sign(B)*sqrt(d)); x1 = q/a; x2 = c/q;`.

### G — Géométrie 2D

- **B-G91** — `Source/box2.cpp:1116` — `Box2::GridSampling(double, Random&)` — Points perturbés hors de la boîte. Le jitter ±d/2 autour de a + i·d, i ∈ [0, nx], fait sortir les échantillons des bords de la boîte, contrairement à la doc. **Correctif :** `x = a[0] + (i + random.Uniform()) * d` avec `i < nx`, ou borner le résultat à la boîte.
- ✅ **B-G92** — `Source/line2.cpp:15` — `Line2::X, Line2::Y` — Membres statiques jamais définis. `const Line2 X(...)` / `Y(...)` au niveau namespace définissent des constantes globales à liaison interne et non les membres statiques déclarés dans `segment.h` : tout usage de `Line2::X` échouera à l'édition de liens (aucun usage actuel). **Correctif :** `const Line2 Line2::X(Vector2::Null, Vector2::X); const Line2 Line2::Y(Vector2::Null, Vector2::Y);`.
- **B-G93** — `Source/circle2.cpp:136` — `Circle2::Inside(const Circle2&)` — Prédicat symétrique, sens du test ambigu (à confirmer). À confirmer : les rayons sont triés (`Math::Sort(rb, ra)`), donc `big.Inside(small)` et `small.Inside(big)` renvoient tous deux vrai ; la doc (« a circle is entirely embedded in another one ») est compatible avec cette lecture symétrique, mais si l'on veut l'asymétrie, la convention de la lib (`Inside(Box2)` = argument dans `*this`) impose « `circle` dans `*this` » et non l'inverse. **Correctif :** trancher la sémantique ; version asymétrique conforme à la lib : `return sqrt(cc) + circle.r <= r;`, sinon documenter la symétrie.
- **B-G94** — `Source/circlearc2.cpp:54` — `CircleArc2::Draw()` — Arcs courts non tracés ou NaN. `n = int((ab - aa) / (π/64))` vaut 0 (rien tracé) ou 1 (`Math::Unit(0, 1)` = 0/0, NaN) pour les arcs courts ; le premier tour trace un segment nul et `brush` est ignoré. **Correctif :** `int n = Math::Max(2, int(...) + 1);` et démarrer la boucle à `i = 1`.
- **B-G95** — `Source/convex2.cpp:745` — `ccw(), dist(), giftWrapping(), GrahamScan(), monotoneChain(), quickHull()` — Code mort à liaison externe, en float, et giftWrapping erroné. Fonctions libres (l.745-949) inutilisées hors convex2.cpp, à liaison externe et noms génériques (risque ODR), calculées en `float`, `giftWrapping` arrêté dès qu'une seule coordonnée coïncide (Source/convex2.cpp:809, `&&` au lieu de `||`), et `using namespace std;` global (l.739). **Correctif :** supprimer le bloc (`Convex2::Hull` suffit) ou le placer dans un `namespace {}` en `double`, avec `while (!(v[0] == hull[0]))`.
- **B-G96** — `Source/ellipse2.cpp:497` — `Ellipse2::Intersect(const Circle2&)` — fabs(d) contredit la doc (à confirmer). À confirmer : la doc annonce « signed distance smaller than the radius » mais le code teste `fabs(d) <= c.Radius()` ; en lecture « disques pleins », un cercle profondément intérieur (d < −r) est raté, en lecture « courbes », fabs est voulu mais un grand cercle centré englobant l'ellipse passe à tort le test : aucune des deux lectures n'est exacte. **Correctif :** fixer la sémantique (`return d <= c.Radius();` pour des disques pleins) et documenter.
- **B-G97** — `Source/framebox2.cpp:41` — `OrientedBox2::OrientedBox2(const Box2&, const Frame2&)` — Ordre de composition discutable (à confirmer). À confirmer : `frame.Composed(Frame2::Translation(box.Center()))` donne x → R·x + t + c (Composed : this.Composed(f) = f∘this, frame2.cpp:101) alors que si `box` est exprimée dans le repère `frame` il faudrait R·(x + c) + t ; sans effet pour les appelants actuels Source/lens2.cpp:153 et Source/lens2.cpp:161 (boîtes centrées). **Correctif :** `:Frame2(Frame2::Translation(box.Center()).Composed(frame))` si telle est la sémantique voulue, et documenter le repère de `box`.
- ✅ **B-G98** — `Include/octogon.h:188` — `IrregularOctogon2::Radius()` — « Rayon » surestimé. Le sommet le plus éloigné est (s+t, s), à distance √((s+t)² + s²) < (s+t)·√2 renvoyé par le code (simple majorant). **Correctif :** `return sqrt(Math::Sqr(s + t) + s * s);`.
- **B-G99** — `Include/irect.h:64` — `IRect2::Intersect()` — Intersection vide non détectable. Pour deux rectangles disjoints la fonction renvoie un rectangle inversé (a > b), sans `IsEmpty()` ni mention dans la doc. **Correctif :** ajouter `bool IsEmpty() const { return a.x > b.x || a.y > b.y; }` et le documenter dans `Intersect()`.
- **B-G100** — `Source/lines2.cpp:137` — `SegmentSet2::GetPolygons()` — Orientation non normalisée dans l'une des deux branches de fermeture. La branche `ja == loop.at(0)` (l.134-138) ajoute le polygone sans test d'aire, contrairement à la branche `jb == loop.at(0)` (l.153-172) qui le réoriente CCW : l'orientation dépend de l'ordre des segments. **Correctif :** factoriser le calcul d'aire et l'appliquer avant chaque `ps.Add(Polygon2(vertices, loop))`.
- ✅ **B-G101** — `Source/polygon2.cpp:1165` — `Polygon2::Resampled()` — Espacement supérieur à la distance « maximale ». `n = int(dist / e) - 1` points intermédiaires donnent `int(dist/e)` morceaux de longueur ≥ e (dist = 1,9, e = 1 : aucun point ajouté). **Correctif :** `int n = int(ceil(dist / e)) - 1;`.
- ~~**B-G102**~~ — `Source/simplex2.cpp:15` — `Simplex2::X2, Simplex2::Y2` — Réseau de Vertex() incohérent avec celui de Corners(). ~~X2 = (1,1)·(F2+G2) et Y2 = rotation de 60° de X2 seraient tournés de 60° par rapport au pas de Corners(), Vertex(i,j) ne désignant pas les sommets de Corners()~~ — faux positif : X2 = (1−2G2)(1,1) = e1+e2 et Y2 = Rot60·X2 = (−G2, 1−G2) = e2 (RotationOneThirdPi vérifiée CCW), même réseau que Corners() avec une indexation différente (Vertex(i,j) = sommet (i, i+j)) ; en revanche Vertex() ignore size (B-G59).
- **B-G103** — `Source/triangle2-intersection.cpp:313` — `tri_tri_intersection_test_3d()` — Segment d'intersection passé par valeur : jamais renvoyé. `source` et `target` sont passés par valeur (le segment calculé par `CONSTRUCT_INTERSECTION` est perdu) et `coplanar` n'est jamais remis à 0 ; impact nul actuellement, la fonction n'étant appelée nulle part (cf. B-G104). **Correctif :** `int tri_tri_intersection_test_3d(..., int& coplanar, Vector& source, Vector& target)` et `coplanar = 0;` en tête.
- **B-G104** — `Source/triangle2-intersection.cpp:56` — `tri_tri_overlap_test_3d(), tri_tri_intersection_test_3d(), coplanar_tri_tri3d()` — Fonctions 3D non déclarées, non documentées, inutilisées. Port de Guigue-Devillers sans déclaration ni appel dans E:\X\Code\Libs, à liaison externe, avec des fonctions `inline` globales aux noms de macros (`CHECK_MIN_MAX`, `TRI_TRI_3D`), commentaires en `/*` (aussi Source/triangle2-intersection.cpp:120 et :313), triangles 3D dans un fichier « 2D ». **Correctif :** les déclarer comme membres statiques de `Triangle` (ou dans un `namespace`) et les documenter en `/*!`, ou les supprimer.
- **B-G106** — `Include/pentagon.h:79` — `Pentagon2::Edge(int k)` — Renvoie `edge[k] * r` alors que `edge[]` est unitaire : la longueur vaut r au lieu de 2r·sin(π/5) ≈ 1,176 r (`Edge()` scalaire, pentagon2.cpp:103, est correct) ; aucun appelant trouvé. **Correctif :** `return Vertex((k + 1) % 5) - Vertex(k);`
- ✅ **B-G105** — `Source/axis2.cpp:61` — `Axis2::Scale(const double&)` — Facteur négatif : axe non inversé, longueur négative. Même défaut que B-K47 en 2D : avec s < 0, `axis` garde son sens alors que b − a s'inverse, et `length` devient négatif. **Correctif :** `length *= fabs(s); if (s < 0.0) axis = -axis;`

- ✅ **B-G108** — `Include/scales.h:28` — `Lune2` — Membres `p`, `q` (points d'intersection) jamais lus et non initialisés si les cercles ne se coupent pas. Le constructeur ignorait le retour de `Circle2::Intersect`, qui laisse `p` et `q` inchangés quand les cercles sont disjoints ou emboîtés. **Correctif appliqué :** membres et appel supprimés (aucun code ne les lisait ; `Normal()` calcule les coins elle-même).

### K — Géométrie 3D de base

- **B-K45** — `Include/circlearc.h:12` — `CircleArc::CircleArc()` — Un arc construit par défaut se comporte comme un cercle complet. Pas d'UB (Vector initialise ses composantes à 0 via `c = {0,0,0}`), mais na, nb, nab restent nuls dans l'arc par défaut, donc `R()`/`Normal()` le traitent comme un cercle complet (s = 0, n·nb = 0) au lieu de l'arc a→b annoncé. **Correctif :** calculer na, nb, nab dans le constructeur par défaut à partir de a, b et de l'axe (comme le constructeur général).
- **B-K46** — `Include/axis.h:136` — `Axis::Radial(const Vector&), Axis2::Radial(), Circle::R(), Disc::Signed(), CircleArc::R(), Triangle::SignedHeight()` — sqrt d'une différence pouvant être négative par arrondi. `sqrt(n*n - y*y)` (ou `sqrt(d - z)`) peut recevoir une valeur légèrement négative par arrondi → NaN (Include/axis.h:136, Include/axis.h:237, Source/circle.cpp:130, Source/disc.cpp:94, Source/circlearc.cpp:162, Source/triangle.cpp:818 où d ≥ z² mathématiquement). **Correctif :** `sqrt(Math::Max(n * n - y * y, 0.0))`.
- ✅ **B-K47** — `Source/axis.cpp:77` — `Axis::Scale(const double&)` — Facteur négatif : axe non inversé, longueur négative. Avec s < 0, `axis` garde son sens alors que b − a s'inverse, et `length` devient négatif (R(), Point() faux). **Correctif :** `a *= s; b *= s; length *= fabs(s); if (s < 0.0) axis = -axis;`
- ✅ **B-K48** — `Include/circle.h:263` — `Circle2::Scaled(), Circle2::Scale()` — Rayon négatif pour un facteur négatif. `r * s` (Include/circle.h:263) et `r *= s` (Include/circle.h:294) sans fabs, contrairement à `Circle::Scale`. **Correctif :** `r * fabs(s)` / `r *= fabs(s)`.
- ✅ **B-K49** — `Source/circlearc.cpp:60` — `CircleArc::Scale()` — Vecteurs de slab non mis à jour pour un facteur négatif. Après une homothétie négative, (a − c) s'inverse alors que l'axe ne change pas : na, nb et nab devraient changer de signe, sinon R()/Normal() faux. **Correctif :** `if (s < 0.0) { na = -na; nb = -nb; nab = -nab; }`
- **B-K50** — `Include/circlearc.h:31` — `CircleArc` — Déclaration friend operator<< avec la mauvaise signature. Le friend déclare `operator<<(std::ostream&, const Circle&)` : aucun opérateur de flux spécifique n'existe pour CircleArc (l'héritage protégé empêche la conversion). **Correctif :** déclarer et définir `friend std::ostream& operator<<(std::ostream&, const CircleArc&);`
- **B-K51** — `Source/camera.cpp:483` — `Camera::PixelToRay(int,int,int,int,int,int,int)` — Échantillons sous-pixel non centrés. Décalages `xa/a - 0.5` dans [−0.5, 0.5 − 1/a], donc biaisés (centrés sur le coin du pixel) ; de plus `y +=` alors que l'axe y est inversé (Source/camera.cpp:483-484). **Correctif :** `x += (xa + 0.5) / a - 0.5; y -= (ya + 0.5) / a - 0.5;` et viser le centre `px + 0.5`.
- **B-K52** — `Source/circle.cpp:173` — `Circle::Circle(const Vector&, const Vector&, const Vector&)` — Seuil de dégénérescence absolu et échec silencieux. `d = |a×b|²` (homogène à une longueur⁴) comparé au seuil absolu 1e-6 : petits triangles déclarés dégénérés et cercle par défaut renvoyé silencieusement. **Correctif :** seuil relatif `fabs(d) > 1e-12 * na * nb`.
- ✅ **B-K53** — `Source/box.cpp:911` — `Box::Cut()` — Sentinelle ambiguë Box::Null. `Box::Null` (boîte ponctuelle à l'origine, non vide au sens de `Empty()`) est renvoyé quand les 8 sommets sont du côté positif, indiscernable d'un résultat valide. **Correctif :** renvoyer une boîte vide explicite (`Box(Vector(Math::Infinity), Vector(-Math::Infinity))`) et le documenter.
- **B-K54** — `Source/tetrahedra.cpp:33` — `Tetrahedra::Tetrahedra(const Vector&, ...)` — Orientation exigée mais jamais vérifiée. Les normales `e01×e02`, `e03×e01`, `e02×e03`, `e13×e12` (Source/tetrahedra.cpp:44-53, Source/tetrahedra-intersection.cpp:164-180) ne sont sortantes que si det < 0 (pour p0 = 0, p1 = X, p2 = Y, p3 = Z, n0 = Z pointe vers l'intérieur) et rien ne le vérifie, alors que `Tetrahedra(const Box&, int)` et `Scaled` négatif violent l'orientation. **Correctif :** dans le constructeur, si `((p[1]-p[0]) / (p[2]-p[0])) * (p[3]-p[0]) > 0`, échanger p[1] et p[2] avant de calculer les normales.
- **B-K55** — `Source/axis.cpp:260` — `Axis::R(const Axis&), Segment::R(const Segment&)` — Seuils absolus dépendant de l'échelle. Seuil absolu 1e-6 sur e (longueur⁴) et sur sN/tN : segments courts traités comme parallèles et distance surestimée (Source/axis.cpp:260, 324-325, Source/segment.cpp:299, 363-364). **Correctif :** seuil relatif `e < 1e-12 * uu * vv` et supprimer les tests `fabs(sN) < 1e-6`.
- ✅ **B-K56** — `Source/rectangle.cpp:137` — `Rectangles::Scale()` — Centre non mis à l'échelle. `c` n'est pas mis à l'échelle (contrairement à Circle, Segment, Quadrangle::Scale) et a, b deviennent négatifs si s < 0. **Correctif :** `c *= s; a *= fabs(s); b *= fabs(s);`
- ✅ **B-K57** — `Source/tetrahedra.cpp:1094` — `Tetrahedra::Scaled(const Vector&), Tetrahedra::Scaled(const double&)` — Orientation inversée pour un facteur négatif. Un facteur négatif (ou un nombre impair de composantes négatives) change le signe du déterminant, donc l'orientation des normales (Source/tetrahedra.cpp:1094-1096, 1104-1106). **Correctif :** échanger p[1] et p[2] quand `s[0]*s[1]*s[2] < 0` (resp. `s < 0`).
- **B-K58** — `Source/triangle.cpp:844` — `Triangle::Subdivide()` — Indices calculés comme si le tableau de sommets était vide. Les indices partent de 0 alors que les sommets sont ajoutés en fin de `vertex` : faux si le tableau n'est pas vide à l'appel. **Correctif :** `const int base = vertex.size();` puis ajouter `base` à chaque indice (ou vider les tableaux et le documenter).
- **B-K59** — `Source/triangle-intersection.cpp:181` — `Intersect(const Triangle&, const Triangle&), Sort(double&, double&, int&), coplanar_tri_tri()` — Code mort et symboles globaux non protégés. La fonction libre `Intersect` n'est déclarée dans aucun header, `coplanar_tri_tri` (l.123) et l'inline global `Sort` (l.12, risque ODR) sont exportés, et les macros `NEWCOMPUTE_INTERVALS` (l.261) et `COMPUTE_INTERVALS_ISECTLINE` (l.347) ne sont jamais invoquées. **Correctif :** placer ces fonctions dans un namespace anonyme (ou `static`) et supprimer la fonction libre et les macros inutilisées.

- ✅ **B-K42** — `Source/tetrahedra.cpp:795, 854` — `Tetrahedra::Normal2(const Vector&)` — Retours `pa0` au lieu de `pa2`/`pa3` dans les régions « Vertex 2 » et « Vertex 3 » de la face 023, et régions non couvertes. Fonction sans appelant (déclarée dans tetrahedra.h:31) ; criticité recalibrée de 🟡 à 🔵 (code mort). Test sur 2·10⁶ couples (tétraèdre, point) contre une référence exacte (point le plus proche sur les 4 faces) : 3,71 % d'écarts ; en corrigeant seulement les deux retours, 0,87 % (points extérieurs pour lesquels aucune région ne retourne et la fonction renvoie le vecteur nul, répartis sur une trentaine de branches, surtout les arêtes) ; `Tetrahedra::Normal()` (utilisée par `R()`) : 0 écart. **Correctif appliqué :** `Normal2` est conservée mais délègue à `Normal` (`return Normal(x);`), doc Doxygen expliquant l'alias et l'ancienne limite (≈540 lignes de code supprimées, historique Git). Vérifié par compilation (`g++ -fsyntax-only`).
### R — Primitives 3D volumiques

- **B-R54** — `Source/cage.cpp:101` — `Cage::Signed()` — Géométrie de la cage non arrondie incohérente avec GetMesh(). `qe = Abs(pi + Vector(t)) - Vector(t)` place les barreaux affleurant les faces de la boîte, alors que `GetMesh()` (Source/cage.cpp:55) et la version arrondie les centrent sur les arêtes. **Correctif :** utiliser `Abs(pi) - Vector(t)` (ou décaler le maillage).
- **B-R55** — `Source/capsule.cpp:869` — `Capsule::FromPoints()` — abs/min/max non qualifiés et variable inutilisée. `min(abs(x0 - tmin), abs(x0 - tmax))` peut se résoudre en `int abs(int)` ou en macros Windows ; `Frame f` (Source/capsule.cpp:826) est inutilisé. **Correctif :** utiliser `fabs`, `Math::Min`, `Math::Max` et supprimer `f`.
- **B-R56** — `Source/conesphere.cpp:114` — `ConeSphere::Normal(), ConeSphere::R()` — Code mort non compilable sous #ifdef SPHERE / #ifdef CYLINDER. Les blocs lignes 114-128, 135-165, 246-262, 270-300 référencent `ha`, `rra`, `rrb`, `ab`, `hb` inexistants ; `rlength` n'est jamais lu et `conelength` ne sert qu'à une normalisation aussitôt écrasée. **Correctif :** supprimer ces blocs et les membres inutiles.
- **B-R57** — `Source/cylinder.cpp:718` — `Cylinder::Intersect(const Ray&)` — Renvoie faux quand l'origine du rayon est dans le cylindre (à confirmer). `if (ta < 0.0) return false;` ignore le cas ta < 0 ≤ tb (origine intérieure), peut-être voulu ; utilisé par `Capsule::Intersect(const Ray&)`. **Correctif :** tester `tb >= 0.0` si le cas intérieur doit être accepté.
- **B-R58** — `Source/ellipsoid.cpp:273` — `operator<<(std::ostream&, const Ellipsoid&)` — Le premier champ affiché est le rayon c, pas le centre. `ellipsoid.c` (3e rayon) est affiché en tête puis à nouveau en fin ; `el` (Source/ellipsoid.cpp:261) est construit pour rien dans FromPoints. **Correctif :** afficher `ellipsoid.Center()` en premier et supprimer `el`.
- ✅ **B-R59** — `Source/icosidodecahedron.cpp:289` — `operator<<(std::ostream&, const Icosidodecahedron&)` — Affiche « Box( ». Le flux écrit `"Box("` au lieu du nom de la classe. **Correctif appliqué :** `"Box("` remplacé par `"Icosidodecahedron("`, comme pour `Icosahedron`.
- **B-R60** — `Source/scales.cpp:24` — `Lune::Lune(const Sphere&, const Sphere&)` — Résultat de l'intersection ignoré. Le booléen de `sphere.Intersect(b, circle)` est ignoré (warning 4189 masqué par `#pragma`) : sphères disjointes → `circle` par défaut utilisé par `Normal()`. **Correctif :** tester le retour (assert ou cercle dégénéré documenté).
- ✅ **B-R61** — `Source/sphereset.cpp:261` — `SphereSet::RemoveDuplicates()` — Seules les sphères incluses dans une sphère suivante sont supprimées. La boucle interne ne teste que `j > i` : le résultat dépend de l'ordre, contrairement à la doc. **Correctif appliqué :** même schéma que B-N17 : boucle sur tous les j ≠ i avec `e = |ci-cj| + ri - rj`, sphère i supprimée si `e < t` pour j > i ou `e < -t` pour j < i ; doc complétée. Vérifié sur 20 000 ensembles aléatoires 3D (t = 0) : plus aucune sphère incluse dans une autre (4 197 ensembles avant), résultat indépendant de l'ordre.
- ✅ **B-R62** — `Include/sphereset.h:55` — `SphereSet::epsilon` — Membre statique déclaré mais jamais défini. Aucune définition de `SphereSet::epsilon` dans Source/ ni Include/ : erreur d'édition de liens à la première utilisation. **Correctif appliqué :** membre supprimé de `Include/sphereset.h`, inutilisé dans tout le code.

### C — Courbes et surfaces paramétriques

- **B-C18** — `Source/bezier-patch.cpp:157` — `BezierPatch::Tangents()` — Facteur nx (resp. ny) au lieu du degré nx-1 (ny-1) : norme des tangentes fausse. Les tangentes sont multipliées par nx (ligne 157) et ny (ligne 165) au lieu des degrés nx-1 et ny-1 : leur norme est fausse, seule la direction de la normale reste juste. **Correctif :** `(nx - 1) * (...)` et `(ny - 1) * (...)`.
- **B-C19** — `Source/clothoid.cpp:114` — `Clothoid2::Draw()` — Arc ouvert dessiné comme un polygone fermé et rempli. `scene.addPolygon()` relie le dernier point au premier et remplit la forme avec `brush` : un segment parasite apparaît entre les extrémités de l'arc. **Correctif :** construire un `QPainterPath` (moveTo/lineTo) et appeler `scene.addPath(path, pen)`.
- **B-C20** — `Source/curvecubic.cpp:327` — `CubicCurve::U()` — Boucle flottante u < 1.0 : le dernier segment peut être ignoré. L'accumulation `u += epsilon` peut donner 0.99999… ou 1.00000…01, si bien que le segment [1-ε, 1] est parcouru ou non et la fonction renvoie 1.0 sans interpolation ; les versions quadriques renvoient en outre `u - epsilon/2`, moins précis (même motif Source/curvecubic2.cpp:266, Source/curvequadric.cpp:293, Source/curvequadric2.cpp:275). **Correctif :** boucle entière `for (int i = 1; i <= n; i++) { double u = double(i) / n; … }`.
- ✅ **B-C21** — `Source/curvecubic2.cpp:96` — `CubicCurve2::Length()` — sqrt sans fabs sur une quartique développée. La quartique |C'|² développée peut être légèrement négative par arrondi près d'un point stationnaire, d'où un NaN, alors que la version 3D utilise `fabs` (curvecubic.cpp:103). **Correctif :** `sqrt(fabs(quartic(t)))` (lignes 96 et 99).
- **B-C22** — `Source/curvequadric2.cpp:657` — `QuadricCurve2::R(const Vector2&, double*, int)` — Paramètre du point le plus proche calculé puis perdu. `t0` est calculé mais jamais renvoyé (variable morte) : l'appelant ne peut pas savoir quel paramètre réalise le minimum. **Correctif :** ajouter un paramètre de sortie `double& u` et y écrire `t0`.
- **B-C23** — `Include/parametric.h:7` — `ParametricCurve, ParametricCurve2, ParametricSurface, BezierPatch` — Classes polymorphes sans destructeur virtuel. Ces classes ont des méthodes virtuelles et sont dérivées (`BezierCurve`, `BezierCurve2`, `BezierCubicPatch`) sans destructeur virtuel : `delete` via un pointeur de base est un UB (même motif Include/surface.h:7, Include/bezier.h:9). **Correctif :** `virtual ~ParametricCurve() = default;` (idem pour les autres).

### S — Champs scalaires et grilles

- ✅ **B-S53** — `Source/analyticscalarfield.cpp:1066` — `AnalyticScalarField::Volume(const Box&, int)` — Variable d'erreur non initialisée. `double e;` est passé à `Volume(box, depth, e)` qui fait `e += ...` : lecture indéterminée (UB formel, valeur ignorée ensuite). **Correctif :** `double e = 0.0;`.
- **B-S54** — `Source/analyticscalarfield.cpp:236` — `AnalyticScalarField::Find(), AnalyticScalarField::Voxelize(const Box&, int, Box&, QVector<Vector>&)` — Convention de signe codée en dur. `Find` teste `v > 0` et `Voxelize` (l.921) garde `Value(p) > 0.0`, alors que `Inside()` vaut `v < 0` par défaut et sert dans `Volume`, `Center`, `Voxelize(int, Voxel&, ...)` : les deux `Voxelize` donnent des résultats complémentaires. **Correctif :** remplacer par `Inside(v)` / `Inside(Value(p))`.
- ~~**B-S55**~~ — `Source/analyticscalarfield.cpp:340` — `AnalyticScalarField::Sample(const Rectangles&, int, int)` — Boîte de taille double. ~~`Box2 box(1.0)` mis à l'échelle par `(r.Width(), r.Height())` donnerait une boîte 2w × 2h~~ — faux positif : `Rectangles::Width()/Height()` renvoient les demi-côtés a, b (Vertex = c ± a·x ± b·y), la boîte obtenue [-a,a]×[-b,b] a donc la bonne taille 2a × 2b.
- **B-S56** — `Source/analyticscalarfield.cpp:428` — `AnalyticScalarField::Cast()` — b non initialisé si n = 0. `Vector() {}` n'initialise pas : pour `n <= 0` la boucle ne s'exécute pas et `0.5 * (a + b)` lit `b` indéterminé. **Correctif :** initialiser `Vector b = a;`.
- **B-S57** — `Source/analyticscalarfield.cpp:161` — `AnalyticScalarField::Hessian()` — Pas de différences finies trop petit pour une dérivée seconde. `Epsilon = 1e-6` aussi pour les dérivées secondes : division par 1e-12, bruit d'arrondi ~1e-4·|f| sur la Hessienne et donc `Curvature()`. **Correctif :** pas dédié `const double h = 1e-4;` dans `Hessian`.
- **B-S58** — `Source/array2.cpp:288` — `Array2::ImageGrid()` — Origine des lignes fausse et image non initialisée. `xa = fmod(box[0][0], a) * a` n'est pas homogène et ne donne pas la première ligne ≥ `box[0][0]` ; la `QImage` n'est jamais initialisée. **Correctif :** `double xa = a * ceil(box[0][0] / a);` (idem y) et `image.fill(Qt::white);`.
- **B-S59** — `Source/diffusion.cpp:28` — `Diffusion::Diffusion(const Array2&, ...)` — Tailles des contraintes non vérifiées. `c_values[i]` lu sans vérifier `c_values.size() >= c_pts.size()`, `c_pts[i]` non testé avec `InsideVertexIndex`, vecteurs passés par valeur. **Correctif :** `const std::vector<QPoint>&`, `assert(c_pts.size() == c_values.size())` et ignorer les points hors grille.
- **B-S60** — `Source/scalarfield-mesh.cpp:486` — `AnalyticScalarField::PolygonizeOctree(int, Mesh&, const Box&, const double&)` — Trace console dans la bibliothèque, n non puissance de 2. `std::cout` systématique, `int(log2(n))` tronque silencieusement (n = 100 → grille de 64) et n ≤ 0 donne un résultat indéfini. **Correctif :** supprimer le `std::cout`, documenter l'arrondi et vérifier `n >= 1`.
- **B-S61** — `Source/scalarfield.cpp:1256` — `ScalarField::DualVertex(), ScalarField::Dual()` — Division par zéro et cache global non réentrant. `v /= double(points)` sans test si le code vaut 0 (cas renvoyé par `getDualPointCode`, l.1179) ; le cache `pointToIndex` (l.837) est une statique de fichier, non réentrante. **Correctif :** `if (points > 0) v /= double(points);` et `std::unordered_map` locale à `Dual()` passée à `getSharedDualPointIndex`.
- **B-S62** — `Source/scalarfield.cpp:1728` — `ScalarField2::CurvatureMin(), ScalarField2::CurvatureMax()` — Racine d'un discriminant négatif. `sqrt(h * h − k)` sans clamp : NaN quand les approximations discrètes donnent H² < K (point ombilic). **Correctif :** `sqrt(Math::Max(h * h - k, 0.0))`.
- **B-S63** — `Source/scalarfield2.cpp:1531` — `ScalarField2::GradientSmooth(int, int)` — Composante y mise à l'échelle par le pas en x. `g *= inversecelldiagonal[0] / 32.0` appliqué aussi à `g[1]` : faux d'un facteur dy/dx pour des cellules non carrées. **Correctif :** `g[0] *= inversecelldiagonal[0] / 32.0; g[1] *= inversecelldiagonal[1] / 32.0;`.
- **B-S64** — `Source/scalarfield2.cpp:2016` — `ScalarField2::Unitize()` — Champ constant envoyé sur 1, en contradiction avec « preserves 0 ». Si `a == b`, `field.fill(1.0)` même pour un champ nul ou négatif, alors que la doc promet de préserver 0 et le signe. **Correctif :** `if (a == b) { if (a != 0.0) field.fill(Math::Sign(a)); return; }`.
- **B-S65** — `Source/scalarfield2.cpp:207` — `ScalarField2::LoadFromR32()` — Fichier non carré : taille du champ incohérente. `nx = ny = int(sqrt(values.size()))` mais `values` est passé sans vérification ni troncature : `field.size() != nx * ny` si le nombre de valeurs n'est pas un carré parfait. **Correctif :** vérifier `nx * nx == values.size()` et renvoyer un champ vide sinon (ou `values.resize(nx * ny)`).

### V — Champs vectoriels et de couleur

- **B-V07** — `Source/vectorfield2.cpp:1023` — `VectorField2::Value(const Vector2&), VectorField::Value(const Vector&), ColorField::Value/Triangular/BiCubic` — Points sur le bord max du domaine considérés comme extérieurs. `InsideCellIndex` exige i < nx-1 : un point exactement sur le bord droit/haut renvoie `Null`/`Black`, et un point légèrement avant `a` est extrapolé au lieu d'être rejeté (Source/vectorfield2.cpp:1023, Source/vectorfield.cpp:68, Source/colorfield.cpp:163, 184, 214). **Correctif :** tester `p` contre la boîte fermée, puis clamper `i` dans [0, nx-2] et recalculer u.
- **B-V08** — `Source/vectorfield2.cpp:1001` — `VectorField2::CreateGradientImage()` — Image 16 bits remplie avec des valeurs 8 bits. L'image `Format_RGBA64` (doc « 16 bits depth ») est remplie via `setPixel(..., QColor.rgb())`, quantifié sur 8 bits ; division par zéro si `max == 0`. **Correctif :** `image.setPixelColor(i, j, QColor::fromRgbF(red, green, 1.0));` et garde `max > 0`.
- **B-V09** — `Source/vectorfield2.cpp:1191` — `VectorField2::LineIntegral()` — Point de départ compté deux fois. Les courbes avant et arrière commencent toutes deux par `ArrayVertex(i, j)` (k = 0) : le sommet de départ reçoit deux fois `x * kernel[0]`. **Correctif :** démarrer la boucle arrière à `k = 1`.
- ✅ **B-V10** — `Source/vectorfield2.cpp:467` — `VectorField2::CreateImage()` — Image non initialisée. `QImage im(2048, 2048, …)` n'est pas remplie avant le rendu : les pixels non couverts contiennent des données indéterminées. **Correctif :** `im.fill(Qt::white);` (ou transparent).
- ✅ **B-V11** — `Source/colorfield.cpp:494` — `ColorField::operator*=(const ScalarField2&)` — L'alpha est aussi multiplié. `Color::operator*=` multiplie les 4 composantes : l'opacité est modifiée alors que la doc parle de mise à l'échelle de la luminance. **Correctif :** `const double al = field[i][3]; field[i] *= s.at(i); field[i][3] = al;`.
- **B-V12** — `Source/colorfield.cpp:142` — `ColorField::CreateImage()` — Canal alpha perdu. `QColor::rgb()` force l'alpha à 255 alors que l'image est en `Format_ARGB32` et que le champ stocke un alpha. **Correctif :** `image.setPixel(i, j, color.rgba());`.
- ✅ **B-V13** — `Source/colorfield.cpp:219` — `ColorField::BiCubic()` — Interpolation « bicubique » à dérivées nulles. Toutes les dérivées passées à `Color::BiCubic` valent `Color::Black` : interpolation de Hermite à tangentes nulles (type smoothstep, plateaux aux sommets), pas bicubique. **Correctif appliqué :** comportement documenté dans le commentaire Doxygen de `ColorField::BiCubic()` (Hermite à dérivées `Color::Black`, plateaux aux sommets, alpha non conforme — voir B-V15 —, `Color::Black` hors domaine, `\sa Color::BiCubic`) ; l'algorithme est inchangé.
- ✅ **B-V14** — `Source/colorfield.cpp:228` — `ColorField::SmoothPoint()` — Code mort, doc en /*. `ColorField::SmoothPoint` n'était appelé nulle part (`Smooth()` réimplémente les mêmes noyaux) et sa doc était en `/*`. **Correctif appliqué :** méthode supprimée (déclaration dans `Include/colorfield.h` et définition dans `Source/colorfield.cpp`).

### N — Bruit et échantillonnage

- **B-N29** — `Source/noise-worm.cpp:82` — `TurbulenceWormNoise::offsets` — Tableau de 8 décalages initialisé avec 7 valeurs. `offsets[8]` n'a que 7 initialiseurs : `offsets[7]` vaut (0,0) et les octaves 7, 15… ne sont pas décalées, contrairement à la doc (ensemble de Poisson). **Correctif :** ajouter la 8e valeur ou déclarer `offsets[7]` et utiliser `i % 7`.
- **B-N30** — `Source/noise-voronoi.cpp:128` — `InfiniteArray::CellVertex()` — Générateur statique partagé modifié dans une méthode const (non thread-safe). Le membre `static Xoshiro256 random` est réensemencé puis consommé dans des méthodes `const` (l.128-131 et l.158-162) : course de données et points non déterministes en évaluation parallèle de tout bruit cellulaire. **Correctif :** utiliser un `Xoshiro256 prng(s);` local (comme `GaborNoise::Cell()`).
- **B-N31** — `Source/noise-voronoi.cpp:193` — `VoroNoise::Value()` — Code mort. `b`, `ibgx`, `ibgy` (l.213-231) sont calculés mais jamais utilisés, et le commentaire l.188 (« twice the diagonal squared ») ne correspond pas à 28.0. **Correctif :** supprimer ces variables et corriger le commentaire.
- **B-N32** — `Source/noise-simplex.cpp:142` — `SimplexNoise::Value()` — Plage effective ≈[-0.5,0.5] au lieu de [-1,1] annoncé (à confirmer). À confirmer : le noyau de rayon² 0.5 avec le facteur 32 de Gustavson (calibré pour 0.6) limiterait une contribution à ≈0.42, soit environ la moitié de [-1,1] annoncé (commentaire l.170, doc de classe l.9), faussant `SimplexTurbulence::Maximum()` d'un facteur ~2 ; non démontré numériquement. **Correctif :** mesurer la plage réelle puis recalibrer le facteur (≈72 estimé) ou documenter la plage.
- **B-N33** — `Source/turbulence.cpp:191` — `SimplexTurbulence::Maximum(), SimplexTurbulence2::Maximum()` — Division par zéro pour alpha = 1. `(1.0 - pow(alpha, octaves)) / (1.0 - alpha)` est recodé au lieu de `Math::Geometric()` qui traite alpha = 1 ; même code Source/turbulence2.cpp:164. **Correctif :** `return v0 + a0 * Math::Geometric(alpha, octaves);`.
- **B-N34** — `Source/discsystem.cpp:374` — `DiscSystem::Steps(), DiscSystem::Cast()` — Particule sautée après suppression. `DeleteParticle(i)` déplace la dernière particule en i puis la boucle incrémente i : la particule déplacée n'est pas traitée ; même motif Source/discsystem.cpp:386 (`Cast()`), où une particule à gradient nul échappe à la suppression. **Correctif :** ne pas incrémenter i après suppression (retour booléen de `Step`/`Cast(int)`) ou boucler à rebours.
- **B-N35** — `Source/discsystem.cpp:335` — `DiscSystem::Step()` — Code mort et normalisation d'un gradient nul. `delta_P = delta_P + gradient;` (l.335) est sans effet, et `Normalized(gradient)` (l.314) est calculé avant le test `sq_gradient_length >= epsilon`, d'où un axe NaN pour un gradient nul. **Correctif :** supprimer la l.335 et ne normaliser qu'après le test.
- **B-N36** — `Source/disctiles.cpp:133` — `DiscTiles::Sample(const ScalarField2&), DiscTileCorner::Sample(const ScalarField2&)` — Le dernier échantillon de chaque tuile n'est jamais retenu. Le seuil `Math::Unit(k, n) = k/(n-1)` vaut 1 pour k = n-1 et `importance > 1` le rejette toujours (NaN pour une tuile d'un seul échantillon) ; même code Source/disctiles.cpp:417. **Correctif :** `if (!(importance > double(k) / double(selected.Size()))) continue;`.
- **B-N37** — `Source/disctiles.cpp:24` — `DiscTiles::Select(), DiscTiles::SelectVertex()` — Débordement d'entier signé (UB formel) dans le hachage. Le hachage de Wang sur `int` (`x * 139`, `a + (a << 3)`, `a * 0x27d4eb2d`, décalage de valeurs négatives) a un comportement indéfini, même si le résultat reste en pratique dans {0,1} ; même code Source/disctiles.cpp:241-251. **Correctif :** calculer en `unsigned int` et renvoyer `a & 1u`.
- **B-N38** — `Source/dla2.cpp:83` — `Dla2::Hit(const Vector2&, int*)` — Tableau de sortie sans capacité. `h[n] = j` écrit autant d'indices que de disques touchés sans capacité passée ni documentée, et le « slight move » de `Agregate()` (Source/dla2.cpp:234) n'exclut pas les chevauchements. **Correctif :** passer la capacité en paramètre et s'arrêter quand elle est atteinte (ou renvoyer un `QVector<int>`).
- **B-N39** — `Source/relaxedarray2.cpp:241` — `RelaxedArray2::Sparsity(), RelaxedArray2::Draw()` — Nombre d'éléments sous-estimé d'une unité. Le nombre d'éléments est pris égal au plus grand indice `last` au lieu de `last + 1` (0 au lieu de 1 pour un seul élément), et `Draw()` (l.311) divise par `m = 0` sur une grille vide. **Correctif :** `return c / double((last + 1) * n);` et tester `n == 0 || c == 0`.
- **B-N40** — `Source/sampling2.cpp:609` — `DiscTile::GetCode(), DiscTileLinked::GetCode()` — Crash sur tuile vide. `p.last()` est appelé sans vérifier que `p` est non vide ; même code Source/disctilelink.cpp:339. **Correctif :** `if (p.isEmpty()) return code + "DiscTile(...,{});\n";` avant la boucle.
- **B-N41** — `Source/sampling2.cpp:437` — `DiscTile::Sample(const Box2&, double(*)(const Vector2&)), DiscTile::Sample(const ScalarField2&)` — Dernier échantillon de la tuile jamais retenu. Même seuil `k/(n-1)` que B-N36 : pour k = n-1, `importance > 1` est toujours faux ; même code Source/sampling2.cpp:478. **Correctif :** `if (!(importance > double(k) / double(p.size()))) continue;`.

### T — Maillages et graphes géométriques

- **B-T16** — `Source/mesh.cpp:1259` — `Mesh::Extrusion(const PointCurve&, const Vector&)` — Normales opposées à l'enroulement des quadrangles. Pour le quadrangle `(2i, 2i+1, 2i+3, 2i+2)`, `(b-a)×(c-a) = d × t` alors que la normale stockée est `t / d = t × d` (`operator/` = produit vectoriel, evector.h:316) ; l'autre surcharge (ligne 1341) est cohérente (à confirmer selon la convention voulue). **Correctif :** `Vector ti = Normalized(d / curve.Tangent(i));` ou inverser l'ordre du quadrangle.
- **B-T17** — `Source/mesh.cpp:370` — `Mesh::Mesh(const Capsule&, int, int)` — Hémisphères : anneau d'équateur dupliqué, dernier anneau confondu avec le pôle. Contrairement au commentaire « does not include equator », la 1re boucle inclut l'équateur (doublon du premier cercle du cylindre) et la seconde atteint π/2 (anneau réduit au pôle), d'où p quadrangles et p triangles d'aire nulle à chaque extrémité ; `n < 4` donne `df = inf` et `m == 1` une division par zéro (même motif Source/mesh.cpp:402, :2234/2282 QuadricCurve, :2356/2404 CubicCurve, :2485/2534 CubicCurveSet, :2620/2663 QuadricCurveSet). **Correctif :** utiliser `j = 1..n-1` pour phi (`f = -π/2 + j*df` puis `f = j*df`) et ajuster `s = p*(2*(n-1)+m)+2`.
- ✅ **B-T18** — `Source/mesh.cpp:489` — `Mesh::AverageEdgeLength()` — Division par zéro sur maillage vide. `length /= varray.size()` renvoie NaN si le maillage est vide et `Smooth()` (ligne 3404) propage ce NaN (même motif Source/mesh2.cpp:666 `Mesh2::AverageEdgeLength`, Source/mesh2.cpp:567 `Mesh2::Aspect` avec `a / (nt / 3)`). **Correctif appliqué :** garde `return 0.0;` sur maillage vide dans `Mesh::AverageEdgeLength()` (mesh.cpp), `Mesh2::AverageEdgeLength()` et `Mesh2::Aspect()` (mesh2.cpp), avec `eturn` documenté ; `Mesh::Smooth()` ne reçoit plus de NaN. Vérifié par compilation (`g++ -fsyntax-only`). La moyenne elle-même est correcte (somme de 3·nt arêtes divisée par `varray.size()` = 3·nt).
- **B-T19** — `Source/mesh.cpp:2178` — `Mesh::SaveObj()` — Précision par défaut de QTextStream (6 chiffres). `QTextStream` écrit les doubles avec 6 chiffres significatifs (perte de précision à l'export) et aucune erreur n'est signalée si l'ouverture échoue. **Correctif :** `out.setRealNumberPrecision(17);` (ou `SmartNotation` + précision 9/17), et retirer l'espace final des lignes `f` (cf. B-T04).
- **B-T20** — `Source/mesh2.cpp:420` — `Mesh2::DelaunayN4()` — Points cocycliques : triangles superposés ; orientation non garantie. `c.Inside(p[w])` stricte accepte tous les triangles d'un quadruplet cocyclique (grille régulière), d'où des triangles qui se chevauchent, et contrairement à `Delaunay()` `SetTrigonometric()` n'est pas appelé. **Correctif :** appeler `mesh.SetTrigonometric(true)` et documenter la limitation (ou départager les cas cocycliques par indice).

### U — Utilitaires, couleur, GPU

- ✅ **B-U23** — `Source/color.cpp:165` — `Color::LerpQt()` — Interpolation de teinte naïve (teinte -1 des gris, pas de bouclage), alpha perdu en CMYK. `hslHue()`/`hsvHue()` renvoient -1 pour un gris, la teinte n'est pas interpolée par le plus court chemin (350°→10° passe par 180°) et la branche CMYK (Source/color.cpp:183-187) ignore l'alpha. **Correctif appliqué :** fonction auxiliaire `LerpHue` (teinte -1 remplacée par celle de l'autre couleur, interpolation par le plus court arc modulo 360, résultat dans [0,359]) utilisée dans les branches Hsl et Hsv, et `Math::Lerp(a.alpha(), b.alpha(), t)` passé à `fromCmyk`. Vérifié sur la logique de `LerpHue` (programme séparé) : 350°→10° à t = 0,5 donne 0° (180° avant), gris→120° donne 120° (59° avant), 907 200 cas sans violation (résultat dans [0,359] et sur le plus court arc) ; avec l'ancienne formule, 32 220 paires sur 129 600 avaient leur milieu hors du plus court arc. Vérifié par compilation (`g++ -fsyntax-only -Wall`).
- **B-U24** — `Source/color.cpp:272` — `Color::Brighten(), Color::Saturate()` — Le canal alpha est réinitialisé à 1. `Brighten` construit `Color(r, g, b)` (Source/color.cpp:272, 276) et `Saturate` passe par `Hsl` (Source/color.cpp:303-305) sans alpha : une couleur semi-transparente redevient opaque (impacte aussi `Palette::Brighten/Saturate`). **Correctif :** propager `c[3]` dans `Brighten` ; dans `Saturate`, `Color r = hsl.ToColor(); r[3] = c[3]; return r;`.
- **B-U25** — `Include/color.h:300` — `Hsl::Brighten()` — Méthode déclarée mais jamais définie. `void Brighten(const double&);` n'a aucune définition : tout appel provoque une erreur d'édition de liens. **Correctif :** supprimer la déclaration ou l'implémenter (`l = Math::Clamp(l + b);`).
- ✅ **B-U26** — `Include/palette.h:66` — `AnalyticPalette::Reversed()` — La fréquence est perdue. `return AnalyticPalette(n, !r);` réinitialise `frequency` à 1.0 (palette 13 / Inigo). **Correctif appliqué :** `return AnalyticPalette(n, !r, frequency);` (palette.h). Vérifié par compilation.
- **B-U27** — `Source/histogram.cpp:224` — `Histogram::CreateImage()` — Fraction cumulée mal normalisée. `Math::Unit(cc, t)` calcule `cc/(t-1)` et non `cc/t` : la fraction dépasse 1 pour le dernier bac, couleur de palette légèrement décalée. **Correctif :** `double f = (t > 0) ? double(cc) / t : 0.0;`.
- **B-U28** — `Include/histogram.h:109` — `Histogram::Interval()` — Largeur de bac calculée avec size-1. `(b - a) / (v.size() - 1)` est incohérent avec la discrétisation (`e = size/(b-a)`, `At()` divise par `v.size()`) et la doc « Return the value in a given bin » est fausse. **Correctif :** `return (b - a) / v.size();` et doc « Returns the width of a bin ».
- **B-U29** — `Source/histogram.cpp:85` — `Histogram::ExportPDF(), Histogram::ExportPNG()` — Titre codé en dur et représentations différentes. Les deux exports utilisent le titre fixe « Slope Histogram » et `ExportPNG` (Source/histogram.cpp:99) trace une courbe (`AddLine`) au lieu de barres, contrairement à `Draw` et `ExportPDF`. **Correctif :** ajouter un paramètre titre et utiliser `AddBars(v, a, b)` dans les deux cas.
- **B-U30** — `Source/chart.cpp:216` — `DrawChart::SetUpChartView()` — Axes ajoutés à chaque appel. `AddAxis()` est rappelé à chaque `Display()/ExportPDF()/ExportPNG()` : `Display()` suivi d'un export ajoute un second jeu d'axes. **Correctif :** `if (chart->axes().isEmpty()) AddAxis();`.
- **B-U31** — `Source/chart.cpp:344` — `DrawChart::DemoChart()` — SetNbTickX appelé deux fois. La seconde ligne devait sans doute être `SetNbTickY(4)`. **Correctif :** `graph.SetNbTickY(4);`.
- **B-U32** — `Source/cpu.cpp:29` — `System::WinGetEnv()` — Valeur tronquée renvoyée comme valide, tampon statique non réentrant. Au-delà de 65535 caractères, `GetEnvironmentVariableA` renvoie la taille requise et le contenu indéfini de `buffer` est renvoyé ; le tampon statique rend la fonction non thread-safe. **Correctif :** `DWORD r = GetEnvironmentVariableA(...); return (r > 0 && r < size) ? buffer : nullptr;`.
- **B-U33** — `Source/system.cpp:52` — `System::SavePdf()` — Code mort et effet de bord sur la scène. `sizebb` calculé puis inutilisé (Source/system.cpp:52-53, 92-93), `painter2.scale(1.0, 1.0)` sans effet (Source/system.cpp:63, 103), et `SavePdf(…, box)`/`Rasterize` modifient définitivement le `sceneRect` de la scène (Source/system.cpp:90, 214). **Correctif :** supprimer le code mort ; sauvegarder et restaurer `sceneRect()`.
- **B-U34** — `Source/system.cpp:286` — `System::RecurseDirectory()` — completeSuffix() ignore les fichiers à plusieurs points. Pour `foo.bar.cpp`, `completeSuffix()` vaut `bar.cpp` et le fichier n'est pas compté (Source/system.cpp:286, 293). **Correctif :** utiliser `info.suffix()`.
- **B-U35** — `Source/gpu-texture.cpp:59` — `GLTexture2D::GetMemoryUsed()` — Taille par texel multipliée par le nombre de canaux. `bytes` est déjà la taille d'un texel complet (4 pour GL_RGBA8) : le multiplier par `channels` surestime la mémoire (×4 pour `Generate(w, h, GL_RGBA8, 4)`). **Correctif :** `return size_t(width) * height * bytes;`.
- **B-U36** — `Source/gpu-texture.cpp:161` — `GLTexture2D::GetQtFormat()` — GL_R32F associé à Format_Grayscale16 (à confirmer). Un texel R32F occupe 4 octets flottants, `Format_Grayscale16` 2 octets entiers : envelopper les données lues avec ce format produit une image fausse (à confirmer). **Correctif :** renvoyer `QImage::Format_Invalid` (ou convertir explicitement) pour GL_R32F.
- **B-U37** — `Source/gpu-shader.cpp:131` — `prepare_source()` — Ligne #version perdue quand definitions est vide. `source.append(version); source.assign(file);` : `assign` écrase la version (branche aujourd'hui inatteignable depuis `reload_program`, qui ajoute toujours un `#define`, mais fausse). **Correctif :** `source.append(file);`.
- **B-U38** — `Source/gpu-shader.cpp:356` — `print_errors()` — Regex non ancrées : numéros de ligne Mesa faux, lignes du log sautées. `regex_search` cherche dans tout le reste du log : sur `0:12(5): error` (Mesa), `re_nvidia` trouve `12(5):` (ligne = colonne 5), et `i += consumed` saute les lignes sans motif (l'ancienne version `sscanf` était ancrée). **Correctif :** `std::regex_search(s, match, re, std::regex_constants::match_continuous)` (motif ancré en début de ligne).
- **B-U39** — `Source/gpu-shader.cpp:48` — `parse_includes()` — #include en tout début de fichier non reconnu. Le motif `\s+#include` exige un blanc avant `#include` : une directive en position 0 n'est pas remplacée et fait échouer la compilation GLSL. **Correctif :** `std::regex r(R"((^|\s)#include\s+<(\S+)>)");` (adapter les indices de groupe).
- **B-U40** — `Source/gpu-shader.cpp:222` — `reload_program()` — glObjectLabel appelé selon une macro de compilation (à confirmer). `#ifdef GL_VERSION_4_3` est toujours vrai avec GLEW : sur un contexte < 4.3 sans KHR_debug, le pointeur de fonction est nul → crash (à confirmer). **Correctif :** tester `if (GLEW_VERSION_4_3 || GLEW_KHR_debug)` à l'exécution.
- **B-U41** — `Shaders/diffusion.glsl:66` — `main()` — Mise à jour de Jacobi incohérente avec la convention du Laplacien (à confirmer). Pour `Σv - n·u = f` Jacobi donne `u = (Σv - f)/n`, mais le code calcule `Σv/n - f` (et la convention de `Laplacian()` du fichier impliquerait `(Σv + f)/n`) : sauf pré-division de `laplacian_target` côté CPU, la solution est mal mise à l'échelle (à confirmer). **Correctif :** `out_diffused[id] = (sum - laplacian_target[id]) / float(cpt);` et documenter la convention attendue.
- **B-U42** — `Shaders/diffusion.glsl:37` — `Laplacian(int), NeighborSum(int)` — Voisins ±1 en indice 1D : bouclage d'une ligne à l'autre (code mort). `getValue(id + 1, cpt)` / `getValue(id - 1, cpt)` ne testent que les bornes du tableau : en bord de ligne le voisin est pris sur la ligne voisine ; ces surcharges ne sont pas utilisées par `main()`. **Correctif :** supprimer ces surcharges ou utiliser la version (x, y).

---

## Problèmes de Documentation 📝

### M — Mathématiques

#### 📝 D-M01 — `Include/mathematics.h:866` — Dérivée documentée fausse (dénominateur non élevé au carré)

La doc indique `2*sqrt(k)*(1-k*x*x)/(k*x*x+1)` alors que la dérivée de 2√k·x/(1+kx²) est 2√k(1-kx²)/(1+kx²)².
**Correctif :** corriger la formule : `2*sqrt(k)*(1-k*x*x)/(k*x*x+1)^2`.

#### 📝 ✅ D-M02 — `Include/mathematics.h:799` — Doc inexacte pour x<0

La doc dit « -x raised to the power exponent e otherwise » alors que le code renvoie `-pow(-x, e)` (extension impaire, signe conservé).
**Correctif :** « returns -(-x)^e for x<0 (odd extension) ».
**Corrigé :** Doc de `Math::Pow` : « the opposite of -x raised to the power e otherwise (odd extension, the sign of x is kept) ».

#### 📝 ✅ D-M03 — `Source/mathematics.cpp:266` — Intervalle signé annoncé [-π,π[ alors que le code produit ]-π,π]

Avec `s == false`, y ∈ [0,2π[ puis `if (y > Math::Pi) y -= 2π` : π reste π et -π n'est jamais produit ; l'intervalle réel est ]-π, π].
**Correctif :** corriger la doc (ou tester `y >= Math::Pi`).
**Corrigé :** Doc de `Math::FoldAngle` : intervalle signé corrigé en ]-π, π] (3 occurrences).

#### 📝 D-M04 — `Source/mathematics.cpp:373` — n documenté « Degree » alors que c'est le nombre de termes (degré n-1)

BernsteinSeries utilise `binomials[n-1]` et produit n termes (n <= 16 imposé par `ui[16]`), incohérent avec `Bernstein(u,n,i)` qui utilise `binomials[n]` (n = degré, n <= 15) ; aucune précondition documentée.
**Correctif :** documenter `\param n Number of terms (degree n-1), n <= 16`.

#### 📝 D-M05 — `Include/mathematics.h:57` — Doc « of two doubles » sur les surcharges float

Les surcharges `float` (lignes 57-58 et 65-66) sont documentées « Minimum/Maximum of two doubles ».
**Correctif :** « of two floats ».

#### 📝 D-M06 — `Include/linear.h:246` — Exemple de code équivalent faux

L'exemple construit `Linear((vb-va)/(b-a), b*va+a*vb)` alors que l'ordonnée à l'origine de la droite passant par (a,va),(b,vb) est `(b*va - a*vb)/(b-a)` ; l'exemple donnerait une mauvaise racine.
**Correctif :** `Linear((vb-va)/(b-a), (b*va-a*vb)/(b-a))`.

#### 📝 ✅ D-M07 — `Include/smooth.h:65` — Doc copiée « Compute the Lipschitz constant »

`CubicFalloff()` (l.64-66) et `Inside()` (l.72-74) sont documentées « Compute the Lipschitz constant » alors qu'elles renvoient le type de falloff et le test d'appartenance à la boule.
**Correctif :** « Return true if cubic falloff » / « Check if a point is inside the support sphere ».
**Corrigé :** `SmoothVertex::CubicFalloff` : « Check if the falloff is cubic » ; `Inside` : « Check if a point is inside the support sphere » avec `param p`.

#### 📝 D-M08 — `Source/ia.cpp:511` — Doc « superior » au lieu de « inferior »

La doc dit « Check if the entire interval is superior to the real » alors que le code teste `b < x` (intervalle entièrement inférieur).
**Correctif :** « inferior to the real ».

#### 📝 ✅ D-M09 — `Include/aa.h:117` — Commentaire /* au lieu de /*! ignoré par Doxygen

Le commentaire de doc de `Aa::Center()` commence par `/*` et est ignoré par Doxygen ; même motif dans Source/aa.cpp:968 (`Aa::operator*=(const Aa&)`).
**Correctif :** remplacer `/*` par `/*!`.
**Corrigé :** `/*` remplacé par `/*!` pour `Aa::Center()` (aa.h) et `Aa::operator*=(const Aa&)` (aa.cpp).

#### 📝 D-M10 — `Source/evector.cpp:42` — FAQ « ascending order » mais Vector::Sort trie en décroissant (Vector2 en croissant)

La FAQ de la classe (l.42) annonce un tri croissant ; `Vector::Sort()` (l.314-368) trie en ordre décroissant (sa propre doc le dit) alors que `Vector2::Sort()` (evector2.cpp:171) trie en ordre croissant, incohérence trompeuse entre 2D et 3D.
**Correctif :** corriger la FAQ (« descending order ») et documenter la différence avec Vector2::Sort (ou harmoniser les deux).

#### 📝 ✅ D-M11 — `Source/evector.cpp:117` — « positive cosine » alors que le cosinus renvoyé est signé

Le code renvoie `(u*v)/(|u||v|)`, valeur dans [-1,1] ; même motif dans Source/evector2.cpp:56.
**Correctif :** « Returns the (signed) cosine of the angle between two vectors ».
**Corrigé :** « (signed) cosine of the angle between two vectors », résultat dans [-1,1], dans evector.cpp et evector2.cpp.

#### 📝 ✅ D-M12 — `Source/ivector2.cpp:8` — Doc de classe « three dimensions » et pcg2d() non documentée

La doc de classe de Vec2I dit « Point in three dimensions » ; `Vec2I::pcg2d()` (l.32) n'a aucun commentaire Doxygen (ni référence à PCG2D de Jarzynski & Olano, ni description de la sortie).
**Correctif :** « Point in two dimensions with integer coordinates » ; ajouter `/*! \brief PCG2D hash (Jarzynski & Olano 2020) ... */`.
**Corrigé :** Doc de classe `Vec2I` : « two dimensions » ; ajout d'un bloc Doxygen pour `Vec2I::pcg2d()` (PCG2D, Jarzynski & Olano 2020).

#### 📝 D-M13 — `Source/matrix.cpp:599` — Nom trompeur : max des |coefficients|, pas la norme infinie matricielle

La norme ∞ matricielle est le max des sommes de lignes de |a_ij| ; le code renvoie `max |r[i]|` (norme « max »), ce qui induit en erreur pour un critère de convergence ; même motif dans Source/matrix2.cpp:256.
**Correctif :** documenter « max-norm (largest absolute coefficient) » ou implémenter `max_i Σ_j |a_ij|`.

#### 📝 D-M14 — `Source/matrix4.cpp:485` — Nom trompeur : la « shear » est placée dans la dernière ligne (termes projectifs)

`Matrix4(a, t, s)` place `s` en r[3], r[7], r[11] (ligne 3) et `operator*(Vector)` divise par `w = v·s + r[15]` : transformation projective, pas un cisaillement ; les commentaires de `Matrix4(const Matrix&)` (Source/matrix4.cpp:69-73) inversent aussi « Translation » et « Shear », contrairement à la doc de classe (translation = A[12..14]).
**Correctif :** renommer en `Projective()` ou documenter « perspective terms (last row) », et corriger les commentaires l.69-73.

#### 📝 D-M15 — `Include/matrix.h:126` — Noms de paramètres aij trompeurs (stockage en ordre colonne)

Stockage par colonne avec `operator()(i,j)` = `r[i + 2j]` (resp. `r[i+3j]`) : le paramètre `a01` est rangé dans r[1], soit l'élément (1,0) ; la doc de Matrix2 ne précise pas l'ordre et celle de Matrix (l.420) dit « column order » mais garde des noms en convention ligne, ce qui est à l'origine des erreurs de Quaternion::RotationMatrix et Matrix::External(a,b) ; même motif en Include/matrix.h:422.
**Correctif :** renommer les paramètres (`c0r0, c0r1, …` ou `a00, a10, a20, a01, …`) et documenter « coefficients given column by column: (0,0),(1,0),(2,0),(0,1)… ».

### P — Polynômes, aléatoire, graphes

#### 📝 D-P01 — `Source/polynomial-solver.cpp:232` — Cite Quartic::Quartic() au lieu de Quartic::Solve(), ordre des racines non précisé

La doc indique « invoking Quartic::Quartic() » (constructeur) au lieu de Quartic::Solve(), et ne précise ni que les racines ne sont pas triées pour n ≤ 4 (triées dans la branche Sturm), ni que la branche Sturm se limite à [-1e7, 1e7].
**Correctif :** « invoking Quartic::Solve(), Cubic::Solve() or Quadric::Solve() ... Roots are not guaranteed to be sorted; for degree ≥ 5 only roots in [-1e7,1e7] are found. »

#### 📝 ✅ D-P02 — `Source/polynomial-solver.cpp:132` — \param atmin, atmax documentés par « ? »

Les paramètres sont les nombres de changements de signe de la suite de Sturm en a et en b ; la phrase « This function first tries the standard bissection method » est inexacte (c'est la regula falsi qui est tentée d'abord).
**Correctif :** `\param atmin, atmax Number of sign changes of the Sturm sequence at a and b.` et corriger la phrase.
**Corrigé :** `param atmin, atmax` documentés (nombres de changements de signe de la suite de Sturm en a et b) ; description corrigée (regula falsi d'abord, bissection en repli, sinon découpage récursif).

#### 📝 D-P03 — `Source/polynomial.cpp:147` — Nommée « bissection » alors qu'il s'agit d'une regula falsi (variante Illinois)

Le code calcule `x = (fb*a - fa*b)/(fb - fa)` avec division par 2 de f à l'extrémité stagnante (méthode Illinois) ; `\param val Value.` ne dit pas que c'est la racine en sortie, le retour (0/1) n'est pas documenté et le paramètre `epsilon` masque le membre statique `Polynomial::epsilon`.
**Correctif :** `\brief Find a root in [a,b] using the Illinois variant of regula falsi.` `\param val Returned root.` `\return 1 if a root was found, 0 otherwise.`

#### 📝 D-P04 — `Source/cubic.cpp:207` — Décrite comme « Bicubic interpolation » alors que c'est une interpolation cubique 1D

Le code évalue la spline de Catmull-Rom entre b et c à partir de quatre valeurs a,b,c,d (rien de bicubique) et \param x ne précise pas que x ∈ [0,1] paramètre l'intervalle [b,c].
**Correctif :** `\brief Cubic (Catmull-Rom) interpolation between b and c using four consecutive values.` `\param x Interpolant in [0,1].`

#### 📝 D-P05 — `Source/quartic.cpp:356` — \param p et q inversés

La doc dit « \param q The quadric. \param p The argument quadric. » alors que le code calcule p(q(x)) : p est la quadrique extérieure et q l'argument.
**Correctif :** `\param p The outer quadric. \param q The argument quadric.`

#### 📝 D-P06 — `Include/quartic.h:11` — //! Empty. placé après la déclaration documente la déclaration suivante

`Quartic() {} //! Empty.` (ligne 11) et `~Quartic() {} //! Empty.` (ligne 15) : pour Doxygen, un `//!` en fin de ligne s'attache au membre suivant (le constructeur à 5 coefficients et `operator[]` reçoivent la doc « Empty. »).
**Correctif :** utiliser `//!<` ou placer `//! Empty.` sur la ligne précédente, comme dans les autres classes.

#### 📝 D-P07 — `Source/quintic.cpp:133` — Doc « over a given interval » et « sorted roots » fausses

La surcharge sans intervalle est documentée « over a given interval » et affirme des racines triées, alors que la cascade Quartic::Solve/Cubic::Solve ne garantit aucun ordre et que la branche Sturm ne cherche que dans [-1e7, 1e7] ; occurrences Source/quintic.cpp:134-137, Source/sextic.cpp:89-92, Source/septic.cpp:89-92.
**Correctif :** `\brief Solve the equation. Roots are not guaranteed to be sorted; roots outside [-1e7,1e7] are ignored for degree ≥ 5.`

#### 📝 D-P08 — `Source/polynomial.cpp:459` — `Polynomial::Integral(const double&)` — rief faux et précondition non vérifiée

Le `rief` dit « with a zero constant term » alors que `a0` fixe le terme constant. Pour `n == MaxDegree` (11), l'écriture `r[i + 1]` déborde de `c[12]` (UB) : la précondition « strictement inférieure au degré maximal » n'est pas vérifiée.
**Correctif :** `rief Compute the antiderivative with constant term a0.` et ajouter `assert(n < MaxDegree)`.

### G — Géométrie 2D

#### 📝 D-G01 — `Source/annulus2.cpp:97` — Doc « squared distance » pour une distance signée

Le \brief de `Signed()` annonce « Compute the squared distance » alors que la fonction renvoie la distance signée non carrée ; le `\param t` de `Intersect` (Source/annulus2.cpp:50) n'indique pas qu'il doit pointer sur au moins 4 doubles.
**Correctif :** `\brief Compute the signed distance to the annulus.` et `\param t Array of (at least) 4 intersection depths.`

#### 📝 D-G02 — `Source/boxset2.cpp:17` — /* au lieu de /*! et commentaire de classe faux

Le commentaire de doc du constructeur commence par `/*` et n'est pas extrait par Doxygen ; dans Include/boxset.h:6, le commentaire `// Sphere class` est faux (classe d'ensemble de boîtes).
**Correctif :** remplacer `/*` par `/*!` ; corriger le commentaire en `// Set of boxes`.

#### 📝 D-G03 — `Source/line2.cpp:24` — Direction non normalisée, contrairement au constructeur depuis un segment

`Line2(o, d)` conserve `d` tel quel alors que `Line2(const Segment2&)` normalise la direction : les profondeurs renvoyées par les fonctions utilisant `Direction()` (ex. `Box2::Intersect(const Line2&,...)`) changent d'unité selon le constructeur, sans que rien ne le documente.
**Correctif :** `\param d Direction (not normalized; intersection depths are expressed in units of |d|).` ou normaliser systématiquement.

#### 📝 D-G04 — `Source/circle2.cpp:566` — /* au lieu de /*! et \param faux

Le bloc commence par `/*` (non extrait) et documente `\param p` et `\param n` alors que l'unique paramètre est `s` ; même motif `/*` dans Include/quadrangle.h:47 (`Quadrangle::Vertex(int)`) et Include/quadrangle.h:99 (`Quadrangle2::Vertex(int)`).
**Correctif :** `/*!` et `\param s Set of points (should not be empty).`

#### 📝 D-G05 — `Source/circlearc2.cpp:28` — c documenté comme « point sur l'arc » alors que c'est le centre

La doc indique « \param c Point on the arc » mais le code délègue à `CircleArc2(c, Norm(c - a), a, b)` (c = centre, sémantique utilisée par `TwinDisc2::Draw()`) : un appelant suivant la doc obtient un cercle faux.
**Correctif :** `\param c Center.` et `\param a,b End points of the arc (|a-c| gives the radius, b should lie on the same circle).`

#### 📝 D-G06 — `Source/convex2.cpp:133` — Valeur de retour documentée de façon incorrecte

La doc dit « returns true if intersection occured » ; la fonction renvoie `true` quand le polygone est entièrement à gauche (aucune coupe) et `false` quand il est entièrement à droite (polygone vidé) : la valeur signifie « le polygone résultant est non vide ».
**Correctif :** `\return true if the remaining polygon (left side of the line) is not empty; the polygon is emptied otherwise.`

#### 📝 D-G07 — `Source/disc2.cpp:83` — Nom trompeur : R() renvoyant une distance signée non carrée

Dans toute la bibliothèque `R()` désigne la distance au carré, mais `Disc2::R(const Disc2&)` renvoie une distance signée non carrée (`Norm(d.c - c) - d.r - r`) ; le \brief le dit déjà explicitement, reste le nom trompeur (question d'API).
**Correctif :** renommer en `Signed(const Disc2&)` (ou conserver en le signalant comme exception de nommage).

#### 📝 D-G08 — `Source/disctwins2.cpp:36` — Doc « Euclidean distance » pour une distance au carré ; Inside parle de « lens »

`R()` renvoie le min de `Disc2::R()` (distance au carré) alors que la doc annonce « the Euclidean distance » ; la doc de `Inside()` (Source/disctwins2.cpp:21) parle de « the lens » alors que la forme est l'union des deux disques.
**Correctif :** `\brief Compute the squared Euclidean distance to the union of the discs.` et `\brief Test if a point is inside the union of the discs.`

#### 📝 ✅ D-G09 — `Source/ellipse2.cpp:35` — Équation de l'ellipse fausse et \brief copiés-collés

La doc écrit « (p - c)^t . A . (p - c) = 0 » alors que l'ellipse est le lieu = 1 (le code calcule bien a = 1/√λ) ; la doc de `Poisson()` (Source/ellipse2.cpp:665) parle de « irregular octogon » et celle de `RandomInside()` (Source/ellipse2.cpp:219) de « circle ».
**Correctif :** `\f$ (p - c)^t . A . (p - c) = 1 \f$`, et corriger les \brief en « inside the ellipse ».
**Corrigé :** équation « (p - c)^t . A . (p - c) = 1 » (le constructeur calcule bien a = 1/√λ), `RandomInside` : « inside the ellipse », `Poisson` : « inside the ellipse ».

#### 📝 ✅ D-G10 — `Source/framebox2.cpp:59` — Doc « squared Euclidean distance » pour une distance signée

Le \brief de `Signed()` est un copier-coller de celui de `R()` (« Compute the squared Euclidean distance to the box »).
**Correctif :** `\brief Compute the signed Euclidean distance to the box.`

#### 📝 ✅ D-G11 — `Include/hexagon.h:55` — Valeur de la constante mal décrite

La doc indique « sin(π/6) = √3/2 » ; or sin(π/6) = 1/2 et √3/2 = sin(π/3) = cos(π/6) (valeur utilisée dans Source/hexagon2.cpp:15).
**Correctif :** `//!< Constant \f$ \sin(\pi/3) = \sqrt{3}/2 \f$.`

#### 📝 ✅ D-G12 — `Include/octogon.h:114` — Signification de s et t mal documentée

D'après `Vertex()`, `Area()` et `Perimeter()`, les côtés droits mesurent 2s et les côtés diagonaux t√2 (s demi-longueur, t côté de l'angle coupé), pas « side length » ; dans Source/irregularoctogon2.cpp:16 et :25 les `\param r, s` ne correspondent pas aux paramètres `s, t`, et `\class IrregularOctogon` (l.8) ne désigne pas la classe.
**Correctif :** `double s = 1.0; //!< Half length of axis aligned edges.`, `double t = 1.0; //!< Leg of the cut corners (diagonal edges have length t√2).`, `\param s,t ...`, `\class IrregularOctogon2 octogon.h`.

#### 📝 ✅ D-G13 — `Include/irect.h:15` — Blocs \brief vides

Tous les blocs de doc de irect.h (l.15, 33, 40, 47, 54, 61) ont un `\brief` vide et aucun `\param` ; rien n'indique que `IRect2(const QRect&)` utilise `right()`/`bottom()` (bornes inclusives) ni la sémantique de `IRect2(int x, int y, int r)` (carré de demi-côté r).
**Correctif :** renseigner chaque `\brief` et les `\param`.

#### 📝 ✅ D-G14 — `Include/ellipse.h:60` — « Major / minor radius » alors que a est la demi-longueur sur l'axe

D'après le membre (`a` : longueur sur l'axe, `b` : rayon radial, l.14) et les constructeurs, rien n'impose a ≥ b (sphéroïde aplati : a < b) ; « Major radius » / « Minor radius » sont trompeurs (classe 3D, hors domaine 2D strict).
**Correctif :** `//! Half length along the axis of revolution.` et `//! Radius in the plane orthogonal to the axis.`

#### 📝 ✅ D-G15 — `Source/particle2.cpp:107` — Doc « centres des particules » alors que le rayon est déjà ajouté

La doc annonce le cercle (resp. la boîte, Source/particle2.cpp:121) englobant les centres et propose `GetCircle().Extended(particles.Radius())`, alors que le code renvoie déjà `Circle2(points).Extended(r)` (resp. `Box2(points).Extended(r)`) : suivre l'exemple ajoute le rayon deux fois.
**Correctif :** `\brief Computes a bounding circle of the particles (centers extended by the radius).` et supprimer l'exemple.
**Corrigé :** `ParticleSet2::GetCircle` et `GetBox` documentées comme englobant les particules (centres étendus du rayon, rayon déjà inclus), exemple `GetCircle().Extended(Radius())` supprimé, `sa` croisés.

#### 📝 ✅ D-G16 — `Source/pentagon2.cpp:9` — « Flat topped » alors que le pentagone a un sommet en haut

`vertex[1] = (0, 1)` est en haut et l'arête plate (v3-v4) en bas (cf. `GetBox()` : y ∈ [−0,809 r, r]).
**Correctif :** `\brief Regular pentagon in the plane, with a vertex on top (flat bottom edge).`

#### 📝 ✅ D-G17 — `Source/polygon2.cpp:430` — Doc « distance » pour une distance au carré

La fonction renvoie `line.R(q.at(k))`, soit `Sqr(nc·ap)/(nc·nc)`, distance au carré, alors que la doc annonce « Compute the distance from a polygon to a line ».
**Correctif :** `\brief Compute the squared distance between the polygon and a line (0 if the line crosses the polygon).`

#### 📝 ✅ D-G18 — `Source/rectangle2.cpp:18` — Doc du constructeur ambiguë sur a, b (demi-dimensions) et le centre

La doc (Source/rectangle2.cpp:18-21) dit « centered at the origin » alors que `c` est un paramètre ; le constructeur, `Vertex()` et `GetBox()` traitent `a`, `b` comme des demi-dimensions alors que `Width()` / `Height()` les renvoient comme largeur et hauteur.
**Correctif :** `\param a,b Half width and half height.`, supprimer « centered at the origin », et signaler au responsable de `rectangle.h` l'ambiguïté de `Width()`/`Height()`.

#### 📝 ✅ D-G19 — `Source/revolution-quadric2.cpp:110` — « Signed » renvoie une distance non signée ; doc « intensity »

La fonction renvoie `sqrt(rr)` (≥ 0, distance au profil, sans test d'intérieur) et sa doc parle d'« intensity » ; le nom laisse croire à une distance négative à l'intérieur.
**Correctif :** `\brief Compute the (unsigned) Euclidean distance to the profile curve.` ou implémenter le signe (intérieur si |px| < rayon du profil à la hauteur py).

#### 📝 ✅ D-G20 — `Source/sector2.cpp:114` — /* au lieu de /*! et « circle » au lieu de « sector »

Les blocs des deux surcharges de `Vertex` (Source/sector2.cpp:114 et :126) commencent par `/*` et ne sont pas extraits ; les \brief de `Translate()` (l.63) et `Scale()` (l.73) parlent de « circle ».
**Correctif :** remplacer par `/*!` et corriger « circle » en « sector ».

#### 📝 ✅ D-G21 — `Source/twistsector2.cpp:7` — Commandes Doxygen mal formées

`class CircleWarp2 twist.h` (l.7) n'a pas de `\` ; le second constructeur (l.23) est documenté en `/*` avec un `\param e` inexistant (l.23-30) ; `Warp()` (l.44) commence par `\param Warping function.` au lieu de `\brief`.
**Correctif :** `\class CircleWarp2 twist.h`, `/*!` sans `\param e` pour le constructeur à 5 paramètres, `\brief Warping function.`

### K — Géométrie 3D de base

#### 📝 ✅ D-K01 — `Source/axis.cpp:164` — Paramètre s mal documenté

La doc dit « Signed distance of the projection » alors que `s` reçoit le paramètre normalisé clampé dans [0,1].
**Correctif :** `\param s Returned normalized parameter of the projection, clamped to [0,1].`
**Corrigé :** `param s` de `Axis::R(const Vector&, double&)` : paramètre normalisé du point le plus proche, dans [0,1] (0 : premier sommet, 1 : second) ; description du segment et du `eturn` ajoutée.

#### 📝 ✅ D-K02 — `Source/camera.cpp:283` — Documentation de la caméra trompeuse

`\param a Distance` pour des angles (l.283, 299, 659, 673) ; `UpDown` « Move camera » alors qu'elle tourne (l.205) ; « in degrees » dans `GetAngleOfViewH` qui renvoie des radians (l.387) ; `InsideFrustum(const Box&)` ne teste que le demi-espace devant l'œil (l.320) ; `\param width, height` du constructeur ignorés (l.30) ; `SetCameraApertureH/V`, `SetFocalLength` sans doc (l.642-655).
**Correctif :** corriger les `\param` en « Rotation angle (radians) », préciser le test de demi-espace, documenter les setters.

#### 📝 D-K03 — `Source/cameraframes.cpp:165` — Fonction non documentée et sémantique implicite

Pas de bloc de doc ; avec plus de 2 clés l'interpolation se fait par abscisse curviligne (temps des clés et drapeaux `acc` ignorés, idem `setTimeSmooth` l.274-286), et avec une seule clé c'est `*this` qui est renvoyé.
**Correctif :** ajouter un bloc `/*! ... */` décrivant ce comportement.

#### 📝 ✅ D-K04 — `Source/framebox.cpp:29` — Constructeurs mal documentés

`OrientedBox(const Vector& diagonal, const Frame&)` : « side lengths » alors que `length` est utilisé comme demi-diagonale (Box(−length, length), l.30-31) ; `OrientedBox(c, x, y, z)` (l.38-39) : « given a box and a Frame » alors que ce sont un centre et trois demi-axes.
**Correctif :** `\param diagonal Half side lengths.` ; `\brief Creates an oriented box given its center and three orthogonal half-axes.`
**Corrigé :** `OrientedBox(diagonal, frame)` : « half-size vector » (demi-longueurs des côtés), rief « Creates a unit oriented box » remplacé ; `OrientedBox(c, x, y, z)` : « center and three orthogonal half-axes » (la longueur des vecteurs donne les demi-côtés).

#### 📝 ✅ D-K05 — `Include/rectangle.h:15` — Demi-côtés présentés comme des tailles

`a, b` documentés « Side lengths » (rectangle.h:15) et « Width and height » (l.89), `Rectangles2::Width()/Height()` (l.147, 155), constructeurs (rectangle.cpp:14, 21) alors que `Vertex()` utilise ±a et ±b (demi-côtés) dans Rectangles et Rectangles2.
**Correctif :** documenter « half side lengths » partout (ou renommer HalfWidth/HalfHeight).

#### 📝 ✅ D-K06 — `Source/slab.cpp:19` — Doc du constructeur et de l'épaisseur erronée

« normal direction and two points » alors que les paramètres sont un plan et une épaisseur ; `e` est une demi-épaisseur documentée « Thickness » (slab.h:9, slab.cpp:22).
**Correctif :** `\brief Create a slab centered on a plane. \param t Half thickness.`

#### 📝 ✅ D-K07 — `Source/ktope.cpp:183` — « Minkowski sum » alors que le code calcule l'enveloppe de l'union

`Min(c[i], b.c[i])` sur les deux plans de chaque paire donne le plus petit k-tope contenant les deux, pas la somme de Minkowski (`c[i] + b.c[i]`), incohérent avec `Box::operator+`.
**Correctif :** corriger la doc (« bounding k-tope of the union ») ou implémenter `ktope.c[i] = c[i] + b.c[i]`.

#### 📝 ✅ D-K08 — `Source/simplex.cpp:24` — Valeur retournée mal décrite

La fonction renvoie les vecteurs point − coin dans l'espace de la grille (x0 − i1 + G3, …), pas les sommets du tétraèdre en coordonnées monde.
**Correctif :** documenter « Returns the offsets from the four simplex corners to the point, in grid units ».

#### 📝 ✅ D-K09 — `Source/quad.cpp:142` — Blocs /* au lieu de /*! ignorés par Doxygen

Blocs de documentation commençant par `/*` simple : Source/quad.cpp:142, Source/quad.cpp:150, Source/quad.cpp:161, Include/rectangle.h:128, Source/segment.cpp:28.
**Correctif :** remplacer `/*` par `/*!`.

#### 📝 ✅ D-K10 — `Source/segment.cpp:11` — Affirmation fausse : Axis n'hérite pas de Segment

« Axis inherits Segment » alors qu'Axis (Include/axis.h:13) est une classe indépendante.
**Correctif :** « Axis is a similar class that also stores the normalized axis vector and its length. »

#### 📝 ✅ D-K11 — `Source/triangle.cpp:256` — Documentation contredisant le code

`InscribedRadius` : la doc parle de « half the ratio of the half perimeter and the surface » alors que le code calcule r = |u×v|/périmètre = aire/demi-périmètre ; `Intersect(const Sphere&)` (l.1011) dit qu'un sommet exactement sur la sphère ne donne pas d'intersection alors que le test `<= r` (l.1033) le compte comme intérieur.
**Correctif :** `\brief ... equal to the area divided by the half perimeter.` ; supprimer ou corriger la phrase sur le cas limite.

#### 📝 D-K12 — `Source/triangle-intersection.cpp:6` — Commentaire sur une option inexistante et retour non documenté

`STRONG_COPLANAR_CHECK` n'apparaît nulle part ailleurs dans le code (aucun test de quasi-coplanarité), et la doc des l.355-368 n'a pas de `\return` ni ne précise que `isectpt1/isectpt2` ne sont pas renseignés quand `coplanar` vaut 1.
**Correctif :** supprimer le commentaire ou implémenter le test (`if (fabs(du0) < epsilon) du0 = 0.0;` …) ; ajouter `\return 1 if the triangles intersect; end points are only set if coplanar == 0.`

### R — Primitives 3D volumiques

#### 📝 ✅ D-R01 — `Source/bubble.cpp:21` — \param dans le désordre

`\param re, ri Interior an exterior radii.` alors que la signature est (c, ri, re).
**Correctif :** écrire `\param ri, re Interior and exterior radii.`

#### 📝 ✅ D-R02 — `Include/cage.h:12` — Commentaire Doxygen mal formé et trompeur

`//! Thickness.` placé après le membre doit être `//!<`, et `t` stocke la demi-épaisseur (constructeur `t(e / 2.0)`).
**Correctif :** écrire `//!< Half thickness.`

#### 📝 ✅ D-R03 — `Source/conetube.cpp:113` — Distance non signée documentée comme signée

`Signed()` renvoie `sqrt(R(p))`, toujours positif (distance à la surface latérale).
**Correctif :** documenter « distance (non signée) à la surface latérale du cône ».

#### 📝 ✅ D-R04 — `Include/cuboctahedron.h:11` — « Radius » ambigu

Les sommets sont r·(±1,±1,0)… : le rayon circonscrit vaut r√2 (= longueur d'arête), r est la demi-arête du cube englobant.
**Correctif :** documenter `//!< Half side of the bounding cube; circumradius is r*sqrt(2).`

#### 📝 ✅ D-R05 — `Include/lens.h:16` — Formule de la hauteur fausse dans le commentaire

`\f$ \sqrt{r^2-l/4} \f$` (lignes 16 et 58) alors que la hauteur vaut √(r² − l²/4) (cf. `Height()`).
**Correctif :** corriger en `\sqrt{r^2-l^2/4}` aux deux endroits.

#### 📝 D-R06 — `Include/lens.h:108` — Le code « équivalent » donné en documentation ne l'est pas

La doc affirme que `Alpha()` équivaut à `acos(Height()/(0.5*Thickness()))` = acos(√(r²−l²/4)/(r−l/2)), alors que le code renvoie `acos(length/(2r))`.
**Correctif :** remplacer par « angle α tel que cos α = l/(2r), l distance entre les centres ».

#### 📝 ✅ D-R07 — `Include/icosahedron.h:45` — Commentaires copiés-collés erronés

`face[20][3]` est documenté « %Array of pentagons » (ce sont des triangles), et `Radius()` (ligne 49) parle de « icosi-dodecahedron ».
**Correctif :** corriger en « Array of triangles » et « icosahedron ».

#### 📝 ✅ D-R08 — `Source/cylinderbox.cpp:6` — Mauvais fichier d'en-tête dans \class

`\class CylinderBox cylinder.h` alors que la classe est déclarée dans cylinderbox.h ; la doc de `GetBox()` (ligne 29) parle de « bounding box of a cylinder ».
**Correctif :** écrire `\class CylinderBox cylinderbox.h` et corriger la doc de `GetBox()`.

#### 📝 ✅ D-R09 — `Source/spheroid.cpp:143` — Commentaire de fonction en /* au lieu de /*!

Le bloc de documentation commence par `/*` : Doxygen l'ignore.
**Correctif :** remplacer par `/*!`.

#### 📝 ✅ D-R10 — `Source/hemisphere.cpp:168` — Doc copiée de Signed()

« Compute the Euclidean signed distance… » pour une fonction booléenne.
**Correctif :** remplacer par « Check if a point is inside the hemisphere. »

#### 📝 ✅ D-R11 — `Source/icosidodecahedron.cpp:8` — Mauvais en-tête et doc de Signed() trompeuse

`\class Icosidodecahedron dodecahedron.h` au lieu de icosidodecahedron.h ; la doc de `Signed()` (ligne 247) parle de « icosahedron » et annonce une approximation (intersection dodécaèdre/icosaèdre) non implémentée.
**Correctif :** corriger l'en-tête et la description de `Signed()`.

#### 📝 ✅ D-R12 — `Source/icosahedron.cpp:245` — \param manquants, doc dupliquée

Les deux constructeurs (lignes 237 et 245) ont la même brève et ne documentent que `n` (ni `c` ni `r`) ; `Subdivide()` (ligne 264) n'a pas de doc ; les sommets y sont normalisés (rayon r) alors qu'`Icosahedron::Vertex()` donne un rayon r·R2.
**Correctif :** compléter les \param, documenter `Subdivide()` et la convention de rayon.

#### 📝 ✅ D-R13 — `Source/particle.cpp:93` — Doc copiée de la version 2D

La doc parle de « bounding circle », de `GetCircle()` et donne un exemple avec `Circle2` (ligne 98) ; `GetBox()` renvoie à `GetCircle()` (ligne 109) et `Intersect(const Sphere&)` (ligne 58) parle de « circle ».
**Correctif :** remplacer par Sphere / `GetSphere()`.

#### 📝 ✅ D-R14 — `Source/revolution-quadric.cpp:125` — Doc « intensity » pour une distance non signée

La brève dit « Compute the intensity at a given point » alors que la fonction renvoie la distance euclidienne (toujours positive) à la surface.
**Correctif :** remplacer par « Compute the (unsigned) Euclidean distance to the surface. »

#### 📝 ✅ D-R15 — `Source/scales.cpp:6` — Classe documentée sous un nom inexistant

`\class Scales scales.h` alors que la classe s'appelle `Lune` ; la doc de `GetBox()` (ligne 51) renvoie à `Scales::GetCylinder()`, inexistant.
**Correctif :** écrire `\class Lune scales.h` et supprimer le renvoi.

#### 📝 ✅ D-R16 — `Source/sector.cpp:34` — Docs copiées de Circle

« bounding box of a circle » (ligne 34), « Translates a circle » (ligne 55), « Uniformly scales a circle » (ligne 65).
**Correctif :** remplacer « circle » par « sector ».

#### 📝 ✅ D-R17 — `Source/sphere.cpp:866` — n documenté comme valeur renvoyée

`\param n Returned normal of the plane.` alors que `n` est un paramètre d'entrée (`const Vector&`) : normale du plan contenant les deux centres, orthogonale à (b − a).
**Correctif :** corriger la description de `n`.

#### 📝 ✅ D-R18 — `Source/sphereset.cpp:242` — Doc copiée de Size()

« Returns the number of Spheres. » pour une fonction qui renvoie la i-ème sphère ; `\param i` manque.
**Correctif :** écrire « Returns the i-th sphere. » et ajouter `\param i`.

#### 📝 ✅ D-R19 — `Source/sun.cpp:101` — Unité des angles renvoyés contredite par le code

La doc annonce des degrés, alors que `azim` (atan2 + π) et `altit` (asin) sont en radians.
**Correctif :** documenter « radians » (ou convertir avec `degs`).

#### 📝 ✅ D-R20 — `Source/taper.cpp:28` — Commentaire inversé sur les vecteurs radial/axial

« Radial and axis vector y and x respectively » (lignes 28 et 45) alors que `Axis::Radial(p, y, x)` renvoie l'axial dans `y` et le radial dans `x`.
**Correctif :** corriger le commentaire (ou renommer en `av`, `rv`).
**Corrigé :** commentaires de `Taper::Transform` et `Taper::InverseTransform` : « Axial and radial vectors y and x respectively » (confirmé par `Axis::Radial(p, av, rv)`, `av` axial et `rv` radial) et commentaire de l'appel : coordonnée axiale du point et vecteurs axial et radial. Vérifié par compilation (`g++ -fsyntax-only -Wall`).

#### 📝 ✅ D-R21 — `Source/torus.cpp:282` — Doc copiée du dodécaèdre

« Generate a random vector inside the dodecahedron. »
**Correctif :** remplacer « dodecahedron » par « torus ».

#### 📝 ✅ D-R22 — `Source/twist.cpp:182` — Exemple de code faux

`t.GetCylinder().GetBox(Box::Unit)` ne compile pas (`GetCylinder` prend un Box, `Cylinder::GetBox()` n'en prend pas) ; la doc d'`InverseTransform()` (ligne 58) dit « Transforms a point ».
**Correctif :** écrire `t.GetCylinder(Box::Unit).GetBox()` et corriger la doc d'`InverseTransform()`.

### C — Courbes et surfaces paramétriques

#### 📝 ✅ D-C01 — `Include/clothoid.h:23` — « Curvilinear abscissa t » trompeur : t n'est l'abscisse curviligne que si B = 1/π

`Vertex(t) = πB·(C(t),S(t))` a une vitesse πB, mais la doc (header et clothoid.cpp:70, 90) présente t comme l'abscisse curviligne dans tous les cas.
**Correctif :** documenter que t est le paramètre de Fresnel et que l'abscisse curviligne vaut s = πB·t.

#### 📝 D-C02 — `Source/curvequadric.cpp:173` — Doc « signed squared distance » fausse

La fonction renvoie `sqrt(R(p,u))`, c'est-à-dire la distance euclidienne positive, ni signée ni au carré.
**Correctif :** `\brief Compute the (positive) Euclidean distance between a point and the curve.`

#### 📝 ✅ D-C03 — `Source/curvequadric.cpp:344` — Doc « length of the cubic curve » sur une quadrique ; \param n absent en 2D

La doc parle de courbe cubique pour une quadrique, et `\param n` manque dans la version 2D (Source/curvequadric2.cpp:352).
**Correctif :** remplacer « cubic » par « quadric » et ajouter `\param n Discretization.`

#### 📝 ✅ D-C04 — `Source/curvecubic.cpp:82` — Formule 2D dans la doc d'une courbe 3D

La formule ne contient que dx/dt et dy/dt alors que le code intègre aussi dz/dt (même motif Source/curvequadric.cpp:313).
**Correctif :** ajouter `+ (dz/dt)^2` dans la formule.

#### 📝 D-C05 — `Source/curvequadric.cpp:197` — Commande Doxygen \warn inexistante

Doxygen ne reconnaît pas `\warn` et l'avertissement n'apparaît pas dans la documentation générée (même motif Source/curvequadric2.cpp:179).
**Correctif :** `\warning`.

#### 📝 ✅ D-C06 — `Source/curvecubic.cpp:263` — Commentaire « Quartic constant coefficients » pour un polynôme de degré 5

Les coefficients calculés sont ceux de la dérivée de degré 5 de la distance au carré (même motif Source/curvecubic2.cpp:309).
**Correctif :** `// Constant coefficients of the quintic (C(t)-p)·C'(t)`.

#### 📝 D-C07 — `Source/curvecubic2.cpp:553` — « Solve quadric equation » pour une cubique ; ordre des profondeurs non documenté

Les racines u ne sont pas triées, donc les profondeurs t renvoyées non plus, et un ancien code de tri subsiste en commentaire (l.672-731, code mort).
**Correctif :** corriger le commentaire, documenter que t n'est pas trié (ou trier avec `Math::Sort`) et supprimer le bloc commenté.

#### 📝 ✅ D-C08 — `Source/curvequadric2.cpp:460` — « \param brush … should the triangle be filled » : brush non utilisé, aucun triangle

Le paramètre `brush` n'est pas utilisé et la fonction ne dessine aucun triangle.
**Correctif :** `\param brush Not used.`

#### 📝 D-C09 — `Include/curve.h:355` — Exemple de code avec un type inexistant CubicCurve3

L'exemple de la doc utilise le type `CubicCurve3`, qui n'existe pas.
**Correctif :** `CubicCurve c3 = CubicCurve(c[0], c[1], z);`

#### 📝 D-C10 — `Include/curve.h:417` — Doc « Access to curve vertices » pour l'accès au drapeau de lissage

Le commentaire décrit l'accès aux sommets alors que la fonction accède au drapeau de lissage.
**Correctif :** `//! Access to the smooth flag of the i-th frame.`

#### 📝 ✅ D-C11 — `Include/tubular.h:56` — Doc « Gets the external radius » pour le rayon interne

Le commentaire décrit le rayon externe alors que la fonction renvoie le rayon interne.
**Correctif :** `//! Gets the internal radius.`

#### 📝 ✅ D-C12 — `Source/bezier-curve.cpp:43` — « Rotates a patch » pour une courbe

Le `\brief` parle d'un patch alors que la fonction tourne une courbe (même motif Source/bezier-curve2.cpp:60).
**Correctif :** `\brief Rotates the curve.`

### S — Champs scalaires et grilles

#### 📝 ✅ D-S01 — `Source/algebraic.cpp:178` — Doc décrivant le cas bas degré

La doc de `HighGradient` dit « high degree (quadric, cubic and quartic) polynomial » alors que la fonction n'est appelée que pour n > 4 ; les constructeurs (l.23, 48) parlent d'un « array pointer null » inexistant ; `Terms()` renvoie 0 pour le constructeur par défaut alors que `c` contient 1 coefficient.
**Correctif :** « Gradient of a polynomial of degree greater than 4, computed with loops. » et retirer la mention de pointeur.
**Corrigé :** `HighGradient` : « Gradient of a polynomial of degree greater than 4 » (avec `sa LowGradient`) ; mention du pointeur nul retirée des deux constructeurs (le second précise que le tableau est redimensionné à (n+1)(n+2)(n+3)/6 termes) ; `Terms()` incohérent pour le constructeur par défaut : `size = 1` y est maintenant affecté (polynôme de degré 0, un coefficient nul), doc complétée. Vérifié par compilation (`g++ -fsyntax-only -Wall`).

#### 📝 D-S02 — `Source/array2.cpp:341` — Convention QRect non standard et non documentée

`QRect(0, 0, nx − 1, ny − 1)` a `right() = nx − 2` ; `ScalarField2`/`VectorField2`/`ScalarHexagonField2` bouclent sur `<= x + width()` alors que `RelaxedArray2::Add` (relaxedarray2.cpp:72-74) boucle sur `<= x + width() − 1` : deux conventions non documentées (aussi array2.cpp:357-360 et Include/array.h:570).
**Correctif :** documenter « the returned rectangle spans vertices [x, x+width] inclusive », ou passer à la convention Qt (`QRect(0, 0, nx, ny)`, `QRect(pa, pb)`) avec des appelants bouclant sur `<= right()`.

#### 📝 D-S03 — `Include/scalarfield.h:164` — Convention de signe contredite par Inside()

La doc dit « sign = true, i.e., negative values inside » mais `AnalyticScalarField2::Inside()` (l.185) renvoie `v > 0.0` pour `sign == true`, à l'inverse de `AnalyticScalarField::Inside()` (l.134).
**Correctif :** aligner la doc sur le code (« positive values inside ») ou inverser le test.

#### 📝 D-S04 — `Include/array.h:265` — Sommet et cellule confondus dans la doc

`VertexIndex` (l.265, 273) et `InverseVertexIndex` (l.281, 296) documentés « index of a given cell », `InsideCellIndex` (l.306) documente i,j comme coordonnées « of the vertex », `VertexInteger(const Vector2&, double&, double&)` (l.462) identique à `CellInteger` sans le dire.
**Correctif :** « Compute the index of a given vertex. » / « Integer coordinates of the cell. ».

#### 📝 ✅ D-S05 — `Source/arrayindex.cpp:6` — Doc de classe inexistante et paramètre ambigu

`\class ArrayIndex arrayindex.h` documente une classe absente de Include/arrayindex.h (seule `DynamicArrayIndex` existe) ; le paramètre `n` (« Subdivison ») est un nombre de sommets.
**Correctif :** supprimer le bloc `\class ArrayIndex` ; `\param n Number of vertices along each axis (n-1 cells).`
**Corrigé :** bloc `ass ArrayIndex` supprimé (classe absente de arrayindex.h, aucune référence dans les dépôts) ; `param n` de `DynamicArrayIndex(const Box&, int)` : nombre de sommets de la grille par axe (n-1 cellules), conforme à `Array(box, n, n, n)` dont la diagonale de cellule est (b-a)/(n-1). Vérifié par compilation (`g++ -fsyntax-only -Wall`).

#### 📝 ✅ D-S06 — `Include/scalarfield.h:425` — Deux surcharges homonymes de sémantique différente

`ScalarField2 MedianFilter() const;` (l.425, renvoie un champ) et `void MedianFilter();` (l.433, en place) ne diffèrent que par `const` : piège silencieux selon la constance de l'objet.
**Correctif :** renommer la version const en `MedianFiltered()`.
**Corrigé :** la version const est renommée `ScalarField2::MedianFiltered()` (scalarfield.h, scalarfield2.cpp) ; `MedianFilter()` reste la version en place ; docs croisées (`sa`). Le seul appelant du dépôt, `m_hf.MedianFilter()` (AppSketch, objet non const), utilise la version en place et n'est pas affecté. Vérifié par compilation (`g++ -fsyntax-only -Wall`).

#### 📝 ✅ D-S07 — `Source/hexagonscalarfield2.cpp:277` — Blocs non Doxygen et docs fausses

Blocs `/*` au lieu de `/*!` (hexagonscalarfield2.cpp:277, integerfield2.cpp:250) ; `Value(int,int)` documenté « Return the field gradient » (hexagonscalarfield2.cpp:473, integerfield2.cpp:153) ; `CreateImage(const LookupPalette&)` sans doc (integerfield2.cpp:194) ; `\class Array2I scalarfield.h` (l.7) au lieu de `arrayinteger.h`.
**Correctif :** `/*!`, « Return the field value at a given array vertex. », ajouter la doc manquante et corriger le header.
**Corrigé :** `/*` remplacés par `/*!` (`ScalarHexagonField2::SmoothPoint`, `Array2I::Crop`) ; `Value(int,int)` : « Return the field value at a given array vertex » (hexagonscalarfield2.cpp, integerfield2.cpp) ; doc ajoutée à `Array2I::CreateImage(const LookupPalette&)` (valeur = indice dans la palette, noir hors palette, confirmé dans `LookupPalette::GetColor`) ; `ass Array2I arrayinteger.h`. Vérifié par compilation (`g++ -fsyntax-only -Wall`).

#### 📝 D-S08 — `Source/scalarfield-mesh.cpp:29` — Exemple de la doc incompatible avec les tables utilisées

La doc cite l'exemple de Bourke (« edgeTable[9] is 905 ») alors que `ScalarField::edgeTable[9]` vaut 2355 et que la numérotation des arêtes diffère (cf. `Box::edge`) ; la phrase sur les ambiguïtés est elle-même ambiguë.
**Correctif :** remplacer l'exemple par un calcul fait avec les tables de scalarfield.cpp et décrire la numérotation des sommets (bit x = 1, y = 2, z = 4) et des arêtes.

#### 📝 D-S09 — `Source/scalarfield-mesh.cpp:334` — Documentation vide ou absente

`OctreeGetSharedDoubleIndex` a un `\brief` vide et parle de « dual point index » pour un cache de valeurs ; `PolygonizeOctree(int, Mesh&, const Box&, const double&)` (l.483) sans doc ; `Octree`/`OctreeCell` et les helpers de scalarfield-wyvill.cpp (`getCubeId`, `computeVertexPosition`, `boxIntersection`) n'ont que des `//`.
**Correctif :** `\brief Return the cached field value at an octree vertex, evaluating it if needed.`, documenter le reste et rendre les helpers `static` (ou namespace anonyme).

#### 📝 D-S10 — `Source/scalarfield.cpp:1704` — Docs absentes, non Doxygen ou fausses

Blocs `/*` l.1704 et 1734, doc de `CurvatureMin` parlant de « Max curvature » ; `EuclideanTransform` (l.1815) sans doc ni convention de signe ; `Save` (l.1650) ne dit pas que les valeurs sont clampées à [−1, 1] et quantifiées sur 8 bits ; `BicubicValue` (l.141) parle de « bernstein coefficients » pour des poids de Catmull-Rom ; `getDualPointCode`/`getSharedDualPointIndex` (l.1126, 1259) sans `\param`.
**Correctif :** `/*!`, « Min curvature », documenter `EuclideanTransform`, la quantification de `Save` et les paramètres manquants.

#### 📝 D-S11 — `Include/scalarfield.h:696` — Tables de marching cubes publiques et modifiables

`static int TriangleTable[256][16]` et `edgeTable[256]` (scalarfield.h:695-696) sont publiques et non `const`, et leur convention d'arêtes (différente de Bourke) n'est documentée nulle part.
**Correctif :** `static const int TriangleTable[256][16];` / `static const int edgeTable[256];` et documenter la numérotation des sommets et des arêtes (cf. `Box::edge`).

#### 📝 D-S12 — `Source/scalarfield2-curvature.cpp:5` — Blocs /* au lieu de /*!

Docs ignorées par Doxygen : scalarfield2-curvature.cpp:5, 38, 71, 105, 137 (`CurvatureProfile`, `Contour`, `Tangential`, `Gaussian`, `Mean`), scalarfield2-smooth.cpp:81 (`SmoothPoint`), scalarfield2.cpp:3331 (`DistanceTransform`) ; signe des courbures non précisé.
**Correctif :** remplacer `/*` par `/*!` et préciser la convention de signe (concave/convexe).

#### 📝 D-S13 — `Source/scalarfield2.cpp:718` — Docs absentes ou fausses

`GetPercentiles` (l.718) et `HistogramTransfer` (l.738) sans doc ; `Cbrt` (l.1831-1838) documentée « quartic root » ; `ScaledNorm` (l.422-427) renvoie sqrt(Σf²)/n avec un commentaire trompeur ; `RotateKernel` (scalarfield2-morph.cpp:71) globale non `static` sans doc, `Convolution` est en fait une corrélation.
**Correctif :** ajouter les docs, « Compute the cubic root », documenter la formule exacte de `ScaledNorm`, rendre `RotateKernel` `static`.

### V — Champs vectoriels et de couleur

#### 📝 ✅ D-V01 — `Source/vectorfield2.cpp:676` — Docs fausses ou incomplètes

`Mul` est documenté « Subtract two vector fields » (l. 676) ; `Lerp` (l. 120-122) décrit `a,b` comme « Scalar fields » et porte `\avx` sans code AVX ; `Divergence(int,int)` commente « Curl along x/y axis » (l. 560, 574) ; `Clone` décrit `radius` comme « Scaling of the effect » (l. 211) ; `CloneBrokenLine` (l. 316-318) n'a aucun `\param` ; `CreateBlueRedImage` est documenté « Create the angle image » (l. 947) ; l'exemple de `Resized` (l. 1349-1350) porte sur `ScalarField`.
**Correctif :** corriger les `\brief` et ajouter les `\param` manquants.

#### 📝 ✅ D-V02 — `Source/vectorfield.cpp:56` — Doc « bi-linear » alors que l'interpolation est trilinéaire

La doc annonce une interpolation bilinéaire (`\sa Math::Bilinear`) alors que le code appelle `Vector::Trilinear`.
**Correctif :** « tri-linear interpolation », `\sa Vector::Trilinear`.

#### 📝 D-V03 — `Include/vectorfield.h:48` — Préconditions de résolution non documentées

Les opérations binaires (`operator+=`, `operator-=`, `Add`, `Sub`, `Lerp`, `operator*=(ScalarField2)` dans Include/vectorfield.h:48-51, 79-80, 89 et Include/colorfield.h:50) supposent des résolutions identiques sans le dire ni le vérifier ; `GetComponent(int)` (Include/vectorfield.h:57) ne précise pas que i∈{0,1}.
**Correctif :** ajouter `\pre` (même résolution) et `\param i 0 for x, 1 for y` ; `assert` dans les implémentations.

### N — Bruit et échantillonnage

#### 📝 ✅ D-N01 — `Source/phasor.cpp:5` — Documentation absente ou fausse pour les bruits phasor

**Résolu par suppression :** PhasorEric (Source/noise-phasor.cpp) et PhasorGundy (Source/phasor.cpp) ont été supprimés ; la nouvelle classe `Phasor2` (`Source/phasornoise2.cpp`) est entièrement documentée (formule, référence Tricard et al. 2019, paramètres).

`phasor_noise()` documente un `\param f` inexistant (l.8) ; `PhasorGundy::Value()` (l.94) n'a aucune doc (plage [0,1] non indiquée) ; Source/noise-phasor.cpp:6 porte `\class Phasor` (inexistante) et les méthodes de `PhasorEric` (Source/noise-phasor.cpp:12, 21, 33, 70, 103, 122, 150) ont un `\brief` vide sans `\param` ; `PhasorGundy` n'a pas de bloc `\class`.
**Correctif :** corriger en `\class PhasorEric`, ajouter `\class PhasorGundy`, renseigner les `\brief`/`\param` et retirer `\param f`.

#### 📝 D-N02 — `Source/noise-gabor.cpp:20` — w0 documenté comme « pulsation » alors que c'est l'orientation

`w0` n'est utilisé que via `cos(w0)`/`sin(w0)` comme direction du porteur (l.63) : c'est l'angle ω0 de Lagae et al., la fréquence étant `F_0` ; même erreur pour `c0, s0` (Include/gabornoise.h:17), et `Value()`/`Variance()` ne précisent pas l'échelle.
**Correctif :** `\param w0 Orientation angle of the sinusoidal carrier (radians).` et corriger le commentaire de `c0, s0`.

#### 📝 D-N03 — `Source/noise-voronoi.cpp:22` — Docs de classes et de retour incorrectes

Les `\class CellularNoise noise.h` (l.23, l.64) et `\class VoroNoise noise.h` (l.168) citent le mauvais header (`voronoise.h`), le premier précède `CellularNoise2` (sans doc de classe) ; `VoroNoise` est décrit « Worley Noise » alors qu'il renvoie la distance au bord de cellule ; `CellVertex(int,int,int)` (l.139) annonce des coordonnées locales alors qu'elles sont absolues ; plages non documentées ; en-tête de fichier « Gabor Noise ».
**Correctif :** corriger les `\class`, la description de VoroNoise, le `\return` et l'en-tête de fichier.

#### 📝 D-N04 — `Source/noise.cpp:7` — Doc en /* et plages de valeurs fausses

Doc de `Hash1d` en `/*` (Source/noise.cpp:7, Source/noise2.cpp:7), `Hash2d/Hash3d/IncrSum/GradientVertex...` non documentés ; les commentaires « Range at -0.5, 0.5 » (Source/noise.cpp:149, :286, :427, Source/noise2.cpp:114) suivis de `+ 0.5` laissent croire à une plage [0,1] non garantie ; `Noise::AtVector()` ignore `quintic` sans le dire.
**Correctif :** passer en `/*!`, documenter la plage réelle (« approximativement [0,1] ») et le comportement de AtVector.

#### 📝 ✅ D-N05 — `Source/turbulence2.cpp:144` — Translation t exprimée en unités de longueur d'onde, non documenté

Pour les turbulences simplex/cellulaire, `t` est soustrait après division par `l0` (Source/turbulence2.cpp:209, :288, Source/turbulence.cpp:236), alors que `NoiseTurbulence(2)::Value()` le soustrait en espace monde (Source/turbulence.cpp:39) ; `CellularTurbulence2::Value()` (l.274), `GetAmplitude/GetWavelength` non documentés ; `NoiseTurbulence2` prend un `Vector` 3D.
**Correctif :** préciser l'unité de `t` dans les `\param` et documenter les méthodes manquantes.
**Corrigé :** `param t` précisé dans les constructeurs de `NoiseTurbulence(2)` (unités des points, soustrait avant la division par la longueur d'onde ; seules les deux premières coordonnées servent en 2D) et de `SimplexTurbulence(2)`/`CellularTurbulence2` (unités de la longueur d'onde de base, translation de t·l dans les unités des points) ; `UnitAt` documenté (point déjà divisé par la longueur d'onde) ; docs ajoutées pour `GetAmplitude`/`GetWavelength` (2D et 3D) et `CellularTurbulence2::Value`.

#### 📝 ✅ D-N06 — `Source/discset.cpp:244` — Nom trompeur et \param absent

`GetSphere(int)` renvoie un `Disc2` et n'a pas de `\param i` ; `R(const Vector2&)` renvoie une distance au carré et `R(const DiscSet2&)` une distance signée non au carré, sans distinction dans Include/discset.h:33.
**Correctif :** renommer en `GetDisc(int)`, ajouter `\param i` et préciser l'unité de chaque `R`.
**Corrigé :** `DiscSet2::GetSphere(int)` renommée `GetDisc(int)` (déclaration dans discset.h ; aucun appelant dans les dépôts), `param i` (indice dans [0,Size()[) et `eturn` ajoutés ; doc de `R(const DiscSet2&)` complétée : distance signée non élevée au carré, à la différence de `R(const Vector2&)`.

#### 📝 D-N07 — `Source/discsystem.cpp:96` — \param et \return absents, constantes non documentées

`Closest()` n'a ni `\param id, cc, ll` ni `\return` ; `Steps(int)`/`Cast()` ne signalent pas les suppressions/scissions de particules ; `Alpha` et `WH_NY` (Include/discsystem.h:98, :104) ne sont pas commentés.
**Correctif :** compléter les `\param`/`\return` et commenter les constantes.

#### 📝 D-N08 — `Source/disctilelink.cpp:154` — Docs en /*, fausses ou absentes

Les trois constructeurs sont documentés en `/*` (l.154, l.208, l.249) ; `Memory()` est décrite « Get the code of the tile » (l.13) ; `AddD()` annonce un tri décroissant alors qu'il est croissant, sans `\param` (l.76-80) ; `Draw()` (l.303) sans doc ; `TrucMuch` non descriptif ; `index[size] = { -1 }` (Include/sampling.h:305-306) n'initialise que le premier élément à -1.
**Correctif :** passer en `/*!`, corriger les `\brief`, renommer `TrucMuch` (p. ex. `NeighborhoodPoints`).

#### 📝 D-N09 — `Source/disctiles.cpp:8` — Docs de classes en /* et méthode de debug non documentée

`\class DiscTiles` (l.8) et `\class DiscTileCorner` (l.227) en `/*` ; `DrawDebug()` (l.191) non documentée alors qu'elle écrit `../tile1.pdf`…`../tile3.pdf` ; `DiscTileCorner::Packing()` se cite elle-même (`\sa`, l.464) ; `Sample(const Box2&)` évoque une fonction d'importance inexistante (l.145, l.429).
**Correctif :** passer en `/*!`, documenter les effets de bord de `DrawDebug`, corriger les `\brief`/`\sa`.

#### 📝 D-N10 — `Source/dla.cpp:43` — Doc incohérente

`Add(int n)` est décrite « Add one particle » alors qu'elle en ajoute n ; `Try()` n'a pas de `\return` ; `Dla::random` (Include/dla.h:28) n'est jamais utilisé (Try utilise `Sphere::RandomNormal()`/`RandomSurface()`), laissant croire la graine contrôlable ; `Dla2::Walk()` (Source/dla2.cpp:146) n'a pas de `\return`.
**Correctif :** corriger le `\brief`, ajouter les `\return`, supprimer ou utiliser `random`.

#### 📝 D-N11 — `Source/eden2.cpp:435` — Fonctions non documentées

`Debug()` (l.435), `Probability()` (l.665, paramètre q inutilisé), `EdenGrowth::Ancestor()`/`Select()` (Source/eden.cpp:194, :207) sans doc ; `Multi()` impose « m ≤ 16 » sans justification ; `Topology()` ne décrit pas son format ; le membre `sphere` (Include/eden.h:87) est commenté « Bounding circle ».
**Correctif :** ajouter les blocs `/*!` avec `\param`/`\return` et corriger les commentaires.

#### 📝 D-N12 — `Source/hexagonarray2.cpp:159` — Docs copiées-collées ou vides

`GetHexagonBox()` (doc l.159-161) reprend « smaller than the bounding box of the hexagons » alors qu'elle renvoie justement cette boîte ; `TriangleInteger()` décrit (u,v) comme coordonnées « dans le triangle » alors que ce sont les parties fractionnaires axiales ; dans Include/hexagonarray.h, `Width()` (l.152), `Height()` (l.160), `HexagonPoint::operator+=`/`-=` (l.239, l.247) ont un `\brief` vide et `HexagonPoint(int, int, int)` (l.212) ignore y sans le dire.
**Correctif :** corriger/compléter les `\brief` et documenter la convention axiale (x, z).

#### 📝 ✅ D-N13 — `Source/hexagonal.cpp:34` — La doc cite Hexagon2::R au lieu de Hexagon2::Signed

La doc de `Signed()` indique « Internally, call Hexagon2::R(const Vector2&) » alors que la fonction appelle `Hexagon2::Signed()` (l.39) ; coquille « Comput » dans `GetBox()` (l.95).
**Correctif :** `Internally, call Hexagon2::Signed(const Vector2&).` et corriger « Compute ».
**Corrigé :** `Hexagonal::Signed` : « Internally, call Hexagon2::Signed(const Vector2&) » ; coquille « Comput » corrigée dans `GetBox()`.

#### 📝 ✅ D-N14 — `Source/relaxedarray2.cpp:220` — \brief tronqué ou erroné

**Corrigé :** `\brief` de `Sparsity()` et de `Add(const Capsule2&)` corrigés, exemple de classe rendu compilable (vérifié), et couplage avec `Array2::VertexIntegerArea()` documenté dans la doc de classe ; le code n'utilise pas `CellIntegerArea()` (comportement inchangé).

« Compute the sparsity of the . » (l.220) ; « captule »/« triangle » dans la doc de `Add(const Capsule2&)` (l.196, l.210) ; exemple de classe non compilable (l.17) ; l'usage de `Array2::VertexIntegerArea()` pour des indices de cellules ne marche que grâce au masque de largeur nx-1, couplage non documenté.
**Correctif :** corriger les `\brief`, l'exemple, et utiliser/documenter `CellIntegerArea()`.

#### 📝 D-N15 — `Source/sampling2.cpp:519` — Docs en /*, copiées-collées ou fausses

`Sample(const Circle2&)` en `/*` (l.519) et, comme `Sample(const Box2&)` (l.490), décrite avec une fonction d'importance inexistante ; `Memory()` décrite « Get the code of the tile » (l.615) ; `Packing()` se cite elle-même (l.576) ; `SphereTile::Generate()` (Source/sampling.cpp:129) annonce n tirages mais boucle de 1 à n-1 ; `SphereTile::a` (Include/sampling.h:145) commenté « Size » ; `Relaxation(int, const double&)` (Source/sampling2.cpp:97) a ses `\param` dans le désordre.
**Correctif :** passer en `/*!` et corriger les `\brief`/`\sa`/`\param`.

### T — Maillages et graphes géométriques

#### 📝 ✅ D-T01 — `Source/mesh.cpp:329` — Nombre de sommets/triangles faux

La doc annonce `2n(n-1)+2` sommets et `4n(n-1)` triangles (copiée de la sphère) alors que le code alloue `s = p*(2*(n/4)+m)+2` sommets.
**Correctif :** documenter `p(2⌊p/4⌋+m)+2` sommets et `2p(2⌊p/4⌋+m-1)+2p` triangles.

#### 📝 ✅ D-T02 — `Source/mesh.cpp:1148` — « Creates the mesh of an octaedron »

Doc copiée de l'octaèdre ; par ailleurs `normals(6)` alloue 6 normales dont seules 5 sont remplies (la 6e reste nulle).
**Correctif :** `\brief Creates the mesh of a pyramid.` et `normals(5)`.

#### 📝 D-T03 — `Source/mesh.cpp:3392` — Formule LaTeX cassée et facteur 1/n_i absent du code

`\f[$]` n'est pas une syntaxe Doxygen valide ; la formule annonce une moyenne `1/n_i` mais le code ne divise pas par la valence (simple somme).
**Correctif :** `\f[` ... `\f]` et retirer `1/n_i` (ou diviser par la valence dans le code).

#### 📝 ✅ D-T04 — `Source/mesh.cpp:547` — Fonction protégée sans documentation

`FindEdge(int, int, int&, int)` (et `Mesh2::FindEdge` mesh2.cpp:850, `Mesh::EdgeCollapse(int)` ligne 622, `Mesh2::EdgeCollapse(int)` mesh2.cpp:721) n'ont aucun bloc `/*!` alors que la sémantique de `k` (sortie) et `s` (début de recherche) n'est pas évidente.
**Correctif :** ajouter `/*! \brief Find half-edge a->b starting at s. \param k Output index. */`.
**Corrigé :** blocs Doxygen ajoutés pour `Mesh::FindEdge`, `Mesh::EdgeCollapse(int)`, `Mesh2::FindEdge` et `Mesh2::EdgeCollapse(int)` (sémantique de `k` en sortie et de `s` début de recherche, effet de l'effondrement, arête de bord sans effet).

#### 📝 ✅ D-T05 — `Source/mesh2.cpp:254` — Fonction publique non documentée

Aucune doc pour la forme alpha (critère : rayon circonscrit > `radius`, triangle supprimé).
**Correctif :** ajouter `/*! \brief Alpha shape... \param points ... \param radius ... */`.
**Corrigé :** bloc Doxygen de `Mesh2::Alpha` (forme alpha : triangulation de Delaunay filtrée par le rayon circonscrit, ordre des triangles non conservé, maillage vide sous trois points).

#### 📝 ✅ D-T06 — `Source/mesh2.cpp:293` — Complexité annoncée O(n log n) fausse

L'implémentation teste tous les triangles pour chaque point et dédoublonne les arêtes en O(E²) : complexité O(n²) au mieux.
**Correctif :** `O(n^2) complexity`.

#### 📝 ✅ D-T07 — `Source/mesh2.cpp:517` — « Translate a mesh » pour Scale

Le `\brief` de `Scale` est copié de `Translate` ; de même Source/mesh2.cpp:995 `SubMesh(const Circle2&)` dit « outside the given polygon ».
**Correctif :** `\brief Scale a mesh.` / `outside the given circle`.
**Corrigé :** `rief Scale a mesh.` pour `Mesh2::Scale(const Vector2&)` ; `SubMesh(const Circle2&)` : « outside the given circle ».

#### 📝 ✅ D-T08 — `Include/mesh.h:536` — « as a 3D Vector » pour un Vector2

Le `\return` décrit un vecteur 3D alors que la fonction renvoie un `Vector2`.
**Correctif :** `\return The wanted vertex.`
**Corrigé :** `eturn The wanted vertex.` pour `Mesh2::Vertex(int)` (mesh.h) ; les trois autres occurrences (« 3D Vector », l.304, 350, 361) concernent `Mesh` et sont exactes.

#### 📝 ✅ D-T09 — `Include/vectorset.h:14` — Commentaire « // Sphere class »

Les commentaires lignes 14 et 120 annoncent « Sphere class » au-dessus de `VectorSet` et `VectorSet2`, et aucune classe n'a de bloc `\class`/`\brief` dans le header.
**Correctif :** `// Set of vectors` / `// Set of 2D vectors`.

#### 📝 ✅ D-T10 — `Include/polygon.h:294` — Fonctions inline sans documentation

`operator[](int)` (lignes 294 et 299) et `Empty()` (ligne 321) n'ont pas de bloc `/*!`.
**Correctif :** ajouter `/*! \brief Access to the i-th vertex. \param i Index. */` et `/*! \brief Check if the polygon has no vertex. */`.

### U — Utilitaires, couleur, GPU

#### 📝 D-U01 — `Source/color.cpp:249` — Docs en /* au lieu de /*! et contenus faux

Blocs `/*` ignorés par Doxygen (Source/color.cpp:249, 258, 280, 297) ; `Wheel` mentionne l'espace « hue-saturation-value » alors que le code utilise HSL ; `Darken` (Source/color.cpp:290) indique « Coefficient added to luminance in Hsl space » alors que le code fait un mélange linéaire RGB vers le noir.
**Correctif :** passer en `/*!`, corriger : Wheel → HSL, Darken → « \param c Coefficient in [0,1], linear blend toward black ».

#### 📝 D-U02 — `Include/color.h:85` — Doc trompeuse (« spectrum », « default=last ») ; plage de Hsl non documentée

Les deux `operator[]` (Include/color.h:85-101) parlent de « channel of the spectrum » et d'une valeur « default=last » inexistante ; la classe `Hsl` (Include/color.h:292) n'a pas de doc et `Hsl::Hsl(h,s,l)` (Source/color-hsl.cpp:11-16) ne précise pas que h est dans [0,1] (et non en degrés).
**Correctif :** « Returns the i-th component (0=red, 1=green, 2=blue, 3=alpha) » ; préciser `\param h Hue in [0,1]`.

#### 📝 D-U03 — `Source/analyticpalette.cpp:449` — Fonctions non documentées, liste des palettes incomplète

Aucune doc pour `WhiteBrown` (l. 449), `GreenOrange` (l. 454), `Inigo` (l. 466, domaine signé de u et rôle de f non précisés), `LookupPalette` (l. 475-485), `GenericPalette::Draw(QGraphicsScene&, const Box2&)` (l. 97) ; la liste de la doc de classe (l. 120-160) s'arrête à l'entrée 10 alors que les identifiants 11-13 existent.
**Correctif :** ajouter les blocs `/*!` et compléter la liste des palettes 11 (White-Brown), 12 (Green-Orange), 13 (Inigo).

#### 📝 D-U04 — `Include/histogram.h:61` — Docs fausses ou incomplètes

`At()` (Include/histogram.h:61-64) est documenté « Compute the entry of a given value » alors qu'il renvoie le centre du bac i ; `Value()` (Include/histogram.h:96-98) n'a pas de `\param i` ; `ExportPDF/ExportPNG` (Source/histogram.cpp:80-94) sont documentés « Draw. » sans `\param filename` ; le membre `e` (Include/histogram.h:13) est commenté en `//` au lieu de `//!`.
**Correctif :** « Returns the center of the i-th bin » ; ajouter les `\param` manquants et passer `e` en `//!`.

#### 📝 D-U05 — `Source/chart.cpp:4` — Classe et méthodes non documentées

Aucun bloc Doxygen dans `chart.h` ni `chart.cpp` (classe, `AddLine`, `AddBars`, `SetLog`, `ExportPDF`…), et l'interdiction de mélanger barres et courbes n'est pas documentée.
**Correctif :** ajouter un `\class DrawChart` et des `/*!` par méthode publique.

#### 📝 ✅ D-U06 — `Source/cpu.cpp:108` — Doc contredite : la fonction lève une exception

La doc dit « Returns an empty string if the resource has not been found » alors que le code lève `std::runtime_error` (Source/cpu.cpp:141) ; le `return QString()` final (Source/cpu.cpp:149) est inatteignable.
**Correctif :** documenter `\throw std::runtime_error` (ou renvoyer `QString()` comme annoncé).

#### 📝 D-U07 — `Source/cpu.cpp:152` — Docs /* et paramètres faux

`/*` au lieu de `/*!` (Source/cpu.cpp:152, 170, 183, 194, 218, 242) ; `MsToMinutes` ne documente pas `min, s, ms` ; `DateTime` (l. 246) documente un `\param n` inexistant ; `Avx` (l. 54) un `\param a` inexistant ; `Compose` (l. 268, 282) utilise `\parame` sans nom ; `LongInteger/VeryLongInteger` annoncent un séparateur espace alors que le code utilise des points ; `SetAvx/Avx` sans `\brief`.
**Correctif :** passer en `/*!` et corriger les `\param` et `\brief`.

#### 📝 D-U08 — `Source/draw.cpp:13` — Paramètres documentés inexistants ou absents

`CreateVector` documente `\param size` inexistant (Source/draw.cpp:13) ; `CreateImage(scene, name, width, rect)` ne documente ni `width` ni `rect` ; `CreateImage(scene, width, rect)` documente `\param name` inexistant (Source/draw.cpp:49) ; la classe `Draw` n'a pas de doc.
**Correctif :** corriger les `\param` (width : largeur max en pixels ; rect : zone de scène, nulle = sceneRect) et documenter la classe.

#### 📝 ✅ D-U09 — `Source/system.cpp:14` — Docs fausses ou absentes

`SavePng` est documenté « Save a scene as a JPG file » (Source/system.cpp:14) ; `FlipHorizontal` « Flip … vertically » (l. 179) ; `SaveSvg` (l. 110) n'a pas de doc ; `RecurseDirectory/CodeLines` ne documentent pas `code`, `glsl` ; `SaveImage` documente `image` comme « The image box ».
**Correctif :** corriger les `\brief` et ajouter les `\param` manquants.

#### 📝 D-U10 — `Source/gpu-texture.cpp:147` — Unités et sémantique des paramètres fausses

`size` est documenté « in bits » (Source/gpu-texture.cpp:147, 253) alors que GL attend des octets (les templates passent `size*sizeof(T)`) ; dans Include/gpu-texture.h:89-99, `a, b` sont décrits comme « Pixel coordinates of the image subset » alors que le code (Source/gpu-texture.cpp:207) utilise `b` comme largeur/hauteur ; `Generate`, `Destroy`, `Bind`, `SetData(ScalarField2)` non documentés.
**Correctif :** « size Size of the buffer in bytes » ; « a Offset, b Size (width, height) of the subset » ; documenter les méthodes manquantes.

#### 📝 D-U11 — `Include/gpu-wrapper.h:7` — Absence de documentation de l'API GPU

Aucune doc pour `GLBuffer`, `GLShader` (Include/gpu-wrapper.h), les fonctions `read_program`… (Include/gpu-shader.h:12-17, notamment les valeurs de retour 0/-1), `glDebugOutput`, `checkGLErrors`, `getErrorMsg` (Source/gpu-debug.cpp:70-140) ; le fait que `read_program` laisse le programme actif (`glUseProgram`) n'est pas documenté.
**Correctif :** ajouter des blocs `/*!` (classe, rôle, propriété du handle, codes de retour).

---

## Optimisations ⚡

### G — Géométrie 2D

#### ⚡ O-G01 — `Source/quad2.cpp:308` — Formule de Bretschneider coûteuse et fragile

La fonction calcule 6 normes (6 racines) puis une racine finale, et la quantité sous la racine peut devenir négative par arrondi (NaN pour un quadrangle dégénéré) ; la formule des diagonales, exacte pour un quadrangle simple, évite toute racine (gain : 7 sqrt supprimées, robustesse accrue).
**Correctif :** `return 0.5 * fabs((p[2] - p[0]) / (p[3] - p[1]));`

### K — Géométrie 3D de base

#### ⚡ ✅ O-K01 — `Source/plane.cpp:227` — Déterminant 3×3 calculé via une Matrix4

Le déterminant passe par une `Matrix4` alors que le produit mixte `na * (nb / nc)` suffit (et `nb / nc` est recalculé l.234) ; fonction appelée O(n³) fois par `ConvexPoints`, gain notable sur ce chemin.
**Correctif :** `Vector bc = nb / nc; double e = na * bc; ... Vector p = (a.Coeff() * bc + b.Coeff() * (nc / na) + c.Coeff() * (na / nb)) / e;`

#### ⚡ O-K02 — `Source/tetrahedra.cpp:354` — Arêtes et normales d'arêtes recalculées à chaque requête

Les arêtes normalisées et les 12 normales d'arêtes (6 racines carrées) sont recalculées à chaque appel (l.354-378, 526-550) alors que `R()`, très sollicitée pour les champs de distance, passe par `Normal()` ; `e23` (l.41) est calculé pour rien dans le constructeur. Gain : suppression de 6 sqrt et 12 produits vectoriels par requête.
**Correctif :** précalculer et stocker les arêtes unitaires (ou les normales d'arêtes) dans le constructeur.

### S — Champs scalaires et grilles

#### ⚡ O-S01 — `Source/analyticscalarfield.cpp:608` — Parcours non contigu en mémoire

`VertexIndex(i,j,k) = (i*ny + j)*nz + k` mais les boucles sont ordonnées k, j, i avec i le plus interne (pas de ny·nz) ; idem l.948-954 avec `CellIndex`.
**Correctif :** ordonner les boucles i, j, k (k le plus interne) ; gain attendu : accès contigus, nettement moins de défauts de cache sur les grandes grilles.

#### ⚡ O-S02 — `Source/scalarfield-mesh.cpp:464` — Sous-cellule construite deux fois, cache par std::map

`OctreeCell oci = c.Sub(i);` est inutilisé et `c.Sub(i)` recalculé ensuite ; les caches `std::map<int64_t, …>` coûtent O(log n) par accès.
**Correctif :** `cells.push(c.Sub(i));` seul et `std::unordered_map` pour `medges`/`mvertexes` ; gain attendu : accès cache en O(1) moyen et une construction de cellule en moins par sous-cellule.

#### ⚡ O-S03 — `Source/scalarfield.cpp:1492` — Lecture des plans z à pas ny·nz

Chaque plan est lu par `at(i, j, k)` avec j en boucle interne : chaque accès saute de nz doubles (même motif l.1537-1541), ce qui ruine le cache sur les grandes grilles.
**Correctif :** copier les plans en une passe contiguë (k le plus interne) ou adopter la disposition `i + nx*(j + ny*k)` ; gain attendu : lectures séquentielles, forte réduction des défauts de cache.

#### ⚡ O-S04 — `Source/scalarfield2-curvature.cpp:19` — Boucles i externe / j interne sur un stockage i + nx*j

Avec `VertexIndex(i,j) = i + nx*j`, la boucle interne sur j saute de nx doubles : scalarfield2-curvature.cpp:19, 53, 86, 119, 151 ; scalarfield2-smooth.cpp:171, 229, 284, 415 ; scalarfield2-morph.cpp:18 ; scalarfield2.cpp:74, 818, 858, 894, 949, 1678, 1715, 1935, 2726, 3391, 3421, 3710, 3757 ; integerfield2.cpp:55, 200, 221 ; hexagonscalarfield2.cpp:59, 163, 366, 425, 530, 558 ; diffusion.cpp:201.
**Correctif :** inverser l'ordre des boucles (j externe, i interne) ; gain attendu : accès contigus, plusieurs fois plus rapide sur les grands terrains.

#### ⚡ O-S05 — `Source/scalarfield2-smooth.cpp:290` — Allocations par pixel et filtres non séparables

`MedianFilter()` alloue un `std::vector` et fait un `sort` complet par pixel (l.290-302) ; `MaxFilter`/`MinFilter` (scalarfield2.cpp:3387, 3417) coûtent O(w²) par pixel.
**Correctif :** `double w[9]` + `std::nth_element`, et deux passes 1D (voire van Herk/Gil-Werman) pour Min/Max ; gain attendu : plus d'allocation par pixel et coût O(w) (ou O(1)) au lieu de O(w²).

### N — Bruit et échantillonnage

#### ⚡ O-N01 — `Source/noise.cpp:278` — Calcul inutile de la valeur du bruit

`Gradient()` calcule `tx, ty, tz`, les produits `txty...` et `sum` (l.235-287) jamais utilisés : seul `Vector(nx, ny, nz)` est renvoyé.
**Correctif :** supprimer les l.235-237 et 278-287 ; gain : suppression d'une interpolation complète par appel.

#### ⚡ O-N02 — `Source/disctilelink.cpp:274` — Calcul de TrucMuch inutilisé

Le constructeur construit `vs` et `ids` via `TrucMuch()` sur une boîte étendue (l.265-274) sans jamais les utiliser : la boucle recalcule les 9 copies par `Vertex(q, j)`.
**Correctif :** supprimer les l.265-274 ; gain : une construction de voisinage étendu (allocations et copies) en moins par tuile.

#### ⚡ O-N03 — `Source/eden2.cpp:618` — Calculs inutiles dans une fonction non implémentée

`Contour()` construit un `GeometricGraph2` et une triangulation de Delaunay complète puis renvoie un `Polygon2()` vide.
**Correctif :** renvoyer directement `Polygon2()` tant que la fonction n'est pas implémentée ; gain : O(n log n) évité par appel.

### T — Maillages et graphes géométriques

#### ⚡ O-T01 — `Source/mesh.cpp:563` — Test O(n²) sur toutes les demi-arêtes

Pour chaque demi-arête, parcours complet de `varray` avec un vecteur temporaire ; une `QHash<QPair<int,int>,int>` comptant les demi-arêtes ramène le test à O(n) (gain : quadratique vers linéaire sur les gros maillages).
**Correctif :** compter les demi-arêtes `(a,b)` dans une table de hachage puis vérifier que chaque `(b,a)` est présent exactement une fois.

---

## Index des problèmes

### B-M — Mathématiques

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-M01 | ✅ | 🔴 | Lecture hors limites de devs_b[dev_b]/devs_e[dev_e] dans un fprintf de débogage | `Source/aa.cpp:1392` |
| B-M02 | ✅ | 🔴 | Boucle de copie jusqu'à length+1 : lecture hors limites des indices/déviations source | `Source/aa.cpp:1695` |
| B-M03 | ⬜ | 🔴 | SVD fausse : indexation ligne-major mêlée à operator() colonne-major, S non affecté | `Source/matrix-algebra.cpp:236` |
| B-M04 | ✅ | 🟠 | Insertion de A[4] mal imbriquée : tri faux même sur une entrée déjà triée | `Source/mathematics.cpp:453` |
| B-M05 | ⬜ | 🟠 | Test de parité faux pour les multiples négatifs de π : intervalle non englobant | `Source/ia.cpp:371` |
| B-M06 | ⬜ | 🟠 | Approximation Minimum : facteur 0.5 manquant sur alpha et gamma | `Source/aa.cpp:1744` |
| B-M07 | ⬜ | 🟠 | Chebyshev, exposant pair : faux point de tangence, encadrement non garanti | `Source/aa.cpp:1174` |
| B-M08 | ⬜ | 🟠 | Rayon du résultat jamais calculé (radius = 0) | `Source/aa.cpp:2814` |
| B-M09 | ⬜ | 🟠 | Hachage PCG sur int signé : débordements signés (UB) et décalages arithmétiques | `Source/ivector2.cpp:34` |
| B-M10 | ✅ | 🟠 | Convention ligne/colonne inversée : la matrice produite est la rotation inverse | `Source/quaternion.cpp:167` |
| B-M11 | ⬜ | 🟠 | Composition dans le mauvais ordre : symétrie par rapport au mauvais plan | `Source/matrix.cpp:185` |
| B-M12 | ⬜ | 🟠 | Code écrit pour la convention vecteur-ligne : l'œil n'est pas envoyé à l'origine | `Source/matrix4.cpp:422` |
| B-M13 | ⬜ | 🟠 | right et up_ non normalisés : la vue contient une mise à l'échelle | `Source/matrix4.cpp:435` |
| B-M14 | ⬜ | 🟡 | Binomial(n>=15, 0) renvoie n au lieu de 1 | `Source/mathematics.cpp:151` |
| B-M15 | ⬜ | 🟡 | Valeurs énormes pour x < -3σ | `Include/mathematics.h:1098` |
| B-M16 | ⬜ | 🟡 | Pas de traitement d'un dénominateur contenant 0 | `Source/ia.cpp:293` |
| B-M17 | ⬜ | 🟡 | Intervalle faux pour les puissances paires (limitation seulement documentée) | `Source/ia.cpp:456` |
| B-M18 | ⬜ | 🟡 | Erreur δ estimée sur 8 échantillons : borne non garantie | `Source/aa.cpp:2795` |
| B-M19 | ⬜ | 🟡 | Retour silencieux de Aa() (=0) hors domaine | `Source/aa.cpp:1798` |
| B-M20 | ⬜ | 🟡 | Accès indexes[-1] si T == 0 | `Source/aa.cpp:61` |
| B-M21 | ⬜ | 🟡 | else if : la borne max n'est pas mise à jour quand la min l'est | `Source/evector.cpp:383` |
| B-M22 | ⬜ | 🟡 | Aucune vérification du nombre de composantes | `Source/evector.cpp:287` |
| B-M23 | ⬜ | 🟡 | Test sur sin(angle) au lieu de l'angle : résultat faux près de kπ | `Source/quaternion.cpp:289` |
| B-M24 | ⬜ | 🟡 | Matrice diagonale : vecteurs propres associés aux mauvaises valeurs propres | `Source/matrix2.cpp:306` |
| B-M25 | ⬜ | 🟡 | Code de retour de Quadric::Solve ignoré | `Source/matrix2.cpp:276` |
| B-M26 | ⬜ | 🟡 | Gram-Schmidt appliqué aux lignes (indexation ligne-major) : M ≠ Q·D·U | `Source/matrix-algebra.cpp:362` |
| B-M27 | ⬜ | 🟡 | Matrice singulière : renvoie silencieusement l'identité | `Source/matrix-mn.cpp:110` |
| B-M28 | ⬜ | 🟡 | Taille des données non vérifiée | `Source/matrix-mn.cpp:37` |
| B-M29 | ⬜ | 🔵 | Décalage de 1 pour les entiers négatifs ou nuls | `Include/mathematics.h:614` |
| B-M30 | ⬜ | 🔵 | Seuil absolu 1e-12 : argument nul pour les petits complexes | `Source/complex.cpp:53` |
| B-M31 | ✅ | 🔵 | Traces de débogage fprintf(stdout, ...) laissées dans le code | `Source/aa.cpp:1500` |
| B-M32 | ⬜ | 🔵 | chebyshevFailed calculé mais jamais utilisé | `Source/aa.cpp:2149` |
| B-M33 | ⬜ | 🔵 | acos sans écrêtage : NaN pour des vecteurs colinéaires | `Source/evector.cpp:180` |
| B-M34 | ✅ | 🔵 | acos(w) sans écrêtage | `Source/quaternion.cpp:187` |
| B-M35 | ⬜ | 🔵 | Renvoie b·aᵀ au lieu de a·bᵀ (à confirmer selon l'intention) | `Source/matrix.cpp:536` |
| B-M36 | ⬜ | 🔵 | Échec de QLAlgorithm ignoré | `Source/matrix-algebra.cpp:648` |
| B-M37 | ⬜ | 🔵 | Une seule racine réelle renvoyée comme valeur propre triple | `Source/matrix-algebra.cpp:678` |
| B-M38 | ✅ | 🔵 | Division par zéro pour la matrice nulle | `Source/matrix-algebra.cpp:707` |
| B-M39 | ✅ | 🔵 | Gauss-Jordan sans pivot partiel : instabilité numérique | `Source/matrix-mn.cpp:109` |
| B-M40 | ✅ | 🔵 | Seuil de singularité absolu 1e-12 | `Source/matrix-mn.cpp:164` |
| B-M41 | ⬜ | 🟡 | Résultat NaN pour une affine de largeur nulle (0/0) | `Source/aa.cpp:1613` |

**9 / 41 corrigés** — restants : 1🔴 · 8🟠 · 16🟡 · 7🔵

### B-P — Polynômes, aléatoire, graphes

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-P01 | ✅ | 🔴 | Composition quartique∘quadrique fausse (p[1] à la place de p[4], q20 au lieu de q21) | `Source/polynomial.cpp:564` |
| B-P02 | ⬜ | 🟡 | Division par zéro si le coefficient dominant est nul (degré non vérifié) | `Source/polynomial-solver.cpp:25` |
| B-P03 | ⬜ | 🟡 | NaN pour une racine triple (Q = R = 0) et pour |R/√Q³| > 1 par arrondi | `Source/cubic.cpp:128` |
| B-P04 | ⬜ | 🟡 | Les trois racines ne sont pas triées contrairement à la documentation | `Source/cubic.cpp:133` |
| B-P05 | ⬜ | 🟡 | Signe du décalage inversé dans les branches racine double (p == 0) | `Source/quartic.cpp:270` |
| B-P06 | ✅ | 🔵 | Copie du dividende par arithmétique de pointeurs sur Polynomial (correcte par accident) | `Source/polynomial.cpp:102` |
| B-P07 | ⬜ | 🔵 | Fonction privée jamais appelée (code mort) | `Source/polynomial-solver.cpp:53` |
| B-P08 | ⬜ | 🔵 | Branche n == -1 inatteignable | `Source/polynomial.cpp:59` |
| B-P09 | ⬜ | 🔵 | Seuil absolu sur le coefficient dominant | `Source/cubic.cpp:96` |
| B-P10 | ⬜ | 🔵 | Seuil absolu sur le discriminant et formule non stable numériquement | `Source/quadric.cpp:159` |
| B-P11 | ⬜ | 🔵 | `IsNull()` ne teste que c[0] quand n > 0 | `Include/polynomial.h:261` |
| B-P12 | ⬜ | 🔵 | `Check()` : seuil absolu 1e-10 sur le coefficient dominant | `Source/polynomial.cpp:90` |

**2 / 12 corrigés** — restants : 0🔴 · 0🟠 · 4🟡 · 6🔵

### B-G — Géométrie 2D

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-G01 | ✅ | 🔴 | Paramètre d'interpolation négatif : tous les points hors du périmètre | `Source/box2.cpp:715` |
| B-G02 | ✅ | 🔴 | Facteur (1.0 - r) au lieu de (d - r) : norme constante | `Source/capsule2.cpp:160` |
| B-G03 | ✅ | 🔴 | Formule entièrement fausse | `Source/circle2.cpp:539` |
| B-G04 | ✅ | 🔴 | Formule de Ramanujan mal recopiée : périmètre toujours négatif | `Source/ellipse2.cpp:521` |
| B-G05 | ✅ | 🔴 | Test inversé et distance au carré comparée au rayon | `Source/hexagon2.cpp:350` |
| B-G06 | ✅ | 🔴 | Ordre de tri inversé et formule de l'arête diagonale fausse | `Source/irregularoctogon2.cpp:149` |
| B-G07 | ✅ | 🔴 | Mêmes erreurs que Signed() et valeurs non élevées au carré | `Source/irregularoctogon2.cpp:193` |
| B-G08 | ✅ | 🔴 | Normale intérieure et apothème faux : distance toujours nulle / négative | `Source/octogon2.cpp:172` |
| B-G09 | ✅ | 🔴 | Normales d'arêtes intérieures utilisées comme normales extérieures | `Source/pentagon2.cpp:35` |
| B-G10 | ✅ | 🔴 | Adaptation du code d'I. Quilez entièrement fausse | `Source/pentagon2.cpp:277` |
| B-G11 | ✅ | 🔴 | tb jamais mis à jour, paramètre t de signe opposé | `Source/pentagon2.cpp:418` |
| B-G12 | ✅ | 🔴 | Normales d'arêtes intérieures : résultat inversé | `Source/quad2.cpp:170` |
| B-G13 | ✅ | 🔴 | Coordonnées barycentriques homogènes non normalisées | `Source/triangle2.cpp:131` |
| B-G14 | ✅ | 🔴 | Test inversé : renvoie toujours faux | `Source/triangle2.cpp:248` |
| B-G15 | ✅ | 🔴 | Test de côté inversé pour un triangle CCW | `Source/triangle2.cpp:349` |
| B-G16 | ⬜ | 🟠 | Profondeurs d'intersection mal réordonnées | `Source/annulus2.cpp:58` |
| B-G17 | ⬜ | 🟠 | Condition inversée : vecteur nul à l'extérieur, non nul dans l'anneau | `Source/annulus2.cpp:123` |
| B-G18 | ⬜ | 🟠 | Coordonnées réelles tronquées en int | `Source/boxset2.cpp:48` |
| B-G19 | ✅ | 🟠 | Paramètres de droite en longueur comparés à [0,1] | `Source/box2.cpp:479` |
| B-G20 | ⬜ | 🟠 | Réflexion par rapport à la normale au lieu de la droite | `Source/line2.cpp:131` |
| B-G21 | ⬜ | 🟠 | Aire fausse : πr² + πrL au lieu de πr² + 2rL | `Source/capsule2.cpp:40` |
| B-G22 | ⬜ | 🟠 | Milieu de la corde du mauvais côté | `Source/circle2.cpp:439` |
| B-G23 | ✅ | 🟠 | Centre et rayon du cercle ignorés | `Source/circle2.cpp:294` |
| B-G24 | ⬜ | 🟠 | Quadrants testés en coordonnées absolues au lieu de relatives au centre | `Source/circlearc2.cpp:74` |
| B-G25 | ⬜ | 🟠 | 7 cas sur 16 de la table de quadrants faux (sens horaire / boîte inversée) | `Source/circlearc2.cpp:147` |
| B-G26 | ⬜ | 🟠 | Distance au cercle complet conservée hors du secteur de l'arc | `Source/circlearc2.cpp:290` |
| B-G27 | ✅ | 🟠 | Distance intérieure positive et calculée avec max au lieu de min | `Source/convex2.cpp:436` |
| B-G28 | ⬜ | 🟠 | Cas « arête » : norme au carré et sens inversé | `Source/convex2.cpp:496` |
| B-G29 | ⬜ | 🟠 | Cercle contenu dans le polygone déclaré disjoint | `Source/convex2.cpp:623` |
| B-G30 | ✅ | 🟠 | Distance à l'arête non normalisée | `Source/diamond2.cpp:112` |
| B-G31 | ✅ | 🟠 | Distance intérieure fausse (positive ou issue du mauvais disque) | `Source/disctwins2.cpp:85` |
| B-G32 | ✅ | 🟠 | Point non exprimé dans le repère local de l'ellipse | `Source/ellipse2.cpp:344` |
| B-G33 | ✅ | 🟠 | Renvoie le rayon de courbure au lieu de la courbure | `Source/ellipse2.cpp:268` |
| B-G34 | ✅ | 🟠 | Terme q[0]²/bb au lieu de q[0]²/aa : norme du gradient fausse | `Source/ellipse2.cpp:146` |
| B-G35 | ✅ | 🟠 | Gradient renvoyé dans le repère local, non ramené en coordonnées monde | `Source/ellipse2.cpp:141` |
| B-G36 | ✅ | 🟠 | Boîte locale construite deux fois trop petite | `Source/framebox2.cpp:157` |
| B-G37 | ✅ | 🟠 | Distance intérieure toujours nulle | `Source/framebox2.cpp:70` |
| B-G38 | ✅ | 🟠 | Distance à l'arête non élevée au carré | `Source/hexagon2.cpp:255` |
| B-G39 | ⬜ | 🟠 | Rayon de l'hexagone ignoré | `Source/hexagonalpha2.cpp:118` |
| B-G40 | ⬜ | 🟠 | Réduction de l'angle modulo π/6 incorrecte | `Source/hexagonalpha2.cpp:112` |
| B-G41 | ⬜ | 🟠 | Lentille incohérente avec l'épaisseur et la hauteur demandées | `Source/lens2.cpp:29` |
| B-G42 | ✅ | 🟠 | Hauteur de la boîte divisée par deux | `Source/lens2.cpp:153` |
| B-G43 | ⬜ | 🟠 | Rayon du cercle argument ignoré | `Source/particle2.cpp:78` |
| B-G44 | ✅ | 🟠 | Seuil r (rayon circonscrit) au lieu de l'apothème | `Source/pentagon2.cpp:197` |
| B-G45 | ⬜ | 🟠 | Partition angulaire décalée de 36° | `Source/pentagon2.cpp:143` |
| B-G46 | ⬜ | 🟠 | Distance à l'arête non carrée et longueur d'arête fausse | `Source/pentagon2.cpp:241` |
| B-G47 | ⬜ | 🟠 | Tirage limité au quadrant x ≥ 0, y ≥ 0 | `Source/pentagon2.cpp:437` |
| B-G48 | ⬜ | 🟠 | Appariement sortie/entrée faux si le premier sommet est à gauche | `Source/polygon2.cpp:553` |
| B-G49 | ⬜ | 🟠 | Centroïde de signe opposé pour un polygone horaire | `Source/polygon2.cpp:764` |
| B-G50 | ⬜ | 🟠 | Sommets donnés dans le sens horaire | `Source/quad2.cpp:45` |
| B-G51 | ⬜ | 🟠 | Vecteur renvoyé dans le repère local du rectangle | `Source/rectangle2.cpp:103` |
| B-G52 | ⬜ | 🟠 | Centre local ajouté en coordonnées monde et demi-tailles permutées | `Source/revolution-quadric2.cpp:219` |
| B-G53 | ⬜ | 🟠 | Aire de la lune du grand disque au lieu du disque a | `Source/scales2.cpp:142` |
| B-G54 | ⬜ | 🟠 | Triangle renvoyé dans l'espace normalisé (taille ignorée) | `Source/simplex2.cpp:66` |
| B-G55 | ⬜ | 🟠 | Distance passée là où une distance au carré est attendue | `Source/smoothbox2.cpp:27` |
| B-G56 | ⬜ | 🟠 | Rayon du disque utilisé à la place du rayon d'atténuation | `Source/smoothdisc2.cpp:55` |
| B-G57 | ⬜ | 🟠 | Mise à l'échelle sans effet | `Source/smoothellipsegaussian2.cpp:120` |
| B-G58 | ⬜ | 🟠 | Profondeurs renvoyées opposées et permutées | `Source/triangle2.cpp:488` |
| B-G59 | ⬜ | 🟠 | Taille du réseau ignorée | `Include/simplex.h:64` |
| B-G60 | ⬜ | 🟡 | Le 4e sommet de la boîte est ignoré | `Source/axis2.cpp:101` |
| B-G61 | ✅ | 🟡 | Boucle infinie si l'ensemble est vide | `Source/boxset2.cpp:142` |
| B-G62 | ⬜ | 🟡 | Rayon traité comme une droite | `Source/box2.cpp:291` |
| B-G63 | ⬜ | 🟡 | Sens de balayage des arcs dépendant de l'orientation absolue | `Source/capsule2.cpp:352` |
| B-G64 | ⬜ | 🟡 | Tri non lexicographique pour la chaîne monotone d'Andrew | `Source/convex2.cpp:375` |
| B-G65 | ⬜ | 🟡 | NaN pour des disques disjoints | `Source/disctwins2.cpp:77` |
| B-G66 | ⬜ | 🟡 | Rayon de l'autre paire ignoré | `Source/disctwins2.cpp:101` |
| B-G67 | ✅ | 🟡 | Division par zéro pour une ellipse circulaire (a == b) | `Source/ellipse2.cpp:293` |
| B-G68 | ⬜ | 🟡 | Axe interpolé non normalisé | `Source/ellipse2.cpp:534` |
| B-G69 | ⬜ | 🟡 | Inverse et composition faux pour une échelle non uniforme avec rotation | `Source/framescaled2.cpp:80` |
| B-G70 | ⬜ | 🟡 | Inverse de la transformation des normales incorrect | `Source/framescaled2.cpp:166` |
| B-G71 | ⬜ | 🟡 | Test de séparation avec le rayon au lieu de l'apothème | `Source/hexagon2.cpp:210` |
| B-G72 | ⬜ | 🟡 | Symétrie en y mal appliquée | `Source/octogon2.cpp:295` |
| B-G73 | ✅ | 🟡 | Segment parasite depuis l'origine | `Source/lens2.cpp:131` |
| B-G74 | ⬜ | 🟡 | NaN pour des disques disjoints | `Source/disctwins2.cpp:117` |
| B-G75 | ⬜ | 🟡 | Dernier segment perdu | `Source/lines2.cpp:305` |
| B-G76 | ⬜ | 🟡 | Accès hors bornes sur un ensemble vide | `Source/lines2.cpp:343` |
| B-G77 | ⬜ | 🟡 | Un triplet de sommets jamais testé | `Source/polygon2.cpp:298` |
| B-G78 | ⬜ | 🟡 | Inclusion non détectée | `Source/polygon2.cpp:614` |
| B-G79 | ⬜ | 🟡 | Somme au lieu du maximum des distances dirigées | `Source/polygon2.cpp:1313` |
| B-G80 | ⬜ | 🟡 | Trous ignorés, incohérence avec Inside() | `Source/polygons2.cpp:107` |
| ~~B-G81~~ | — | 🟡 | ~~Une seule racine du trinôme testée~~ (faux positif) | `Source/quad2.cpp:250` |
| B-G82 | ⬜ | 🟡 | Coefficients de la cubique faux si l'extrémité t=1 est la plus proche | `Source/revolution-quadric2.cpp:76` |
| B-G83 | ✅ | 🟡 | Intervalle ya, yb non mis à l'échelle | `Source/revolution-quadric2.cpp:198` |
| B-G84 | ✅ | 🟡 | Fonction non implémentée renvoyant silencieusement le vecteur nul | `Source/scales2.cpp:98` |
| B-G85 | ✅ | 🟡 | Boîte fausse pour une demi-ouverture supérieure à π/2 | `Source/sector2.cpp:38` |
| B-G86 | ✅ | 🟡 | Coefficient de tapering a ignoré | `Source/taper2.cpp:47` |
| B-G87 | ✅ | 🟡 | Signe opposé à la convention trigonométrique | `Source/triangle2.cpp:58` |
| B-G88 | ⬜ | 🟡 | Identité dans la bande d'extension e : déformation discontinue | `Source/twistsector2.cpp:59` |
| B-G89 | ✅ | 🟡 | NaN possible pour un point sur l'axe | `Source/sector2.cpp:94` |
| B-G90 | ⬜ | 🟡 | Déformation discontinue au bord du disque | `Source/taper2.cpp:39` |
| B-G91 | ⬜ | 🔵 | Points perturbés hors de la boîte | `Source/box2.cpp:1116` |
| B-G92 | ✅ | 🔵 | Membres statiques jamais définis | `Source/line2.cpp:15` |
| B-G93 | ⬜ | 🔵 | Prédicat symétrique, sens du test ambigu (à confirmer) | `Source/circle2.cpp:136` |
| B-G94 | ⬜ | 🔵 | Arcs courts non tracés ou NaN | `Source/circlearc2.cpp:54` |
| B-G95 | ⬜ | 🔵 | Code mort à liaison externe, en float, et giftWrapping erroné | `Source/convex2.cpp:745` |
| B-G96 | ⬜ | 🔵 | fabs(d) contredit la doc (à confirmer) | `Source/ellipse2.cpp:497` |
| B-G97 | ⬜ | 🔵 | Ordre de composition discutable (à confirmer) | `Source/framebox2.cpp:41` |
| B-G98 | ✅ | 🔵 | « Rayon » surestimé | `Include/octogon.h:188` |
| B-G99 | ⬜ | 🔵 | Intersection vide non détectable | `Include/irect.h:64` |
| B-G100 | ⬜ | 🔵 | Orientation non normalisée dans l'une des deux branches de fermeture | `Source/lines2.cpp:137` |
| B-G101 | ✅ | 🔵 | Espacement supérieur à la distance « maximale » | `Source/polygon2.cpp:1165` |
| ~~B-G102~~ | — | 🔵 | ~~Réseau de Vertex() incohérent avec celui de Corners()~~ (faux positif) | `Source/simplex2.cpp:15` |
| B-G103 | ⬜ | 🔵 | Segment d'intersection passé par valeur : jamais renvoyé | `Source/triangle2-intersection.cpp:313` |
| B-G104 | ⬜ | 🔵 | Fonctions 3D non déclarées, non documentées, inutilisées | `Source/triangle2-intersection.cpp:56` |
| B-G105 | ✅ | 🔵 | Facteur négatif : axe non inversé, longueur négative | `Source/axis2.cpp:61` |
| B-G106 | ⬜ | 🔵 | `Pentagon2::Edge(int)` : longueur r au lieu de la longueur d'arête | `Include/pentagon.h:79` |
| B-G107 | ✅ | 🟡 | Distance au carré inexacte (calculée à partir de Signed) | `Source/scales2.cpp:79` |
| B-G108 | ✅ | 🔵 | Membres p, q jamais lus et non initialisés si les cercles ne se coupent pas | `Include/scales.h:28` |

**44 / 106 corrigés** — restants : 0🔴 · 30🟠 · 21🟡 · 11🔵

### B-K — Géométrie 3D de base

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-K01 | ✅ | 🔴 | Le point visé est réfléchi derrière l'œil à chaque appel | `Source/camera.cpp:668` |
| B-K02 | ✅ | 🔴 | Test SAT segment/boîte : vecteur du segment utilisé au lieu des demi-dimensions de la boîte | `Source/segment.cpp:404` |
| B-K03 | ✅ | 🔴 | Bornes jamais mises à jour et signe faux : renvoie toujours true | `Source/ktope.cpp:120` |
| B-K04 | ⬜ | 🔴 | Changement de repère faux dans le test OBB/OBB | `Source/framebox.cpp:155` |
| B-K05 | ✅ | 🔴 | Arguments de Plane inversés : normale nulle | `Source/slab.cpp:34` |
| B-K06 | ✅ | 🔴 | Profondeurs d'intersection fausses | `Source/slab.cpp:77` |
| B-K07 | ✅ | 🔴 | Test inversé | `Source/slab.cpp:92` |
| B-K08 | ⬜ | 🔴 | Test de recouvrement des intervalles faux | `Source/ktope.cpp:176` |
| B-K09 | ✅ | 🔴 | Compteur réinitialisé à chaque face, test sur la mauvaise face : renvoie toujours false | `Source/tetrahedra.cpp:102` |
| B-K10 | ⬜ | 🟠 | Test SAT segment/boîte faux sur les axes principaux (direction non normalisée) | `Source/box.cpp:1418` |
| B-K11 | ⬜ | 🟠 | Facteurs hLength et vLength inversés | `Source/camera.cpp:514` |
| B-K12 | ⬜ | 🟠 | start() relu pendant la modification des clés | `Source/cameraframes.cpp:307` |
| B-K13 | ⬜ | 🟠 | Rotation calculée avec un vecteur up non orthogonal à la visée | `Source/camera.cpp:217` |
| B-K14 | ⬜ | 🟠 | Valeur absolue manquante : distance nulle sur la moitié de l'espace | `Source/framebox.cpp:74` |
| B-K15 | ⬜ | 🟠 | Les 12 plans diagonaux restent à 0 | `Source/ktope.cpp:52` |
| B-K16 | ⬜ | 🟠 | Inverse fausse dès que l'échelle n'est pas 1 | `Source/framescaled.cpp:83` |
| B-K17 | ⬜ | 🟠 | Échelle appliquée avant la rotation et avec le mauvais facteur | `Source/framescaled.cpp:179` |
| B-K18 | ⬜ | 🟠 | Axe de rotation non normalisé | `Source/frame.cpp:113` |
| B-K19 | ⬜ | 🟠 | Valeur absolue erronée : symétrique faux du côté négatif | `Source/plane.cpp:332` |
| B-K20 | ⬜ | 🟠 | Point de la droite d'intersection faux | `Source/plane.cpp:259` |
| B-K21 | ⬜ | 🟠 | Centre décalé de la demi-longueur précédente | `Source/raystep.cpp:68` |
| B-K22 | ⬜ | 🟠 | Aire 4 fois trop petite | `Include/rectangle.h:60` |
| B-K23 | ⬜ | 🟠 | Coefficients x²y et xy² intervertis | `Include/quadricsurface.h:34` |
| B-K24 | ⬜ | 🟠 | Seconde phase : côté testé opposé à celui de la première phase | `Source/tetrahedra-intersection.cpp:198` |
| B-K25 | ⬜ | 🟠 | Trois des six tétraèdres de découpe ont des normales rentrantes | `Include/tetrahedra.h:65` |
| B-K26 | ⬜ | 🟠 | Formule sans rapport avec l'aire | `Source/tetrahedra.cpp:259` |
| B-K27 | ⬜ | 🟠 | Vecteur TS issu du mauvais sommet | `Source/triangle.cpp:1059` |
| B-K28 | ⬜ | 🟡 | Paramètres width/height/field ignorés, constructeur à champ de vision inappelable | `Source/camera.cpp:33` |
| B-K29 | ✅ | 🟡 | Risque de fiasco d'ordre d'initialisation statique | `Source/axis.cpp:17` |
| B-K30 | ⬜ | 🟡 | Clé de même temps que la dernière dupliquée | `Source/cameraframes.cpp:61` |
| B-K31 | ✅ | 🟡 | Accès hors bornes sur ensemble vide | `Source/box.cpp:233` |
| B-K32 | ⬜ | 🟡 | Division par zéro pour un axe dégénéré | `Source/axis.cpp:28` |
| B-K33 | ⬜ | 🟡 | Division par zéro pour un point sur l'axe du cercle | `Source/circle.cpp:152` |
| B-K34 | ✅ | 🟠 | Cas parallèle : renvoie la longueur au carré du vecteur directeur | `Source/line.cpp:50` |
| B-K35 | ✅ | 🟡 | État global statique : non réentrant | `Source/tetrahedra-intersection.cpp:8` |
| B-K36 | ⬜ | 🟡 | Sentinelle Vector::Null : le sommet à l'origine est perdu | `Source/plane.cpp:289` |
| B-K37 | ✅ | 🟡 | Normale transformée comme une différence de points | `Source/plane.cpp:167` |
| B-K38 | ✅ | 🟡 | Composition inexacte pour une échelle non uniforme | `Source/framescaled.cpp:205` |
| B-K39 | ⬜ | 🟡 | Normale 3D tronquée en Vector2 | `Source/quadricsurface.cpp:60` |
| B-K40 | ⬜ | 🟡 | Translation de signe opposé | `Source/quadricsurface.cpp:69` |
| B-K41 | ⬜ | 🟡 | Comparaison du sommet 3 avec un produit scalaire erroné | `Source/tetrahedra.cpp:510` |
| B-K42 | ✅ | 🔵 | Mauvais vecteur renvoyé dans deux régions de sommet | `Source/tetrahedra.cpp:833` |
| B-K43 | ⬜ | 🟡 | Paramètre V0 de la macro ignoré : le test « U contenu dans V » ne fonctionne pas | `Source/triangle-intersection.cpp:106` |
| B-K44 | ✅ | 🟡 | Intersections derrière l'origine du rayon acceptées | `Source/triangle.cpp:131` |
| B-K45 | ⬜ | 🔵 | Un arc construit par défaut se comporte comme un cercle complet | `Include/circlearc.h:12` |
| B-K46 | ⬜ | 🔵 | sqrt d'une différence pouvant être négative par arrondi | `Include/axis.h:136` |
| B-K47 | ✅ | 🔵 | Facteur négatif : axe non inversé, longueur négative | `Source/axis.cpp:77` |
| B-K48 | ✅ | 🔵 | Rayon négatif pour un facteur négatif | `Include/circle.h:263` |
| B-K49 | ✅ | 🔵 | Vecteurs de slab non mis à jour pour un facteur négatif | `Source/circlearc.cpp:60` |
| B-K50 | ⬜ | 🔵 | Déclaration friend operator<< avec la mauvaise signature | `Include/circlearc.h:31` |
| B-K51 | ⬜ | 🔵 | Échantillons sous-pixel non centrés | `Source/camera.cpp:483` |
| B-K52 | ⬜ | 🔵 | Seuil de dégénérescence absolu et échec silencieux | `Source/circle.cpp:173` |
| B-K53 | ✅ | 🔵 | Sentinelle ambiguë Box::Null | `Source/box.cpp:911` |
| B-K54 | ⬜ | 🔵 | Orientation exigée mais jamais vérifiée | `Source/tetrahedra.cpp:33` |
| B-K55 | ⬜ | 🔵 | Seuils absolus dépendant de l'échelle | `Source/axis.cpp:260` |
| B-K56 | ✅ | 🔵 | Centre non mis à l'échelle | `Source/rectangle.cpp:137` |
| B-K57 | ✅ | 🔵 | Orientation inversée pour un facteur négatif | `Source/tetrahedra.cpp:1094` |
| B-K58 | ⬜ | 🔵 | Indices calculés comme si le tableau de sommets était vide | `Source/triangle.cpp:844` |
| B-K59 | ⬜ | 🔵 | Code mort et symboles globaux non protégés | `Source/triangle-intersection.cpp:181` |

**21 / 59 corrigés** — restants : 2🔴 · 18🟠 · 9🟡 · 9🔵

### B-R — Primitives 3D volumiques

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-R01 | ⬜ | 🔴 | Accès hors bornes au tableau triangle pour les faces triangulaires | `Include/icosidodecahedron.h:106` |
| B-R02 | ✅ | 🔴 | Lecture du membre r non initialisé dans le constructeur délégué | `Source/boxsphere.cpp:69` |
| B-R03 | ✅ | 🔴 | Distance segment-segment calculée comme si les deux segments étaient de longueur 1 | `Source/capsule.cpp:226` |
| B-R04 | ⬜ | 🔴 | Signe faux dans la quadrique du rayon du cône : intersection rayon/cône fausse | `Source/cone.cpp:97` |
| B-R05 | ✅ | 🔴 | Distance toujours nulle : Inside() toujours faux et boucle infinie dans RandomInside() | `Source/dodecahedron-rhombic.cpp:85` |
| B-R06 | ✅ | 🔴 | Affectations inversées : ta/tb jamais mis à jour, renvoie toujours true | `Source/dodecahedron.cpp:121` |
| B-R07 | ⬜ | 🔴 | Lecture du membre b non initialisé et rayons mal affectés | `Source/ellipsoid.cpp:47` |
| B-R08 | ✅ | 🔴 | Boucle infinie : while (true) sans sortie | `Source/hemisphere.cpp:263` |
| B-R09 | ✅ | 🔴 | Tables triangle, pentagon, normal factices : tous les plans de faces sont faux | `Source/icosidodecahedron.cpp:124` |
| B-R10 | ⬜ | 🟠 | Volume faux : la correction des 12 arêtes est oubliée | `Source/boxsphere.cpp:90` |
| B-R11 | ⬜ | 🟠 | Aire fausse : la correction des arêtes est oubliée | `Source/boxsphere.cpp:101` |
| B-R12 | ⬜ | 🟠 | L'intervalle [v0, v1] est supposé toujours intérieur au cône infini | `Source/cone.cpp:103` |
| B-R13 | ⬜ | 🟠 | Distance signée positive à l'intérieur des cônes avec ra ≤ rb | `Source/cone.cpp:759` |
| B-R14 | ⬜ | 🟠 | Vecteur distance faux sur la partie conique | `Source/conesphere.cpp:216` |
| B-R15 | ⬜ | 🟠 | Vecteur distance faux vers la sphère de l'extrémité b | `Source/conesphere.cpp:197` |
| B-R16 | ⬜ | 🟠 | Normales des faces triangulaires fausses (7 sur 8) et carrés dans le mauvais ordre | `Source/cuboctahedron.cpp:27` |
| B-R17 | ⬜ | 🟠 | Quatre faces sur six ont des sommets dans un ordre non cyclique (quadrilatère croisé) | `Source/cuboid.cpp:28` |
| B-R18 | ⬜ | 🟠 | Volume divisé par 8 | `Include/octahedron.h:52` |
| B-R19 | ⬜ | 🟠 | Aire doublée | `Include/octahedron.h:66` |
| B-R20 | ⬜ | 🟠 | Le rayon transformé ignore le rayon d'origine | `Source/cylinder.cpp:820` |
| B-R21 | ⬜ | 🟠 | Test paroi/disques incohérent : les disques ne sont jamais choisis | `Source/cylinder.cpp:435` |
| B-R22 | ⬜ | 🟠 | Le centre c est ignoré | `Source/dodecahedron-rhombic.cpp:58` |
| B-R23 | ⬜ | 🟠 | Boîte englobante trop petite | `Include/dodecahedron.h:118` |
| B-R24 | ⬜ | 🟠 | Longueur d'arête fausse et rayon r incohérent avec les sommets | `Source/dodecahedron.cpp:80` |
| B-R25 | ⬜ | 🟠 | Conventions de repère incohérentes entre constructeurs et requêtes | `Source/ellipsoid.cpp:98` |
| B-R26 | ⬜ | 🟠 | Division par e² au lieu de l'excentricité e | `Source/spheroid.cpp:162` |
| B-R27 | ⬜ | 🟠 | Teste la mauvaise moitié de la sphère | `Source/hemisphere.cpp:173` |
| B-R28 | ⬜ | 🟠 | Utilise le rayon de la demi-sphère au lieu du rayon de Poisson | `Source/hemisphere.cpp:333` |
| B-R29 | ⬜ | 🟠 | Signe du déplacement inversé : les points s'attirent au lieu de se repousser | `Source/hemisphere.cpp:334` |
| B-R30 | ⬜ | 🟠 | Renvoie la distance au carré à l'extérieur | `Source/hemispheredisc.cpp:197` |
| B-R31 | ⬜ | 🟠 | Lentille construite incohérente avec l'épaisseur et la hauteur demandées | `Source/lens.cpp:26` |
| B-R32 | ⬜ | 🟠 | Le rayon de la sphère testée est ignoré | `Source/particle.cpp:64` |
| B-R33 | ✅ | 🟠 | Volume divisé par 8 | `Source/pyramid.cpp:65` |
| B-R34 | ✅ | 🟠 | Aire latérale doublée et base oubliée | `Source/pyramid.cpp:75` |
| B-R35 | ⬜ | 🟠 | Cubique construite avec le décalage de la mauvaise extrémité | `Source/revolution-quadric.cpp:91` |
| B-R36 | ⬜ | 🟠 | Fonction inachevée renvoyant des vecteurs sans rapport avec la distance | `Source/scales.cpp:123` |
| B-R37 | ⬜ | 🟠 | Volume de la lentille d'intersection : d² utilisé à la place de d | `Source/sphere.cpp:781` |
| B-R38 | ⬜ | 🟠 | Centre du cercle d'intersection du mauvais côté | `Source/sphere.cpp:654` |
| B-R39 | ⬜ | 🟠 | Points renvoyés relatifs à l'origine au lieu du centre de sa | `Source/sphere.cpp:888` |
| B-R40 | ⬜ | 🟠 | Initialisation avec la distance au carré au lieu de la distance signée | `Source/sphereset.cpp:92` |
| B-R41 | ⬜ | 🟠 | Le facteur de tapering est ignoré | `Source/taper.cpp:86` |
| B-R42 | ⬜ | 🟠 | Constante de Lipschitz sous-estimée | `Source/twist.cpp:125` |
| B-R43 | ✅ | 🟡 | Vecteur distance de sens inversé dans la cavité intérieure | `Source/bubble.cpp:112` |
| B-R44 | ✅ | 🟡 | Membre statique jamais défini : variable globale Unit définie à la place | `Source/capsule.cpp:23` |
| B-R45 | ⬜ | 🟡 | Division par zéro si ra = 0, NaN si abs(ra − rb) > length | `Source/conesphere.cpp:37` |
| B-R46 | ✅ | 🟡 | sc_ha et sc_hra non mis à jour | `Source/conesphere.cpp:73` |
| B-R47 | ⬜ | 🟡 | NaN pour les points sur l'axe ou dans les plans des extrémités | `Source/conesphere.cpp:173` |
| B-R48 | ✅ | 🟡 | Initialisation par défaut incohérente (sinus/cosinus faux) | `Include/sector.h:13` |
| B-R49 | ✅ | 🟡 | NaN pour une sphère (a = b) | `Source/spheroid.cpp:160` |
| B-R50 | ⬜ | 🟡 | length et height non mis à jour | `Source/lens.cpp:139` |
| B-R51 | ⬜ | 🟡 | Boîte trop petite pour un angle supérieur à π/2 | `Source/sector.cpp:38` |
| B-R52 | ⬜ | 🟡 | Faux négatifs quand le plan d'intersection n'est pas entre les centres | `Source/sphere.cpp:645` |
| B-R53 | ⬜ | 🟡 | Accès hors bornes pour un ensemble vide | `Source/sphere.cpp:694` |
| B-R54 | ⬜ | 🔵 | Géométrie de la cage non arrondie incohérente avec GetMesh() | `Source/cage.cpp:101` |
| B-R55 | ⬜ | 🔵 | abs/min/max non qualifiés et variable inutilisée | `Source/capsule.cpp:869` |
| B-R56 | ⬜ | 🔵 | Code mort non compilable sous #ifdef SPHERE / #ifdef CYLINDER | `Source/conesphere.cpp:114` |
| B-R57 | ⬜ | 🔵 | Renvoie faux quand l'origine du rayon est dans le cylindre (à confirmer) | `Source/cylinder.cpp:718` |
| B-R58 | ⬜ | 🔵 | Le premier champ affiché est le rayon c, pas le centre | `Source/ellipsoid.cpp:273` |
| B-R59 | ✅ | 🔵 | Affiche « Box( » | `Source/icosidodecahedron.cpp:289` |
| B-R60 | ⬜ | 🔵 | Résultat de l'intersection ignoré | `Source/scales.cpp:24` |
| B-R61 | ✅ | 🔵 | Seules les sphères incluses dans une sphère suivante sont supprimées | `Source/sphereset.cpp:261` |
| B-R62 | ✅ | 🔵 | Membre statique déclaré mais jamais défini | `Include/sphereset.h:55` |
| B-R63 | ⬜ | 🟡 | Plans parallèles au rayon ignorés dans Intersect (dodécaèdre, icosaèdre, icosidodécaèdre) | `Source/dodecahedron.cpp:112` |

**16 / 63 corrigés** — restants : 3🔴 · 31🟠 · 7🟡 · 6🔵

### B-C — Courbes et surfaces paramétriques

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-C01 | ✅ | 🔴 | Dérivée seconde fausse : base de Bernstein de mauvais degré, dernier terme omis | `Source/bezier-curve.cpp:172` |
| B-C02 | ✅ | 🔴 | Même erreur que BezierCurve::Second() en 2D : dérivée seconde fausse | `Source/bezier-curve2.cpp:189` |
| B-C03 | ⬜ | 🟠 | Paramétrage transposé vs BezierPatch : normales du maillage évaluées en (v,u) | `Source/bezier-patch-cubic.cpp:68` |
| B-C04 | ✅ | 🟠 | Pente B incohérente avec Curvature() : courbures demandées non respectées | `Source/clothoid.cpp:59` |
| B-C05 | ✅ | 🟠 | Maximum de courbure : extrema intérieurs pris sans valeur absolue | `Source/curvecubic2.cpp:535` |
| B-C06 | ⬜ | 🟡 | Normale NaN sur les bords dégénérés (patches de la théière) | `Source/bezier-patch.cpp:131` |
| B-C07 | ⬜ | 🟡 | Aucune vérification nx,ny ≤ 16 : débordement des tableaux sur la pile | `Source/bezier-patch.cpp:105` |
| B-C08 | ⬜ | 🟡 | Avec 3 points de contrôle, la courbe s'arrête au milieu de [c1,c2] | `Source/bezier-quadric-curve.cpp:21` |
| B-C09 | ✅ | 🟡 | Approximation de Heald valable seulement pour t ≥ 0 | `Source/clothoid.cpp:75` |
| B-C10 | ⬜ | 🟡 | Boucle infinie quand a == b (entrée dégénérée, motif répété) | `Source/curvecubic.cpp:104` |
| B-C11 | ✅ | 🟡 | Pas de borne : boucle infinie sur courbe dégénérée, t > 1 si s > longueur | `Source/curvecubic.cpp:137` |
| B-C12 | ✅ | 🟡 | Abscisse curviligne sous-estimée : dernier segment [u_last, t] ignoré (motif répété) | `Source/curvecubic2.cpp:291` |
| B-C13 | ✅ | 🟡 | Repère initial dupliqué (motif répété) | `Source/curvecubic2.cpp:462` |
| B-C14 | ⬜ | 🟡 | Repère de Frenet NaN sur portion rectiligne ou tangente verticale | `Source/curvecubic.cpp:394` |
| B-C15 | ⬜ | 🟡 | Quadrique dégénérée en segment : distance réduite aux extrémités | `Source/curvequadric.cpp:202` |
| B-C16 | ⬜ | 🟡 | NaN si la quadrique est rectiligne (k = 0) ou linéaire (aa = 0) | `Source/curvequadric.cpp:364` |
| B-C17 | ⬜ | 🟡 | Accès hors limites si moins de 3 points | `Source/curvequadric.cpp:124` |
| B-C18 | ⬜ | 🔵 | Facteur nx (resp. ny) au lieu du degré nx-1 (ny-1) : norme des tangentes fausse | `Source/bezier-patch.cpp:157` |
| B-C19 | ⬜ | 🔵 | Arc ouvert dessiné comme un polygone fermé et rempli | `Source/clothoid.cpp:114` |
| B-C20 | ⬜ | 🔵 | Boucle flottante u < 1.0 : le dernier segment peut être ignoré | `Source/curvecubic.cpp:327` |
| B-C21 | ✅ | 🔵 | sqrt sans fabs sur une quartique développée | `Source/curvecubic2.cpp:96` |
| B-C22 | ⬜ | 🔵 | Paramètre du point le plus proche calculé puis perdu | `Source/curvequadric2.cpp:657` |
| B-C23 | ⬜ | 🔵 | Classes polymorphes sans destructeur virtuel | `Include/parametric.h:7` |

**9 / 23 corrigés** — restants : 0🔴 · 1🟠 · 8🟡 · 5🔵

### B-S — Champs scalaires et grilles

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-S01 | ✅ | 🔴 | Polynôme de degré 0 : aucune racine trouvée pour n ≥ 3 | `Source/algebraic.cpp:415` |
| B-S02 | ✅ | 🔴 | Rapport inversé : volume faux | `Source/analyticscalarfield.cpp:1056` |
| B-S03 | ✅ | 🔴 | Double décalage des indices : accès hors bornes | `Source/voxel.cpp:276` |
| B-S04 | ✅ | 🔴 | Voisinage 3×3 mal rempli | `Source/analyticscalarfield2.cpp:42` |
| B-S05 | ✅ | 🔴 | Différences secondes divisées par ε au lieu de ε² | `Source/analyticscalarfield2.cpp:71` |
| B-S06 | ⬜ | 🔴 | Récursion infinie : la résolution des niveaux grossiers ne diminue pas | `Source/diffusion.cpp:254` |
| B-S07 | ⬜ | 🔴 | Lecture hors bornes aux niveaux grossiers du V-cycle | `Source/diffusion.cpp:110` |
| B-S08 | ⬜ | 🔴 | Écriture dans un QVector vide | `Source/integerfield2.cpp:105` |
| B-S09 | ⬜ | 🔴 | Écriture hors bornes sur la pile, pas de grille ignoré | `Source/scalarfield.cpp:803` |
| B-S10 | ⬜ | 🔴 | Pas d'indice incompatibles avec la disposition mémoire | `Source/scalarfield.cpp:163` |
| B-S11 | ✅ | 🔴 | Boucle interne qui incrémente i au lieu de j | `Source/scalarfield2.cpp:1381` |
| B-S12 | ✅ | 🔴 | Bornes de boucle inversées : accès hors bornes | `Source/scalarfield2.cpp:3246` |
| B-S13 | ✅ | 🔴 | Chemin AVX : calcule a + (1−t)·b au lieu de (1−t)·a + t·b | `Source/scalarfield2.cpp:2481` |
| B-S14 | ⬜ | 🟠 | Constante de Lipschitz toujours nulle | `Source/algebraic.cpp:59` |
| B-S15 | ⬜ | 🟠 | Centre de gravité non normalisé | `Source/analyticscalarfield.cpp:1077` |
| B-S16 | ⬜ | 🟠 | Configuration non permutée : distance signée fausse | `Source/voxel.cpp:324` |
| B-S17 | ✅ | 🟠 | Opérateurs composés sans effet | `Include/scalaralpha.h:106` |
| B-S18 | ✅ | 🟠 | Courbure moyenne jamais calculée, gaussienne écrasée | `Source/analyticscalarfield2.cpp:116` |
| B-S19 | ⬜ | 🟠 | Voisins verticaux indexés avec ±ny au lieu de ±nx | `Source/scalarfield2-smooth.cpp:96` |
| B-S20 | ⬜ | 🟠 | Distance calculée au point central fixe | `Source/hexagonscalarfield2.cpp:265` |
| B-S21 | ⬜ | 🟠 | Pas de grille hexagonale ignoré : gradient et Lipschitz faux | `Source/hexagonscalarfield2.cpp:494` |
| B-S22 | ⬜ | 🟠 | Indices de stockage faux : conversion axial → offset erronée (cause : B-N11) | `Source/hexagonscalarfield2.cpp:184` |
| B-S23 | ⬜ | 🟠 | Pas d'échantillonnage égal à la taille du domaine | `Source/hexagonscalarfield2.cpp:45` |
| B-S24 | ⬜ | 🟠 | Dérivées croisées deux fois trop grandes | `Source/scalarfield.cpp:772` |
| B-S25 | ⬜ | 🟠 | Le champ n'est pas redimensionné après lecture de l'en-tête | `Source/scalarfield.cpp:1691` |
| B-S26 | ✅ | 🟠 | Pas de grille faux pour ∂²/∂y² et pour le terme croisé | `Source/scalarfield2.cpp:1583` |
| B-S27 | ⬜ | 🟠 | Fenêtre transposée : lecture hors bornes sur grille non carrée | `Source/scalarfield2.cpp:2733` |
| B-S28 | ⬜ | 🟠 | Grille non carrée : boîte doublée et transposition au lieu de rotation | `Source/scalarfield2.cpp:404` |
| B-S29 | ⬜ | 🟠 | Marching squares inachevé : seuls les cas 14 et 15 produisent des triangles | `Source/scalarfield2.cpp:3629` |
| B-S30 | ⬜ | 🟡 | Débordement de pile si n > 8, doc annonçant degré 15 | `Source/algebraic.cpp:131` |
| B-S31 | ⬜ | 🟡 | Filtrage des racines incorrect après permutation | `Source/algebraic.cpp:387` |
| B-S32 | ⬜ | 🟡 | Accès hors bornes pour n = 0 (constructeur par défaut) | `Source/algebraic.cpp:171` |
| B-S33 | ⬜ | 🟡 | Tableaux statiques partagés : course de données en rendu parallèle | `Source/algebraic.cpp:10` |
| B-S34 | ⬜ | 🟡 | Pas limité à epsilon (Min au lieu de Max) et convention de signe ignorée | `Source/analyticscalarfield.cpp:672` |
| B-S35 | ⬜ | 🟡 | Paramètre epsilon passé comme longueur à Dichotomy | `Source/analyticscalarfield.cpp:738` |
| B-S36 | ⬜ | 🟡 | Nombre de cellules utilisé comme nombre de sommets | `Include/array.h:921` |
| B-S37 | ⬜ | 🟡 | Champ transposé | `Source/analyticscalarfield.cpp:375` |
| B-S38 | ⬜ | 🟡 | Pile statique globale : non réentrant | `Source/analyticscalarfield.cpp:486` |
| B-S39 | ⬜ | 🟡 | Borne maximale des cellules = nx−1 au lieu de nx−2 | `Source/array.cpp:231` |
| B-S40 | ⬜ | 🟡 | Clamp au nombre de sommets et taille fixe des tables | `Source/arrayindex.cpp:66` |
| B-S41 | ⬜ | 🟡 | Buffer GL in_laplacian_buffer jamais libéré | `Source/diffusion.cpp:46` |
| B-S42 | ⬜ | 🟡 | Une couche de cubes de trop hors de la boîte | `Source/scalarfield-mesh.cpp:133` |
| B-S43 | ⬜ | 🟡 | Collisions de clés de cubes pour des coordonnées négatives ou grandes | `Source/scalarfield-wyvill.cpp:466` |
| B-S44 | ⬜ | 🟡 | Chargements/écritures AVX alignés sur des données non alignées sur 32 octets (à confirmer) | `Source/scalarfield.cpp:287` |
| B-S45 | ⬜ | 🟡 | Axes de l'image inversés : accès hors bornes si l'image n'est pas carrée | `Source/scalarfield.cpp:1790` |
| B-S46 | ⬜ | 🟡 | Fuites mémoire à chaque appel | `Source/scalarfield2.cpp:3339` |
| B-S47 | ⬜ | 🟡 | Points intérieurs initialisés avec la valeur du champ au lieu de 0 | `Source/scalarfield2.cpp:3346` |
| B-S48 | ⬜ | 🟡 | Orientation du cas 1 incohérente avec les autres cas | `Source/scalarfield2.cpp:2957` |
| B-S49 | ✅ | 🟡 | Accès hors bornes pour le percentile 1.0 | `Source/scalarfield2.cpp:729` |
| B-S50 | ⬜ | 🟡 | Seuils des classes sans décalage par le minimum | `Source/scalarfield2.cpp:662` |
| B-S51 | ⬜ | 🟡 | Noyau de largeur nulle : champ rempli de NaN | `Source/scalarfield2.cpp:3451` |
| B-S52 | ⬜ | 🟡 | Écriture de fichiers PNG de débogage à chaque itération | `Source/scalarfield2-morph.cpp:51` |
| B-S53 | ✅ | 🔵 | Variable d'erreur non initialisée | `Source/analyticscalarfield.cpp:1066` |
| B-S54 | ⬜ | 🔵 | Convention de signe codée en dur | `Source/analyticscalarfield.cpp:236` |
| ~~B-S55~~ | — | 🔵 | ~~Boîte de taille double~~ (faux positif) | `Source/analyticscalarfield.cpp:340` |
| B-S56 | ⬜ | 🔵 | b non initialisé si n = 0 | `Source/analyticscalarfield.cpp:428` |
| B-S57 | ⬜ | 🔵 | Pas de différences finies trop petit pour une dérivée seconde | `Source/analyticscalarfield.cpp:161` |
| B-S58 | ⬜ | 🔵 | Origine des lignes fausse et image non initialisée | `Source/array2.cpp:288` |
| B-S59 | ⬜ | 🔵 | Tailles des contraintes non vérifiées | `Source/diffusion.cpp:28` |
| B-S60 | ⬜ | 🔵 | Trace console dans la bibliothèque, n non puissance de 2 | `Source/scalarfield-mesh.cpp:486` |
| B-S61 | ⬜ | 🔵 | Division par zéro et cache global non réentrant | `Source/scalarfield.cpp:1256` |
| B-S62 | ⬜ | 🔵 | Racine d'un discriminant négatif | `Source/scalarfield.cpp:1728` |
| B-S63 | ⬜ | 🔵 | Composante y mise à l'échelle par le pas en x | `Source/scalarfield2.cpp:1531` |
| B-S64 | ⬜ | 🔵 | Champ constant envoyé sur 1, en contradiction avec « preserves 0 » | `Source/scalarfield2.cpp:2016` |
| B-S65 | ⬜ | 🔵 | Fichier non carré : taille du champ incohérente | `Source/scalarfield2.cpp:207` |
| B-S66 | ⬜ | 🟡 | `K(box, nx, ny, nz)` stocke la constante de Lipschitz dans un Voxel d'entiers | `Source/analyticscalarfield.cpp:166` |

**13 / 65 corrigés** — restants : 5🔴 · 13🟠 · 23🟡 · 11🔵

### B-V — Champs vectoriels et de couleur

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-V01 | ⬜ | 🔴 | Écriture dans un QVector vide (champ jamais dimensionné) | `Source/vectorfield2.cpp:43` |
| B-V02 | ⬜ | 🔴 | Indice source non borné : lecture hors du champ | `Source/vectorfield2.cpp:238` |
| B-V03 | ⬜ | 🟠 | La norme résultante n'est pas ramenée dans [0,1] | `Source/vectorfield2.cpp:804` |
| B-V04 | ✅ | 🟡 | Schéma « prédiction-correction » seulement d'ordre 1 | `Source/vectorfield2.cpp:1084` |
| B-V05 | ⬜ | 🟡 | _mm256_load_pd/_mm256_store_pd sur données non garanties alignées (à confirmer) | `Source/vectorfield2.cpp:608` |
| B-V06 | ⬜ | 🟡 | Accès hors limites pour une grille de largeur ou hauteur 1 | `Source/vectorfield2.cpp:563` |
| B-V07 | ⬜ | 🔵 | Points sur le bord max du domaine considérés comme extérieurs | `Source/vectorfield2.cpp:1023` |
| B-V08 | ⬜ | 🔵 | Image 16 bits remplie avec des valeurs 8 bits | `Source/vectorfield2.cpp:1001` |
| B-V09 | ⬜ | 🔵 | Point de départ compté deux fois | `Source/vectorfield2.cpp:1191` |
| B-V10 | ✅ | 🔵 | Image non initialisée | `Source/vectorfield2.cpp:467` |
| B-V11 | ✅ | 🔵 | L'alpha est aussi multiplié | `Source/colorfield.cpp:494` |
| B-V12 | ⬜ | 🔵 | Canal alpha perdu | `Source/colorfield.cpp:142` |
| B-V13 | ✅ | 🔵 | Interpolation « bicubique » à dérivées nulles | `Source/colorfield.cpp:219` |
| B-V14 | ✅ | 🔵 | Code mort, doc en /* | `Source/colorfield.cpp:228` |
| B-V15 | ⬜ | 🟡 | Dérivées d'alpha égales à 1 dans BiCubic() | `Source/colorfield.cpp:230` |

**5 / 15 corrigés** — restants : 2🔴 · 1🟠 · 3🟡 · 4🔵

### B-N — Bruit et échantillonnage

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-N01 | ✅ | 🔴 | Lecture hors bornes de directionmap / frequencymap | `Source/noise-phasor.cpp:45` |
| B-N02 | ⬜ | 🔴 | Débordement du tableau local subset[37] en 3D | `Source/eden.cpp:112` |
| B-N03 | ⬜ | 🔴 | Utilise l'ensemble frontière s au lieu du tableau strahler | `Source/eden2.cpp:342` |
| B-N04 | ⬜ | 🔴 | Table d'enfants entièrement fausse | `Source/eden2.cpp:241` |
| B-N05 | ⬜ | 🔴 | Indice de cellule borné à nx-1 au lieu de nx-2 : écriture hors tableau | `Source/relaxedarray.cpp:52` |
| B-N06 | ⬜ | 🔴 | Points ramenés modulo la taille d'une cellule au lieu de celle de la tuile | `Source/sampling.cpp:107` |
| B-N07 | ✅ | 🟠 | Impulsions dans [-0.5,0.5] au lieu de [0,1] : discontinuités aux bords de cellules | `Source/phasor.cpp:64` |
| B-N08 | ✅ | 🟠 | Indice de cellule par troncature au lieu de floor | `Source/phasor.cpp:16` |
| B-N09 | ⬜ | 🟠 | Les octaves deviennent plus grossières au lieu de plus fines | `Source/noise-worm.cpp:119` |
| B-N10 | ⬜ | 🟠 | Aire de cellule 4 fois trop grande | `Source/hexagonarray2.cpp:115` |
| B-N11 | ⬜ | 🟠 | Conversion axiale vers décalée de signe inversé, et troncature au lieu de floor | `Source/hexagonarray2.cpp:38` |
| B-N12 | ⬜ | 🟠 | Grille d'accélération trop grossière : propriété de Poisson non garantie | `Source/sampling.cpp:122` |
| B-N13 | ⬜ | 🟠 | Mauvais décalage torique près des bords haut et bas | `Source/sampling2.cpp:271` |
| B-N14 | ✅ | 🟡 | Accès hors bornes de amplitudemap si le point sort du carré unité | `Source/noise-phasor.cpp:157` |
| B-N15 | ✅ | 🟡 | Plage [-2,0] au lieu de [-1,1] annoncé | `Source/noise-voronoi.cpp:58` |
| B-N16 | ⬜ | 🟡 | Constante de Lipschitz sous-estimée (2 au lieu de ≥3) | `Source/noise.cpp:436` |
| B-N17 | ✅ | 🟡 | Disque inclus dans un disque d'indice inférieur conservé | `Source/discset.cpp:261` |
| B-N18 | ⬜ | 🟡 | Cache de voisins mélangeant distances et distances au carré | `Source/discsystem.cpp:150` |
| B-N19 | ⬜ | 🟡 | La particule fille reçoit un rayon r/2 au lieu de r/√2 | `Source/discsystem.cpp:284` |
| B-N20 | ⬜ | 🟡 | Écriture hors tableau quand maxi ≥ size | `Source/disctilelink.cpp:106` |
| B-N21 | ⬜ | 🟡 | Boucle infinie si l'ensemble de germes est vide | `Source/dla.cpp:54` |
| B-N22 | ⬜ | 🟡 | Débordement possible de subset[16] en 2D | `Source/eden2.cpp:143` |
| B-N23 | ⬜ | 🟡 | Indice -1 quand aucune croissance n'est possible | `Source/eden2.cpp:719` |
| B-N24 | ⬜ | 🟡 | La cellule ajoutée n'est pas vivante | `Source/eden.cpp:177` |
| B-N25 | ⬜ | 🟡 | Corruption de l'arbre pour m = 1 et division par zéro pour m = 0 | `Source/eden2.cpp:593` |
| B-N26 | ⬜ | 🟡 | Dernière colonne et dernière ligne exclues du masque | `Source/hexagonarray2.cpp:80` |
| B-N27 | ⬜ | 🟡 | Accès hors bornes pour un point hors de la grille | `Include/relaxedarray.h:51` |
| B-N28 | ⬜ | 🟡 | Indice de grille négatif si la tuile a moins de 3 cellules par axe | `Source/sampling.cpp:168` |
| B-N29 | ⬜ | 🔵 | Tableau de 8 décalages initialisé avec 7 valeurs | `Source/noise-worm.cpp:82` |
| B-N30 | ⬜ | 🔵 | Générateur statique partagé modifié dans une méthode const (non thread-safe) | `Source/noise-voronoi.cpp:128` |
| B-N31 | ⬜ | 🔵 | Code mort | `Source/noise-voronoi.cpp:193` |
| B-N32 | ⬜ | 🔵 | Plage effective ≈[-0.5,0.5] au lieu de [-1,1] annoncé (à confirmer) | `Source/noise-simplex.cpp:142` |
| B-N33 | ⬜ | 🔵 | Division par zéro pour alpha = 1 | `Source/turbulence.cpp:191` |
| B-N34 | ⬜ | 🔵 | Particule sautée après suppression | `Source/discsystem.cpp:374` |
| B-N35 | ⬜ | 🔵 | Code mort et normalisation d'un gradient nul | `Source/discsystem.cpp:335` |
| B-N36 | ⬜ | 🔵 | Le dernier échantillon de chaque tuile n'est jamais retenu | `Source/disctiles.cpp:133` |
| B-N37 | ⬜ | 🔵 | Débordement d'entier signé (UB formel) dans le hachage | `Source/disctiles.cpp:24` |
| B-N38 | ⬜ | 🔵 | Tableau de sortie sans capacité | `Source/dla2.cpp:83` |
| B-N39 | ⬜ | 🔵 | Nombre d'éléments sous-estimé d'une unité | `Source/relaxedarray2.cpp:241` |
| B-N40 | ⬜ | 🔵 | Crash sur tuile vide | `Source/sampling2.cpp:609` |
| B-N41 | ⬜ | 🔵 | Dernier échantillon de la tuile jamais retenu | `Source/sampling2.cpp:437` |

**6 / 41 corrigés** — restants : 5🔴 · 5🟠 · 12🟡 · 13🔵

### B-T — Maillages et graphes géométriques

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-T01 | ⬜ | 🔴 | Grilles : indice linéaire j*nx+i avec i<ny (débordement si nx>ny) | `Source/mesh.cpp:251` |
| B-T02 | ⬜ | 🔴 | Indices de normales positifs non décrémentés (test inversé) | `Source/mesh.cpp:1628` |
| B-T03 | ⬜ | 🟠 | Indexation relative (indices négatifs) fausse | `Source/mesh.cpp:1596` |
| B-T04 | ⬜ | 🟠 | Espace en fin de ligne f : chargement abandonné ; narray désynchronisé sans normales | `Source/mesh.cpp:1578` |
| B-T05 | ⬜ | 🟠 | Parseur OBJ regex : sommets ignorés (indices décalés), faces sans normale, quads perdus | `Source/mesh.cpp:1410` |
| B-T06 | ⬜ | 🟠 | Normales indexées comme les sommets | `Source/mesh.cpp:645` |
| B-T07 | ⬜ | 🟠 | Inversion de l'orientation sans permuter narray | `Source/mesh.cpp:2081` |
| B-T08 | ⬜ | 🟠 | Les 20 faces triangulaires sont commentées : maillage troué | `Source/mesh.cpp:2862` |
| B-T09 | ⬜ | 🟠 | Poids tronqué en entier | `Include/geometricgraph.h:17` |
| B-T10 | ⬜ | 🟡 | Cône pointu avec apex en a : repère non recalculé après l'échange | `Source/mesh.cpp:2019` |
| B-T11 | ⬜ | 🟡 | Enroulement opposé à la normale z | `Source/mesh.cpp:1893` |
| B-T12 | ⬜ | 🟡 | Enroulement des faces indépendant du sens de la normale | `Source/mesh.cpp:3291` |
| B-T13 | ✅ | 🟡 | Facteur négatif : normales inversées mais pas l'enroulement | `Source/mesh.cpp:2132` |
| B-T14 | ⬜ | 🟡 | Itération avant : triangles déplacés jamais examinés | `Source/mesh2.cpp:703` |
| B-T15 | ⬜ | 🟡 | Tolérance absolue eps = 1e-4 sur des distances au carré | `Source/mesh2.cpp:300` |
| B-T16 | ⬜ | 🔵 | Normales opposées à l'enroulement des quadrangles | `Source/mesh.cpp:1259` |
| B-T17 | ⬜ | 🔵 | Hémisphères : anneau d'équateur dupliqué, dernier anneau confondu avec le pôle | `Source/mesh.cpp:370` |
| B-T18 | ✅ | 🔵 | Division par zéro sur maillage vide | `Source/mesh.cpp:489` |
| B-T19 | ⬜ | 🔵 | Précision par défaut de QTextStream (6 chiffres) | `Source/mesh.cpp:2178` |
| B-T20 | ⬜ | 🔵 | Points cocycliques : triangles superposés ; orientation non garantie | `Source/mesh2.cpp:420` |

**2 / 20 corrigés** — restants : 2🔴 · 7🟠 · 5🟡 · 4🔵

### B-U — Utilitaires, couleur, GPU

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-U01 | ✅ | 🔴 | La roue chromatique renvoie toujours du blanc | `Source/color.cpp:255` |
| B-U02 | ✅ | 🔴 | Ancres inversées en ordre décroissant : palette inversée entièrement fausse | `Source/palette.cpp:253` |
| B-U03 | ⬜ | 🟠 | Encodage compact incohérent (RGBA vs ARGB), sans clamp | `Include/color.h:142` |
| B-U04 | ⬜ | 🟠 | Échelle XYZ ×100 incohérente avec Lab(const Xyz&) | `Source/color-lab.cpp:52` |
| B-U05 | ⬜ | 🟠 | Plages d'axes fausses : branche initiale morte et min au lieu de max | `Source/chart.cpp:194` |
| B-U06 | ⬜ | 🟠 | Fuite de QChart/QChartView et fenêtre ouverte à chaque export | `Include/chart.h:9` |
| B-U07 | ⬜ | 🟠 | Modulo 2014 au lieu de 1024 | `Source/cpu.cpp:302` |
| B-U08 | ✅ | 🟡 | Recherche du segment par soustractions : dépassement d'indice possible, coût O(n) | `Source/color.cpp:137` |
| B-U09 | ⬜ | 🟡 | Raccourci « diverging » (type==1) : ignore les ancres et ne borne pas t | `Source/palette.cpp:161` |
| B-U10 | ⬜ | 🟡 | Pas de garde sur palette vide / tailles incohérentes | `Source/palette.cpp:144` |
| B-U11 | ⬜ | 🟡 | Crash sur ensemble vide, UB si toutes les valeurs sont égales | `Source/histogram.cpp:51` |
| B-U12 | ⬜ | 🟡 | Division par zéro si l'histogramme est vide | `Source/histogram.cpp:227` |
| B-U13 | ⬜ | 🟡 | Déréférencement de pointeur nul sans série | `Source/chart.cpp:316` |
| B-U14 | ⬜ | 🟡 | Accès x[0] sur vecteur vide et fuite de la série en cas d'erreur | `Source/chart.cpp:59` |
| B-U15 | ⬜ | 🟡 | dwMemoryLoad (pourcentage) divisé par 1024 | `Source/cpu.cpp:341` |
| B-U16 | ⬜ | 🟡 | Valeurs 64 bits tronquées en int par LongInteger(int) | `Source/cpu.cpp:342` |
| B-U17 | ⬜ | 🟡 | Casse incorrecte QPainterpath : échec de compilation sous Linux | `Include/core-qt.h:6` |
| B-U18 | ⬜ | 🟡 | Les items enfants sont retournés deux fois | `Source/system.cpp:166` |
| B-U19 | ⬜ | 🟡 | Échec de compilation/linkage ignoré | `Source/gpu-shader.cpp:261` |
| B-U20 | ⬜ | 🟡 | Le handle GL n'est pas remis à 0 après suppression | `Include/gpu-wrapper.h:135` |
| B-U21 | ⬜ | 🟡 | Fuite de texture si Generate est rappelé | `Source/gpu-texture.cpp:87` |
| B-U22 | ⬜ | 🟡 | Pas de contrôle de taille, poursuite après erreur de format | `Source/gpu-texture.cpp:112` |
| B-U23 | ✅ | 🔵 | Interpolation de teinte naïve (teinte -1 des gris, pas de bouclage), alpha perdu en CMYK | `Source/color.cpp:165` |
| B-U24 | ⬜ | 🔵 | Le canal alpha est réinitialisé à 1 | `Source/color.cpp:272` |
| B-U25 | ⬜ | 🔵 | Méthode déclarée mais jamais définie | `Include/color.h:300` |
| B-U26 | ✅ | 🔵 | La fréquence est perdue | `Include/palette.h:66` |
| B-U27 | ⬜ | 🔵 | Fraction cumulée mal normalisée | `Source/histogram.cpp:224` |
| B-U28 | ⬜ | 🔵 | Largeur de bac calculée avec size-1 | `Include/histogram.h:109` |
| B-U29 | ⬜ | 🔵 | Titre codé en dur et représentations différentes | `Source/histogram.cpp:85` |
| B-U30 | ⬜ | 🔵 | Axes ajoutés à chaque appel | `Source/chart.cpp:216` |
| B-U31 | ⬜ | 🔵 | SetNbTickX appelé deux fois | `Source/chart.cpp:344` |
| B-U32 | ⬜ | 🔵 | Valeur tronquée renvoyée comme valide, tampon statique non réentrant | `Source/cpu.cpp:29` |
| B-U33 | ⬜ | 🔵 | Code mort et effet de bord sur la scène | `Source/system.cpp:52` |
| B-U34 | ⬜ | 🔵 | completeSuffix() ignore les fichiers à plusieurs points | `Source/system.cpp:286` |
| B-U35 | ⬜ | 🔵 | Taille par texel multipliée par le nombre de canaux | `Source/gpu-texture.cpp:59` |
| B-U36 | ⬜ | 🔵 | GL_R32F associé à Format_Grayscale16 (à confirmer) | `Source/gpu-texture.cpp:161` |
| B-U37 | ⬜ | 🔵 | Ligne #version perdue quand definitions est vide | `Source/gpu-shader.cpp:131` |
| B-U38 | ⬜ | 🔵 | Regex non ancrées : numéros de ligne Mesa faux, lignes du log sautées | `Source/gpu-shader.cpp:356` |
| B-U39 | ⬜ | 🔵 | #include en tout début de fichier non reconnu | `Source/gpu-shader.cpp:48` |
| B-U40 | ⬜ | 🔵 | glObjectLabel appelé selon une macro de compilation (à confirmer) | `Source/gpu-shader.cpp:222` |
| B-U41 | ⬜ | 🔵 | Mise à jour de Jacobi incohérente avec la convention du Laplacien (à confirmer) | `Shaders/diffusion.glsl:66` |
| B-U42 | ⬜ | 🔵 | Voisins ±1 en indice 1D : bouclage d'une ligne à l'autre (code mort) | `Shaders/diffusion.glsl:37` |

**5 / 42 corrigés** — restants : 0🔴 · 5🟠 · 14🟡 · 18🔵

### D — Documentation

| ID | ✅ | Domaine | Description courte | Fichier |
|---|:---:|---|---|---|
| D-M01 | ⬜ | Mathématiques | Dérivée documentée fausse (dénominateur non élevé au carré) | `Include/mathematics.h:866` |
| D-M02 | ✅ | Mathématiques | Doc inexacte pour x<0 | `Include/mathematics.h:799` |
| D-M03 | ✅ | Mathématiques | Intervalle signé annoncé [-π,π[ alors que le code produit ]-π,π] | `Source/mathematics.cpp:266` |
| D-M04 | ⬜ | Mathématiques | n documenté « Degree » alors que c'est le nombre de termes (degré n-1) | `Source/mathematics.cpp:373` |
| D-M05 | ⬜ | Mathématiques | Doc « of two doubles » sur les surcharges float | `Include/mathematics.h:57` |
| D-M06 | ⬜ | Mathématiques | Exemple de code équivalent faux | `Include/linear.h:246` |
| D-M07 | ✅ | Mathématiques | Doc copiée « Compute the Lipschitz constant » | `Include/smooth.h:65` |
| D-M08 | ⬜ | Mathématiques | Doc « superior » au lieu de « inferior » | `Source/ia.cpp:511` |
| D-M09 | ✅ | Mathématiques | Commentaire /* au lieu de /*! ignoré par Doxygen | `Include/aa.h:117` |
| D-M10 | ⬜ | Mathématiques | FAQ « ascending order » mais Vector::Sort trie en décroissant (Vector2 en croissant) | `Source/evector.cpp:42` |
| D-M11 | ✅ | Mathématiques | « positive cosine » alors que le cosinus renvoyé est signé | `Source/evector.cpp:117` |
| D-M12 | ✅ | Mathématiques | Doc de classe « three dimensions » et pcg2d() non documentée | `Source/ivector2.cpp:8` |
| D-M13 | ⬜ | Mathématiques | Nom trompeur : max des |coefficients|, pas la norme infinie matricielle | `Source/matrix.cpp:599` |
| D-M14 | ⬜ | Mathématiques | Nom trompeur : la « shear » est placée dans la dernière ligne (termes projectifs) | `Source/matrix4.cpp:485` |
| D-M15 | ⬜ | Mathématiques | Noms de paramètres aij trompeurs (stockage en ordre colonne) | `Include/matrix.h:126` |
| D-P01 | ⬜ | Polynômes, aléatoire, graphes | Cite Quartic::Quartic() au lieu de Quartic::Solve(), ordre des racines non précisé | `Source/polynomial-solver.cpp:232` |
| D-P02 | ✅ | Polynômes, aléatoire, graphes | \param atmin, atmax documentés par « ? » | `Source/polynomial-solver.cpp:132` |
| D-P03 | ⬜ | Polynômes, aléatoire, graphes | Nommée « bissection » alors qu'il s'agit d'une regula falsi (variante Illinois) | `Source/polynomial.cpp:147` |
| D-P04 | ⬜ | Polynômes, aléatoire, graphes | Décrite comme « Bicubic interpolation » alors que c'est une interpolation cubique 1D | `Source/cubic.cpp:207` |
| D-P05 | ⬜ | Polynômes, aléatoire, graphes | \param p et q inversés | `Source/quartic.cpp:356` |
| D-P06 | ⬜ | Polynômes, aléatoire, graphes | //! Empty. placé après la déclaration documente la déclaration suivante | `Include/quartic.h:11` |
| D-P07 | ⬜ | Polynômes, aléatoire, graphes | Doc « over a given interval » et « sorted roots » fausses | `Source/quintic.cpp:133` |
| D-P08 | ⬜ | Polynômes, aléatoire, graphes | `Integral(a0)` : brief faux, précondition n < MaxDegree non vérifiée | `Source/polynomial.cpp:459` |
| D-G01 | ⬜ | Géométrie 2D | Doc « squared distance » pour une distance signée | `Source/annulus2.cpp:97` |
| D-G02 | ⬜ | Géométrie 2D | /* au lieu de /*! et commentaire de classe faux | `Source/boxset2.cpp:17` |
| D-G03 | ⬜ | Géométrie 2D | Direction non normalisée, contrairement au constructeur depuis un segment | `Source/line2.cpp:24` |
| D-G04 | ⬜ | Géométrie 2D | /* au lieu de /*! et \param faux | `Source/circle2.cpp:566` |
| D-G05 | ⬜ | Géométrie 2D | c documenté comme « point sur l'arc » alors que c'est le centre | `Source/circlearc2.cpp:28` |
| D-G06 | ⬜ | Géométrie 2D | Valeur de retour documentée de façon incorrecte | `Source/convex2.cpp:133` |
| D-G07 | ⬜ | Géométrie 2D | Nom trompeur : R() renvoyant une distance signée non carrée | `Source/disc2.cpp:83` |
| D-G08 | ⬜ | Géométrie 2D | Doc « Euclidean distance » pour une distance au carré ; Inside parle de « lens » | `Source/disctwins2.cpp:36` |
| D-G09 | ✅ | Géométrie 2D | Équation de l'ellipse fausse et \brief copiés-collés | `Source/ellipse2.cpp:35` |
| D-G10 | ✅ | Géométrie 2D | Doc « squared Euclidean distance » pour une distance signée | `Source/framebox2.cpp:59` |
| D-G11 | ✅ | Géométrie 2D | Valeur de la constante mal décrite | `Include/hexagon.h:55` |
| D-G12 | ✅ | Géométrie 2D | Signification de s et t mal documentée | `Include/octogon.h:114` |
| D-G13 | ✅ | Géométrie 2D | Blocs \brief vides | `Include/irect.h:15` |
| D-G14 | ✅ | Géométrie 2D | « Major / minor radius » alors que a est la demi-longueur sur l'axe | `Include/ellipse.h:60` |
| D-G15 | ✅ | Géométrie 2D | Doc « centres des particules » alors que le rayon est déjà ajouté | `Source/particle2.cpp:107` |
| D-G16 | ✅ | Géométrie 2D | « Flat topped » alors que le pentagone a un sommet en haut | `Source/pentagon2.cpp:9` |
| D-G17 | ✅ | Géométrie 2D | Doc « distance » pour une distance au carré | `Source/polygon2.cpp:430` |
| D-G18 | ✅ | Géométrie 2D | Doc du constructeur ambiguë sur a, b (demi-dimensions) et le centre | `Source/rectangle2.cpp:18` |
| D-G19 | ✅ | Géométrie 2D | « Signed » renvoie une distance non signée ; doc « intensity » | `Source/revolution-quadric2.cpp:110` |
| D-G20 | ✅ | Géométrie 2D | /* au lieu de /*! et « circle » au lieu de « sector » | `Source/sector2.cpp:114` |
| D-G21 | ✅ | Géométrie 2D | Commandes Doxygen mal formées | `Source/twistsector2.cpp:7` |
| D-K01 | ✅ | Géométrie 3D de base | Paramètre s mal documenté | `Source/axis.cpp:164` |
| D-K02 | ✅ | Géométrie 3D de base | Documentation de la caméra trompeuse | `Source/camera.cpp:283` |
| D-K03 | ⬜ | Géométrie 3D de base | Fonction non documentée et sémantique implicite | `Source/cameraframes.cpp:165` |
| D-K04 | ✅ | Géométrie 3D de base | Constructeurs mal documentés | `Source/framebox.cpp:29` |
| D-K05 | ✅ | Géométrie 3D de base | Demi-côtés présentés comme des tailles | `Include/rectangle.h:15` |
| D-K06 | ✅ | Géométrie 3D de base | Doc du constructeur et de l'épaisseur erronée | `Source/slab.cpp:19` |
| D-K07 | ✅ | Géométrie 3D de base | « Minkowski sum » alors que le code calcule l'enveloppe de l'union | `Source/ktope.cpp:183` |
| D-K08 | ✅ | Géométrie 3D de base | Valeur retournée mal décrite | `Source/simplex.cpp:24` |
| D-K09 | ✅ | Géométrie 3D de base | Blocs /* au lieu de /*! ignorés par Doxygen | `Source/quad.cpp:142` |
| D-K10 | ✅ | Géométrie 3D de base | Affirmation fausse : Axis n'hérite pas de Segment | `Source/segment.cpp:11` |
| D-K11 | ✅ | Géométrie 3D de base | Documentation contredisant le code | `Source/triangle.cpp:256` |
| D-K12 | ⬜ | Géométrie 3D de base | Commentaire sur une option inexistante et retour non documenté | `Source/triangle-intersection.cpp:6` |
| D-R01 | ✅ | Primitives 3D volumiques | \param dans le désordre | `Source/bubble.cpp:21` |
| D-R02 | ✅ | Primitives 3D volumiques | Commentaire Doxygen mal formé et trompeur | `Include/cage.h:12` |
| D-R03 | ✅ | Primitives 3D volumiques | Distance non signée documentée comme signée | `Source/conetube.cpp:113` |
| D-R04 | ✅ | Primitives 3D volumiques | « Radius » ambigu | `Include/cuboctahedron.h:11` |
| D-R05 | ✅ | Primitives 3D volumiques | Formule de la hauteur fausse dans le commentaire | `Include/lens.h:16` |
| D-R06 | ⬜ | Primitives 3D volumiques | Le code « équivalent » donné en documentation ne l'est pas | `Include/lens.h:108` |
| D-R07 | ✅ | Primitives 3D volumiques | Commentaires copiés-collés erronés | `Include/icosahedron.h:45` |
| D-R08 | ✅ | Primitives 3D volumiques | Mauvais fichier d'en-tête dans \class | `Source/cylinderbox.cpp:6` |
| D-R09 | ✅ | Primitives 3D volumiques | Commentaire de fonction en /* au lieu de /*! | `Source/spheroid.cpp:143` |
| D-R10 | ✅ | Primitives 3D volumiques | Doc copiée de Signed() | `Source/hemisphere.cpp:168` |
| D-R11 | ✅ | Primitives 3D volumiques | Mauvais en-tête et doc de Signed() trompeuse | `Source/icosidodecahedron.cpp:8` |
| D-R12 | ✅ | Primitives 3D volumiques | \param manquants, doc dupliquée | `Source/icosahedron.cpp:245` |
| D-R13 | ✅ | Primitives 3D volumiques | Doc copiée de la version 2D | `Source/particle.cpp:93` |
| D-R14 | ✅ | Primitives 3D volumiques | Doc « intensity » pour une distance non signée | `Source/revolution-quadric.cpp:125` |
| D-R15 | ✅ | Primitives 3D volumiques | Classe documentée sous un nom inexistant | `Source/scales.cpp:6` |
| D-R16 | ✅ | Primitives 3D volumiques | Docs copiées de Circle | `Source/sector.cpp:34` |
| D-R17 | ✅ | Primitives 3D volumiques | n documenté comme valeur renvoyée | `Source/sphere.cpp:866` |
| D-R18 | ✅ | Primitives 3D volumiques | Doc copiée de Size() | `Source/sphereset.cpp:242` |
| D-R19 | ✅ | Primitives 3D volumiques | Unité des angles renvoyés contredite par le code | `Source/sun.cpp:101` |
| D-R20 | ✅ | Primitives 3D volumiques | Commentaire inversé sur les vecteurs radial/axial | `Source/taper.cpp:28` |
| D-R21 | ✅ | Primitives 3D volumiques | Doc copiée du dodécaèdre | `Source/torus.cpp:282` |
| D-R22 | ✅ | Primitives 3D volumiques | Exemple de code faux | `Source/twist.cpp:182` |
| D-C01 | ✅ | Courbes et surfaces paramétriques | « Curvilinear abscissa t » trompeur : t n'est l'abscisse curviligne que si B = 1/π | `Include/clothoid.h:23` |
| D-C02 | ⬜ | Courbes et surfaces paramétriques | Doc « signed squared distance » fausse | `Source/curvequadric.cpp:173` |
| D-C03 | ✅ | Courbes et surfaces paramétriques | Doc « length of the cubic curve » sur une quadrique ; \param n absent en 2D | `Source/curvequadric.cpp:344` |
| D-C04 | ✅ | Courbes et surfaces paramétriques | Formule 2D dans la doc d'une courbe 3D | `Source/curvecubic.cpp:82` |
| D-C05 | ⬜ | Courbes et surfaces paramétriques | Commande Doxygen \warn inexistante | `Source/curvequadric.cpp:197` |
| D-C06 | ✅ | Courbes et surfaces paramétriques | Commentaire « Quartic constant coefficients » pour un polynôme de degré 5 | `Source/curvecubic.cpp:263` |
| D-C07 | ⬜ | Courbes et surfaces paramétriques | « Solve quadric equation » pour une cubique ; ordre des profondeurs non documenté | `Source/curvecubic2.cpp:553` |
| D-C08 | ✅ | Courbes et surfaces paramétriques | « \param brush … should the triangle be filled » : brush non utilisé, aucun triangle | `Source/curvequadric2.cpp:460` |
| D-C09 | ⬜ | Courbes et surfaces paramétriques | Exemple de code avec un type inexistant CubicCurve3 | `Include/curve.h:355` |
| D-C10 | ⬜ | Courbes et surfaces paramétriques | Doc « Access to curve vertices » pour l'accès au drapeau de lissage | `Include/curve.h:417` |
| D-C11 | ✅ | Courbes et surfaces paramétriques | Doc « Gets the external radius » pour le rayon interne | `Include/tubular.h:56` |
| D-C12 | ✅ | Courbes et surfaces paramétriques | « Rotates a patch » pour une courbe | `Source/bezier-curve.cpp:43` |
| D-S01 | ✅ | Champs scalaires et grilles | Doc décrivant le cas bas degré | `Source/algebraic.cpp:178` |
| D-S02 | ⬜ | Champs scalaires et grilles | Convention QRect non standard et non documentée | `Source/array2.cpp:341` |
| D-S03 | ⬜ | Champs scalaires et grilles | Convention de signe contredite par Inside() | `Include/scalarfield.h:164` |
| D-S04 | ⬜ | Champs scalaires et grilles | Sommet et cellule confondus dans la doc | `Include/array.h:265` |
| D-S05 | ✅ | Champs scalaires et grilles | Doc de classe inexistante et paramètre ambigu | `Source/arrayindex.cpp:6` |
| D-S06 | ✅ | Champs scalaires et grilles | Deux surcharges homonymes de sémantique différente | `Include/scalarfield.h:425` |
| D-S07 | ✅ | Champs scalaires et grilles | Blocs non Doxygen et docs fausses | `Source/hexagonscalarfield2.cpp:277` |
| D-S08 | ⬜ | Champs scalaires et grilles | Exemple de la doc incompatible avec les tables utilisées | `Source/scalarfield-mesh.cpp:29` |
| D-S09 | ⬜ | Champs scalaires et grilles | Documentation vide ou absente | `Source/scalarfield-mesh.cpp:334` |
| D-S10 | ⬜ | Champs scalaires et grilles | Docs absentes, non Doxygen ou fausses | `Source/scalarfield.cpp:1704` |
| D-S11 | ⬜ | Champs scalaires et grilles | Tables de marching cubes publiques et modifiables | `Include/scalarfield.h:696` |
| D-S12 | ⬜ | Champs scalaires et grilles | Blocs /* au lieu de /*! | `Source/scalarfield2-curvature.cpp:5` |
| D-S13 | ⬜ | Champs scalaires et grilles | Docs absentes ou fausses | `Source/scalarfield2.cpp:718` |
| D-V01 | ✅ | Champs vectoriels et de couleur | Docs fausses ou incomplètes | `Source/vectorfield2.cpp:676` |
| D-V02 | ✅ | Champs vectoriels et de couleur | Doc « bi-linear » alors que l'interpolation est trilinéaire | `Source/vectorfield.cpp:56` |
| D-V03 | ⬜ | Champs vectoriels et de couleur | Préconditions de résolution non documentées | `Include/vectorfield.h:48` |
| D-N01 | ✅ | Bruit et échantillonnage | Documentation absente ou fausse pour les bruits phasor | `Source/phasor.cpp:5` |
| D-N02 | ⬜ | Bruit et échantillonnage | w0 documenté comme « pulsation » alors que c'est l'orientation | `Source/noise-gabor.cpp:20` |
| D-N03 | ⬜ | Bruit et échantillonnage | Docs de classes et de retour incorrectes | `Source/noise-voronoi.cpp:22` |
| D-N04 | ⬜ | Bruit et échantillonnage | Doc en /* et plages de valeurs fausses | `Source/noise.cpp:7` |
| D-N05 | ✅ | Bruit et échantillonnage | Translation t exprimée en unités de longueur d'onde, non documenté | `Source/turbulence2.cpp:144` |
| D-N06 | ✅ | Bruit et échantillonnage | Nom trompeur et \param absent | `Source/discset.cpp:244` |
| D-N07 | ⬜ | Bruit et échantillonnage | \param et \return absents, constantes non documentées | `Source/discsystem.cpp:96` |
| D-N08 | ⬜ | Bruit et échantillonnage | Docs en /*, fausses ou absentes | `Source/disctilelink.cpp:154` |
| D-N09 | ⬜ | Bruit et échantillonnage | Docs de classes en /* et méthode de debug non documentée | `Source/disctiles.cpp:8` |
| D-N10 | ⬜ | Bruit et échantillonnage | Doc incohérente | `Source/dla.cpp:43` |
| D-N11 | ⬜ | Bruit et échantillonnage | Fonctions non documentées | `Source/eden2.cpp:435` |
| D-N12 | ⬜ | Bruit et échantillonnage | Docs copiées-collées ou vides | `Source/hexagonarray2.cpp:159` |
| D-N13 | ✅ | Bruit et échantillonnage | La doc cite Hexagon2::R au lieu de Hexagon2::Signed | `Source/hexagonal.cpp:34` |
| D-N14 | ✅ | Bruit et échantillonnage | \brief tronqué ou erroné | `Source/relaxedarray2.cpp:220` |
| D-N15 | ⬜ | Bruit et échantillonnage | Docs en /*, copiées-collées ou fausses | `Source/sampling2.cpp:519` |
| D-T01 | ✅ | Maillages et graphes géométriques | Nombre de sommets/triangles faux | `Source/mesh.cpp:329` |
| D-T02 | ✅ | Maillages et graphes géométriques | « Creates the mesh of an octaedron » | `Source/mesh.cpp:1148` |
| D-T03 | ⬜ | Maillages et graphes géométriques | Formule LaTeX cassée et facteur 1/n_i absent du code | `Source/mesh.cpp:3392` |
| D-T04 | ✅ | Maillages et graphes géométriques | Fonction protégée sans documentation | `Source/mesh.cpp:547` |
| D-T05 | ✅ | Maillages et graphes géométriques | Fonction publique non documentée | `Source/mesh2.cpp:254` |
| D-T06 | ✅ | Maillages et graphes géométriques | Complexité annoncée O(n log n) fausse | `Source/mesh2.cpp:293` |
| D-T07 | ✅ | Maillages et graphes géométriques | « Translate a mesh » pour Scale | `Source/mesh2.cpp:517` |
| D-T08 | ✅ | Maillages et graphes géométriques | « as a 3D Vector » pour un Vector2 | `Include/mesh.h:536` |
| D-T09 | ✅ | Maillages et graphes géométriques | Commentaire « // Sphere class » | `Include/vectorset.h:14` |
| D-T10 | ✅ | Maillages et graphes géométriques | Fonctions inline sans documentation | `Include/polygon.h:294` |
| D-U01 | ⬜ | Utilitaires, couleur, GPU | Docs en /* au lieu de /*! et contenus faux | `Source/color.cpp:249` |
| D-U02 | ⬜ | Utilitaires, couleur, GPU | Doc trompeuse (« spectrum », « default=last ») ; plage de Hsl non documentée | `Include/color.h:85` |
| D-U03 | ⬜ | Utilitaires, couleur, GPU | Fonctions non documentées, liste des palettes incomplète | `Source/analyticpalette.cpp:449` |
| D-U04 | ⬜ | Utilitaires, couleur, GPU | Docs fausses ou incomplètes | `Include/histogram.h:61` |
| D-U05 | ⬜ | Utilitaires, couleur, GPU | Classe et méthodes non documentées | `Source/chart.cpp:4` |
| D-U06 | ✅ | Utilitaires, couleur, GPU | Doc contredite : la fonction lève une exception | `Source/cpu.cpp:108` |
| D-U07 | ⬜ | Utilitaires, couleur, GPU | Docs /* et paramètres faux | `Source/cpu.cpp:152` |
| D-U08 | ⬜ | Utilitaires, couleur, GPU | Paramètres documentés inexistants ou absents | `Source/draw.cpp:13` |
| D-U09 | ✅ | Utilitaires, couleur, GPU | Docs fausses ou absentes | `Source/system.cpp:14` |
| D-U10 | ⬜ | Utilitaires, couleur, GPU | Unités et sémantique des paramètres fausses | `Source/gpu-texture.cpp:147` |
| D-U11 | ⬜ | Utilitaires, couleur, GPU | Absence de documentation de l'API GPU | `Include/gpu-wrapper.h:7` |

**80 / 142 corrigés**

### O — Optimisations

| ID | ✅ | Domaine | Description courte | Fichier |
|---|:---:|---|---|---|
| O-G01 | ⬜ | Géométrie 2D | Formule de Bretschneider coûteuse et fragile | `Source/quad2.cpp:308` |
| O-K01 | ✅ | Géométrie 3D de base | Déterminant 3×3 calculé via une Matrix4 | `Source/plane.cpp:227` |
| O-K02 | ⬜ | Géométrie 3D de base | Arêtes et normales d'arêtes recalculées à chaque requête | `Source/tetrahedra.cpp:354` |
| O-S01 | ⬜ | Champs scalaires et grilles | Parcours non contigu en mémoire | `Source/analyticscalarfield.cpp:608` |
| O-S02 | ⬜ | Champs scalaires et grilles | Sous-cellule construite deux fois, cache par std::map | `Source/scalarfield-mesh.cpp:464` |
| O-S03 | ⬜ | Champs scalaires et grilles | Lecture des plans z à pas ny·nz | `Source/scalarfield.cpp:1492` |
| O-S04 | ⬜ | Champs scalaires et grilles | Boucles i externe / j interne sur un stockage i + nx*j | `Source/scalarfield2-curvature.cpp:19` |
| O-S05 | ⬜ | Champs scalaires et grilles | Allocations par pixel et filtres non séparables | `Source/scalarfield2-smooth.cpp:290` |
| O-N01 | ⬜ | Bruit et échantillonnage | Calcul inutile de la valeur du bruit | `Source/noise.cpp:278` |
| O-N02 | ⬜ | Bruit et échantillonnage | Calcul de TrucMuch inutilisé | `Source/disctilelink.cpp:274` |
| O-N03 | ⬜ | Bruit et échantillonnage | Calculs inutiles dans une fonction non implémentée | `Source/eden2.cpp:618` |
| O-T01 | ⬜ | Maillages et graphes géométriques | Test O(n²) sur toutes les demi-arêtes | `Source/mesh.cpp:563` |

**1 / 12 corrigés**

---

## Priorités de correction recommandées

### Urgences absolues (crash / UB garanti)
1. **B-V01**, **B-S08** — écriture dans un `QVector` jamais dimensionné : corruption mémoire à chaque construction de `VectorField2`/`VectorField` depuis un champ analytique et de `IntegerField2`.
2. **B-T01** — générateurs de grilles de `Mesh`/`Mesh2` : écriture hors tableau dès que nx > ny.
3. **B-N05**, **B-S03** ✅, **B-S07**, **B-S09**, **B-S12** ✅ — indices de cellules ou de niveaux hors bornes dans `RelaxedArray`, `Voxel`, le V-cycle de `diffusion.cpp`, `ScalarField` et `ScalarField2`.
4. **B-N02** — débordement de `subset[37]` sur la pile dans `EdenGrowth::GrowthSites()` (amas 3D denses).
5. **B-R01**, **B-N01** ✅, **B-N03**, **B-V02**, **B-M02** — lectures hors bornes (`Icosidodecahedron::Vertex`, cartes de `PhasorEric`, `EdenGrowth2::Strahler`, `VectorField2::Clone`, `Aa_pow`).
6. **B-M01** ✅ — lecture hors bornes dans un `fprintf` de débogage d'`Aa_pow` : supprimer les traces (règle aussi B-M31).
7. **B-R02** ✅, **B-R07** — membres lus avant initialisation dans les constructeurs délégués de `BoxSphere` et `Ellipsoid`.
8. **B-R05** ✅, **B-R08** ✅, **B-S06**, **B-S11** ✅ — boucles ou récursions infinies (`RhombicDodecahedron::RandomInside`, `HemiSphere::RandomInside`, multigrille de `diffusion.cpp`, boucle de `ScalarField2`).

### Corrections à fort impact (résultats faux courants)
9. **B-K02** ✅, **B-K03** ✅, **B-K04**, **B-K06** ✅, **B-K07** ✅, **B-K08**, **B-K09** ✅, **B-R03** ✅, **B-R04**, **B-R06** ✅ — requêtes géométriques 3D fausses (SAT segment/boîte, k-topes, OBB/OBB, slabs, tétraèdre, distance entre capsules, rayon/cône, intersections des polyèdres) : tout code de lancer de rayons ou de collision qui les utilise est faux.
10. **B-G01** ✅ à **B-G15** ✅ — primitives 2D : normales d'arêtes orientées vers l'intérieur (motif `Orthogonal()` dans `Pentagon2`, `Quad2`, `Triangle2`, `Convex2`), distances signées et normales fausses (`Circle2`, `Capsule2`, `Octogon2`, `IrregularOctogon2`), `Ellipse2::Perimeter` négatif, `Box2::RandomOn` hors périmètre.
11. **B-M03**, **B-P01** ✅, **B-C01** ✅, **B-C02** ✅ — algèbre : SVD, composition quartique∘quadrique et dérivée seconde des courbes de Bézier entièrement fausses.
12. **B-S01** ✅, **B-S02** ✅, **B-S04** ✅, **B-S05** ✅, **B-S10**, **B-S13** ✅ — champs scalaires : racines, volumes, hessiens, pas d'indices et interpolation AVX faux.
13. **B-T02** — normales d'OBJ décalées dans `Mesh::LoadTim()` ; à traiter avec **B-T04** (un fichier écrit par `SaveObj()` ne se recharge pas).
14. **B-K01** ✅, **B-K05** ✅, **B-R09** ✅, **B-N04**, **B-N06**, **B-U01** ✅, **B-U02** ✅ — caméra FPS retournée, normale nulle de `Slab`, tables factices de l'icosidodécaèdre, topologie et relaxation d'Eden/SphereTile, roue chromatique blanche, palette inversée constante.
15. Les **🟠** ensuite, par grappes : volumes et aires des polyèdres (domaine R), conventions ligne/colonne des matrices et quaternions (B-M : rotation inverse, `Symmetry`, `LookAt`), arithmétique d'intervalles et affine (`cos(Ia)`, `Aa::Pow`, `sin(Aa)`), transformations `FrameScaled` (domaine K), parseurs OBJ (B-T04, B-T05), conversions de couleur (B-U03, B-U04).

### À planifier (robustesse, cas limites)
- **Entrées dégénérées** (🟡) : divisions par zéro et NaN sur objets vides ou aplatis (courbes rectilignes, maillages et histogrammes vides, matrices singulières, `Length(a, a)`), boucles infinies sur entrée vide (`Dla::Add`, `InverseArcLength`).
- **Bornes et indices** (🟡) : accès hors bornes sur grilles de largeur 1 et points hors domaine (`RelaxedArray2::At`, `PhasorEric::Value`, `VectorField2::Divergence`), tailles non vérifiées (`MatrixMN`, `FromString`, `BezierPatch`).
- **Erreurs ignorées** (🟡) : codes de retour des solveurs (`Quadric::Solve`, `QLAlgorithm`), compilation et linkage des shaders, ressources OpenGL non libérées ou handles non remis à 0.
- **Précision numérique** (🔵) : seuils absolus (`Cubic::Solve`, `Quadric::Solve`, `Complex::Arg`, `MatrixMN::LogDet`), `acos` non borné, accumulations flottantes dans les boucles de longueur d'arc.
- **Code mort et traces** (🔵) : `fprintf` de débogage d'`aa.cpp`, branches inatteignables, fonctions jamais appelées, destructeurs virtuels manquants.
- **Entrées « à confirmer »** (🔵) : à trancher avec l'auteur avant correction (sens de `Circle2::Inside(Circle2)`, `OrientedBox2(Box2, Frame2)`, plage du bruit simplex 3D, alignement AVX, schéma de Jacobi de `diffusion.glsl`).
- **Documentation** (📝) : `/*` à transformer en `/*!`, `\brief` copiés-collés, formules et `\param` faux.

### Ajouts du ré-audit du 2026-10-22

- **B-R63** 🟡 — rayons parallèles à une face mal traités par `Intersect` des polyèdres platoniques (révélé par la correction de B-R06) : à traiter avec les corrections à fort impact du domaine R.
- **B-S66** 🟡 — `AnalyticScalarField::K(box, nx, ny, nz)` tronque la constante de Lipschitz en entier.
- **B-P11**, **B-P12**, **B-G106** 🔵 — `IsNull()`, seuil absolu de `Check()` et `Pentagon2::Edge(int)` ; **D-P08** — `Integral()` : doc et dépassement de `c[12]` pour n = MaxDegree.
- Entrées à nuancer sans changement de statut : B-N37 (UB signé confirmé, mais résultat jamais négatif sur 4 M de couples testés), B-U20 (`GLBuffer::Destroy` corrigé, `GLShader` et textures non), B-S44 (`Lerp` AVX de `ScalarField` corrigé, restent les occurrences de `scalarfield2.cpp`), B-G65 (`Signed()` de `TwinDisc2` corrigé, `Draw()` NaN si l > 2r), B-G99, D-U01, D-V03 (doc partiellement corrigée).
- Note sur B-R49 (✅) : seul un commentaire « a ≠ b » a été ajouté, `Spheroid::Area` renvoie encore NaN pour a = b.
