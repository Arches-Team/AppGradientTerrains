# Audit LibHeightfield — Rapport complet

**Date :** 2026-10-10  
**Périmètre :** 13 headers + 44 fichiers source C++17 + 16 shaders GLSL (compute)  
**Dépendances :** Qt6 (Core, Gui, Widgets, Charts), OpenGL 4.5 (compute shaders), GLEW, STL, OpenMP, LibCore (corelib)  
**Domaines :** H — Heightfield (cœur) · A — Analyse de terrain CPU · T — Simulation de terrain CPU · L — Couches de matériaux · R — Rendu (shader de terrain) · G — Érosion GPU · P — Analyse GPU / stream power  
**Auditeurs :** 6 agents parallèles (H ; A ; T ; L + R ; G ; P), consolidation avec vérification dans le code des entrées 🔴/🟠 et contrôle ponctuel des numéros de ligne 🟡/🔵  
**Hors périmètre :** `Temp/` (répertoire de build), LibCore (lue pour la sémantique uniquement)

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
| H — Heightfield (cœur) | 2 | 4 | 9 | 13 | **28** | **3** |
| A — Analyse de terrain CPU | 2 | 1 | 6 | 11 | **20** | **4** |
| T — Simulation de terrain CPU | 3 | 11 | 15 | 23 | **52** | **12** |
| L — Couches de matériaux | 4 | 7 | 3 | 8 | **22** | **3** |
| R — Rendu (shader de terrain) | 0 | 0 | 5 | 12 | **17** | **13** |
| G — Érosion GPU | 1 | 10 | 20 | 17 | **48** | **7** |
| P — Analyse GPU / stream power | 2 | 10 | 19 | 12 | **43** | **4** |
| **Total bugs** | **14** | **43** | **77** | **96** | **230** | **46** |
| Documentation 📝 | — | — | — | — | **138** | **34** |
| Optimisations ⚡ | — | — | — | — | **41** | **7** |

| Restants à corriger | 🔴 | 🟠 | 🟡 | 🔵 | Bugs | 📝 | ⚡ |
|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Ouverts** | **10** | **22** | **68** | **84** | **184** | **104** | **34** |

---

## Bugs Critiques 🔴

### H — Heightfield (cœur)

#### 🔴 B-H01 — `Source/heightfield.cpp:1305` — `HeightField::Carve(const CubicCurve&, …)` — Carving inversé (poids appliqué au terrain au lieu de la cible)

`a = Cubic::SmoothCompact(sqrt(d), r, rb)` vaut 1 près de la courbe et 0 à distance `rb`. Le mélange `field = (1-a)*zu + a*field` laisse donc le terrain intact sur la courbe et impose l'altitude `zu` en bordure de zone : on obtient un anneau à l'altitude de la courbe au lieu d'un creusement le long de celle-ci. Le même motif se trouve dans `Carve(const Segment&, …)` (`heightfield.cpp:1346`) et `Carve(const QuadricCurve&, …)` (`heightfield.cpp:1387`) ; `Carve(CubicCurveSet)` et `Carve(QuadricCurveSet)` délèguent et sont donc aussi faux. À comparer avec `Carve(SmoothDisc2)` (`heightfield-sculpt.cpp:21`), qui applique bien le poids à la cible.

**Correctif :**
```cpp
// heightfield.cpp:1305, 1346 et 1387
field[VertexIndex(x, y)] = a * zu + (1.0 - a) * field[VertexIndex(x, y)];
```

#### 🔴 B-H02 — `Source/heightfield-next.cpp:104` — `HeightFieldNext8::FlowTo(const QPoint&, int)` — Sélection de la k-ième direction fausse (FlowTo(p) entièrement faux)

`n += phi & 1; if (n == k) return next[i];` incrémente avant de tester. Pour `k = 0` (appel par `FlowTo(p)`), si le bit 0 est nul la fonction renvoie `next[0]`, qui n'est pas une direction d'écoulement ; si le bit 0 est positionné, `n` passe à 1 et la fonction renvoie `QPoint(0,0)`. Pour `k ≥ 1`, l'indexation est de fait 1-based, contrairement à la doc.

**Correctif :**
```cpp
int n = 0;
for (int i = 0; i < 8; i++, phi >>= 1)
{
  if (phi & 1)
  {
    if (n == k) return next[i];
    n++;
  }
}
return QPoint(0, 0);
```


### A — Analyse de terrain CPU

#### 🔴 B-A01 — `Source/heightfield-geomorphons.cpp:60` — `HeightField::Geomorphons` — Classification inversée (sommet → cuvette, crête → vallée)

`z = zp - zq` : `np` compte les voisins plus bas et `nm` les voisins plus hauts. La table `index[nm][np]` suit la convention GRASS (ligne = voisins plus bas). Un sommet isolé donne `nm = 0, np = 8` et `index[0][8] = 9` (pit). Toute la classification est permutée (peak↔pit, ridge↔valley, shoulder↔footslope, spur↔hollow). `GeomorphonsTangent` compte dans l'autre sens et donne le résultat opposé sur le même terrain.

**Correctif :**
```cpp
double z = zq - zp;   // + si le voisin est plus haut
```

#### 🔴 B-A02 — `Source/heightfield-geomorphons.cpp:190` — `HeightField::GeomorphonsAggregated` — Classification agrégée inversée

Même inversion (`z = zp - zq`, l.183) avec la table agrégée : un sommet donne `index[0][8] = 3` (valley).

**Correctif :**
```cpp
double z = zq - zp;
```


### T — Simulation de terrain CPU

#### 🔴 B-T01 — `Source/heightfieldsand.cpp:524` — `HeightFieldSediment::HydraulicWeatheringUntilStabilizationAAAA` — Boucle infinie : eau réinitialisée à chaque itération

`water` et `suspended` sont recréés à 10.0 dans la boucle (l.524-525) : `water += deltawater` est perdu, `HydraulicWeatheringStepAAAA` renvoie toujours false et `while (!stabilized)` ne termine jamais (`iterations` jamais testé).

**Correctif :**
```cpp
ScalarField2 water(Box2(a, b), nx, ny, 10.0);
ScalarField2 suspended(Box2(a, b), nx, ny, 10.0);
const unsigned int maxIter = 10000;
while (!stabilized && iterations < maxIter)
{
  // … deltas locaux, sans recréer water/suspended
}
```

#### 🔴 B-T02 — `Source/terrain-hydraulic.cpp:339` — `LayerFieldErosion::HydraulicWeatheringUntilStabilization` — Boucle infinie dès qu'il y a de l'eau

`HydraulicWeatheringStep` renvoie false pour une cellule en eau sans aval (l.338-339). L'eau finit dans les minima locaux, n'est jamais évaporée, et la boucle l.278 ne termine jamais (pas de borne). Appelée par AppErosion. La doc l.268-271 affirme le contraire.

**Correctif :**
```cpp
if (n == 0) return true;            // cuvette : stable
// et dans UntilStabilization :
while (!stabilized && iterations < maxIter) { … HydraulicEvaporate(); … }
```

#### 🔴 ✅ B-T03 — `Source/terrain-droplet.cpp:60` — `LayerFieldErosion::Droplet(QPoint, double)` — `slope` non initialisé et téléportation en (0,0)

Si `found == false`, `flowouside != 0` et q n'est pas au bord, aucune branche ne fait `break` : `slope` (non initialisé) est passé à `ErodedMaterial` (UB), puis `q = pp` avec `pp = QPoint()` : la goutte saute en (0,0).

**Correctif :**
```cpp
if (!found)
{
  flow[n] = q; addedbedrock[n] = 0.0;
  addedsand[n] = (flowouside != 0.0 && BorderVertexIndex(q)) ? (1.0 - flowouside) * V_T_S : V_T_S;
  n++;
  break;
}
```


### L — Couches de matériaux

#### 🔴 B-L01 — `Source/layer-generic.cpp:51` — `LayerStack::UpdateHeightField` — Boucle j avec les bornes de i : écritures hors bornes

`for (int j = area.x(); j < area.x() + area.width(); j++)` : si nx > ny + 1, j dépasse ny-1 (corruption mémoire) ; pour un rectangle décalé, mauvaises lignes. En outre `AreaInteger()` vaut `QRect(0,0,nx-1,ny-1)` et la dernière ligne/colonne n'est jamais mise à jour.

**Correctif :**
```cpp
QRect area(0, 0, nx, ny);
if (a != QRect()) area = a.intersected(area);
for (int i = area.left(); i <= area.right(); i++)
  for (int j = area.top(); j <= area.bottom(); j++)
    e(i, j) = bedrock.at(i, j) + (r ? rocks.at(i, j) : 0.0) + (s ? sand.at(i, j) : 0.0);
```

#### 🔴 B-L02 — `Source/layer-generic.cpp:112` — `LayerStack::CreateWaterGeometry` — Table de triangulation hors bornes

`triTable` contient 8, 9, 10 utilisés comme indices de `int e[6]` (`e[triTable[index][k]]`, l.217-224 et 244-250) : lectures hors bornes sur la pile (UB).

**Correctif :**
Reconstruire `triTable` avec des indices dans [0,5] cohérents avec le remplissage de `e`, ou réécrire la fonction.

#### 🔴 B-L03 — `Source/layer-generic.cpp:190` — `LayerStack::CreateWaterGeometry` — `abxy` jamais initialisé (écriture dans `aby`)

La boucle des arêtes diagonales écrit `aby[i]` au lieu de `abxy[i]` : `abxy` est lu non initialisé (l.215, 238) et `aby` est écrasé. Plus généralement, les tableaux d'indices ne sont affectés que sous condition.

**Correctif :**
```cpp
abxy[i] = vertex.size();
// et std::vector<int> a(nx, -1), …, abxy(nx - 1, -1);
```

#### 🔴 B-L04 — `Source/layer-generic.cpp:266` — `LayerStack::CreateWaterGeometry` — Format d'indices incohérent ; tableau d'indices de normales faux

Le premier triangle ajoute des paires (indice, 0) (l.219-224), le second des indices simples ; puis `Mesh(vertex, normal, triangles, triangles)` utilise le même tableau pour les normales alors que `normal` est vide ou d'un élément : triangles faux et accès hors bornes dans les normales.

**Correctif :**
```cpp
// va : 3 indices de sommets par triangle ; na : 0, 0, 0
normal.append(Vector::Z);
return Mesh(vertex, normal, va, na);
```


### G — Érosion GPU

#### 🔴 ✅ B-G01 — `Shaders/heightfield_hydraulic_particle.glsl:304` — `main` — L'érosion par particules ne modifie jamais le terrain

La boucle d'érosion (l.300-305) calcule `deltaSediment` sans l'écrire dans `bedrock[]` ni l'ajouter à `sediment` : la goutte ne transporte rien, le dépôt est nul, la simulation ne fait rien (cf. commentaire « AXEL : this doesn't work anymore », gputerrainsimuparticle.cpp:16).

**Correctif :**
```glsl
float deltaSediment = min(bedrock[erodeIndex], weightedErodeAmount);
bedrock[erodeIndex] -= deltaSediment;
sediment += deltaSediment;
```


### P — Analyse GPU / stream power

#### 🔴 ✅ B-P01 — `Source/gpuanalysis.cpp:238` — `GPUHeightFieldAnalysis::InitForDoubleBuffer` — Uniform dvec2 affecté en float : Gradient/Laplacian divisent par 0

`SetUniform("cellDiagonalDouble", float, float)` appelle `glUniform2f` alors que l'uniform est `dvec2` (glsl:33) : `GL_INVALID_OPERATION`, l'uniform reste à (0,0) et `Gradient()`/`Laplacian()` (glsl:470-528) divisent par 0.

**Correctif :** (appliqué) surcharge `GLShader::SetUniform(const char*, double, double)` (`glUniform2d`) réactivée dans LibCore (`gpu-wrapper.h`, déclaration et définition qui étaient commentées) ; elle ne peut changer aucun appel existant, puisque tout appel `(nom, double, double)` tombait jusqu'ici sur le template supprimé. Puis :
```cpp
shader.SetUniform("cellDiagonalDouble", cellDiagonal[0], cellDiagonal[1]);
```

#### 🔴 ✅ B-P02 — `Source/gpuanalysis.cpp:299` — `GPUHeightFieldAnalysis::Laplacian` — Lecture d'un buffer jamais écrit

`func == 9` écrit `outBufferDouble` (binding 5) ; le C++ lit `outBuffer` (float, binding 0) via `GetDataToScalarField()`, alloué sans données.

**Correctif :** (appliqué) nouvelle méthode protégée `GetDoubleDataToScalarField()` (relecture de `outDoubleBuffer`), utilisée par `Laplacian()` et par `Gradient()` à la place de sa boucle ; barrière `GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT` avant la relecture dans ces deux fonctions.
```cpp
ScalarField2 ret = GetDoubleDataToScalarField();
```


---

## Bugs Haute criticité 🟠

### H — Heightfield (cœur)

#### 🟠 B-H03 — `Source/heightfield.cpp:457` — `HeightField::FlowField(ScalarField2&)` — Direction d'écoulement écrite dans les cellules aval

`direction(flow.q[j]) = AverageFlowDirection(p, …)` écrit la direction de `p` dans chacune de ses cellules aval, chaque cellule étant écrasée par le dernier amont traité. Les fosses reçoivent une direction (la doc dit « Pits have a null flow vector »), les sommets reçoivent un vecteur nul et les autres cellules la direction d'un voisin amont arbitraire.

**Correctif :**
```cpp
if (n > 0)
{
  direction(p) = AverageFlowDirection(p, flow.q, flow.s, n);
  const double sp = stream(p);
  for (int j = 0; j < n; j++)
    stream(flow.q[j]) += sp * flow.sn[j];
}
```

#### 🟠 ✅ B-H04 — `Source/heightfield.cpp:1722` — `HeightField::EncodeSize()` — Dimensions formatées avec 1 chiffre significatif

`.arg(d[0], 0, 'g', 1)` produit « 8e+02 » pour 800 et « 4e+03 » pour 3687. La doc annonce des entiers (`size=[800,1200]`), et `HeightField::Size()` (`heightfield.cpp:1741`) attend `\d+` : le décodage échoue (boîte nulle) ou donne une valeur fausse.

**Correctif :**
```cpp
QString s = QString("size=[%1,%2] height=[%3,%4]").arg(qRound(d[0])).arg(qRound(d[1])).arg(qRound(a)).arg(qRound(b));
```

#### 🟠 ✅ B-H05 — `Source/heightfield.cpp:1663` — `HeightField::FastProceduralErosion` — Fuite d'un programme GL à chaque appel, échec de compilation ignoré

`read_program()` compile et lie un programme à chaque appel ; `glDeleteProgram` n'est jamais appelé. L'échec de compilation ou d'édition de liens n'est pas testé (`read_program` renvoie le programme même en cas d'échec, jamais 0) : la fonction relit alors un buffer de sortie non initialisé et l'écrit dans le terrain.

**Correctif :** (appliqué : test de `GL_LINK_STATUS`, libération par `release_program`, qui détache et supprime aussi les shaders)
```cpp
GLuint rasterizerShader = read_program(ba.data());

// Leave the terrain unchanged if the shader could not be compiled or linked
GLint linked = GL_FALSE;
glGetProgramiv(rasterizerShader, GL_LINK_STATUS, &linked);
if (linked == GL_FALSE)
{
  std::cerr << "FastProceduralErosion : shader compilation or link failed" << std::endl;
  release_program(rasterizerShader);
  return;
}
// …
glUseProgram(0);
release_program(rasterizerShader);
```

#### 🟠 ✅ B-H06 — `Source/heightfield.cpp:1695` — `HeightField::FastProceduralErosion` — Dispatch carré sans garde côté shader : bords écrasés de façon concurrente

Le dispatch vaut `max(nx,ny)/8+1` dans les deux dimensions et `Shaders/heightfield_procedural_erosion.glsl:193-212` ne teste pas `id.x >= WIDTH || id.y >= HEIGHT`. Comme `index()` borne les coordonnées par `clamp`, les invocations hors domaine (au moins une rangée, beaucoup plus pour un terrain non carré) écrivent dans la dernière ligne/colonne des valeurs calculées pour un point hors domaine : bord droit et haut non déterministes.

**Correctif :**
```cpp
glDispatchCompute((GetSizeX() + 7) / 8, (GetSizeY() + 7) / 8, 1);
```
```glsl
// heightfield_procedural_erosion.glsl, début de main()
if (id.x >= WIDTH || id.y >= HEIGHT) return;
```


### A — Analyse de terrain CPU

#### 🟠 ✅ B-A03 — `Source/heightfield-fill.cpp:77` — `HeightField::ThermalStep` — Inverse de la longueur diagonale fausse (×2)

`irad = Norm(inversecelldiagonal)` vaut √2/dx pour des cellules carrées au lieu de 1/(√2·dx) : les pentes diagonales sont surestimées d'un facteur 2, le transfert se déclenche à la moitié du seuil, d'où une érosion anisotrope.

**Correctif :** (appliqué, tel que proposé)
```cpp
double irad = 1.0 / Norm(celldiagonal);
```


### T — Simulation de terrain CPU

#### 🟠 ✅ B-T04 — `Source/heightfieldsand.cpp:125` — `HeightFieldSediment::BedrockLevel` — Flatten appelé avec une altitude comme facteur d'échelle

Le 3e paramètre de `ScalarField2::Flatten` est `scaling` ; la fonction ramène vers `Value(center)`, pas vers z. Avec z = 500 la correction est multipliée par 500.

**Correctif :** (appliqué, tel que proposé, avec un test numérique)
```cpp
const QRect area = bedrock.VertexIntegerArea(Circle2(c, r));
for (int y = area.y(); y <= area.y() + area.height(); y++)
  for (int x = area.x(); x <= area.x() + area.width(); x++)
  {
    double u = SquaredNorm(c - bedrock.ArrayVertex(x, y));
    if (u < r * r) bedrock(x, y) = Math::Lerp(bedrock.at(x, y), z, Cubic::Smooth(u, r * r));
  }
```

#### 🟠 ✅ B-T05 — `Source/heightfieldsand.cpp:214` — `HeightFieldSediment::Intersect` — Couche de sable ignorée

Le ray-marching teste `bedrock.Value(p)` alors que la surface est bedrock+sand : intersections et point renvoyé faux dès qu'il y a du sable.

**Correctif :** (appliqué, tel que proposé, avec un test numérique)
```cpp
double z = bedrock.Value(p) + sand.Value(p);
if (z > p[2]) { q = Vector(p[0], p[1], z); … }
```

#### 🟠 B-T06 — `Source/heightfieldsand.cpp:200` — `HeightFieldSediment::Intersect` — Paramètres de rayon 2D utilisés en 3D

`ta`, `tb` sont calculés sur `Ray2(ray)` dont la direction est normalisée en 2D, puis utilisés dans `ray(t)` 3D : point d'entrée faux, `tb` trop petit pour un rayon oblique ; direction NaN pour un rayon vertical.

**Correctif :**
```cpp
const double s = Norm(Vector2(ray.Direction()));
if (s == 0.0) { /* rayon vertical : traiter à part */ }
ta /= s; tb /= s;
```

#### 🟠 ✅ B-T07 — `Source/heightfieldsand.cpp:671` — `HeightFieldSediment::HydraulicWeatheringStepAAAA` — Matériau en suspension dupliqué

`suspended(q) += rews[k]` modifie le champ en place sans décrémenter `suspended(p)` : création de masse et dépendance à l'ordre de parcours.

**Correctif :** (appliqué : nouveau paramètre `deltasuspended`, déclaration mise à jour dans `heightfieldsediment.h:63`, delta appliqué par l'appelant comme `deltawater`)
```cpp
// HydraulicWeatheringStepAAAA(…, ScalarField2& deltawater, ScalarField2& deltasuspended)
deltasuspended(q) += rews[k];   // dans la boucle
deltasuspended(p) -= ss;        // après la boucle

// HydraulicWeatheringUntilStabilizationAAAA
ScalarField2 deltasuspended(Box2(a, b), nx, ny, 0.0);
// …
suspended += deltasuspended;
suspended.CutEpsilon();
```

#### 🟠 ✅ B-T08 — `Source/terrain-hydraulic.cpp:360` — `LayerFieldErosion::HydraulicWeatheringStep` — Matériau en suspension dupliqué

Même défaut : `suspended(q) += rews[k]` sans retrait en p ; contredit la doc l.31 (« order-independent updates »).

**Correctif :** (appliqué : nouveau paramètre `deltasuspended`, déclaration mise à jour dans `layersfielderosion.h:38`, `\param` ajouté, delta appliqué par `HydraulicWeatheringIterations` et `HydraulicWeatheringUntilStabilization`)
```cpp
// HydraulicWeatheringStep(int x, int y, …, ScalarField2& deltawater, ScalarField2& deltasuspended)
deltasuspended(q) += rews[k];   // dans la boucle
deltasuspended(p) -= ss;        // après la boucle

// Appelants
ScalarField2 deltasuspended(Box2(a, b), nx, ny, 0.0);
// …
suspended += deltasuspended;
suspended.CutEpsilon();
```

#### 🟠 ✅ B-T09 — `Source/terrain-droplet.cpp:118` — `LayerFieldErosion::Droplet(QPoint, double)` — Pas courant perdu quand la goutte s'évapore

Le `break` (V_T == 0) précède `n++` : l'érosion, le dépôt et `fallen` (toute la charge) du dernier pas ne sont jamais appliqués aux cartes : perte de masse systématique.

**Correctif :**
```cpp
if (V_T == 0.0) { n++; break; }
```

#### 🟠 B-T10 — `Source/terrain-hydraulic-dynamic.cpp:162` — `LayerFieldErosion::DynamicWeatheringStep` — Transfert d'eau excessif : oscillation sans fin

`ww = min(mm, water(p))` avec `mm` la plus petite pente `z/length[k]` : dans le cas axial on transfère toute la différence z au lieu de z/2 ; la différence s'inverse à chaque pas et `DynamicWeatheringUntilStabilization` (l.60-95, sans borne) ne termine pas. Unités incohérentes (pente vs hauteur).

**Correctif :**
```cpp
ww = Math::Min(water(p), 0.5 * zmax);   // zmax : plus grande marche
// + borne d'itérations dans UntilStabilization
```

#### 🟠 B-T11 — `Source/terrain-lightning.cpp:169` — `LayerFieldErosion::LightningWeatheringStep` — Ligne dupliquée : masse de roches doublée

`deltaRocks(q) += e[k] * aa;` apparaît l.167 et l.169 : chaque voisin reçoit deux fois la part retirée de p.

**Correctif :**
Supprimer la ligne 169.

#### 🟠 ✅ B-T12 — `Source/terrain-lightning.cpp:41` — `LayerFieldErosion::Lightning` — Bouclier inversé

`shield = SmoothStep(rocks+sand, 0, 2)` vaut 0 sur bedrock nu ; `blasted = e*shield` n'arrache donc rien sur bedrock nu et le maximum sous couverture épaisse, à l'inverse de la doc l.32.

**Correctif :**
```cpp
double blasted = e * (1.0 - shield);
```

#### 🟠 B-T13 — `Source/terrain-vegetation.cpp:90` — `LayerFieldErosion::ProceduralVegetation` — Végétation et herbe toujours nulles

`drainage.Normalize()` ramène le drainage dans [0,1] ; `SmoothStep(drainage, 1.0, 12.0)` (l.99) et `SmoothStep(drainage, 100.0, 1000.0)` (l.106) valent 0 partout : la fonction efface végétation et herbe.

**Correctif :**
Supprimer `drainage.Normalize();` (seuils en aire drainée) ou recalibrer les seuils sur [0,1].

#### 🟠 ✅ B-T14 — `Source/analyticfield.cpp:85` — `AnalyticHeightField::AverageSlope` — Somme renvoyée au lieu de la moyenne

La fonction renvoie la somme de 2n pentes (×16 pour n = 8).

**Correctif :**
```cpp
return s / (2.0 * n);
```


### L — Couches de matériaux

#### 🟠 ✅ B-L05 — `Source/layer-generic.cpp:150` — `LayerStack::CreateWaterGeometry` — Sommets d'arêtes jamais calculés

Les sommets d'intersection sont ajoutés comme `Vector(0,0,0)` (l.150, 182, 191) et `bx[i]` prend `Vertex(i, j+1)` (l.172) : tous les triangles de rive ont un sommet à l'origine.

**Correctif :** (appliqué) l'interpolation proposée revient toujours au sommet sec (la hauteur d'eau est nulle d'un côté, t vaut 0 ou 1) ; elle est écrite directement par une fonction locale `Shore(i0, j0, i1, j1)` qui renvoie le sommet sec de l'arête, où la surface de l'eau rejoint le sol. Elle est utilisée pour les quatre familles d'arêtes (ax, bx, aby et diagonale) :
```cpp
auto Shore = [&](int i0, int j0, int i1, int j1) { return (water.at(i0, j0) > 0.0) ? Vertex(i1, j1) : Vertex(i0, j0); };
```
La diagonale est encore rangée dans `aby[i]` au lieu de `abxy[i]` (B-L03).

#### 🟠 B-L06 — `Source/layer-generic.cpp:140` — `LayerStack::CreateWaterGeometry` — Surface de l'eau collée au sol

`vertex.append(Vertex(i, 0))` (et l.163) utilise `bedrock + sand` sans ajouter la hauteur d'eau.

**Correctif :**
```cpp
vertex.append(Vertex(i, j) + Vector(0.0, 0.0, water.at(i, j)));
```

#### 🟠 ✅ B-L07 — `Source/layer-generic.cpp:255` — `LayerStack::CreateWaterGeometry` — Indices d'arêtes horizontales non échangés

Seuls `a` et `b` sont échangés en fin de rangée : `ax` reste figé sur j = 0.

**Correctif :** (appliqué)
```cpp
std::swap(a, b);
std::swap(ax, bx);
```

#### 🟠 B-L08 — `Source/layer-generic.cpp:301` — `LayerStack::CreateCubes` — Cubes de roches posés à l'altitude de leur épaisseur

`p.ToVector(Rocks(i, j))` au lieu de `Bedrock(i, j)` : les roches flottent ou s'enfoncent (le sable est correctement posé l.308).

**Correctif :**
```cpp
rockscubes.append(FrameScaled(p.ToVector(Bedrock(i, j)), celldiagonal.ToVector(Rocks(i, j))));
```

#### 🟠 B-L09 — `Include/layersfielderosion.h:6` — `LayerFieldErosion (Subdivide/Scale/Translate hérités)` — Champs propres non mis à jour

`bedrockflow`, `sandflow`, `waterflow`, `suspended` ne sont pas traités par `LayerStack::Subdivide/Scale/Translate` (non virtuelles, layers.h:26, 51, 76-77) : après `Subdivide`, les routines d'érosion lisent hors bornes (ex. `HydraulicEvaporate`).

**Correctif :**
Redéfinir `Subdivide`, `Scale`, `Translate` dans `LayerFieldErosion` (appel de la base puis traitement des 4 champs) ; les rendre `virtual`.

#### 🟠 B-L10 — `Source/layer-stack.cpp:199` — `LayerStack::Intersect` — Paramètres de rayon 2D utilisés en 3D

Même défaut que `HeightFieldSediment::Intersect` : `Ray2(ray)` normalise la direction 2D, `ray(t)` est 3D ; intersections manquées pour un rayon plongeant, NaN pour un rayon vertical ; pas fixe `t += 1.0`.

**Correctif :**
```cpp
const double s = Norm(Vector2(ray.Direction()));
ta /= s; tb /= s;   // + cas vertical, pas ∝ Norm(celldiagonal)
```

#### 🟠 B-L11 — `Source/layer-stack.cpp:213` — `LayerStack::Intersect` — Sable et roches ignorés

Le test utilise `bedrock.Value(p)` alors que `Height(p)` inclut sable et roches : picking sous la surface visible.

**Correctif :**
```cpp
if (Height(Vector2(p)) > p[2]) { q = Vector(p[0], p[1], Height(Vector2(p))); … }
```


### G — Érosion GPU

#### 🟠 ✅ B-G02 — `Source/gputerrainsimu.cpp:256` — `GPUHydraulicErosionGrid::GetData (3 surcharges)` — Relecture de l'avant-dernier pas

Après `std::swap(out*, *)` (l.153-155) le plus récent est dans `bedrockBuffer`/`waterBuffer`/`sedimentBuffer`, mais les `GetData` lisent `out*` (l.256, 274, 277, 292, 295, 298) : un pas de retard ; après `Step(1)`, terrain initial.

**Correctif :**
Lire `bedrockBuffer`, `sedimentBuffer`, `waterBuffer` dans les `GetData`.

#### 🟠 B-G03 — `Source/gputerrainsimu.cpp:258` — `GPUHydraulicErosionGrid::GetData(ScalarField2&)` — Dénormalisation par une constante 3000

`Init` normalise `(h - h_min)/(h_max - h_min)*scale` (l.67) ; `GetData` reconstruit `3000./scale*data` en ignorant h_min et h_max.

**Correctif :**
```cpp
hf[i] = h_min + double(data[i]) / scale * (h_max - h_min);   // h_min, h_max stockés en membres
```

#### 🟠 B-G04 — `Shaders/heightfield_hydraulic.glsl:310` — `main` — Dureté utilisateur écrasée

`hardnessFactor = GetHardness(Point, id)` (l.309) est écrasé par la fonction « terrasses » l.310 ; le buffer `hardness[]` (binding 7) n'est jamais lu : `UseHardness`, `SetHardnessFunction`, `SetHardnessAlpha` sans effet.

**Correctif :**
Supprimer la ligne 310 et lire `hardness[index]` quand `useHardness == 1`.

#### 🟠 ✅ B-G05 — `Source/gputerrainsimu_test.cpp:330` — `GPUHydraulicErosionGrid_Test::GetData` — Bedrock/eau d'un pas en retard, sédiment déposé à jour

Bindings 0-2 = `temp*` (entrée) et 3-5 = buffers principaux (sortie) ; après le swap (l.245-248), le plus récent est dans `temp*`, mais `GetData` lit `bedrockBuffer`/`waterBuffer` (l.330, 366, 389, 399), alors que `sedDepoBuffer` est à jour : mélange des pas n-1 et n.

**Correctif :**
Inverser les bindings 0-2 et 3-5 (convention des bindings 14/15).

#### 🟠 ✅ B-G06 — `Shaders/heightfield_hydraulic_particle.glsl:158` — `ToIndex1D` — Index 1D calculé avec ny

`return y * ny + x;` alors que `CalculateHeightAndGradient` (l.171) utilise nx : dépôts/érosions au mauvais endroit, voire hors buffer, si nx ≠ ny.

**Correctif :**
```glsl
return y * nx + x;
```

#### 🟠 ✅ B-G07 — `Source/gputerrainsimuparticle.cpp:254` — `GPUHydraulicErosionParticle::SetUniforms` — Décalages du pinceau calculés avec ny

`brushY * ny + brushX` (l.254 et 321) alors que `Init` (l.88) utilise nx : pinceau déformé ou hors bornes si nx ≠ ny.

**Correctif :**
```cpp
brushIndexOffsets.push_back(brushY * nx + brushX);
```

#### 🟠 ✅ B-G08 — `Source/gputerrainsimu_test.cpp:498` — `GPUMapProcessing::Init` — Dispatch dépendant de nx seul

`max(buffer_size_x, buffer_size_x)` : si ny > nx, les lignes au-delà ne sont jamais lissées.

**Correctif :**
```cpp
dispatch_size = (max(buffer_size_x, buffer_size_y) / 8) + 1;
```

#### 🟠 ✅ B-G09 — `Source/gputhermal.cpp:213` — `GPUThermalErosion::GetData` — Sédiment relu avec un pas de retard

Après `std::swap(sedimentBuffer, outSedimentBuffer)` (l.154, 182), les `GetData` (l.213, 227) et `GetData() const` (l.238) lisent `outSedimentBuffer`.

**Correctif :**
Lire `sedimentBuffer`.

#### 🟠 B-G10 — `Shaders/heightfield_thermal.glsl:83` — `main` — Mode local : cellules hors disque non recopiées

`if (local && !IsInDomain(p)) return;` n'écrit pas `out_sediment[id]` : avec le ping-pong, ces cellules alternent entre deux états périmés.

**Correctif :**
```glsl
int id = ToIndex1D(x, y);
if (local && !IsInDomain(p)) { out_sediment[id] = sediment[id]; return; }
```

#### 🟠 B-G11 — `Shaders/deposition.glsl:219` — `main` — Carte source ignorée

`in_source[]` (binding 6) n'est jamais lu, `float source = 1.;` (l.161) inutilisé ; le sédiment est produit partout (`sed += 0.1 * streamPower`) : `GPU_Deposition::SetSourceMap` (gpu_deposition.cpp:146-151) sans effet.

**Correctif :**
```glsl
sed += 0.1 * streamPower * in_source[id];
```


### P — Analyse GPU / stream power

#### 🟠 B-P03 — `Shaders/heightfield_analysis.glsl:672` — `main (func 6/7)` — GetRange réduit des entrées non initialisées si nx < 256

`main` sort pour `i >= nx` avant `func == 6` : seules `outBufferVec[0..nx-1]` sont calculées, mais la réduction `func 7` lit 256 entrées (buffer alloué sans données, gpuanalysis.cpp:62/173). Pour nx·ny < 256, `part == 0`.

**Correctif :**
Traiter `func 6/7` avant les tests sur nx/ny (indexation 1D), avec tranche plafonnée (voir B-P13).

#### 🟠 B-P04 — `Source/gpuanalysis.cpp:603` — `GPUHeightFieldAnalysis::CountPits` — Compteur atomique jamais remis à zéro

`outIntBuffer` réalloué sans données puis `atomicAdd(outInt, pits)` (glsl:705) ; idem `CountPits(const HeightField&)` (l.636).

**Correctif :**
```cpp
int zero = 0;
glClearNamedBufferData(outIntBuffer.GetBuffer(), GL_R32I, GL_RED_INTEGER, GL_INT, &zero);
```

#### 🟠 B-P05 — `Source/gpuanalysis.cpp:828` — `GPUHeightFieldAnalysis::VisibilityIndex` — Accumulation sur buffer non initialisé

`func 11` fait `outBuffer[id] += Viewshed(i,j)` (glsl:750-752) sur un buffer alloué sans données (l.114).

**Correctif :**
```cpp
glClearNamedBufferData(outBuffer.GetBuffer(), GL_R32F, GL_RED, GL_FLOAT, nullptr);
```

#### 🟠 ✅ B-P06 — `Source/gpustreampoweroptimized.cpp:453` — `GPUStreamPowerOptimized::GetData (4 surcharges)` — Relecture de l'état du pas précédent

Après `std::swap` (l.165-166, 217-218), le plus récent est dans `bedrockBuffer`/`streamBuffer` ; les accesseurs lisent `out*` (l.453, 474, 479, 490, 495, 510).

**Correctif :**
Lire `bedrockBuffer` et `streamBuffer` dans tous les accesseurs.

#### 🟠 B-P07 — `Shaders/heightfield_streampower_optimized.glsl:454` — `main` — Cartes m et n ignorées

`m = GetM(p)`, `n = GetN(p)` (l.449-450) sont calculés mais l'équation utilise `p_sa`, `p_sl` : `SetMParameter`/`SetNParameter` sans effet.

**Correctif :**
```glsl
float spe = ke * pow(data.y, m) * pow(pslope, n);
```

#### 🟠 B-P08 — `Shaders/heightfield_streampower_optimized.glsl:457` — `main` — Dureté relue = taux d'érosion

`out_hardness[id] = hh;` (l.445) est écrasé par `out_hardness[id] = spe;` : `GetHardness` renvoie le taux d'érosion.

**Correctif :**
Supprimer la ligne 457.

#### 🟠 B-P09 — `Shaders/iterative_drainage.glsl:94` — `main / WeightNext` — Lectures hors bornes en bord de grille

`WeightNext` (l.67-69) appelle `WeightedSlopes` sur une cellule hors grille ; `main` lit `in_drainage[ToIndex1D(np)]` pour les voisins hors grille.

**Correctif :**
```glsl
ivec2 np = pos + next8[i];
if (np.x < 0 || np.y < 0 || np.x >= buffer_size_x || np.y >= buffer_size_y) continue;
```

#### 🟠 ✅ B-P10 — `Source/ms_spe_gpu.cpp:210` — `GPUMapAdd::Init` — Dispatch dépendant de nx seul

`max(buffer_size_x, buffer_size_x)` : si ny > nx, une bande de lignes n'est jamais écrite par `map_add` et vaut 0 ou une valeur périmée après le swap.

**Correctif :** (appliqué, tel que proposé ; test GPU sur une grille 16 × 100 : 1216 cellules sur 1600 n'étaient pas calculées, toutes le sont désormais)
```cpp
dispatch_size = (max(buffer_size_x, buffer_size_y) / 8) + 1;
```

#### 🟠 B-P11 — `Source/ms_spe_gpu.cpp:7` — `MS_SPE_GPU::Init` — Handles remis à 0 : fuite à chaque ré-Init

`shader_program` et les 4 buffers sont remis à 0 (l.7-13), rendant inopérantes les gardes `if (… == 0)` (l.31, 45-52).

**Correctif :**
Supprimer les lignes 7-13 (membres déjà initialisés dans le `.h`).

#### 🟠 B-P12 — `Source/gpuanalysis.cpp:53` — `Init, InitForTexture, InitForSingleBuffer, InitForDoubleBuffer` — Programme GL perdu à chaque appel d'analyse

Chaque méthode publique appelle un `Init*` qui fait `shader.Initialize()` (l.53, 102, 162, 213) sans libérer le précédent ; fuite récurrente sur une instance réutilisée.

**Partiellement corrigé** (avec O-R03) : `shader.Destroy()` ajouté avant `shader.Initialize()` dans `InitForTexture` (l.101) ; restent `Init` (l.53), `InitForSingleBuffer` (l.162) et `InitForDoubleBuffer` (l.213).

**Correctif :**
Compiler une seule fois (`if (program == 0)`) ou `shader.Destroy()` avant.


---

## Bugs Moyenne criticité 🟡

### H — Heightfield (cœur)

- **B-H07** — `Source/heightfield.cpp:1644` — `HeightField::ErosionAmpli` — **Terrain non carré rendu carré.** `Resized(current_size * factor, current_size * factor, false)` utilise `GetSizeX()` pour les deux dimensions : distorsion anisotrope dès la première itération si nx ≠ ny. **Correctif :** `*this = Resized(GetSizeX() * factor, GetSizeY() * factor, false);`
- **B-H08** — `Source/heightfield.cpp:1741` — `HeightField::Size(const QString&)` — **Regex refusant les altitudes négatives, échec silencieux.** `height=\[(\d+),(\d+)\]` ne reconnaît pas un terrain sous le niveau de la mer ; en l'absence de correspondance la fonction renvoie une boîte nulle sans signaler l'échec. **Correctif :** `(-?\d+)` pour les hauteurs et un retour d'échec explicite (bool ou boîte vide documentée).
- **B-H09** — `Source/heightfield.cpp:1040` — `HeightField::Shade` — **Division par `sum` nul et taille de `intensity` non vérifiée.** `s = s / sum;` donne inf/NaN si `sum == 0` ; `intensity.at(i)` (l.1037) suppose `intensity.size() >= lights.size()`. **Correctif :** `Q_ASSERT(intensity.size() == lights.size()); if (sum <= 0.0) return 0.0;`
- **B-H10** — `Source/heightfield.cpp:1185` — `HeightField::GradientDistance` — **Résolution de `h` non vérifiée.** `h.Normal(i, j)` est indexé avec nx, ny de `*this` : accès hors bornes si `h` est plus petit. **Correctif :** `assert(h.GetSizeX() == nx && h.GetSizeY() == ny);` (et le documenter).
- **B-H11** — `Source/heightfield-access.cpp:220` — `TerrainDX::TerrainDX` — **Cellule terminale jamais comptée dans l'accumulation.** La boucle `while (dirx(it,jt) != 0) { accumulaton(it,jt)++; … }` s'arrête avant de compter la cellule finale (fosse ou bord) : exutoires à 0, `isRiver` et `getMeanFlow` faux aux exutoires. **Correctif :** Ajouter `accumulaton(it, jt)++;` après la boucle.
- **B-H12** — `Source/heightfield-access.cpp:333` — `TerrainDX::getFlowInput` — **Lecture hors bornes pour une cellule de bord.** `dirx.at(i + q.x(), j + q.y())` est lu sans test de domaine (contrairement à `getRiverInput`/`isSource` qui passent par `isRiver`). **Correctif :** `if (!dirx.InsideVertexIndex(i + q.x(), j + q.y())) continue;`
- **B-H13** — `Source/heightfield-next.cpp:58` — `HeightFieldNext8::Flow` — **Mode « steepest » comparant des dénivelés bruts.** `step` n'est pas divisé par la distance (1 ou √2) : les voisins diagonaux sont systématiquement favorisés, ce n'est pas le D8 de plus grande pente annoncé. **Correctif :** `double slope = step * inverselength[k]; if (slope < z) { u = 1 << k; z = slope; }`
- **B-H14** — `Source/heightfield-mesh.cpp:61` — `HeightField::CreateMeshSide` — **Faces latérales et normales orientées vers l'intérieur (à confirmer).** Pour le côté j=0 le triangle (base_k, top_k, top_k+1) a une normale +y et `normal[0] = (0,1,0)` (l.75-82) ; idem pour les autres côtés. Les faces pointent vers l'intérieur du bloc alors que la surface est orientée vers +z : éclairage inversé, faces éliminées par le culling. À confirmer selon la convention de `Mesh`. **Correctif :** Inverser l'ordre des sommets des triangles latéraux et les normales `(0,-1,0)`, `(1,0,0)`, `(0,1,0)`, `(-1,0,0)`.
- **B-H15** — `Source/heightfield-lighting.cpp:68` — `HeightField::Sun` — **Division par zéro si aucun échantillon diurne.** Si le soleil n'est jamais au-dessus de l'horizon (nuit polaire, offsets hors plage), `n == 0` et `sun *= 1.0/0` donne NaN. **Correctif :** `if (n > 0) sun *= 1.0 / double(n);`

### A — Analyse de terrain CPU

- ✅ **B-A04** — `Source/heightfield-metrics.cpp:514` — `HeightField::LocalVariance` — **Annulation catastrophique → NaN.** `sqrt((n·Σv² − (Σv)²)/…)` peut avoir un numérateur légèrement négatif sur une zone plate élevée ; même formule dans `LocalVariance_SAT` (l.593) où les SAT de v² atteignent ~1e14. **Correctif :** (appliqué) `LocalVariance` calcule en deux passes (moyenne de la fenêtre puis somme des carrés des écarts), sans annulation ; `LocalVariance_SAT` construit ses SAT sur les valeurs centrées sur la moyenne globale et borne le numérateur par `Math::Max(0.0, …)`.
- ✅ **B-A05** — `Source/heightfield-metrics.cpp:513` — `HeightField::LocalVariance` — **NaN pour w ≤ 1.** w2 = 0 ⇒ n = 1 ⇒ 0/0 sur tout le champ ; `LocalVariance_SAT` traite ce cas, contrairement à ce qu'affirme le `\deprecated` (« same result »). **Correctif :** (appliqué) `if (n < 2) continue;` : la cellule garde la valeur 0, comme dans `LocalVariance_SAT`.
- ✅ **B-A06** — `Source/heightfield-metrics.cpp:300` — `HeightField::SurfaceRoughness` — **GaussianBlur(0) produit un champ NaN.** w2 = w/2 = 0 pour w ≤ 1 ⇒ sigma = 0 dans `GaussianBlur` ⇒ noyau NaN. **Correctif :** `HeightField blurred = (w2 > 0) ? HeightField(GaussianBlur(w2)) : *this;`
- **B-A07** — `Source/heightfield-geomorphons.cpp:112` — `HeightField::GeomorphonsTangent` — **Tangentes diagonales surestimées de √2.** `dir = Vector2(next[k])` non normalisé : en diagonale la distance parcourue est t·√2 mais la pente vaut dh/t. **Correctif :** `Vector2 dir = Normalized(Vector2(next[k].x(), next[k].y()));`
- **B-A08** — `Source/heightfield-geomorphons.cpp:114` — `HeightField::GeomorphonsTangent` — **Directions sortantes comptées comme « plus bas ».** Si le premier échantillon est hors domaine, `slope` reste à -1e12 ⇒ `nm++` : bords classés en crêtes/sommets ; `Geomorphons` ignore au contraire les voisins hors domaine. **Correctif :** `if (slope == -1e12) continue;`
- **B-A09** — `Source/heightfield-geomorphons.cpp:119` — `HeightField::GeomorphonsTangent` — **Échantillon sur le dernier sommet lu à 0 (à confirmer).** Le rayon tombe exactement sur des sommets ; `Inside(p)` accepte x = b[0] mais `ScalarField2::Value` y renvoie 0 (InsideCellIndex exige i < nx-1) selon l'arrondi. **Correctif :** Échantillonner sur la grille entière (`at(i + k·dx, j + k·dy)`) avec `InsideVertexIndex`.

### T — Simulation de terrain CPU

- **B-T15** — `Source/heightfieldsand.cpp:665` — `HeightFieldSediment::HydraulicWeatheringStepAAAA` — **Érosion cumulée supérieure au sable disponible.** Chaque direction érode jusqu'à `sand.at(p)` : sable négatif possible. **Correctif :** Budget de sable décrémenté entre les directions.
- **B-T16** — `Source/heightfieldsand.cpp:246` — `StabilizeQueue::StabilizeQueue` — **Dernier sommet jamais stabilisé.** `ib = n-1` exclut p[n-1] de la file alors que `us[n-1] = true` empêche son ajout ; file pleine indiscernable de file vide. **Correctif :** Compteur `count` et `ib = 0`.
- **B-T17** — `Source/heightfieldsand.cpp:354` — `HeightFieldSediment::Stabilize` — **Quantité déplacée non bornée : oscillation possible.** `bb = min(e, sand(p))` n'est pas borné par l'excédent de pente ; la boucle `while(!qq.Empty())` (l.354-373) n'a pas de borne. **Correctif :** `bb = min(e, sand(p), 0.5 * maxHeight)` et compteur de pops.
- **B-T18** — `Source/heightfieldsand.cpp:737` — `HeightFieldSediment::SelectFlowDirection` — **`length[k]` indexé par candidat.** `k` est l'indice dans FlowStruct, pas la direction : facteur √2 erroné sur une partie des tirages. **Correctif :** Mémoriser `flow.i[n] = i` et utiliser `length[flow.i[k]]`.
- **B-T19** — `Source/heightfieldsand.cpp:444` — `HeightFieldSediment::CheckFlowSlope` — **Pente en unités d'indice.** `-step / length[i]` sans taille de cellule, alors que `Droplet` passe une pente physique à `ErodedMaterial` : résultats dépendants de la résolution. **Correctif :** `flow.s[n] = -step / (celldiagonal[0] * length[i]);`
- **B-T20** — `Source/terrain-hydraulic.cpp:354` — `LayerFieldErosion::HydraulicWeatheringStep` — **Érosion cumulée supérieure au sable disponible.** Chaque direction érode jusqu'à `sand.at(p)`. **Correctif :** Budget de sable décrémenté.
- **B-T21** — `Source/terrain-hydraulic-dynamic.cpp:8` — `LayerFieldErosion::DynamicImpact` — **Paramètre de pluie ignoré.** Le paramètre est anonyme ; 0.25 m est ajouté en dur. **Correctif :** `void DynamicImpact(const double& r) { water += r; }`
- **B-T22** — `Source/terrain-hydraulic-dynamic.cpp:184` — `LayerFieldErosion::DynamicWeatheringStep` — **Sable cumulé sur 8 directions > disponible.** `DynamicTransport` plafonne à `sand.at(p)` non mis à jour (l.184-199). **Correctif :** Budget décrémenté.
- **B-T23** — `Source/terrain-lightning.cpp:77` — `LayerFieldErosion::LightningWeatheringUntilStabilization` — **Pas de borne ni de CutEpsilon (à confirmer).** La condition `rocks(x,y) == 0.0` (l.107) peut ne jamais être atteinte à cause des résidus flottants. **Correctif :** Borne d'itérations, `rocks.CutEpsilon()`, seuil > ε.
- ✅ **B-T24** — `Source/terrain-image.cpp:87` — `LayerFieldErosion::Texture` — **Pixel composé écrasé par une palette de pente.** La couleur calculée l.30-84 est écrasée l.87 (reste de débogage). **Correctif :** (appliqué) ligne 87 supprimée, ainsi que le champ `slope` (l.22) devenu inutile.
- **B-T25** — `Source/hexagonheightfield.cpp:60` — `HexagonHeightField::Normal` — **Anneau de voisins de grille carrée.** Les triangles utilisés (l.52-123) ignorent la parité de colonne de la grille hexagonale ; incohérent avec `CreateMesh` (l.168-197). **Correctif :** Choisir l'anneau selon `i & 1` ou utiliser `HexagonArray2::Next(i, j, k)`.
- **B-T26** — `Source/hexagonheightfield.cpp:55` — `HexagonHeightField::Normal` — **Hors bornes pour nx ou ny = 1.** Les branches de coin lisent `Vertex(i+1, …)`. **Correctif :** `if (nx < 2 || ny < 2) return Vector::Z;`
- ✅ **B-T27** — `Source/analyticmusgrave.cpp:52` — `AnalyticMusgrave::Value` — **Pondération inter-octaves désactivée.** `signal *= weight` est commenté : `gain` est sans effet, ce n'est pas le ridged multifractal annoncé. **Correctif :** (appliqué) Rétablir `signal *= weight;`.
- ✅ **B-T28** — `Source/analyticmusgrave.cpp:48` — `AnalyticMusgrave::Value` — **Remappage [-1,1] seulement sur la 1re octave.** La l.33 fait `noise*2-1`, pas la l.48. **Correctif :** (appliqué) `signal = noise.Value(q / wave - tt) * 2.0 - 1.0;`
- ✅ **B-T29** — `Source/analyticmusgrave.cpp:85` — `AnalyticRidged::Value` — **Décalage `t` ignoré.** `p = q / l0` n'utilise pas la translation documentée. **Correctif :** (appliqué) `Vector2 p = q / l0 - t;`, même convention que `SimplexTurbulence2::UnitAt()` (soustraction de t après division par la longueur d'onde).

### L — Couches de matériaux

- **B-L12** — `Source/layer-stack.cpp:135` — `LayerStack::Normal(const Vector2&, bool)` — **Hors bornes sur le bord max.** `InsideVertexIndex` accepte q.x() == nx-1 puis `Vertex(q.x()+1, …)` est lu ; u négatif accepté juste à gauche de a. **Correctif :** `InsideCellIndex` ou bornage de q, u, v.
- **B-L13** — `Source/layer-stack.cpp:419` — `LayerStack::Scale` — **Densités multipliées par s.** `ScalarField2::Scale(double)` met aussi les valeurs à l'échelle : `vegetation`, `alpha`, `grass` deviennent fausses (l.419-432). **Correctif :** `vegetation.Scale(Vector2(s));` etc. pour ces champs.
- ✅ **B-L14** — `Source/layer-generic.cpp:127` — `LayerStack::CreateWaterGeometry` — **`new int[nx - 1]` sur terrain vide.** nx = 0 ⇒ `bad_array_new_length` ; allocations brutes non protégées. **Correctif :** (appliqué, `Math::Swap` remplacé par `std::swap` et `delete[]` supprimés) `if (nx < 2 || ny < 2) return Mesh();` et `std::vector<int>`.

### R — Rendu (shader de terrain)

- **B-R01** — `Source/heightfieldshader.cpp:696` — `HeightFieldShader::RiverWidth` — **Seuil supérieur à toutes les aires : image entièrement opaque.** `sa` nul ⇒ `Normalize()` remplit de 1.0 : chaque cellule devient une rivière de largeur maximale. **Correctif :** Tester `GetRange` et renvoyer une image transparente si max ≤ 0.
- ✅ **B-R02** — `Source/heightfieldshader.cpp:454` — `HeightFieldShader::IsoLines` — **Troncature vers zéro et dh ≤ 0.** `int(a / dh)` fusionne ]-dh, dh[ ; `int(inf)` UB si height ≤ 0 (l.454-455, 491-492). **Correctif :** (appliqué) avec la réécriture de B-R06 : `int(floor(a / dh))`, `int(ceil(b / dh))` et image transparente renvoyée si `height <= 0` ou `n < 1`.
- **B-R03** — `Source/heightfieldshader.cpp:311` — `HeightFieldShader::BrownShading(double, double)` — **NaN pour un terrain plat.** `(h - h_min)/(h_max - h_min)`. **Correctif :** `d > 0 ? (h - h_min) / d : 0.0`.
- **B-R04** — `Source/heightfieldshader.cpp:243` — `HeightFieldShader::LaplacianShading` — **Division par zéro, Scale touchant le domaine.** Laplacien nul ⇒ inf·0 ; `Scale(double)` modifie aussi le domaine. **Correctif :** `if (m > 0.0) laplacian *= 1.0 / m;`
- **B-R05** — `Source/heightfieldshader.cpp:625` — `HeightFieldShader::CartographicGreen` — **Chargement du dégradé non vérifié.** `Gradients/cartography.png` absent ⇒ `Palette` sur image nulle. **Correctif :** Tester `image.isNull()`.

### G — Érosion GPU

- **B-G12** — `Shaders/heightfield_hydraulic_test.glsl:45` — `(globales)` — **Initialiseurs de globales dépendant d'uniforms (à confirmer selon le driver).** `const float cellArea` etc. (l.45-47) et `deposition.glsl:33` : non conforme GLSL 4.50 §4.3 ; refusé par les drivers stricts. **Correctif :** Calculer ces valeurs au début de `main()`.
- **B-G13** — `Source/gputhermal.cpp:68` — `GPUThermalErosion::Init(const HeightField&)` — **Bedrock alloué sans données.** `SetData(..., 0, ...)` : contenu indéfini alors que le commentaire l.59 suppose 0. **Correctif :** `std::vector<float> zeros(totalBufferSize, 0.f)`.
- **B-G14** — `Source/gputerrainsimu.cpp:78` — `Init (classes basées sur GLShader)` — **Programme GL perdu à chaque ré-Init.** `GLShader::Initialize` écrase `program` : gputerrainsimu.cpp:78, gputerrainsimuparticle.cpp:107, gputhermal.cpp:57 et 112. **Correctif :** `simulationShader.Destroy()` avant `Initialize`.
- **B-G15** — `Source/gputerrainsimu_test.cpp:35` — `~GPUHydraulicErosionGrid_Test` — **hardnessBuffer jamais libéré.** Généré l.113-114, absent du destructeur (l.35-54). **Correctif :** `glDeleteBuffers(1, &hardnessBuffer.buffer);`
- **B-G16** — `Source/gputerrainsimu_test.cpp:330` — `GetData/Set* (Grid_Test, ThermalDouble, ThermalSimple, Deposition, Particle, Thermal)` — **Copies sans contrôle de taille.** Copie de `sizeof(float)*totalBufferSize` vers/depuis un champ de taille arbitraire : gputerrainsimu_test.cpp:330, 335, 350, 366, 371, 389, 394, 399, 413, 418, 456, 472-474 ; gputhermaldouble.cpp:132, 138, 152 ; gputhermalsimple.cpp:156, 170, 185 ; gpu_deposition.cpp:149, 213, 231, 249, 267 ; gputerrainsimuparticle.cpp:196, 228, 361-366, 382-387, 399-400 ; gputhermal.cpp:210-229 ; gputerrainsimu.cpp:334. **Correctif :** `assert(sf.VertexSize() == totalBufferSize)` et vecteurs dimensionnés par `totalBufferSize`.
- **B-G17** — `Source/gputerrainsimu.cpp:67` — `GPUHydraulicErosionGrid::Init` — **Division par zéro sur terrain plat.** `(h_max - h_min)` nul ⇒ NaN. **Correctif :** `range = (h_max > h_min) ? h_max - h_min : 1.0`.
- **B-G18** — `Source/gputerrainsimu.cpp:274` — `GPUHydraulicErosionGrid::GetData(bedrock, sediments[, water])` — **Valeurs normalisées renvoyées.** Pas de dénormalisation (l.274-300), contrairement à `GetData(hf)`. **Correctif :** Même dénormalisation que B-G03.
- **B-G19** — `Shaders/heightfield_hydraulic.glsl:260` — `main` — **Bords périodiques (tore).** `% ivec2(nx,ny)` : eau et sédiments traversent d'un bord à l'autre. **Correctif :** `clamp(tap, ivec2(0), ivec2(nx-1, ny-1))`.
- **B-G20** — `Source/gputerrainsimuparticle.cpp:338` — `SetUniforms(float,float,float,int,int)` — **Nom d'uniform erroné.** `"maxLifeTime"` alors que le shader déclare `maxLifetime` (glsl:57). **Correctif :** `"maxLifetime"`.
- **B-G21** — `Shaders/heightfield_hydraulic_particle.glsl:171` — `CalculateHeightAndGradient / main` — **Accès hors bornes pour une goutte en bord.** Départ possible sur la dernière ligne/colonne ; `nodeIndexNW + nx + 1` et `sediments[dropletIndex + nx + 1]` (l.279-282) hors buffer. **Correctif :** Rejeter les départs hors `[borderSize, n - borderSize)`.
- **B-G22** — `Shaders/heightfield_hydraulic_particle.glsl:152` — `GetHardness` — **clamp avec min > max.** `clamp(h * hardness[index], 0.6f, h)` indéfini pour h < 0.6. **Correctif :** `clamp(…, min(0.6, h), h)`.
- **B-G23** — `Shaders/heightfield_hydraulic_particle.glsl:280` — `main` — **Course de données sur sediments[] (connue).** `+=` non atomiques par 64 invocations (l.279-282). **Correctif :** Virgule fixe + `atomicAdd`.
- **B-G24** — `Shaders/heightfield_hydraulic_test.glsl:190` — `main` — **Sédiment détruit dans les cuvettes.** `if (outSlope > 0. || true)` : une cuvette perd une fraction de sédiment que personne ne reçoit. **Correctif :** Supprimer `|| true`.
- **B-G25** — `Shaders/heightfield_hydraulic_test.glsl:236` — `main` — **Vitesses érosion/dépôt incohérentes.** Terrain : `erosionSpeed` ; sédiment : `depositionSpeed` (l.235-237) : masse non conservée. **Correctif :** Même vitesse des deux côtés.
- **B-G26** — `Shaders/heightfield_hydraulic_test.glsl:119` — `GetFlowWeighted` — **pow d'une base négative.** `pow(Slope(...), flow_p)` indéfini pour une pente < 0 ; même motif deposition.glsl:115. **Correctif :** `pow(max(Slope(...), 0.0), flow_p)`.
- **B-G27** — `Shaders/heightfield_thermal_double.glsl:111` — `main` — **Masse créée.** Chaque receveur prend `min(1, h_sed/matter)*matter` au donneur, qui ne cède que `min(distributeMul, …)*matter` (l.117). **Correctif :** Répartir le stock du donneur entre ses receveurs.
- **B-G28** — `Shaders/heightfield_thermal_simple.glsl:105` — `main` — **Échange non conservatif.** Le receveur décide avec son propre `tanAngle` (l.109) ; corrigé dans `heightfield_thermal_simple_fix.glsl:111`, non utilisé. **Correctif :** Adopter la version `_fix`.
- **B-G29** — `Shaders/heightfield_thermal.glsl:123` — `main` — **Sédiment négatif, unité douteuse.** `z = sediment - distributeMul*matter` non borné ; `matter = eps * cellDiag.x * cellDiag.y` est une aire (l.122 ; aussi thermal_double l.106, thermal_simple l.114). **Correctif :** `max(z, 0.)` et revoir l'unité de `matter`.
- **B-G30** — `Source/gputerrainsimu.cpp:70` — `Init (plusieurs classes)` — **Casse « LibHeightField » dans les chemins.** gputerrainsimu.cpp:70, gputerrainsimu_test.cpp:80, gputhermal.cpp:49 et 104, gputhermalsimple.cpp:50, gpu_deposition.cpp:74 : échec sur système sensible à la casse. **Correctif :** `/LibHeightfield/LibHeightfield/Shaders/…`.
- **B-G31** — `Include/gputerrainsimu.h:100` — `GPUHydraulicErosionGrid_Test` — **GetResource dans un initialiseur de membre.** Lève `std::runtime_error` dès la construction si le fichier est absent. **Correctif :** Résoudre le chemin dans `Init`.

### P — Analyse GPU / stream power

- **B-P13** — `Shaders/heightfield_analysis.glsl:719` — `main (func 6)` — **Dernières cellules ignorées par GetRange.** `part = (nx*ny)/groupSize` tronque. **Correctif :** `part = (nx*ny + groupSize - 1) / groupSize; end = min(start + part, nx*ny);`
- **B-P14** — `Shaders/heightfield_analysis.glsl:175` — `Height(vec2) / Height(int,int)` — **Décodage 16 bits faux.** `col.b + col.g/255.` vaut ≈ (256/255)·y et ignore l'octet faible (l.175, 182) : biais ≈ 0,4 % de l'amplitude. **Correctif :** `(col.b*255.*65536. + col.g*255.*256. + col.r*255.) / 16777215.`
- **B-P15** — `Shaders/heightfield_analysis.glsl:202` — `Normal` — **Normale trop inclinée de √2.** `length(e.xy)` au lieu de 2·dx (l.200-204). **Correctif :** Composante z = `2.0*cellDiagonal.x*cellDiagonal.y` avec dérivées pondérées.
- **B-P16** — `Shaders/heightfield_streampower_optimized.glsl:155` — `GetUplift` — **Précédence de `!` : garde inopérante.** `!(i >= 0) && …` ne sort que si i < 0 : lecture hors buffer en x = nx-1. **Correctif :** `if (!((i >= 0) && (i < nxUplift-1) && (j >= 0) && (j < nyUplift-1))) return 1.0f;`
- **B-P17** — `Shaders/heightfield_streampower_optimized.glsl:370` — `GetFlowWeighted` — **pow d'une base négative.** Pente montante négative (aussi ms_spe_gpu.glsl:120). **Correctif :** `pow(max(Slope(...), 0.0), flow_p)`.
- **B-P18** — `Source/gpustreampoweroptimized.cpp:83` — `GPUStreamPowerOptimized::Init` — **Buffers lus sans initialisation.** `streamBuffer` (l.83) et `boundaryBuffer` (l.92) alloués sans données, lus au premier pas. **Correctif :** Initialiser à 0.
- **B-P19** — `Shaders/ms_spe_gpu.glsl:170` — `main` — **Bords érodés malgré « do not erode borders ».** La branche bord (l.168-172) n'écrit pas `out_delta[id]` : un delta périmé est ajouté par `map_add` si `smooth_steps > 0`. **Correctif :** `out_delta[id] = 0.0;`
- **B-P20** — `Shaders/map_add.glsl:42` — `main` — **Élévations négatives forcées à 0.** `max(0., …)`. **Correctif :** Retirer ou paramétrer.
- **B-P21** — `Source/gpu_drainage.cpp:105` — `GPU_Drainage::GetData` — **Débordement si le champ est plus petit.** `data` dimensionné par `drainage.VertexSize()` (l.105-108). **Correctif :** `std::vector<float> data(total_buffer_size)` + assert.
- **B-P22** — `Source/ms_spe_gpu.cpp:247` — `GPUMapAdd::SetCoeffMap` — **Lecture hors bornes / front() sur vecteur vide.** Taille non vérifiée ; avant `Init`, `front()` sur vecteur vide (UB). **Correctif :** Vérifier tailles et `total_buffer_size > 0`.
- **B-P23** — `Source/gpustreampoweroptimized.cpp:393` — `GPUStreamPowerOptimized::SetBoundary` — **Taille non vérifiée.** `&boundary[0]` lu sur `totalBufferSize` entiers. **Correctif :** `assert` et `const&`.
- **B-P24** — `Source/gpustreampoweroptimized.cpp:131` — `GPUStreamPowerOptimized::Step` — **`static int stepId` partagé.** Local statique à chaque surcharge (l.131, 181), partagé entre instances, jamais remis à 0. **Correctif :** Membre remis à 0 dans `Init`.
- **B-P25** — `Shaders/heightfield_procedural_erosion.glsl:196` — `main` — **Pas en y faux pour une grille non carrée.** `dy = WORLD_SIZE/HEIGHT` suppose une étendue en y égale à x (l.196, 201). **Correctif :** Passer `cellDiagonal` en uniform.
- **B-P26** — `Source/gpuanalysis.cpp:49` — `Init* (plusieurs classes)` — **`std::cin.get(); exit(-1);` dans la bibliothèque.** gpuanalysis.cpp:49-50, 96-97, 156-157, 207-208 ; gpustreampoweroptimized.cpp:70-71 ; gpu_drainage.cpp:42-43 ; ms_spe_gpu.cpp:36-37 ; `GPUMapAdd::Init` (ms_spe_gpu.cpp:214-216) ne teste pas le chemin vide. **Correctif :** Code d'erreur ou exception.
- **B-P27** — `Source/gpustreampoweroptimized.cpp:66` — `GPUStreamPowerOptimized::Init` — **Casse « LibHeightField » dans le chemin.** Échec sur système sensible à la casse. **Correctif :** `/LibHeightfield/LibHeightfield/…`.
- **B-P28** — `Source/gpustreampoweroptimized.cpp:74` — `GPUStreamPowerOptimized::Init` — **Programme perdu à chaque ré-Init.** `Initialize()` sans `Destroy()`. **Correctif :** `simulationShader.Destroy()` avant.
- **B-P29** — `Source/ms_spe_gpu.cpp:101` — `~MS_SPE_GPU` — **Buffers delta jamais libérés.** `in_delta_h_buffer`, `out_delta_h_buffer` (l.101-111). **Correctif :** Ajouter les `glDeleteBuffers`.
- **B-P30** — `Source/gpuheightfield.cpp:68` — `GPUHeightField::GetData` — **Lecture d'un buffer inexistant.** Le constructeur depuis `GLuint` (l.42-46) ne crée pas `outBuffer`. **Correctif :** Lire `inBuffer` si `outBuffer == 0`.
- **B-P31** — `Shaders/heightfield_analysis.glsl:470` — `Gradient / Laplacian` — **Hors bornes pour nx ou ny < 3.** heightfield_analysis.glsl:470, 506, 520 ; heightfield_streampower_optimized.glsl:224, 231. **Correctif :** Retourner 0 si nx < 3 || ny < 3.

---

## Bugs Basse criticité 🔵

### H — Heightfield (cœur)

- **B-H16** — `Source/heightfield-lighting.cpp:54` — `HeightField::Sun` — **Mois/jour passés hors plage à Sun::Angles.** `i / 30.0` et `double(i % 30)` sont tronqués vers des `int` dans [0,12] et [0,29], alors que `Sun::Day` attend 1..12 et 1..31 ; i=0 tombe en décembre de l'année précédente. **Correctif :** `int(i / 30) + 1`, `i % 30 + 1` (ou conversion jour de l'année → date).
- **B-H17** — `Source/heightfield-carve.cpp:21` — `comp::operator()` — **Départage comparant lhs à lui-même.** `const QPoint& rhs = a.second;` au lieu de `b.second` : le départage à altitude égale est toujours faux (ordre faible strict préservé, pas d'UB). **Correctif :** `const QPoint& rhs = b.second;` et déclarer l'opérateur `const`.
- **B-H18** — `Source/heightfield-carve.cpp:78` — `HeightField::CompleteBreach` — **Trace de débogage.** `std::cout << island_threshold` dans une fonction de bibliothèque, appelée à chaque passe de `SmoothBreachMultiScale`. **Correctif :** Supprimer la ligne.
- **B-H19** — `Source/heightfield.cpp:1446` — `HeightField::SmoothBreachMultiScale` — **Trace de débogage.** `std::cout` et un `GetRange` supplémentaire (l.1440-1443) uniquement pour l'affichage. **Correctif :** Supprimer la trace et le second `GetRange`.
- **B-H20** — `Source/heightfield.cpp:1463` — `HeightField::SmoothBreachGeometric` — **Condition de boucle toujours vraie.** `last_iter >= 0` reste vrai pour a > 0 : toujours 200 itérations ; pour a > 1, `int(last_iter)` déborde (UB). **Correctif :** `while (last_iter >= 1.0 && k < 200)` et précondition 0 < a < 1 documentée.
- **B-H21** — `Source/heightfield.cpp:1067` — `HeightField::Draw` — **Carré de 2×2 cellules par échantillon.** `Box2(ArrayVertex(i,j) - celldiagonal, ArrayVertex(i,j) + celldiagonal)` : les carrés se chevauchent et débordent du domaine. **Correctif :** Utiliser `0.5 * celldiagonal`.
- **B-H22** — `Source/heightfield.cpp:1064` — `HeightField::Draw` — **Division par zéro pour un terrain plat.** `Linear::Step(at, a, b)` avec `a == b` donne une couleur NaN. **Correctif :** `if (a == b) b = a + 1.0;`
- **B-H23** — `Source/heightfield.cpp:263` — `HeightField::Vertex(const Vector2&, bool)` — **Altitude 0 sur le bord haut et hors domaine.** `InsideCellIndex` exige i < nx-1 : un point exactement en x = b[0] ou y = b[1] renvoie z = 0 ; même motif dans `Normal(const Vector2&, bool)` (l.122-123) qui renvoie `Vector::Z`. Le comportement hors domaine n'est pas documenté. **Correctif :** Borner i dans [0, nx-2] (u = 1) sur le bord et documenter la valeur hors domaine.
- **B-H24** — `Source/heightfield.cpp:1137` — `HeightField::Up / Down` — **Critères asymétriques.** `Up` teste `at(q) > z + 0.0001` (seuil absolu en mètres), `Down` (l.1164) `at(q) < z` sans seuil : q peut être « down » de p sans que p soit « up » de q. **Correctif :** Utiliser `HeightField::flat` dans les deux fonctions.
- **B-H25** — `Source/heightfield.cpp:1225` — `HeightField::Cross` — **Nombre d'échantillons incorrect pour n < 2.** Pour n < 2 la fonction renvoie quand même 2 valeurs. **Correctif :** `if (n < 2) return { GetHeight(a) };` ou documenter n ≥ 2.
- **B-H26** — `Source/heightfield.cpp:847` — `HeightField::IntersectBox` — **Sorties modifiées puis échec quand l'origine est dans la boîte (à confirmer).** Si ta < 0, t et n sont modifiés puis la fonction renvoie false : un rayon partant de l'intérieur n'est jamais traité. **Correctif :** Clarifier l'intention ; au minimum documenter le retour et l'état des paramètres de sortie.
- **B-H27** — `Include/heightfield.h:19` — `FlowStruct::i` — **Sentinelle -1 initialisée sur un seul élément.** `int i[8] = { -1 };` n'initialise que i[0] ; i[1..7] valent 0, qui est un index de direction valide. **Correctif :** `int i[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };`
- **B-H28** — `Source/heightfield.cpp:1606` — `HeightField::ErosionDeposition` — **Offset de bruit tiré par `rand()`.** Résultat non reproductible et dépendant de l'état global, contrairement au reste de la bibliothèque qui utilise `Random`. **Correctif :** Passer un `Random&` en paramètre.

### A — Analyse de terrain CPU

- **B-A10** — `Source/heightfield-metrics.cpp:176` — `HeightField::HillslopeAsymmetry` — **Fenêtre SAT décalée d'une cellule.** `imax = min(x+W, nx)` couvre [x−W, x+W−1] : biais directionnel (l.175-176). **Correctif :** `int imax = std::min(x + W + 1, nx); int jmax = std::min(y + W + 1, ny);`
- **B-A11** — `Source/heightfield-metrics.cpp:32` — `HeightField::HillslopeAsymmetry` — **Normalisation de direction morte.** `dir -= TwoPi` après usage ; le recalage de dp/dn (l.55-64) ne corrige qu'un tour. **Correctif :** Normaliser `direction` avant, puis `std::remainder(aspect - dirPositive, Math::TwoPi)`.
- **B-A12** — `Source/heightfield-metrics.cpp:239` — `HeightField::AreaRatio` — **Index −1 pour nx ou ny = 1.** `cellTrianglesArea(i, ny - 2)` (l.239) et `(nx - 2, j)` (l.243) hors bornes sur grille dégénérée. **Correctif :** `if (nx < 2 || ny < 2) return ScalarField2(Box2(a, b), nx, ny, 1.0);`
- **B-A13** — `Source/heightfield-metrics.cpp:382` — `TPI, TPI_SAT, Peakedness, ReliefSteepness, Jut, Rut, HillslopeAsymmetry` — **Demi-fenêtre j calculée avec dx.** `w2 = int(r / celldiagonal[0])` sert aussi pour l'axe j : l.382, 422, 704, 761, 831, 896 et 74. Faux si dy ≠ dx. **Correctif :** `w2y = int(r / celldiagonal[1])` pour j.
- **B-A14** — `Source/heightfield-metrics.cpp:639` — `HeightField::TerrainRuggednessIndex` — **Moyenne absolue divisée par 9 au lieu de 8.** La cellule centrale (écart nul) est incluse dans n (l.635). **Correctif :** `if (wi == i && wj == j) continue;`
- **B-A15** — `Source/heightfield-fill.cpp:78` — `HeightField::ThermalStep` — **Dernière ligne/colonne jamais stabilisées.** Les boucles i < nx-1, j < ny-1 omettent les arêtes x de j = ny-1 et les arêtes y de i = nx-1. **Correctif :** Ajouter deux boucles de bord.
- **B-A16** — `Source/heightfield-flow.cpp:121` — `HeightField::CheckFlowDirectionsAngle` — **num et steepest non renseignés.** Contrairement à `CheckFlowSlope`, `Steepest()` lirait `q[-1]` ensuite. **Correctif :** `flow.num = n;` et suivi de `steepest` comme l.67-75.
- **B-A17** — `Source/heightfield-flow.cpp:47` — `HeightField::CheckFlowSlope` — **steepest non réinitialisé.** Si n == 0, `steepest` garde une valeur antérieure (appelants actuels protégés par n > 0). **Correctif :** `flow.steepest = -1;`
- **B-A18** — `Source/heightfield-flow.cpp:89` — `HeightField::CheckFlowSlope` — **Opérateur virgule résiduel.** `slopesum += flow.s[k], power;` : `power` évalué puis ignoré. **Correctif :** `slopesum += flow.s[k];`
- **B-A19** — `Source/heightfield-flow.cpp:16` — `FlowStruct::Add` — **Pas de contrôle num < 8.** Réutiliser une FlowStruct déborde q/h/s/i (aucun appelant trouvé, à confirmer). **Correctif :** `assert(num < 8);` et méthode `Reset()`.
- **B-A20** — `Source/heightfield-fill.cpp:9` — `DIRECTIONS, State, Element, InitBorders` — **Symboles globaux à liaison externe (risque ODR).** `Element`, `CLOSED/OPEN`, `InitBorders` (l.9-64) dans l'espace global. **Correctif :** Namespace anonyme.

### T — Simulation de terrain CPU

- **B-T30** — `Source/heightfieldsand.cpp:386` — `HeightFieldSediment::StabilizeStep` — **Angle de repos codé en dur.** `const double a = 0.84;` masque `Array2::a` et ignore `sar`. **Correctif :** Paramètre `sar`, variable renommée.
- **B-T31** — `Source/heightfieldsand.cpp:617` — `HeightFieldSediment::ErodedMaterial` — **Érosion du bedrock non bornée par la capacité.** `V_E_R = ErodeBedrock(slope)*V_T` peut dépasser `V_E - V_G_S` (l.615-622). **Correctif :** `V_E_R = Math::Min(ErodeBedrock(slope) * V_T, V_E - V_G_S);`
- **B-T32** — `Source/heightfieldsand.cpp:422` — `HeightFieldSediment::CheckFlowSlope` — **FlowStruct partiellement rempli.** `i[]`, `steepest`, `num` non renseignés (l.422-458). **Correctif :** Renseigner comme heightfield-flow.cpp:61-81.
- **B-T33** — `Source/heightfieldsand.cpp:326` — `HeightFieldSediment::Stabilize` — **Champs alloués inutilisés.** `moved` et `deltaSand` (l.326-327). **Correctif :** Supprimer.
- **B-T34** — `Source/heightfieldsand.cpp:711` — `(global)` — **`static RandomFast rf` inutilisé.** Jamais utilisé dans le fichier. **Correctif :** Supprimer.
- **B-T35** — `Source/heightfieldsand.cpp:42` — `HeightFieldSediment(const ScalarField2&, const ScalarField2&)` — **Grilles non vérifiées.** `s` peut avoir une autre taille que `b`. **Correctif :** `assert` sur nx, ny.
- **B-T36** — `Source/terrain-hydraulic.cpp:405` — `LayerFieldErosion::HydraulicTransport` — **`dbedrock` toujours 0.** L'érosion du bedrock annoncée n'est pas implémentée (l.400-410). **Correctif :** Implémenter ou corriger la doc.
- **B-T37** — `Source/terrain-hydraulic.cpp:379` — `HydraulicAmountS` — **Fonction libre à liaison externe.** Non déclarée en en-tête : risque ODR. **Correctif :** `static`.
- **B-T38** — `Source/terrain-hydraulic-dynamic.cpp:217` — `LayerFieldErosion::DynamicTransport` — **`dbedrock` toujours 0.** `deltarock` jamais modifié (l.217-234). **Correctif :** Implémenter ou supprimer.
- **B-T39** — `Source/terrain-hydraulic-dynamic.cpp:31` — `LayerFieldErosion::DynamicWeatheringIterations` — **Code de débogage vide.** `if ((i == 30) && (j == 30)) {}` et boucle vide l.39-44. **Correctif :** Supprimer.
- **B-T40** — `Source/terrain-hydraulic-dynamic.cpp:46` — `LayerFieldErosion::DynamicWeatheringIterations` — **Champ `flow` calculé puis ignoré.** Accumulé (l.17) puis `Step(0.0, fb)` (division par zéro si fb = 0) et jamais utilisé ; idem l.65, 91. **Correctif :** Supprimer.
- **B-T41** — `Source/terrain-hydraulic-dynamic.cpp:131` — `LayerFieldErosion::DynamicWeatheringStep` — **Pente en unités d'indice.** `z / length[k]` sans `celldiagonal[0]`. **Correctif :** Diviser par la taille de cellule.
- **B-T42** — `Source/terrain-hydraulic-dynamic.cpp:207` — `DynamicAmountS` — **Doublon de HydraulicAmountS à liaison externe.** Fonction libre identique. **Correctif :** `static` ou mutualiser.
- **B-T43** — `Source/terrain-thermal.cpp:91` — `LayerFieldErosion::StabilizeRocksSand` — **Calculs morts.** `vertexes`, `eboul`, `deltaeboul` (l.91-112) jamais exploités. **Correctif :** Supprimer.
- **B-T44** — `Source/terrain-thermal.cpp:148` — `LayerFieldErosion::StabilizeStep` — **Angle de repos codé en dur.** `const double a = 0.84;` masque `Array2::a`. **Correctif :** Paramètre.
- **B-T45** — `Source/terrain-vegetation.cpp:23` — `LayerFieldErosion::VegetationGrowingStep` — **Croissance non bornée.** Densité multipliée jusqu'à 1.09 par pas sans clamp. **Correctif :** `Clamp` final.
- **B-T46** — `Source/terrain-grass.cpp:12` — `LayerFieldErosion::GrassGrowingStep` — **Croissance non bornée.** Même motif que VegetationGrowingStep. **Correctif :** `Clamp` final.
- **B-T47** — `Source/terrain.cpp:130` — `CARVE` — **Affectation morte.** `z = QEE.at(i).Scalar()` immédiatement écrasé l.131. **Correctif :** Supprimer.
- **B-T48** — `Source/terrain.cpp:5` — `LayerFieldErosion::rf` — **Membre statique inutilisé.** Aucune utilisation. **Correctif :** Supprimer.
- **B-T49** — `Source/terrain.cpp:107` — `CARVE / TRYY` — **Fonctions libres de test exposées.** Liaison externe, absentes des en-têtes (CARVE l.107, TRYY l.171). Précision : `TRYY` est en fait une méthode publique déclarée dans layersfielderosion.h:88 (voir D-L12) ; seule `CARVE` est une fonction libre. **Correctif :** `static` ou suppression.
- **B-T50** — `Source/heightfielderosion.cpp:109` — `HeightFieldErosion::StreamPowerErosion` — **Aire drainée × longueur (à confirmer).** `StreamAreaSteepest() * Norm(CellDiagonal())` n'est pas une aire. **Correctif :** `* CellArea()` si l'intention est une aire.
- **B-T51** — `Source/hexagonheightfield.cpp:221` — `HexagonHeightField::CreateHexes` — **Base des prismes à z = -10.** Échelle négative si `at(i,j) < -10` (l.220-222). **Correctif :** `GetRange().Min()` moins une marge.
- **B-T52** — `Source/analyticfield.cpp:94` — `AnalyticHeightField::Cross` — **Nombre d'échantillons incorrect pour n < 2.** Renvoie 2 valeurs. **Correctif :** `if (n <= 1) return { Height(a) };`

### L — Couches de matériaux

- **B-L15** — `Source/layer-stack.cpp:236` — `LayerStack::WaterFlood(const Vector2&, …) / WaterDry` — **Dernière ligne/colonne exclues.** `QRect view(0, 0, nx-1, ny-1)` (l.236, 285). **Correctif :** `QRect view(0, 0, nx, ny);`
- **B-L16** — `Source/layer-stack.cpp:255` — `LayerStack::WaterFlood(const Vector2&, …)` — **Niveau du sol incohérent avec la version globale.** `bedrock + sand` ici, `+ rocks` l.355 ; snow/grass/vegetation non remis à zéro. **Correctif :** Même définition et même remise à zéro.
- **B-L17** — `Source/layer-stack.cpp:333` — `LayerStack::GetTrees` — **Arbres enterrés sous le sable.** Altitude `bedrock.Value(…)`. **Correctif :** `Height(Vector2(x + a, y + b))`.
- **B-L18** — `Source/layer-stack.cpp:101` — `LayerStack::Vertex(int,int) / Vertex(const Vector2&)` — **Conventions différentes.** `Vertex(i,j)` exclut les roches, `Vertex(p)` (l.117-120) les inclut via `Height`. **Partiellement corrigé :** les deux conventions sont désormais documentées (layer-stack.cpp:17-20, 99-100), mais restent différentes. **Correctif :** Convention unique documentée.
- **B-L19** — `Source/layer-generic.cpp:293` — `LayerStack::CreateCubes` — **Base fixée à -10.** Échelle négative si `Bedrock < -10` (l.292-294). **Correctif :** `bedrock.GetRange` moins epsilon.
- **B-L20** — `Source/layer-generic.cpp:66` — `LayerStack::Save` — **Retours de save() ignorés, plages codées en dur.** `-bedrock.png` contient socle+roches+sable ; water/snow/alpha non sauvegardés (l.66-84). **Correctif :** Tester `save()`, plages via `GetRange`.
- **B-L21** — `Include/layersfielderosion.h:19` — `LayerFieldErosion(const HeightField&)` — **Constructeur non `explicit`.** Conversion implicite coûteuse. **Correctif :** `explicit`.
- **B-L22** — `Source/layer-bedrock.cpp:1` — `(fichier)` — **Unité de compilation vide.** Ne contient que `#include "layers.h"`. **Correctif :** Supprimer le fichier.

### R — Rendu (shader de terrain)

- ✅ **B-R06** — `Source/heightfieldshader.cpp:459` — `HeightFieldShader::IsoLines` — **Isoligne majeure manquée sur pente forte.** Seuls na ou nb multiples de n sont testés (l.459, 495). **Correctif :** (appliqué) réécriture vectorielle : chaque niveau `i * height / n` entre `floor(min)` et `ceil(max)` est extrait par `LineSegments()`, majeur si `i % n == 0`, puis dessiné dans une `QGraphicsScene` rendue par `Draw::CreateImage` (4096 pixels sur le plus grand côté) et mise à l'échelle de la grille si `scale` vaut true. Modifications ultérieures de l'utilisateur : couleurs de type `Color`, `scale` à false par défaut, trait cosmétique de 1 pixel, et cadrage sur la boîte du terrain désactivé (argument commenté, heightfieldshader.cpp:388).
- ✅ **B-R07** — `Source/heightfieldshader.cpp:477` — `HeightFieldShader::IsoLines` — **Alpha perdu dans la seconde passe.** `rgba()` puis `rgb()` (l.497-503) ; un pixel majeur peut être rétrogradé. **Correctif :** (appliqué) supprimé par la réécriture de B-R06 : plus de passes par pixel, rendu ARGB transparent.
- ✅ **B-R08** — `Source/heightfieldshader.cpp:816` — `HeightFieldShader::RiverWidth` — **Disque tronqué et facteur √2.** `re = w * Norm(CellDiagonal())` vs fenêtre `int(w)+1` (l.793, 805-808). **Correctif :** (appliqué) rayon conservé en diagonales de cellule (comme documenté) ; fenêtre calculée par axe, `ni = ceil(re / CellDiagonal()[0])` et `nj = ceil(re / CellDiagonal()[1])`, ce qui couvre tout le disque, y compris sur des cellules non carrées.
- ✅ **B-R09** — `Source/heightfieldshader.cpp:812` — `HeightFieldShader::RiverWidth` — **Deux décalages d'un qui se compensent.** `QRect mask(0,0,nx-1,ny-1)` et boucles `<= y + height`. **Correctif :** (appliqué) `QRect mask(0, 0, nx, ny)` et boucles `y <= area.bottom()`, `x <= area.right()`.
- ✅ **B-R10** — `Source/heightfieldshader.cpp:682` — `HeightFieldShader::CartographicGreen` — **Scale réduit aussi le domaine.** Analyses sur un terrain miniature (en partie absorbé par Normalize, à confirmer). **Correctif :** (appliqué) `sf *= 3000.0 / 65535.0;`
- ✅ **B-R11** — `Source/heightfieldshader.cpp:691` — `CartographicGreen / LaplacianShading` — **Rééchantillonnage carré fixe.** 2048² ou 4096² (l.302-310) : terrain non carré déformé. **Correctif :** (appliqué) fonction locale `ResizedAspect(a, n)` donnant la résolution dont le plus grand côté vaut n avec le rapport d'aspect conservé ; utilisée pour les rééchantillonnages 2048/4096 de `LaplacianShading` (seuils testés sur le plus grand côté) et 2048 de `CartographicGreen`.
- ✅ **B-R12** — `Source/heightfieldshader.cpp:701` — `HeightFieldShader::CartographicGreen` — **Boucle d'histogramme non bornée.** `while (histo[i].second < 0.8) i++;` (l.701-704, 714-717). **Correctif :** (appliqué) `i < histo.size() - 1 &&` dans les deux boucles.
- ✅ **B-R13** — `Source/heightfieldshader.cpp:311` — `HeightFieldShader::LaplacianShading` — **Traces de débogage.** `std::cout` l.311, 315, 319, 325, 329, 362. **Correctif :** (appliqué) suppression des six `std::cout` / `System::ShowElapsed` et du `QElapsedTimer` devenu inutile.
- ✅ **B-R14** — `Source/heightfieldshader.cpp:332` — `HeightFieldShader::LaplacianShading` — **setPixelColor dans une région OpenMP (à confirmer).** `detach()` non synchronisé (l.332-359). **Correctif :** (appliqué) suppression du `#pragma omp parallel for` : la boucle de post-traitement est séquentielle.
- ✅ **B-R15** — `Source/heightfieldshader.cpp:200` — `HeightFieldShader::RadianceScaling` — **Hessienne fausse sur le bord.** Voisins hors domaine remplacés par 0 (l.200-208) ; pas de division par la taille de cellule (à confirmer). **Correctif :** (appliqué) suppression de `RadianceScaling()` et de son seul appelant `ImageShadeRadianceScaling()` (aucun appelant hors de la classe), ainsi que des fonctions libres `ImageShadeRadianceScalingWarp`, `ImageShadeRadianceScalingTanh` et `ImageShadeRadianceScalingCurvature` devenues mortes ; déclarations retirées de `heightfieldshader.h`.
- ✅ **B-R16** — `Source/heightfieldshader.cpp:149` — `ImageShadeRadianceScalingsilhouetteWeight` — **Code mort à l'ordre d'arguments faux.** Jamais appelée ; ordre GLSL de SmoothStep. **Correctif :** (appliqué) fonction supprimée (aucun appelant dans le dépôt, non déclarée dans un en-tête).
- ✅ **B-R17** — `Include/heightfieldshader.h:8` — `HeightFieldShader` — **Référence pendante sur temporaire.** `const HeightField& heightfield` accepte un temporaire. **Correctif :** (appliqué) au lieu de `= delete`, qui casserait les nombreux appelants sûrs du type `HeightFieldShader(sf).Relief()` (conversion implicite depuis `ScalarField2`), constructeur `HeightFieldShader(HeightField&&)` qui stocke le temporaire dans un membre `owned` vers lequel pointe `heightfield` ; constructeur de copie et affectation supprimés. Corrige les cas réellement pendants, par exemple `HeightFieldShader hs(hf.Resized(...))` (ILand, iland.cpp:870) ou `HeightFieldShader shader(GetHeightField())` (AppNetwork, ILand) si `GetHeightField()` renvoie par valeur.

### G — Érosion GPU

- **B-G32** — `Source/gputerrainsimu.cpp:71` — `Init (plusieurs classes)` — **Tests `isEmpty()` morts avec `exit(-1)`.** `GetResource` lève une exception : gputerrainsimu.cpp:71-76, gputerrainsimu_test.cpp:82-87, gputerrainsimuparticle.cpp:100-105, gputhermal.cpp:50-55 et 105-110, gputhermaldouble.cpp:47-51, gputhermalsimple.cpp:52-56, gpu_deposition.cpp:76-80. **Correctif :** `try/catch` et propagation d'erreur.
- **B-G33** — `Source/gputhermaldouble.cpp:33` — `Init` — **Membres nx/ny masqués.** `int nx = …` local (gputhermaldouble.cpp:33-34, gputhermalsimple.cpp:38-39, gpu_deposition.cpp:60-61). **Correctif :** `nx = hf.GetSizeX();`
- **B-G34** — `Source/gputhermaldouble.cpp:79` — `Init` — **glUniform3f vers des vec2.** `a`/`b` (gputhermaldouble.cpp:79-80, gputhermalsimple.cpp:89-90) : GL_INVALID_OPERATION, a = (0,0). **Correctif :** `glUniform2f`.
- **B-G35** — `Source/gputerrainsimuparticle.cpp:134` — `Init` — **precipitationBuffer initialisé deux fois.** Copier-coller l.131-135. **Correctif :** Supprimer l.134-135.
- **B-G36** — `Source/gputerrainsimuparticle.cpp:126` — `Init` — **outHardnessBuffer non initialisé.** Le shader n'écrit que les cellules érodées. **Correctif :** Initialiser à zéro.
- **B-G37** — `Shaders/heightfield_hydraulic_particle.glsl:117` — `GetHardness` — **Facteur 50 appliqué à toutes les fonctions.** `if` commenté (l.115-119). **Correctif :** Rétablir le `if`.
- **B-G38** — `Shaders/heightfield_hydraulic.glsl:325` — `main` — **Borne de prélèvement inefficace (à confirmer).** `w / sedimentToHeight` n'empêche pas w - pickup < 0. **Correctif :** `min(…, fragColour.w)`.
- **B-G39** — `Source/gputerrainsimu_test.cpp:283` — `GPUHydraulicErosionGrid_Test::SetMaxWater` — **No-op.** Appel commenté, pas d'uniform `maxWater` (l.281-286). **Correctif :** Implémenter ou supprimer.
- **B-G40** — `Source/gputerrainsimu_test.cpp:261` — `GPUHydraulicErosionGrid_Test::SetUniforms` — **Paramètres sans effet.** `evaporation` (glsl:242 commenté) et `hardnessFunctionIndex` (l.239). **Correctif :** Retirer ou réactiver.
- **B-G41** — `Shaders/heightfield_hydraulic_test.glsl:256` — `main` — **Dureté augmentant l'érosion (à confirmer).** `data.w += new_sed_h * hardnessFactor`. **Correctif :** Diviser par la dureté ou renommer.
- **B-G42** — `Shaders/heightfield_hydraulic_test.glsl:165` — `main` — **Hauteur complète sans couche déposée.** `out_fullHeight = frag.w + frag.y`. **Correctif :** Ajouter `out_sed_hf`.
- **B-G43** — `Source/gputerrainsimu_test.cpp:386` — `GetData` — **Trace de débogage.** `std::cout`. **Correctif :** Supprimer.
- **B-G44** — `Include/gputerrainsimu.h:94` — `GPUMapProcessing` — **Paramètres jamais envoyés.** `kw` (h:94), `cellSize` (test.cpp:496). **Correctif :** Supprimer ou transmettre.
- **B-G45** — `Shaders/heightfield_hydraulic_V0.glsl:1` — `(fichier)` — **Shader mort.** Référencé seulement par LibHeightfield.vcxproj:82 et le `.filters`. **Correctif :** Supprimer.
- **B-G46** — `Shaders/heightfield_thermal_simple_fix.glsl:1` — `(fichier)` — **Shader corrigé mais inutilisé.** Référencé seulement par LibHeightfield.vcxproj:87 (voir B-G28). **Correctif :** Le substituer à `heightfield_thermal_simple.glsl`.
- **B-G47** — `Shaders/heightfield_hydraulic.glsl:79` — `(divers)` — **Code mort dans les shaders et le C++.** hydraulic.glsl : random (79), ridge (98), Height (102), Side (106) ; hydraulic_test.glsl : ComputeIncomingFlowSteepest (95-106) ; deposition.glsl : ComputeIncomingFlowSteepest (101-110), Point3D (58), branche vide (206-217), uniforms k/p_sa/p_sl/dt (36-39) ; heightfield_thermal.glsl : GetHardnessStatic (46) ; particle.glsl : randomSedimentationFreq (68) ; gputerrainsimu_test.cpp : SwapGPUBuffers (192-205). **Correctif :** Nettoyer.
- **B-G48** — `Include/gputerrainsimu.h:25` — `Classes GPU (règle de trois)` — **Copie possible de classes possédant des ressources GL.** GPUHydraulicErosionGrid (25), GPUMapProcessing (72), GPUHydraulicErosionGrid_Test (97), GPUHydraulicErosionParticle (159), GPUThermalErosion (272), GPUThermalErosionSimple (300), GPUThermalErosionDouble (329), GPU_Deposition (gpusediment.h:6). Aucune copie constatée. **Correctif :** `= delete` sur copie et affectation.

### P — Analyse GPU / stream power

- **B-P32** — `Shaders/heightfield_analysis.glsl:727` — `main (func 7)` — **Réduction concurrente non synchronisée.** 8 invocations i == 0 écrivent `outBufferVec[0]` ; pas de barrière avant la relecture (gpuanalysis.cpp:672-678, 715-721). Bénin en pratique. **Correctif :** `if (i > 0 || j > 0) return;` et `GL_BUFFER_UPDATE_BARRIER_BIT`.
- **B-P33** — `Include/gpuheightfield.h:1` — `GPUHeightField` — **Classe morte.** Aucune utilisation dans E:/X/Code ; destructeur vide, copiable. **Correctif :** Supprimer ou finaliser.
- **B-P34** — `Shaders/heightfield_streampower_optimized.glsl:268` — `GetHardnessStatic` — **Test sur d2 au lieu de d3.** Copier-coller (d3 calculé l.267). **Correctif :** `if (d3 < 0)`.
- **B-P35** — `Shaders/heightfield_streampower_optimized.glsl:288` — `GetHardnessStatic` — **Trou à z == 1500.** `else if (p.z > 1500.0f)` (l.287-290). **Correctif :** `else h = 0.1f;`
- **B-P36** — `Shaders/heightfield_streampower_optimized.glsl:420` — `main` — **out_hardness non écrit en bord.** Retours anticipés (l.420-432). **Correctif :** `out_hardness[id] = 1.0;`
- **B-P37** — `Shaders/map_processing.glsl:43` — `main` — **Moyenne divisée par 9 en bord.** 4 ou 6 échantillons en bord. **Correctif :** Compter les voisins valides.
- **B-P38** — `Shaders/iterative_drainage.glsl:27` — `SlopeNext` — **Pente sans taille de cellule.** Faux si dx ≠ dy (l.27-28). **Correctif :** Uniform `cellDiagonal`.
- **B-P39** — `Shaders/heightfield_analysis.glsl:364` — `Viewshed` — **Seuils magiques, échec traité comme invisible (à confirmer).** `error < 250.`, hauteur 1.8 (l.364-369). **Correctif :** Uniforms et `!hit` ⇒ visible.
- **B-P40** — `Source/gpuanalysis.cpp:296` — `(relectures GPU→CPU)` — **Barrière GL_BUFFER_UPDATE_BARRIER_BIT absente (à confirmer).** gpuanalysis.cpp:296, 330, 373, 420, 457, 491, 523, 554, 759, 797, 839 (aucune en 673, 716) ; gpustreampoweroptimized.cpp:160, 212 ; gpu_drainage.cpp:94 ; ms_spe_gpu.cpp:261. **Partiellement corrigé** (avec B-P02) : barrière ajoutée dans `Gradient()` et `Laplacian()` seulement. **Correctif :** `GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT`.
- **B-P41** — `Include/gpu_drainage.h:7` — `Classes GPU (règle de trois)` — **Copie possible (à confirmer).** GPU_Drainage (gpu_drainage.h:7), MS_SPE_GPU et GPUMapAdd (ms_spe_gpu.h:7, 29), GPUHeightFieldAnalysis et GPUStreamPowerOptimized (gputerrainsimu.h). **Correctif :** `= delete` sur la copie.
- **B-P42** — `Include/gpu_drainage.h:7` — `GPU_Drainage` — **Classe dupliquée dans une application (à confirmer).** ExpHugo/AppAmpli redéfinit la même classe : violation ODR si liée à la bibliothèque. **Correctif :** Supprimer la copie applicative.
- **B-P43** — `Source/gpu_drainage.cpp:112` — `GetData / VisibilityIndex` — **Traces de débogage.** gpu_drainage.cpp:112-114 ; gpuanalysis.cpp:830, 844, 847. **Correctif :** Supprimer.

---

## Problèmes de Documentation 📝

### H — Heightfield (cœur)

#### 📝 D-H01 — `Include/heightfield.h:255` — `flat` décrit comme négatif

« Small negative epsilon value » alors que `flat = +1.0e-8` (heightfield.cpp:44) ; c'est `-flat` qui sert de seuil.  
**Correctif :** `//!< Small positive epsilon; downward steps must be lower than -flat.`

#### 📝 ✅ D-H02 — `Include/heightfield.h:74` — SteepestSlope : brief copié de Steepest

« Steepest downward point » alors que la fonction renvoie une pente.  
**Correctif :** (appliqué) `\brief Slope of the steepest downward direction.`

#### 📝 D-H03 — `Include/heightfield.h:66` — Steepest/SteepestSlope : précondition absente

Si aucune direction descendante, `steepest == -1` et `q[-1]`/`s[-1]` sont lus (UB).  
**Correctif :** `\pre num > 0` ou `assert(steepest >= 0)`.

#### 📝 D-H04 — `Include/heightfield.h:24` — Commentaire non Doxygen sur `num`

`//GUNDY ADD NUMBER OF DIRECTIONS IN IT`.  
**Correctif :** `int num = 0; //!< Number of downward directions.`

#### 📝 D-H05 — `Include/heightfield.h:14` — Classes sans bloc `\class`

`FlowStruct` (l.14), `HeightField` (l.100) et `TerrainDX` (l.323) n'ont pas de doc de classe.  
Précision : faux positif pour `HeightField`, dont le bloc `\class` existe dans heightfield.cpp:38 ; seuls `FlowStruct` et `TerrainDX` restent à documenter.  
**Correctif :** Ajouter `/*! \class … \brief … */`.

#### 📝 D-H06 — `Include/heightfield.h:347` — TerrainDX::isRiver non documenté

`\brief` vide ; seuil codé en dur (accumulation > 200) non mentionné.  
**Correctif :** Documenter i, j et le seuil.

#### 📝 D-H07 — `Include/heightfield.h:326` — TerrainDX : membres mal décrits

`terrain` est documenté « Reference to elevation » mais c'est une copie ; `accumulaton` (l.328) est une faute de frappe.  
**Correctif :** Corriger la doc et renommer en `accumulation`.

#### 📝 D-H08 — `Source/heightfield.cpp:1758` — LaplacianSmoothing et SmoothClamp sans doc

Fonctions publiques sans bloc Doxygen (`LaplacianSmoothing` l.1758, `SmoothClamp` l.1843) ; constantes (b-a)/512 et plage [-0.75e, 1.25e] non documentées ; sigma = 0 donne NaN.  
**Partiellement corrigé** (commit e86c3068) : `\brief` et `\param` ajoutés ; restent les constantes (b-a)/512, la plage [-0.75e, 1.25e] et le NaN pour sigma = 0.  
**Correctif :** Ajouter `\brief` et `\param`.

#### 📝 D-H09 — `Source/heightfield.cpp:1797` — LaplacianSmoothing : commentaire faux

« inverse distance » alors que le code utilise `Sqr(inverselength[k])` ; commentaires en français ; `std::vector next` local (l.1765) masque `Array2::next`.  
**Correctif :** Corriger le commentaire et renommer la variable.

#### 📝 D-H10 — `Source/heightfield.cpp:720` — WetnessIndex : \param c faux

c = 0 ne calcule pas le log de l'aire drainée mais log(A/s), +inf si s = 0.  
**Correctif :** Corriger la description de c.

#### 📝 D-H11 — `Source/heightfield.cpp:301` — StreamLength : \param steepest absent

Unité (cellules, non monde) non précisée.  
**Correctif :** Ajouter `\param steepest` et l'unité.

#### 📝 D-H12 — `Source/heightfield.cpp:408` — AverageFlowDirection : brief incomplet, liaison externe

`\brief Compute` ; paramètres non documentés ; fonction libre non `static`.  
**Correctif :** Documenter et déclarer `static`.

#### 📝 D-H13 — `Source/heightfield.cpp:542` — Slope(bool) : `\sa ()` vide ; masquage de `Box2::a/b`

Des variables/paramètres `a`, `b` masquent `Box2::a`/`Box2::b` : Slope l.544, WetnessIndex l.724, StreamPower l.749, Draw l.1054, Carve l.1303/1344/1385, Cross l.1217, EncodeSize l.1719, Size l.1738, SmoothClamp l.1847.  
**Correctif :** Supprimer la référence vide et renommer (`zmin`/`zmax`).

#### 📝 ✅ D-H14 — `Source/heightfield.cpp:812` — Signed : doc imprécise

Fautes (« do the heightfield », « embedded an infinite ») ; la valeur est un écart vertical, pas une distance signée euclidienne.  
**Correctif :** (appliqué) `\brief` « signed field of the volume below the heightfield » ; description du volume (prisme vertical infini sur le domaine, sous la surface) et du calcul (maximum de l'écart vertical et de la distance signée horizontale au domaine) ; `\warning` : pas une distance euclidienne, surestimée sur les pentes fortes ; `\return` ajouté.

#### 📝 D-H15 — `Source/heightfield.cpp:1044` — Draw : \param brush ignoré

Le paramètre est documenté mais inutilisé (warning masqué par pragma).  
**Correctif :** Le retirer ou l'utiliser.

#### 📝 D-H16 — `Source/heightfield.cpp:1191` — Sea : \param height absent

Paramètre non documenté.  
**Correctif :** Ajouter `\param height`.

#### 📝 D-H17 — `Source/heightfield.cpp:1003` — Shade : doc approximative

« Shade a vector » au lieu de « a point » ; `\param sum` vague ; pas de `\return`.  
**Correctif :** Corriger et ajouter `\return`.

#### 📝 ✅ D-H18 — `Source/heightfield.cpp:1712` — Fautes de frappe

« sise » (EncodeSize l.1712), « parralelizable » (StabilizeSlope l.1552).  
**Correctif :** (appliqué) « size » (EncodeSize) et « parallelizable » (StabilizeSlope).

#### 📝 D-H19 — `Source/heightfield.cpp:1648` — FastProceduralErosion : style `@param` sans `\brief`

Bloc au style différent du reste du fichier.  
**Correctif :** Uniformiser en `\brief`/`\param`.

#### 📝 D-H20 — `Source/heightfield-access.cpp:272` — TerrainDX : docs copiées-collées

`isSource` (l.272) et `isRiverBorder` (l.299) ont la doc « Get the coded list direction… \return List » alors qu'elles renvoient un bool ; `\param i, j` absents pour `getRiverInput`, `getFlowInput`, `getMeanFlow` ; constantes 0.42/0.69 non sourcées ; variables `list` (l.305) et `cpt` (l.216) inutilisées.  
**Correctif :** Réécrire les docs et supprimer les variables mortes.

#### 📝 D-H21 — `Source/heightfield-carve.cpp:45` — CompleteBreach : paramètre non documenté, code mort

`\param island_threshold` absent ; `mode`/`fill_depressions` constants, branche `else {}` vide (l.150-153), `flood_array` et bloc l.181-191 jamais exécutés ; enums non scopés `UNVISITED/VISITED/EDGE` et `comp` dans l'espace global.  
**Correctif :** Documenter, supprimer le code mort, namespace anonyme.

#### 📝 D-H22 — `Source/heightfield-next.cpp:12` — HeightFieldNext8 : doc fausse

Le mode non-steep active tous les bits descendants (MFD), pas « D∞ » ; « average flow » (l.30) trompeur ; `under` inutilisée (l.36, 52).  
**Correctif :** Corriger la doc, supprimer `under`.

#### 📝 D-H23 — `Source/heightfield-sculpt.cpp:5` — Carve(SmoothDisc2) : « Flatten »

La fonction déplace vers l'altitude e ; faute « reion ».  
**Correctif :** Corriger la doc.

#### 📝 D-H24 — `Source/heightfield-lighting.cpp:45` — Référence liée à un temporaire

`const double& longitude = 0.0;` légal mais trompeur.  
**Correctif :** `const double longitude = 0.0;`

#### 📝 D-H25 — `Source/heightfield.cpp:84` — Subdivide : doc imprécise

Seuls les nouveaux centres de cellule (i, j impairs) sont perturbés.  
**Correctif :** Préciser.

#### 📝 D-H26 — `Source/heightfield-mesh.cpp:186` — CreateCubes : conventions non documentées

Cubes de p à p+celldiagonal (décalés d'une demi-cellule, débordant en dernière ligne/colonne) ; hauteur négative si at < z.  
**Correctif :** Documenter.

#### 📝 D-H27 — `Source/heightdune.cpp:33` — Faute et nom de fichier trompeur

« rige » ; le fichier implémente `AnalyticFieldDune` (analyticfield.h).  
**Correctif :** Corriger et renommer le fichier.

#### 📝 D-H28 — `Include/heightfield-qt.h:1` — Commentaire faux, pas de garde d'inclusion

« Forward declare… » alors que le fichier agrège des en-têtes ; pas de `#pragma once`.  
**Correctif :** Corriger et ajouter `#pragma once`.

#### 📝 D-H29 — `Source/heightfield-sculpt.cpp:14` — Dépendance implicite au masque de VertexIntegerArea

Les boucles `y <= area.y() + area.height()` ne restent en bornes que grâce au masque `QRect(0,0,nx-1,ny-1)` de LibCore ; mêmes boucles heightfield.cpp:1292, 1333, 1374.  
**Correctif :** Commenter la dépendance ou passer à `area.top()..area.bottom()`.

### A — Analyse de terrain CPU

#### 📝 D-A01 — `Source/heightfield-geomorphons.cpp:7` — hollow/spur convexe/concave inversés

hollow est concave, spur convexe (Jasiewicz & Stepinski 2013) ; même erreur l.83.  
**Correctif :** « 4: hollow (concave), 6: spur (convex) ».

#### 📝 D-A02 — `Source/heightfield-geomorphons.cpp:71` — GeomorphonsTangent : paramètres incomplets

`maxDist < 0` = sans limite non documenté ; `flatTangent` non défini ; codes de sortie absents.  
**Correctif :** Compléter `\param` et `\return`.

#### 📝 D-A03 — `Source/heightfield-geomorphons.cpp:9` — « Slope threshold » appliqué sans √2

height = t·celldiagonal[0] aussi en diagonale (l.33, 165) ; pas de `\return`.  
**Correctif :** Documenter le seuil comme différence d'altitude par cellule.

#### 📝 D-A04 — `Source/heightfield-metrics.cpp:471` — LocalVariance renvoie un écart-type

Nom trompeur ; phrase l.473 (« take the average over all the domain ») fausse ; même nom pour `LocalVariance_SAT`.  
**Correctif :** Préciser « local sample standard deviation » et supprimer l.473.

#### 📝 D-A05 — `Source/heightfield-metrics.cpp:603` — TRI : « RMS » faux

Le code renvoie sqrt(Σd²) sans division par n.  
**Correctif :** « square root of the sum of squared differences (Riley 1999) ».

#### 📝 D-A06 — `Source/heightfield-metrics.cpp:689` — Peakedness : « lower or equal » vs `<`

Le code compte `<` (l.724) et renvoie une fraction [0,1], pas un pourcentage.  
**Correctif :** Aligner doc et code.

#### 📝 D-A07 — `Source/heightfield-metrics.cpp:208` — \param w imprécis, \return absent

Fenêtre effective 2·(w/2)+1 (AreaRatio l.208, SurfaceRoughness l.292, LocalVariance l.482, LocalVariance_SAT l.532, TRI l.608, LocalRange l.653).  
**Correctif :** « Window size (odd, ≥ 3) » + `\return`.

#### 📝 D-A08 — `Source/heightfield-metrics.cpp:937` — FractLaplacian : doc incomplète

Faute « Fractionnal » ; `\param s Real.` ; pas de `\return` ; clamp de bord non mentionné (l.937-942, 974-978).  
**Correctif :** Compléter.

#### 📝 D-A09 — `Source/heightfield-metrics.cpp:334` — Code commenté et include inutile

Bloc mort l.334-340 et `#include "irect.h"` l.359 au milieu du fichier.  
**Correctif :** Supprimer.

#### 📝 D-A10 — `Source/heightfield-fill.cpp:130` — FillDepressions : \return faux

La version sans eps renvoie toujours 0 (l.263).  
**Correctif :** Corriger `\return`.

#### 📝 D-A11 — `Source/heightfield-fill.cpp:43` — InitBorders et helpers en `//`

Pas de Doxygen (l.9-64).  
**Correctif :** `/*! \brief … */`.

#### 📝 D-A12 — `Source/heightfield-fill.cpp:66` — ThermalStep : unité et emplacement

Unité de `slope`, k = 0.25 non documentés ; fonction placée dans le fichier « fill ».  
**Correctif :** Documenter et déplacer.

#### 📝 D-A13 — `Source/heightfield-flow.cpp:34` — CheckFlowSlope/CheckFlowDirectionsAngle : \return absent

Unité des pentes non documentée ; `\param s Slope` (l.114) trompeur.  
**Correctif :** Ajouter `\return` et préciser.

### T — Simulation de terrain CPU

#### 📝 D-T01 — `Source/heightfieldsand.cpp:226` — Fonctions sans doc ou \brief vide

StabilizeQueue (l.226), Pop (l.259), Empty (l.269), Push (l.274), HydraulicWeatheringUntilStabilizationAAAA (l.508), HydraulicWeatheringStepAAAA (l.626), Deposit (l.682), ErodeSand (l.690), ErodeBedrock (l.698).  
**Correctif :** Ajouter `\brief`, `\param`, `\return`.

#### 📝 D-T02 — `Source/heightfieldsand.cpp:549` — Evaporate : \param absents

removed, solubility, water, suspended, fallen non documentés.  
**Correctif :** Compléter.

#### 📝 D-T03 — `Source/heightfieldsand.cpp:377` — StabilizeStep : \param absents, commentaire faux

« no rocks » (l.383) alors que le code teste le sable.  
**Correctif :** Compléter et corriger.

#### 📝 ✅ D-T04 — `Source/heightfieldsand.cpp:76` — Caractères mal encodés

« 31� », « 40� » (l.76, 294, 386) ; terrain-thermal.cpp:147.  
**Correctif :** (appliqué) les quatre caractères de remplacement U+FFFD remplacés par « ° » en UTF-8, l'encodage des deux fichiers (qui contiennent déjà « ³ », « ≈ », « × ») ; « respose » corrigé en « repose » sur les mêmes lignes.

#### 📝 ✅ D-T05 — `Source/heightfieldsand.cpp:130` — Fautes de frappe

« ulift » (l.130), « Cmompute » (l.592).  
**Correctif :** (appliqué) « uplift » et « Compute ».

#### 📝 D-T06 — `Include/heightfieldsediment.h:73` — C_T_S : commentaire non Doxygen

`// Capacity Transported Sand` au-dessus.  
**Correctif :** `//!< Capacity of transported sand.`

#### 📝 D-T07 — `Source/terrain-droplet.cpp:5` — Fonctions sans doc

ErodeDampeningVegetation (l.5), DepositDampeningVegetation (l.10).  
**Correctif :** Documenter ou supprimer.

#### 📝 ✅ D-T08 — `Source/terrain-droplet.cpp:18` — Exemple \code non compilable

Appelle la surcharge privée `Droplet(QPoint, double)`.  
**Correctif :** (appliqué) exemple `\code` remplacé par une phrase : la position est ramenée au sommet le plus proche par VertexInteger(), puis la goutte est simulée par la surcharge privée Droplet(QPoint, const double&) ; « dropplet » corrigé dans la doc de cette surcharge.

#### 📝 D-T09 — `Source/terrain-droplet.cpp:32` — Droplet : doc incomplète

Faute « dropplet » ; évaporation, `flowouside` et `maxSteps` non mentionnés.  
**Partiellement corrigé** (avec D-T08) : « dropplet » corrigé ; évaporation, `flowouside` et `maxSteps` restent à documenter.  
**Correctif :** Compléter.

#### 📝 D-T10 — `Source/terrain-hydraulic.cpp:389` — HydraulicTransport : doc contredit le code

La doc demande d'initialiser les sorties ; le code les met à 0.  
**Correctif :** Aligner la doc.

#### 📝 D-T11 — `Source/terrain-hydraulic.cpp:62` — FlowStep : doc vague

Pas de `\return` (toujours true, FlowOneStep toujours false).  
**Correctif :** Compléter.

#### 📝 ✅ D-T12 — `Source/terrain-hydraulic.cpp:12` — HydraulicEvaporate : brief minimal

`evaporation` et `C_T_S` non mentionnés.  
**Correctif :** (appliqué) description de l'évaporation (hauteur d'eau diminuée de `evaporation`, sans devenir négative) et du dépôt de l'excédent de sable en suspension au-delà de `C_T_S` fois la hauteur d'eau ; `\sa HeightFieldSediment::Evaporate()`.

#### 📝 ✅ D-T13 — `Source/terrain-hydraulic-dynamic.cpp:3` — \brief vides

DynamicImpact (l.3), DynamicWeatheringIterations (l.12), UntilStabilization (l.57), Step (l.97), DynamicAmountS (l.204), DynamicTransport (l.214).  
**Correctif :** (appliqué) les six fonctions documentées : pluie uniforme de 0,25 m (paramètre inutilisé), itérations simultanées, `\warning` sur l'absence de borne de UntilStabilization, règles de Step (évaporation sous 0,1 mm, seuil de 1 cm, `\return`), formule de DynamicAmountS, transport du sable seul dans DynamicTransport.

#### 📝 D-T14 — `Source/terrain-lightning.cpp:4` — LightningErosion : doc copiée

Parle de gouttes et de « water elevation ».  
**Correctif :** Décrire les impacts de foudre et l'intensité e.

#### 📝 ✅ D-T15 — `Source/terrain-lightning.cpp:32` — Lightning : « shielded by bedrock »

Devrait être « rocks or sand ».  
**Correctif :** (appliqué) « shielded by the rocks and sand layers », atténuation progressive jusqu'à 2 m ; « 95% of the blasted bedrock ».

#### 📝 ✅ D-T16 — `Source/terrain-lightning.cpp:52` — \brief vides

LightningWeatheringIterations (l.52), UntilStabilization (l.74), Step (l.101).  
**Correctif :** (appliqué) trois fonctions documentées : itérations simultanées, `\warning` sur l'absence de borne, règle d'éboulement de Step (angle de repos 0,3, au plus 5 cm partagés, `\return`). La doc décrit le partage voulu ; la masse doublée relève de B-T11.

#### 📝 D-T17 — `Source/terrain-thermal.cpp:21` — ThermalImpactSlope : doc fausse

Annonce bedrock → rocks ; le code fait rocks → sand et bedrock → sand.  
**Correctif :** Corriger.

#### 📝 D-T18 — `Source/terrain-thermal.cpp:216` — ThermalDropletErosion : nom trompeur

Lance des gouttes hydrauliques `Droplet(p, 150.0)` (valeur codée en dur).  
**Correctif :** Renommer/documenter.

#### 📝 D-T19 — `Include/layersfielderosion.h:14` — Membre `flowouside` mal nommé et mal décrit

Faute (« flowoutside ») ; « Percentage » alors que c'est une fraction [0,1].  
**Correctif :** Renommer et corriger.

#### 📝 D-T20 — `Source/terrain-vegetation.cpp:15` — VegetationGrowingStep : « eight neighbours »

La boucle inclut la cellule elle-même.  
**Correctif :** Corriger.

#### 📝 D-T21 — `Source/terrain-vegetation.cpp:97` — Commentaires de seuils obsolètes

Seuils 0.0-0.1 et 100-1000 ne correspondent pas au code (l.97, 104) ; « treated a soil ».  
**Correctif :** Mettre à jour.

#### 📝 ✅ D-T22 — `Source/terrain-grass.cpp:5` — \brief vides

GrassGrowingStep (l.5), GrassSpawn (l.38) — qui écrase `grass`.  
**Correctif :** (appliqué) GrassGrowingStep (diffusion de 1 % vers la cellule et ses huit voisines) et GrassSpawn (SmoothStep du sable de 5 à 15 cm, `\warning` : écrase `grass`) documentées.

#### 📝 ✅ D-T23 — `Source/terrain-image.cpp:6` — Texture : \brief vide

Commentaire l.67 (0.1 m) ≠ `sand > 0` ; couleurs `cisolines` inutilisées (l.18-19).  
**Correctif :** (appliqué) `\brief` décrivant l'ordre des matériaux et l'ombrage ; commentaire corrigé en « Sand color wherever there is sand » ; `cisolines` et `cisolinesBold` supprimées.

#### 📝 ✅ D-T24 — `Source/terrain.cpp:75` — GetStats : \brief vide

« Height » n'affiche que le bedrock (l.97) ; espace manquante l.96.  
**Correctif :** (appliqué) `\brief` ajouté ; « Height » affiche désormais l'altitude totale (bedrock + rocks + sand) et une ligne « Bedrock » l'intervalle du bedrock ; espace ajoutée dans « %1 × %2 ».

#### 📝 D-T25 — `Source/terrain.cpp:107` — CARVE, TRYY sans doc

Noms non descriptifs.  
**Correctif :** Documenter ou retirer.

#### 📝 D-T26 — `Source/heightfielderosion.cpp:22` — HillSlope : signe faux dans la formule

La doc donne h − k·t·Δh ; le code (correct) fait h + k·t·Δh.  
**Correctif :** Corriger la formule.

#### 📝 D-T27 — `Source/heightfielderosion.cpp:37` — DebrisSlope : ‖∇h‖ annoncé

Le code utilise `AverageSlope()`.  
**Correctif :** Corriger.

#### 📝 D-T28 — `Source/heightfielderosion.cpp:77` — StreamPowerErosion : SetBorder(0) non documenté

Les bords sont forcés à 0 à chaque appel (l.87).  
**Correctif :** Documenter.

#### 📝 D-T29 — `Source/heightfielderosion.cpp:5` — \brief « Erosion. »

Description minimale de la classe.  
**Correctif :** Compléter.

#### 📝 D-T30 — `Source/hexagonheightfield.cpp:47` — Normal : doc ne correspond pas aux triangles

Voir B-T25.  
**Correctif :** Corriger avec le code.

#### 📝 D-T31 — `Include/hexagonheightfield.h:9` — Section `protected:` vide

Cosmétique.  
**Correctif :** Supprimer.

#### 📝 D-T32 — `Include/analyticfield.h:52` — `//!` au lieu de `//!<`

Membres `noise` (l.52) et `H` (l.73) : la doc est rattachée au membre suivant.  
**Correctif :** `//!<`.

#### 📝 D-T33 — `Include/analyticfield.h:74` — Membres sans doc

`lacunarity`, `gain`, `tt` (l.74-77).  
**Correctif :** Ajouter `//!<`.

#### 📝 D-T34 — `Include/analyticfield.h:105` — Défaut `Vector::Null` pour un Vector2

Conversion implicite.  
**Correctif :** `Vector2::Null`.

#### 📝 ✅ D-T35 — `Include/analyticfield.h:5` — ~~Classes sans \class dans l'en-tête~~ (faux positif)

~~AnalyticFieldDune (l.49), AnalyticRidged (l.101).~~  
**Faux positif :** la bibliothèque place les blocs `\class` dans les .cpp, et ils existent depuis l'origine : `\class AnalyticFieldDune` (heightdune.cpp:4) et `\class AnalyticRidged` (analyticmusgrave.cpp:62).

#### 📝 D-T36 — `Source/analyticfield.cpp:8` — Deux epsilons coexistent

`static double Epsilon` non const, utilisé seulement par `Slope(p, d)`.  
**Correctif :** `const` et documenter.

#### 📝 D-T37 — `Source/analyticmusgrave.cpp:21` — Ordre de la liste d'initialisation

Diffère de l'ordre de déclaration (-Wreorder).  
**Correctif :** Réordonner.

#### 📝 D-T38 — `Source/analyticmusgrave.cpp:16` — \param o « Number of signals »

Doc de `gain` trompeuse tant que B-T27 n'est pas corrigé.  
**Correctif :** « Number of octaves ».

#### 📝 ✅ D-T39 — `Source/heightfieldsand.cpp:116` — BedrockLevel : doc juste, code faux

Voir le bug BedrockLevel.  
**Correctif :** (appliqué avec B-T04) le code nivelle désormais vers z comme annoncé ; la doc a été précisée (pondération décroissante du centre au bord du disque).

### L — Couches de matériaux

#### 📝 D-L01 — `Source/layer-geometry.cpp:86` — GetColorVector : doc copiée

« Compute the vertex… » alors que la fonction renvoie une couleur.  
**Correctif :** Corriger.

#### 📝 D-L02 — `Source/layer-stack.cpp:113` — Vertex(const Vector2&) : doc copiée de Height

Renvoie un sommet 3D.  
**Correctif :** Corriger.

#### 📝 D-L03 — `Source/layer-stack.cpp:320` — GetTrees : doc vide

`\brief \warn`.  
**Correctif :** Documenter l'échantillonnage.

#### 📝 ✅ D-L04 — `Source/layer-generic.cpp:270` — CreateCubes : \param mal ordonnés

`\param cube Domain.` est une sortie ; ordre v/g différent de la signature.  
**Correctif :** (appliqué) `\param cube` décrit comme boîte de référence renvoyée, ensembles de frames décrits comme sorties (ajout en fin), ordre b, r, s, w, v, g explicité ; phrase descriptive remontée avant les `\param`.

#### 📝 ✅ D-L05 — `Source/layer-generic.cpp:36` — UpdateHeightField : \param incomplets

« Rock and flags. » ; QRect() vide = tout le domaine non dit.  
**Correctif :** (appliqué) `\param a` : zone en coordonnées entières, QRect() vide = tout le domaine ; `\param r,s` : « Rock and sand flags ».

#### 📝 D-L06 — `Source/layer-generic.cpp:105` — CreateWaterGeometry présentée comme fonctionnelle

Fonction inachevée ; fautes « assing » (l.207, 234).  
**Correctif :** `\warning` tant que non corrigée.

#### 📝 ✅ D-L07 — `Include/layers.h:105` — « elevation » pour des épaisseurs

Rocks, Grass, Water (l.105-157) ; grass est une densité.  
**Correctif :** (appliqué) « rocks layer thickness (meters) », « grass density », « water depth (meters) » pour les deux surcharges de Rocks, Grass et Water (layers.h:106-151).

#### 📝 ✅ D-L08 — `Source/layer-stack.cpp:345` — WaterFlood(const double&) : grass non mentionné

La couche grass est aussi remise à zéro (l.365).  
**Correctif :** (appliqué) doc réécrite : niveau absolu en mètres, couches snow, grass et vegetation remises à zéro sous l'eau, eau retirée ailleurs.

#### 📝 D-L09 — `Source/layer-stack.cpp:167` — Variables locales a, b masquant Box2::a/b

GetBox (l.167), GetTrees (l.331-332).  
**Correctif :** Renommer.

#### 📝 ✅ D-L10 — `Include/layers.h:5` — Sémantique des couches non documentée

Épaisseurs vs densités, roches hors Vertex.  
**Correctif :** (appliqué) membres de `layers.h` documentés (épaisseurs en mètres : rocks, water ; densités : grass, vegetation ; couches d'attributs : snow, alpha ; roches incluses dans `Height()` mais pas dans `Vertex(int, int)`), et convention décrite dans le bloc `\class LayerStack` (`layer-stack.cpp:14`).

#### 📝 ✅ D-L11 — `Include/layersfielderosion.h:9` — bedrockflow/sandflow même commentaire

« Flowing sediments. » pour les deux.  
**Correctif :** (appliqué) `bedrockflow` : « Detached bedrock (accumulated amount of eroded bedrock), see BedrockDetachment(). » ; `sandflow` : « Flowing sand (accumulated amount of eroded and transported sand), see SandFlow(). »

#### 📝 D-L12 — `Include/layersfielderosion.h:85` — TRYY dans l'API publique

Nom non significatif.  
**Correctif :** Renommer ou retirer.

#### 📝 ✅ D-L13 — `Include/layersfielderosion.h:62` — Accesseurs sans doc

BedrockDetachment, SandFlow, WaterFlow.  
**Correctif :** (appliqué) commentaires `//!` sur BedrockDetachment, SandFlow et WaterFlow.

### R — Rendu (shader de terrain)

#### 📝 D-R01 — `Source/heightfieldshader.cpp:279` — BrownShading sans doc

Pas de bloc Doxygen (l.279, 287).  
**Correctif :** Documenter.

#### 📝 ✅ D-R02 — `Source/heightfieldshader.cpp:141` — Fonctions libres sans doc et exportées

ImageShadeRadianceScaling* (l.142-171).  
**Correctif :** (appliqué) fonctions supprimées avec B-R15.

#### 📝 D-R03 — `Source/heightfieldshader.cpp:111` — Relief : \param enhanced absent

Paramètre non documenté.  
**Correctif :** Ajouter.

#### 📝 ✅ D-R04 — `Source/heightfieldshader.cpp:233` — ImageShadeRadianceScaling : \param absents

color1, color2.  
**Correctif :** (appliqué) fonction supprimée avec B-R15.

#### 📝 ✅ D-R05 — `Source/heightfieldshader.cpp:173` — RadianceScaling : doc mal formée

« Returns » en texte libre.  
**Correctif :** (appliqué) description des deux lumières en texte, `\param pos` et `\param factor` reformatés, `\return` : champ à la résolution du terrain, valeurs bornées à [0, 1] (et non « normalisées » : le code fait un `Clamp`). La fonction a depuis été supprimée avec B-R15.

#### 📝 ✅ D-R06 — `Source/heightfieldshader.cpp:620` — Eric : nom et doc copiés

Aucun éclairage n'est calculé.  
**Correctif :** (appliqué) méthode renommée `ShadedReliefFractionnalGradient()` (déclaration `heightfieldshader.h:22`, définition `heightfieldshader.cpp:633`) ; doc Doxygen réécrite (niveau de gris 0.80 + 0.05 z + 0.10 l + 0.05 s, paramètres des calculs GPU, `\warning` contexte OpenGL, `\return`). Les 10 appels hors bibliothèque ont été mis à jour : AppTerrainEditor (`qte-main-window.cpp:465`), ExpAntony/AppAntony (`qte-main-window.cpp:494`), ExpCelian/ILand (`callback-debug.cpp:303`, `callback-scene.cpp:604, 674, 927, 959`, `qte-main-window.cpp:266, 344, 363`).

#### 📝 ✅ D-R07 — `Source/heightfieldshader.cpp:558` — ShadedRelief : doc identique à Relief

Laplacien fractionnaire et GPU non mentionnés.  
**Correctif :** (appliqué) `\brief` propre à la fonction ; description des termes (couleur selon l'altitude, lumière diffuse, laplacien fractionnaire et accessibilité calculés sur GPU, laplacien) ; `\warning` sur le contexte OpenGL requis ; `\return`.

#### 📝 D-R08 — `Source/heightfieldshader.cpp:214` — LaplacianShading : taille de sortie non documentée

2048²/4096², contexte GL requis ; bloc terminé par `* */`.  
**Partiellement corrigé** (avec D-R15) : `\warning` sur le contexte OpenGL ajouté et bloc refermé par `*/` ; reste à documenter la taille de sortie (plus grand côté ramené à 2048 ou 4096, rapport d'aspect conservé depuis B-R11).  
**Correctif :** Compléter.

#### 📝 D-R09 — `Source/heightfieldshader.cpp:585` — CartographicGreen : commentaire « 1024 » faux

Aucun rééchantillonnage en 1024 ; dépendances non documentées.  
**Correctif :** Corriger.

#### 📝 D-R10 — `Source/heightfieldshader.cpp:596` — Variable locale masquant le membre heightfield

`HeightField heightfield(sf);`.  
**Correctif :** Renommer `hf2048`.

#### 📝 ✅ D-R11 — `Source/heightfieldshader.cpp:776` — RiverWidth : fautes et unités

« with », « rive » ; unité de size.  
**Correctif :** (appliqué) fautes corrigées (« width », « river ») ; transformation de l'aire drainée décrite ; `\param t` en unités de `StreamAreaSteepest()` ; `\param size` exprimé en diagonales de cellule (rayon maximal 1 + size), et non en pixels ; `\return` ajouté (image transparente hors des rivières).

#### 📝 ✅ D-R12 — `Source/heightfieldshader.cpp:772` — CartographicGreenYellow : GPU non mentionné

Contexte GL requis.  
**Correctif :** (appliqué avec D-R15) `\warning Requires a current OpenGL context, since the fractional Laplacian is computed with GPUHeightFieldAnalysis.`

#### 📝 D-R13 — `Source/heightfieldshader.cpp:398` — Code mort commenté

Ancienne IsoLines (l.398-442), bloc l.795-800.  
**Correctif :** Supprimer.

#### 📝 ✅ D-R14 — `Source/heightfieldshader.cpp:285` — Includes redondants en milieu de fichier

`cpu.h` (l.285), `gputerrainsimu.h` (l.556).  
**Correctif :** (appliqué) les deux includes supprimés, déjà présents en tête de fichier.

#### 📝 ✅ D-R15 — `Include/heightfieldshader.h:5` — Durée de vie et méthodes GPU non documentées

Référence stockée ; méthodes nécessitant un contexte GL.  
**Correctif :** (appliqué) bloc `\class` de heightfieldshader.cpp complété : durée de vie (référence pour un terrain nommé, copie pour un temporaire depuis B-R17, shader non copiable) et liste des méthodes GPU ; `\warning` sur le contexte OpenGL ajouté à LaplacianShading, CartographicGreen et CartographicGreenYellow (ShadedRelief et ShadedReliefFractionnalGradient l'avaient déjà).

### G — Érosion GPU

#### 📝 D-G01 — `Source/gputerrainsimu.cpp:7` — `\class GPUHydraulicErosion` inexistante

Bloc copié dans gputerrainsimu_test.cpp:7.  
**Correctif :** Nom correct.

#### 📝 D-G02 — `Source/gputerrainsimu.cpp:235` — GetSpeeds « no transfer back »

La fonction fait un transfert GPU→CPU.  
**Correctif :** Corriger.

#### 📝 D-G03 — `Source/gputerrainsimu.cpp:202` — SetUniforms sans \param

gputerrainsimu.cpp:202-204, gputerrainsimu_test.cpp:253-255.  
**Correctif :** Ajouter.

#### 📝 D-G04 — `Source/gputerrainsimu.cpp:312` — Mode « 3 = Faulting » inexistant

SetHardnessFunction (l.312-322, test.cpp:432-442).  
**Correctif :** Corriger.

#### 📝 D-G05 — `Source/gputhermal.cpp:205` — GetData mal documenté

« newest bedrock » pour bedrock+sédiment (l.205, 219).  
**Correctif :** Corriger.

#### 📝 D-G06 — `Source/gputhermal.cpp:192` — noiseWavelength par défaut faux

0.00023 vs 0.0023 (glsl:20).  
**Correctif :** Corriger.

#### 📝 D-G07 — `Shaders/heightfield_thermal_double.glsl:93` — Plage d'angle contredite

6°–54° annoncés, `mix(0.3, 0.7, t)` (l.99).  
**Correctif :** Corriger.

#### 📝 D-G08 — `Source/gputerrainsimuparticle.cpp:16` — Commentaire « doesn't work anymore »

À résoudre ou `\todo` ; coquille « alpja » (l.192).  
**Correctif :** Mettre à jour.

#### 📝 D-G09 — `Source/gputhermaldouble.cpp:7` — Fonctions sans doc

gputhermaldouble.cpp (l.7-145), gputhermalsimple.cpp (l.7-177), gputerrainsimu_test.cpp GetDataDebug (404), GPUMapProcessing (481-530).  
**Correctif :** Documenter.

#### 📝 D-G10 — `Include/gputerrainsimu.h:9` — Classes et membres sans doc

GLOptionalStorageBuffer (9), GLOptionalBuffer (18), GPUMapProcessing (72), SetSmoothSteps (144), nombreux membres sans `//!<` ; gpusediment.h:9-26.  
**Correctif :** Documenter.

#### 📝 D-G11 — `Source/gputerrainsimu_test.cpp:530` — Ping-pong de GPUMapProcessing::Step non documenté

Résultat dans `in` ; noms trompeurs.  
**Correctif :** Documenter.

#### 📝 D-G12 — `Source/gputhermal.cpp:59` — « bedrock set to 0 » non garanti

Voir bedrock non initialisé.  
**Correctif :** Corriger.

### P — Analyse GPU / stream power

#### 📝 D-P01 — `Source/gpuanalysis.cpp:80` — InitForTexture : précision surestimée

16 bits effectifs.  
**Correctif :** Corriger.

#### 📝 D-P02 — `Source/gpuanalysis.cpp:471` — \param s mal décrits

SoftShadows (ouverture du cône), Diffuse l.536 (exposant).  
**Correctif :** Corriger.

#### 📝 ✅ D-P03 — `Source/gpuanalysis.cpp:276` — Laplacian « double precision »

Relit un buffer float.  
**Correctif :** (appliqué avec B-P02) la fonction relit désormais le buffer double ; la doc « in double precision » est exacte.

#### 📝 D-P04 — `Shaders/heightfield_analysis.glsl:283` — Commentaires faux

Hash [0,1) et non [-1,1] ; « Mximum » (l.246) ; question « Gundy ? » (l.175).  
**Correctif :** Corriger.

#### 📝 D-P05 — `Include/gputerrainsimu.h:379` — maxSteps = 2256 suspect (à confirmer)

Non documenté.  
**Correctif :** Vérifier.

#### 📝 ✅ D-P06 — `Source/gpustreampoweroptimized.cpp:9` — `\class GPUStreamPowerImproved`

Nom de classe faux.  
**Correctif :** (appliqué) `\class GPUStreamPowerOptimized gputerrainsimu.h`.

#### 📝 ✅ D-P07 — `Source/gpustreampoweroptimized.cpp:279` — Erreurs de \param

SetNParameter « m » ; SetLaplacianParameter l.301 `laplacianMap`.  
**Correctif :** (appliqué) SetNParameter : « parameter 'n' (slope exponent) » et `\param nParam` décrit ; SetLaplacianParameter : `\param laplacianParam` au lieu de `laplacianMap`.

#### 📝 D-P08 — `Source/gpustreampoweroptimized.cpp:49` — Init : doc fausse

Flux non initialisé, ressources recréées.  
**Correctif :** Corriger.

#### 📝 D-P09 — `Shaders/heightfield_streampower_optimized.glsl:172` — Retargeting et threshold inopérants

GetRetargetingFactor jamais appelé ; threshold = 0 (l.455).  
**Correctif :** Retirer ou documenter.

#### 📝 D-P10 — `Shaders/heightfield_streampower_optimized.glsl:422` — Aire drainée élémentaire = longueur (à confirmer)

`length(cellDiag)` (l.422, 429, 435 ; ms_spe_gpu.glsl:164).  
**Correctif :** Documenter ou `cellDiag.x*cellDiag.y`.

#### 📝 D-P11 — `Shaders/map_processing.glsl:42` — Commentaire et uniform obsolètes

« threshold » sans rapport (l.13, 42).  
**Correctif :** Nettoyer.

#### 📝 D-P12 — `Shaders/map_add.glsl:17` — GetHardness morte

Appel commenté l.43.  
**Correctif :** Supprimer.

#### 📝 D-P13 — `Shaders/ms_spe_gpu.glsl:167` — Bords et max_spe non documentés

« do not erode borders » faux ; SetThreshold est un écrêtage.  
**Correctif :** Documenter.

#### 📝 D-P14 — `Source/ms_spe_gpu.cpp:5` — MS_SPE_GPU / GPUMapAdd sans doc

Aucun `\class` ni `\brief` ; message l.35 nommant une fonction inexistante.  
**Correctif :** Documenter.

#### 📝 D-P15 — `Source/gpu_drainage.cpp:4` — GPU_Drainage sans doc

Constructeur, Init, Reset, Step, GetData.  
**Correctif :** Documenter.

#### 📝 D-P16 — `Include/gpuheightfield.h:43` — GPUHeightField : doc incomplète

Swap `\brief` vide ; destructeur ne libérant rien ; `\param` du constructeur absents.  
**Correctif :** Compléter.

#### 📝 D-P17 — `Shaders/heightfield_procedural_erosion.glsl:17` — Dimensions et division par zéro non documentées

WORLD_SIZE = étendue en x ; `EROSION_GAIN == 1` (l.165).  
**Correctif :** Documenter.

---

## Optimisations ⚡

### H — Heightfield (cœur)

#### ⚡ O-H01 — `Source/heightfield.cpp:457` — AverageFlowDirection recalculée n fois

Appelée dans la boucle j avec les mêmes arguments (voir B-H03).  
**Correctif :** Sortir l'appel de la boucle.

#### ⚡ O-H02 — `Source/heightfield.cpp:506` — StreamAreaLimited : CheckFlowSlope recalculé à chaque itération

n·nx·ny appels alors que le terrain ne change pas.  
**Correctif :** Précalculer un `QVector<FlowStruct>` avant la boucle k.

#### ⚡ O-H03 — `Source/heightfield.cpp:1016` — Shade : GetBox() et K() recalculés à chaque point

Deux parcours complets de la grille par point shadé, soit O(N) par pixel.  
**Correctif :** Passer la boîte et la constante de Lipschitz en paramètres.

#### ⚡ O-H04 — `Source/heightfield-access.cpp:209` — TerrainDX : accumulation en O(N·L)

Le chemin D8 complet est suivi depuis chaque cellule (O(N²) au pire).  
**Correctif :** Tri par altitude décroissante et propagation en une passe (O(N log N)).

#### ⚡ O-H05 — `Source/heightfield.cpp:1583` — StabilizeSlope : distances recalculées

`Norm(ArrayVertex(p) - ArrayVertex(np))` pour chaque cellule, voisin et itération.  
**Correctif :** Précalculer les 8 distances.

#### ⚡ O-H06 — `Source/heightfield.cpp:1667` — FastProceduralErosion : buffer `dataSlope` inutilisé

Allocation de `totalBufferSize` floats jamais utilisée.  
**Correctif :** Supprimer `dataSlope`.

#### ⚡ O-H07 — `Source/heightfield-mesh.cpp:21` — CreateMeshSide : reserve double

`indexes.reserve(6 * 2 * n)` réserve le double des 6·n indices ajoutés.  
**Correctif :** `indexes.reserve(6 * n)`.

### A — Analyse de terrain CPU

#### ⚡ O-A01 — `Source/heightfield-metrics.cpp:774` — ReliefSteepness/Jut/Rut : distances recalculées

sqrt et `Vertex()` recalculés par paire (l.774-796, 844-863, 909-928).  
**Correctif :** Précalculer les décalages (di, dj, dist) du disque ; `#pragma omp parallel for`.

#### ⚡ O-A02 — `Source/heightfield-metrics.cpp:956` — FractLaplacian : pow par voisin

`pow(i*i+j*j, 1+s)` évalué nx·ny·(2n+1)·n fois (l.956, 966).  
**Correctif :** Table de poids précalculée.

#### ⚡ O-A03 — `Source/heightfield-metrics.cpp:248` — AreaRatio en force brute

O(N·w²) ; `areaPlane` = count·cellPlanarArea.  
**Correctif :** SAT des aires.

#### ⚡ O-A04 — `Source/heightfield-metrics.cpp:630` — TRI quadratique en force brute

Σ(v−hp)² se calcule en O(1) avec les SAT de LocalVariance_SAT.  
**Correctif :** Réutiliser les SAT (valeurs centrées).

#### ⚡ ✅ O-A05 — `Source/heightfield-fill.cpp:212` — FillDepressions : indexation x*size_y + y

Ordre mémoire différent de `VertexIndex` (l.212, 247, 249, 319, 371, 373).  
**Correctif :** (appliqué) `state[VertexIndex(x, y)]` dans les deux FillDepressions et dans InitBorders, et `state` passé de `std::vector<State>` (4 octets par sommet) à `std::vector<unsigned char>` (1 octet). Équivalence vérifiée bit à bit sur 480 résultats (120 terrains de 1×1 à 257×101, bruit, paliers, plat, cuvette ; les quatre surcharges) ; en /O2, de l'ordre de 13 % plus rapide sur 2048² (0,90 s au lieu de 1,05 s).

### T — Simulation de terrain CPU

#### ⚡ O-T01 — `Source/heightfieldsand.cpp:209` — Intersect : pas fixe de 1.0

Lent sur grand domaine, imprécis si cellule < 1 m.  
**Correctif :** Pas proportionnel à la cellule + dichotomie finale.

#### ⚡ O-T02 — `Source/terrain-hydraulic.cpp:214` — WorldmachineSeveralStep duplique WorldmachineStep

Copie intégrale (maintenabilité).  
**Correctif :** Appeler `WorldmachineStep(e)` `steps` fois.

#### ⚡ ✅ O-T03 — `Source/terrain-image.cpp:87` — AnalyticPalette construite par pixel

Construction dans la boucle.  
**Correctif :** (appliqué, via B-T24) la ligne qui construisait la palette a été supprimée.

#### ⚡ O-T04 — `Source/terrain.cpp:125` — CARVE en O(N·(r/h)²)

Disque jusqu'à 250 m par sommet.  
**Correctif :** Seuil de drainage, rayon plafonné.

#### ⚡ O-T05 — `Source/analyticfield.cpp:72` — AverageSlope : directions comptées deux fois

Pour n pair, t + π/2 fait déjà partie de l'échantillonnage : 2× trop d'évaluations.  
**Correctif :** n directions sur [0, π) sans orthogonale.

#### ⚡ O-T06 — `Source/analyticmusgrave.cpp:53` — pow(frequency, -H) par octave

Recalculé à chaque octave.  
**Correctif :** Facteur cumulé.

### L — Couches de matériaux

#### ⚡ O-L01 — `Source/layer-stack.cpp:241` — QSet<int> pour des indices denses

WaterFlood/WaterDry (l.241-243, 262, 290-292, 310).  
**Correctif :** `std::vector<char> visited(nx * ny)`.

#### ⚡ O-L02 — `Source/layer-stack.cpp:192` — Intersect dupliqué de HeightFieldSediment::Intersect

Copie intégrale.  
**Correctif :** Factoriser.

#### ⚡ O-L03 — `Source/layer-generic.cpp:285` — CreateCubes sans reserve

`bedrockcubes` reçoit nx·ny éléments.  
**Correctif :** `reserve`.

#### ⚡ O-L04 — `Include/layersfielderosion.h:62` — Accesseurs renvoyant des champs par valeur

BedrockDetachment, SandFlow, WaterFlow copient tout le champ.  
**Correctif :** Retour par `const&`.

### R — Rendu (shader de terrain)

#### ⚡ ✅ O-R01 — `Source/heightfieldshader.cpp:615` — Slope() calculé et inutilisé

`slopeField` dans CartographicGreen, sur 2048².  
**Correctif :** (appliqué) ligne supprimée.

#### ⚡ ✅ O-R02 — `Source/heightfieldshader.cpp:463` — GetRange inutilisé

`fractional.GetRange(u, v)` dans ShadedRelief : parcours complet sans usage.  
**Correctif :** (appliqué) `GetRange` et variables `u`, `v` supprimés.

#### ⚡ ✅ O-R03 — `Source/heightfieldshader.cpp:459` — Deux GPUHeightFieldAnalysis temporaires

ShadedRelief (l.459, 464) et Eric, renommée ShadedReliefFractionnalGradient (l.525, 531), recréent buffers et shaders.  
**Correctif :** (appliqué) une seule instance `GPUHeightFieldAnalysis analysis` par fonction (buffers et texture réutilisés). `InitForTexture()` recompile toujours le programme à chaque analyse : un `shader.Destroy()` y a été ajouté avant `shader.Initialize()` pour libérer le programme précédent, sinon le partage de l'instance fuyait un programme GL par appel (gpuanalysis.cpp).

#### ⚡ ✅ O-R04 — `Source/heightfieldshader.cpp:772` — Aller-retour QColor → Color → QColor

CartographicGreenYellow (l.772-777).  
**Correctif :** (appliqué) `Color color(qcolor)` et la reconstruction `QColor(255. * color[0], …)` supprimés : `getHsvF` est appelé directement sur `texture.pixelColor(i, j)` (supprime aussi la troncature double → int de la reconstruction).

#### ⚡ ✅ O-R05 — `Source/heightfieldshader.cpp:67` — Boucles i externe sur QImage

Accès non contigus via setPixel : ReliefLighting (l.67-83), Relief (l.118-143), ShadedRelief (l.471-493), ShadedReliefFractionnalGradient (l.536-547), RiverWidth (l.683-741).  
**Correctif :** (appliqué) dans les cinq boucles, j externe, pointeur de ligne `QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(j))` et écriture `line[i] = c.GetQt().rgb()` (`rgba()` dans RiverWidth pour conserver l'alpha) ; la boucle de dispersion des disques de RiverWidth est laissée telle quelle (l'ordre de parcours y départage les ex aequo).

### G — Érosion GPU

#### ⚡ O-G01 — `Source/gputerrainsimu.cpp:146` — Uniforms constants repositionnés à chaque pas

gputerrainsimu.cpp:146-148, 185-190 ; gputerrainsimu_test.cpp:238-239 ; gputerrainsimuparticle.cpp:166-172 ; gputhermal.cpp:149, 175-177.  
**Correctif :** Sortir de la boucle.

#### ⚡ O-G02 — `Source/gputerrainsimuparticle.cpp:175` — 64 gouttes par dispatch

`glDispatchCompute(1,1,1)` par pas ; `dispatchSize = 64` (l.59) inutilisé.  
**Correctif :** Un seul dispatch de `dispatchSize` groupes.

#### ⚡ O-G03 — `Shaders/heightfield_hydraulic_test.glsl:108` — ~256 lectures SSBO par cellule

8 voisins × 8 Slope (l.108-147).  
**Correctif :** Voisinage 5×5 en tableau local.

#### ⚡ O-G04 — `Shaders/deposition.glsl:124` — Poids d'écoulement calculés deux fois

StreamIncomingWeighted et SedIncomingWeighted (l.124-150).  
**Correctif :** Fusionner.

### P — Analyse GPU / stream power

#### ⚡ O-P01 — `Source/gpuanalysis.cpp:45` — Recompilation du shader à chaque appel

l.45-53, 92-100, 152-160, 203-211.  
**Correctif :** Compiler une fois (règle aussi B-P12).

#### ⚡ O-P02 — `Source/gpuanalysis.cpp:43` — Dispatch carré max(nx, ny)

gpuanalysis.cpp:43, 90, 150, 201 ; gpustreampoweroptimized.cpp:59 ; gpu_drainage.cpp:28 ; ms_spe_gpu.cpp:19.  
**Correctif :** `glDispatchCompute((nx+7)/8, (ny+7)/8, 1)`.

#### ⚡ O-P03 — `Source/gpuanalysis.cpp:828` — VisibilityIndex en N² dispatches

glFinish et cout par ligne.  
**Correctif :** Dispatch 3D par lots de points de vue.

#### ⚡ O-P04 — `Shaders/heightfield_analysis.glsl:713` — GetRange redondant et séquentiel

Chaque tranche calculée 8 fois.  
**Correctif :** Réduction en mémoire `shared` ou atomics.

#### ⚡ O-P05 — `Source/gpustreampoweroptimized.cpp:150` — Uniforms invariants ré-envoyés

l.150-157, 200-209 ; ms_spe_gpu.cpp:154-155.  
**Correctif :** Sortir de la boucle.

#### ⚡ O-P06 — `Shaders/ms_spe_gpu.glsl:169` — Écriture inutile de out_terrain

Écrasé par map_add.  
**Correctif :** Supprimer.

#### ⚡ O-P07 — `Source/ms_spe_gpu.cpp:24` — Buffers liés mais inutilisés

in_hardness_buffer, in_delta (ms_spe_gpu.cpp:24, 57-58, 81-82, 163 ; glsl:14, 18, 41-58).  
**Correctif :** Supprimer.

#### ⚡ O-P08 — `Source/gpu_drainage.cpp:68` — GetBox() inutilisé

Parcours complet.  
**Correctif :** Supprimer.

#### ⚡ O-P09 — `Shaders/heightfield_analysis.glsl:179` — texture() pour un accès entier

Height(int,int).  
**Correctif :** `texelFetch`.

#### ⚡ O-P10 — `Source/gpustreampoweroptimized.cpp:77` — Indices GL_STREAM_READ inadaptés

gpuheightfield.cpp:31, 34 ; gpuanalysis.cpp:167, 218 ; gpustreampoweroptimized.cpp:77-107 ; gpu_drainage.cpp:59-65 ; ms_spe_gpu.cpp:64-82.  
**Correctif :** `GL_DYNAMIC_COPY`.

---

## Index des problèmes

### B-H — Heightfield (cœur)

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-H01 | ⬜ | 🔴 | Carving inversé (poids appliqué au terrain au lieu de la cible) | `Source/heightfield.cpp:1305` |
| B-H02 | ⬜ | 🔴 | Sélection de la k-ième direction fausse (FlowTo(p) entièrement faux) | `Source/heightfield-next.cpp:104` |
| B-H03 | ⬜ | 🟠 | Direction d'écoulement écrite dans les cellules aval | `Source/heightfield.cpp:457` |
| B-H04 | ✅ | 🟠 | Dimensions formatées avec 1 chiffre significatif | `Source/heightfield.cpp:1722` |
| B-H05 | ✅ | 🟠 | Fuite d'un programme GL à chaque appel, échec de compilation ignoré | `Source/heightfield.cpp:1663` |
| B-H06 | ✅ | 🟠 | Dispatch carré sans garde côté shader : bords écrasés de façon concurrente | `Source/heightfield.cpp:1695` |
| B-H07 | ⬜ | 🟡 | Terrain non carré rendu carré | `Source/heightfield.cpp:1644` |
| B-H08 | ⬜ | 🟡 | Regex refusant les altitudes négatives, échec silencieux | `Source/heightfield.cpp:1741` |
| B-H09 | ⬜ | 🟡 | Division par `sum` nul et taille de `intensity` non vérifiée | `Source/heightfield.cpp:1040` |
| B-H10 | ⬜ | 🟡 | Résolution de `h` non vérifiée | `Source/heightfield.cpp:1185` |
| B-H11 | ⬜ | 🟡 | Cellule terminale jamais comptée dans l'accumulation | `Source/heightfield-access.cpp:220` |
| B-H12 | ⬜ | 🟡 | Lecture hors bornes pour une cellule de bord | `Source/heightfield-access.cpp:333` |
| B-H13 | ⬜ | 🟡 | Mode « steepest » comparant des dénivelés bruts | `Source/heightfield-next.cpp:58` |
| B-H14 | ⬜ | 🟡 | Faces latérales et normales orientées vers l'intérieur (à confirmer) | `Source/heightfield-mesh.cpp:61` |
| B-H15 | ⬜ | 🟡 | Division par zéro si aucun échantillon diurne | `Source/heightfield-lighting.cpp:68` |
| B-H16 | ⬜ | 🔵 | Mois/jour passés hors plage à Sun::Angles | `Source/heightfield-lighting.cpp:54` |
| B-H17 | ⬜ | 🔵 | Départage comparant lhs à lui-même | `Source/heightfield-carve.cpp:21` |
| B-H18 | ⬜ | 🔵 | Trace de débogage | `Source/heightfield-carve.cpp:78` |
| B-H19 | ⬜ | 🔵 | Trace de débogage | `Source/heightfield.cpp:1446` |
| B-H20 | ⬜ | 🔵 | Condition de boucle toujours vraie | `Source/heightfield.cpp:1463` |
| B-H21 | ⬜ | 🔵 | Carré de 2×2 cellules par échantillon | `Source/heightfield.cpp:1067` |
| B-H22 | ⬜ | 🔵 | Division par zéro pour un terrain plat | `Source/heightfield.cpp:1064` |
| B-H23 | ⬜ | 🔵 | Altitude 0 sur le bord haut et hors domaine | `Source/heightfield.cpp:263` |
| B-H24 | ⬜ | 🔵 | Critères asymétriques | `Source/heightfield.cpp:1137` |
| B-H25 | ⬜ | 🔵 | Nombre d'échantillons incorrect pour n < 2 | `Source/heightfield.cpp:1225` |
| B-H26 | ⬜ | 🔵 | Sorties modifiées puis échec quand l'origine est dans la boîte (à confirmer) | `Source/heightfield.cpp:847` |
| B-H27 | ⬜ | 🔵 | Sentinelle -1 initialisée sur un seul élément | `Include/heightfield.h:19` |
| B-H28 | ⬜ | 🔵 | Offset de bruit tiré par `rand()` | `Source/heightfield.cpp:1606` |

**3 / 28 corrigés** — restants : 2🔴 · 1🟠 · 9🟡 · 13🔵

### B-A — Analyse de terrain CPU

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-A01 | ⬜ | 🔴 | Classification inversée (sommet → cuvette, crête → vallée) | `Source/heightfield-geomorphons.cpp:60` |
| B-A02 | ⬜ | 🔴 | Classification agrégée inversée | `Source/heightfield-geomorphons.cpp:190` |
| B-A03 | ✅ | 🟠 | Inverse de la longueur diagonale fausse (×2) | `Source/heightfield-fill.cpp:77` |
| B-A04 | ✅ | 🟡 | Annulation catastrophique → NaN | `Source/heightfield-metrics.cpp:514` |
| B-A05 | ✅ | 🟡 | NaN pour w ≤ 1 | `Source/heightfield-metrics.cpp:513` |
| B-A06 | ✅ | 🟡 | GaussianBlur(0) produit un champ NaN | `Source/heightfield-metrics.cpp:300` |
| B-A07 | ⬜ | 🟡 | Tangentes diagonales surestimées de √2 | `Source/heightfield-geomorphons.cpp:112` |
| B-A08 | ⬜ | 🟡 | Directions sortantes comptées comme « plus bas » | `Source/heightfield-geomorphons.cpp:114` |
| B-A09 | ⬜ | 🟡 | Échantillon sur le dernier sommet lu à 0 (à confirmer) | `Source/heightfield-geomorphons.cpp:119` |
| B-A10 | ⬜ | 🔵 | Fenêtre SAT décalée d'une cellule | `Source/heightfield-metrics.cpp:176` |
| B-A11 | ⬜ | 🔵 | Normalisation de direction morte | `Source/heightfield-metrics.cpp:32` |
| B-A12 | ⬜ | 🔵 | Index −1 pour nx ou ny = 1 | `Source/heightfield-metrics.cpp:239` |
| B-A13 | ⬜ | 🔵 | Demi-fenêtre j calculée avec dx | `Source/heightfield-metrics.cpp:382` |
| B-A14 | ⬜ | 🔵 | Moyenne absolue divisée par 9 au lieu de 8 | `Source/heightfield-metrics.cpp:639` |
| B-A15 | ⬜ | 🔵 | Dernière ligne/colonne jamais stabilisées | `Source/heightfield-fill.cpp:78` |
| B-A16 | ⬜ | 🔵 | num et steepest non renseignés | `Source/heightfield-flow.cpp:121` |
| B-A17 | ⬜ | 🔵 | steepest non réinitialisé | `Source/heightfield-flow.cpp:47` |
| B-A18 | ⬜ | 🔵 | Opérateur virgule résiduel | `Source/heightfield-flow.cpp:89` |
| B-A19 | ⬜ | 🔵 | Pas de contrôle num < 8 | `Source/heightfield-flow.cpp:16` |
| B-A20 | ⬜ | 🔵 | Symboles globaux à liaison externe (risque ODR) | `Source/heightfield-fill.cpp:9` |

**4 / 20 corrigés** — restants : 2🔴 · 0🟠 · 3🟡 · 11🔵

### B-T — Simulation de terrain CPU

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-T01 | ⬜ | 🔴 | Boucle infinie : eau réinitialisée à chaque itération | `Source/heightfieldsand.cpp:524` |
| B-T02 | ⬜ | 🔴 | Boucle infinie dès qu'il y a de l'eau | `Source/terrain-hydraulic.cpp:339` |
| B-T03 | ✅ | 🔴 | `slope` non initialisé et téléportation en (0,0) | `Source/terrain-droplet.cpp:60` |
| B-T04 | ✅ | 🟠 | Flatten appelé avec une altitude comme facteur d'échelle | `Source/heightfieldsand.cpp:125` |
| B-T05 | ✅ | 🟠 | Couche de sable ignorée | `Source/heightfieldsand.cpp:214` |
| B-T06 | ⬜ | 🟠 | Paramètres de rayon 2D utilisés en 3D | `Source/heightfieldsand.cpp:200` |
| B-T07 | ✅ | 🟠 | Matériau en suspension dupliqué | `Source/heightfieldsand.cpp:671` |
| B-T08 | ✅ | 🟠 | Matériau en suspension dupliqué | `Source/terrain-hydraulic.cpp:360` |
| B-T09 | ✅ | 🟠 | Pas courant perdu quand la goutte s'évapore | `Source/terrain-droplet.cpp:118` |
| B-T10 | ⬜ | 🟠 | Transfert d'eau excessif : oscillation sans fin | `Source/terrain-hydraulic-dynamic.cpp:162` |
| B-T11 | ⬜ | 🟠 | Ligne dupliquée : masse de roches doublée | `Source/terrain-lightning.cpp:169` |
| B-T12 | ✅ | 🟠 | Bouclier inversé | `Source/terrain-lightning.cpp:41` |
| B-T13 | ⬜ | 🟠 | Végétation et herbe toujours nulles | `Source/terrain-vegetation.cpp:90` |
| B-T14 | ✅ | 🟠 | Somme renvoyée au lieu de la moyenne | `Source/analyticfield.cpp:85` |
| B-T15 | ⬜ | 🟡 | Érosion cumulée supérieure au sable disponible | `Source/heightfieldsand.cpp:665` |
| B-T16 | ⬜ | 🟡 | Dernier sommet jamais stabilisé | `Source/heightfieldsand.cpp:246` |
| B-T17 | ⬜ | 🟡 | Quantité déplacée non bornée : oscillation possible | `Source/heightfieldsand.cpp:354` |
| B-T18 | ⬜ | 🟡 | `length[k]` indexé par candidat | `Source/heightfieldsand.cpp:737` |
| B-T19 | ⬜ | 🟡 | Pente en unités d'indice | `Source/heightfieldsand.cpp:444` |
| B-T20 | ⬜ | 🟡 | Érosion cumulée supérieure au sable disponible | `Source/terrain-hydraulic.cpp:354` |
| B-T21 | ⬜ | 🟡 | Paramètre de pluie ignoré | `Source/terrain-hydraulic-dynamic.cpp:8` |
| B-T22 | ⬜ | 🟡 | Sable cumulé sur 8 directions > disponible | `Source/terrain-hydraulic-dynamic.cpp:184` |
| B-T23 | ⬜ | 🟡 | Pas de borne ni de CutEpsilon (à confirmer) | `Source/terrain-lightning.cpp:77` |
| B-T24 | ✅ | 🟡 | Pixel composé écrasé par une palette de pente | `Source/terrain-image.cpp:87` |
| B-T25 | ⬜ | 🟡 | Anneau de voisins de grille carrée | `Source/hexagonheightfield.cpp:60` |
| B-T26 | ⬜ | 🟡 | Hors bornes pour nx ou ny = 1 | `Source/hexagonheightfield.cpp:55` |
| B-T27 | ✅ | 🟡 | Pondération inter-octaves désactivée | `Source/analyticmusgrave.cpp:52` |
| B-T28 | ✅ | 🟡 | Remappage [-1,1] seulement sur la 1re octave | `Source/analyticmusgrave.cpp:48` |
| B-T29 | ✅ | 🟡 | Décalage `t` ignoré | `Source/analyticmusgrave.cpp:85` |
| B-T30 | ⬜ | 🔵 | Angle de repos codé en dur | `Source/heightfieldsand.cpp:386` |
| B-T31 | ⬜ | 🔵 | Érosion du bedrock non bornée par la capacité | `Source/heightfieldsand.cpp:617` |
| B-T32 | ⬜ | 🔵 | FlowStruct partiellement rempli | `Source/heightfieldsand.cpp:422` |
| B-T33 | ⬜ | 🔵 | Champs alloués inutilisés | `Source/heightfieldsand.cpp:326` |
| B-T34 | ⬜ | 🔵 | `static RandomFast rf` inutilisé | `Source/heightfieldsand.cpp:711` |
| B-T35 | ⬜ | 🔵 | Grilles non vérifiées | `Source/heightfieldsand.cpp:42` |
| B-T36 | ⬜ | 🔵 | `dbedrock` toujours 0 | `Source/terrain-hydraulic.cpp:405` |
| B-T37 | ⬜ | 🔵 | Fonction libre à liaison externe | `Source/terrain-hydraulic.cpp:379` |
| B-T38 | ⬜ | 🔵 | `dbedrock` toujours 0 | `Source/terrain-hydraulic-dynamic.cpp:217` |
| B-T39 | ⬜ | 🔵 | Code de débogage vide | `Source/terrain-hydraulic-dynamic.cpp:31` |
| B-T40 | ⬜ | 🔵 | Champ `flow` calculé puis ignoré | `Source/terrain-hydraulic-dynamic.cpp:46` |
| B-T41 | ⬜ | 🔵 | Pente en unités d'indice | `Source/terrain-hydraulic-dynamic.cpp:131` |
| B-T42 | ⬜ | 🔵 | Doublon de HydraulicAmountS à liaison externe | `Source/terrain-hydraulic-dynamic.cpp:207` |
| B-T43 | ⬜ | 🔵 | Calculs morts | `Source/terrain-thermal.cpp:91` |
| B-T44 | ⬜ | 🔵 | Angle de repos codé en dur | `Source/terrain-thermal.cpp:148` |
| B-T45 | ⬜ | 🔵 | Croissance non bornée | `Source/terrain-vegetation.cpp:23` |
| B-T46 | ⬜ | 🔵 | Croissance non bornée | `Source/terrain-grass.cpp:12` |
| B-T47 | ⬜ | 🔵 | Affectation morte | `Source/terrain.cpp:130` |
| B-T48 | ⬜ | 🔵 | Membre statique inutilisé | `Source/terrain.cpp:5` |
| B-T49 | ⬜ | 🔵 | Fonctions libres de test exposées | `Source/terrain.cpp:107` |
| B-T50 | ⬜ | 🔵 | Aire drainée × longueur (à confirmer) | `Source/heightfielderosion.cpp:109` |
| B-T51 | ⬜ | 🔵 | Base des prismes à z = -10 | `Source/hexagonheightfield.cpp:221` |
| B-T52 | ⬜ | 🔵 | Nombre d'échantillons incorrect pour n < 2 | `Source/analyticfield.cpp:94` |

**12 / 52 corrigés** — restants : 2🔴 · 4🟠 · 11🟡 · 23🔵

### B-L — Couches de matériaux

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-L01 | ⬜ | 🔴 | Boucle j avec les bornes de i : écritures hors bornes | `Source/layer-generic.cpp:51` |
| B-L02 | ⬜ | 🔴 | Table de triangulation hors bornes | `Source/layer-generic.cpp:112` |
| B-L03 | ⬜ | 🔴 | `abxy` jamais initialisé (écriture dans `aby`) | `Source/layer-generic.cpp:190` |
| B-L04 | ⬜ | 🔴 | Format d'indices incohérent ; tableau d'indices de normales faux | `Source/layer-generic.cpp:266` |
| B-L05 | ✅ | 🟠 | Sommets d'arêtes jamais calculés | `Source/layer-generic.cpp:150` |
| B-L06 | ⬜ | 🟠 | Surface de l'eau collée au sol | `Source/layer-generic.cpp:140` |
| B-L07 | ✅ | 🟠 | Indices d'arêtes horizontales non échangés | `Source/layer-generic.cpp:255` |
| B-L08 | ⬜ | 🟠 | Cubes de roches posés à l'altitude de leur épaisseur | `Source/layer-generic.cpp:301` |
| B-L09 | ⬜ | 🟠 | Champs propres non mis à jour | `Include/layersfielderosion.h:6` |
| B-L10 | ⬜ | 🟠 | Paramètres de rayon 2D utilisés en 3D | `Source/layer-stack.cpp:199` |
| B-L11 | ⬜ | 🟠 | Sable et roches ignorés | `Source/layer-stack.cpp:213` |
| B-L12 | ⬜ | 🟡 | Hors bornes sur le bord max | `Source/layer-stack.cpp:135` |
| B-L13 | ⬜ | 🟡 | Densités multipliées par s | `Source/layer-stack.cpp:419` |
| B-L14 | ✅ | 🟡 | `new int[nx - 1]` sur terrain vide | `Source/layer-generic.cpp:127` |
| B-L15 | ⬜ | 🔵 | Dernière ligne/colonne exclues | `Source/layer-stack.cpp:236` |
| B-L16 | ⬜ | 🔵 | Niveau du sol incohérent avec la version globale | `Source/layer-stack.cpp:255` |
| B-L17 | ⬜ | 🔵 | Arbres enterrés sous le sable | `Source/layer-stack.cpp:333` |
| B-L18 | ⬜ | 🔵 | Conventions différentes | `Source/layer-stack.cpp:101` |
| B-L19 | ⬜ | 🔵 | Base fixée à -10 | `Source/layer-generic.cpp:293` |
| B-L20 | ⬜ | 🔵 | Retours de save() ignorés, plages codées en dur | `Source/layer-generic.cpp:66` |
| B-L21 | ⬜ | 🔵 | Constructeur non `explicit` | `Include/layersfielderosion.h:19` |
| B-L22 | ⬜ | 🔵 | Unité de compilation vide | `Source/layer-bedrock.cpp:1` |

**3 / 22 corrigés** — restants : 4🔴 · 5🟠 · 2🟡 · 8🔵

### B-R — Rendu (shader de terrain)

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-R01 | ⬜ | 🟡 | Seuil supérieur à toutes les aires : image entièrement opaque | `Source/heightfieldshader.cpp:696` |
| B-R02 | ✅ | 🟡 | Troncature vers zéro et dh ≤ 0 | `Source/heightfieldshader.cpp:454` |
| B-R03 | ⬜ | 🟡 | NaN pour un terrain plat | `Source/heightfieldshader.cpp:311` |
| B-R04 | ⬜ | 🟡 | Division par zéro, Scale touchant le domaine | `Source/heightfieldshader.cpp:243` |
| B-R05 | ⬜ | 🟡 | Chargement du dégradé non vérifié | `Source/heightfieldshader.cpp:625` |
| B-R06 | ✅ | 🔵 | Isoligne majeure manquée sur pente forte | `Source/heightfieldshader.cpp:459` |
| B-R07 | ✅ | 🔵 | Alpha perdu dans la seconde passe | `Source/heightfieldshader.cpp:477` |
| B-R08 | ✅ | 🔵 | Disque tronqué et facteur √2 | `Source/heightfieldshader.cpp:816` |
| B-R09 | ✅ | 🔵 | Deux décalages d'un qui se compensent | `Source/heightfieldshader.cpp:812` |
| B-R10 | ✅ | 🔵 | Scale réduit aussi le domaine | `Source/heightfieldshader.cpp:682` |
| B-R11 | ✅ | 🔵 | Rééchantillonnage carré fixe | `Source/heightfieldshader.cpp:691` |
| B-R12 | ✅ | 🔵 | Boucle d'histogramme non bornée | `Source/heightfieldshader.cpp:701` |
| B-R13 | ✅ | 🔵 | Traces de débogage | `Source/heightfieldshader.cpp:311` |
| B-R14 | ✅ | 🔵 | setPixelColor dans une région OpenMP (à confirmer) | `Source/heightfieldshader.cpp:332` |
| B-R15 | ✅ | 🔵 | Hessienne fausse sur le bord | `Source/heightfieldshader.cpp:200` |
| B-R16 | ✅ | 🔵 | Code mort à l'ordre d'arguments faux | `Source/heightfieldshader.cpp:149` |
| B-R17 | ✅ | 🔵 | Référence pendante sur temporaire | `Include/heightfieldshader.h:8` |

**13 / 17 corrigés** — restants : 0🔴 · 0🟠 · 4🟡 · 0🔵

### B-G — Érosion GPU

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-G01 | ✅ | 🔴 | L'érosion par particules ne modifie jamais le terrain | `Shaders/heightfield_hydraulic_particle.glsl:304` |
| B-G02 | ✅ | 🟠 | Relecture de l'avant-dernier pas | `Source/gputerrainsimu.cpp:256` |
| B-G03 | ⬜ | 🟠 | Dénormalisation par une constante 3000 | `Source/gputerrainsimu.cpp:258` |
| B-G04 | ⬜ | 🟠 | Dureté utilisateur écrasée | `Shaders/heightfield_hydraulic.glsl:310` |
| B-G05 | ✅ | 🟠 | Bedrock/eau d'un pas en retard, sédiment déposé à jour | `Source/gputerrainsimu_test.cpp:330` |
| B-G06 | ✅ | 🟠 | Index 1D calculé avec ny | `Shaders/heightfield_hydraulic_particle.glsl:158` |
| B-G07 | ✅ | 🟠 | Décalages du pinceau calculés avec ny | `Source/gputerrainsimuparticle.cpp:254` |
| B-G08 | ✅ | 🟠 | Dispatch dépendant de nx seul | `Source/gputerrainsimu_test.cpp:498` |
| B-G09 | ✅ | 🟠 | Sédiment relu avec un pas de retard | `Source/gputhermal.cpp:213` |
| B-G10 | ⬜ | 🟠 | Mode local : cellules hors disque non recopiées | `Shaders/heightfield_thermal.glsl:83` |
| B-G11 | ⬜ | 🟠 | Carte source ignorée | `Shaders/deposition.glsl:219` |
| B-G12 | ⬜ | 🟡 | Initialiseurs de globales dépendant d'uniforms (à confirmer selon le driver) | `Shaders/heightfield_hydraulic_test.glsl:45` |
| B-G13 | ⬜ | 🟡 | Bedrock alloué sans données | `Source/gputhermal.cpp:68` |
| B-G14 | ⬜ | 🟡 | Programme GL perdu à chaque ré-Init | `Source/gputerrainsimu.cpp:78` |
| B-G15 | ⬜ | 🟡 | hardnessBuffer jamais libéré | `Source/gputerrainsimu_test.cpp:35` |
| B-G16 | ⬜ | 🟡 | Copies sans contrôle de taille | `Source/gputerrainsimu_test.cpp:330` |
| B-G17 | ⬜ | 🟡 | Division par zéro sur terrain plat | `Source/gputerrainsimu.cpp:67` |
| B-G18 | ⬜ | 🟡 | Valeurs normalisées renvoyées | `Source/gputerrainsimu.cpp:274` |
| B-G19 | ⬜ | 🟡 | Bords périodiques (tore) | `Shaders/heightfield_hydraulic.glsl:260` |
| B-G20 | ⬜ | 🟡 | Nom d'uniform erroné | `Source/gputerrainsimuparticle.cpp:338` |
| B-G21 | ⬜ | 🟡 | Accès hors bornes pour une goutte en bord | `Shaders/heightfield_hydraulic_particle.glsl:171` |
| B-G22 | ⬜ | 🟡 | clamp avec min > max | `Shaders/heightfield_hydraulic_particle.glsl:152` |
| B-G23 | ⬜ | 🟡 | Course de données sur sediments[] (connue) | `Shaders/heightfield_hydraulic_particle.glsl:280` |
| B-G24 | ⬜ | 🟡 | Sédiment détruit dans les cuvettes | `Shaders/heightfield_hydraulic_test.glsl:190` |
| B-G25 | ⬜ | 🟡 | Vitesses érosion/dépôt incohérentes | `Shaders/heightfield_hydraulic_test.glsl:236` |
| B-G26 | ⬜ | 🟡 | pow d'une base négative | `Shaders/heightfield_hydraulic_test.glsl:119` |
| B-G27 | ⬜ | 🟡 | Masse créée | `Shaders/heightfield_thermal_double.glsl:111` |
| B-G28 | ⬜ | 🟡 | Échange non conservatif | `Shaders/heightfield_thermal_simple.glsl:105` |
| B-G29 | ⬜ | 🟡 | Sédiment négatif, unité douteuse | `Shaders/heightfield_thermal.glsl:123` |
| B-G30 | ⬜ | 🟡 | Casse « LibHeightField » dans les chemins | `Source/gputerrainsimu.cpp:70` |
| B-G31 | ⬜ | 🟡 | GetResource dans un initialiseur de membre | `Include/gputerrainsimu.h:100` |
| B-G32 | ⬜ | 🔵 | Tests `isEmpty()` morts avec `exit(-1)` | `Source/gputerrainsimu.cpp:71` |
| B-G33 | ⬜ | 🔵 | Membres nx/ny masqués | `Source/gputhermaldouble.cpp:33` |
| B-G34 | ⬜ | 🔵 | glUniform3f vers des vec2 | `Source/gputhermaldouble.cpp:79` |
| B-G35 | ⬜ | 🔵 | precipitationBuffer initialisé deux fois | `Source/gputerrainsimuparticle.cpp:134` |
| B-G36 | ⬜ | 🔵 | outHardnessBuffer non initialisé | `Source/gputerrainsimuparticle.cpp:126` |
| B-G37 | ⬜ | 🔵 | Facteur 50 appliqué à toutes les fonctions | `Shaders/heightfield_hydraulic_particle.glsl:117` |
| B-G38 | ⬜ | 🔵 | Borne de prélèvement inefficace (à confirmer) | `Shaders/heightfield_hydraulic.glsl:325` |
| B-G39 | ⬜ | 🔵 | No-op | `Source/gputerrainsimu_test.cpp:283` |
| B-G40 | ⬜ | 🔵 | Paramètres sans effet | `Source/gputerrainsimu_test.cpp:261` |
| B-G41 | ⬜ | 🔵 | Dureté augmentant l'érosion (à confirmer) | `Shaders/heightfield_hydraulic_test.glsl:256` |
| B-G42 | ⬜ | 🔵 | Hauteur complète sans couche déposée | `Shaders/heightfield_hydraulic_test.glsl:165` |
| B-G43 | ⬜ | 🔵 | Trace de débogage | `Source/gputerrainsimu_test.cpp:386` |
| B-G44 | ⬜ | 🔵 | Paramètres jamais envoyés | `Include/gputerrainsimu.h:94` |
| B-G45 | ⬜ | 🔵 | Shader mort | `Shaders/heightfield_hydraulic_V0.glsl:1` |
| B-G46 | ⬜ | 🔵 | Shader corrigé mais inutilisé | `Shaders/heightfield_thermal_simple_fix.glsl:1` |
| B-G47 | ⬜ | 🔵 | Code mort dans les shaders et le C++ | `Shaders/heightfield_hydraulic.glsl:79` |
| B-G48 | ⬜ | 🔵 | Copie possible de classes possédant des ressources GL | `Include/gputerrainsimu.h:25` |

**7 / 48 corrigés** — restants : 0🔴 · 4🟠 · 20🟡 · 17🔵

### B-P — Analyse GPU / stream power

| ID | ✅ | Sév | Description courte | Fichier |
|---|:---:|:---:|---|---|
| B-P01 | ✅ | 🔴 | Uniform dvec2 affecté en float : Gradient/Laplacian divisent par 0 | `Source/gpuanalysis.cpp:238` |
| B-P02 | ✅ | 🔴 | Lecture d'un buffer jamais écrit | `Source/gpuanalysis.cpp:299` |
| B-P03 | ⬜ | 🟠 | GetRange réduit des entrées non initialisées si nx < 256 | `Shaders/heightfield_analysis.glsl:672` |
| B-P04 | ⬜ | 🟠 | Compteur atomique jamais remis à zéro | `Source/gpuanalysis.cpp:603` |
| B-P05 | ⬜ | 🟠 | Accumulation sur buffer non initialisé | `Source/gpuanalysis.cpp:828` |
| B-P06 | ✅ | 🟠 | Relecture de l'état du pas précédent | `Source/gpustreampoweroptimized.cpp:453` |
| B-P07 | ⬜ | 🟠 | Cartes m et n ignorées | `Shaders/heightfield_streampower_optimized.glsl:454` |
| B-P08 | ⬜ | 🟠 | Dureté relue = taux d'érosion | `Shaders/heightfield_streampower_optimized.glsl:457` |
| B-P09 | ⬜ | 🟠 | Lectures hors bornes en bord de grille | `Shaders/iterative_drainage.glsl:94` |
| B-P10 | ✅ | 🟠 | Dispatch dépendant de nx seul | `Source/ms_spe_gpu.cpp:210` |
| B-P11 | ⬜ | 🟠 | Handles remis à 0 : fuite à chaque ré-Init | `Source/ms_spe_gpu.cpp:7` |
| B-P12 | ⬜ | 🟠 | Programme GL perdu à chaque appel d'analyse | `Source/gpuanalysis.cpp:53` |
| B-P13 | ⬜ | 🟡 | Dernières cellules ignorées par GetRange | `Shaders/heightfield_analysis.glsl:719` |
| B-P14 | ⬜ | 🟡 | Décodage 16 bits faux | `Shaders/heightfield_analysis.glsl:175` |
| B-P15 | ⬜ | 🟡 | Normale trop inclinée de √2 | `Shaders/heightfield_analysis.glsl:202` |
| B-P16 | ⬜ | 🟡 | Précédence de `!` : garde inopérante | `Shaders/heightfield_streampower_optimized.glsl:155` |
| B-P17 | ⬜ | 🟡 | pow d'une base négative | `Shaders/heightfield_streampower_optimized.glsl:370` |
| B-P18 | ⬜ | 🟡 | Buffers lus sans initialisation | `Source/gpustreampoweroptimized.cpp:83` |
| B-P19 | ⬜ | 🟡 | Bords érodés malgré « do not erode borders » | `Shaders/ms_spe_gpu.glsl:170` |
| B-P20 | ⬜ | 🟡 | Élévations négatives forcées à 0 | `Shaders/map_add.glsl:42` |
| B-P21 | ⬜ | 🟡 | Débordement si le champ est plus petit | `Source/gpu_drainage.cpp:105` |
| B-P22 | ⬜ | 🟡 | Lecture hors bornes / front() sur vecteur vide | `Source/ms_spe_gpu.cpp:247` |
| B-P23 | ⬜ | 🟡 | Taille non vérifiée | `Source/gpustreampoweroptimized.cpp:393` |
| B-P24 | ⬜ | 🟡 | `static int stepId` partagé | `Source/gpustreampoweroptimized.cpp:131` |
| B-P25 | ⬜ | 🟡 | Pas en y faux pour une grille non carrée | `Shaders/heightfield_procedural_erosion.glsl:196` |
| B-P26 | ⬜ | 🟡 | `std::cin.get(); exit(-1);` dans la bibliothèque | `Source/gpuanalysis.cpp:49` |
| B-P27 | ⬜ | 🟡 | Casse « LibHeightField » dans le chemin | `Source/gpustreampoweroptimized.cpp:66` |
| B-P28 | ⬜ | 🟡 | Programme perdu à chaque ré-Init | `Source/gpustreampoweroptimized.cpp:74` |
| B-P29 | ⬜ | 🟡 | Buffers delta jamais libérés | `Source/ms_spe_gpu.cpp:101` |
| B-P30 | ⬜ | 🟡 | Lecture d'un buffer inexistant | `Source/gpuheightfield.cpp:68` |
| B-P31 | ⬜ | 🟡 | Hors bornes pour nx ou ny < 3 | `Shaders/heightfield_analysis.glsl:470` |
| B-P32 | ⬜ | 🔵 | Réduction concurrente non synchronisée | `Shaders/heightfield_analysis.glsl:727` |
| B-P33 | ⬜ | 🔵 | Classe morte | `Include/gpuheightfield.h:1` |
| B-P34 | ⬜ | 🔵 | Test sur d2 au lieu de d3 | `Shaders/heightfield_streampower_optimized.glsl:268` |
| B-P35 | ⬜ | 🔵 | Trou à z == 1500 | `Shaders/heightfield_streampower_optimized.glsl:288` |
| B-P36 | ⬜ | 🔵 | out_hardness non écrit en bord | `Shaders/heightfield_streampower_optimized.glsl:420` |
| B-P37 | ⬜ | 🔵 | Moyenne divisée par 9 en bord | `Shaders/map_processing.glsl:43` |
| B-P38 | ⬜ | 🔵 | Pente sans taille de cellule | `Shaders/iterative_drainage.glsl:27` |
| B-P39 | ⬜ | 🔵 | Seuils magiques, échec traité comme invisible (à confirmer) | `Shaders/heightfield_analysis.glsl:364` |
| B-P40 | ⬜ | 🔵 | Barrière GL_BUFFER_UPDATE_BARRIER_BIT absente (à confirmer) | `Source/gpuanalysis.cpp:296` |
| B-P41 | ⬜ | 🔵 | Copie possible (à confirmer) | `Include/gpu_drainage.h:7` |
| B-P42 | ⬜ | 🔵 | Classe dupliquée dans une application (à confirmer) | `Include/gpu_drainage.h:7` |
| B-P43 | ⬜ | 🔵 | Traces de débogage | `Source/gpu_drainage.cpp:112` |

**4 / 43 corrigés** — restants : 0🔴 · 8🟠 · 19🟡 · 12🔵

### D — Documentation

| ID | ✅ | Domaine | Description courte | Fichier |
|---|:---:|---|---|---|
| D-H01 | ⬜ | Heightfield (cœur) | `flat` décrit comme négatif | `Include/heightfield.h:255` |
| D-H02 | ✅ | Heightfield (cœur) | SteepestSlope : brief copié de Steepest | `Include/heightfield.h:74` |
| D-H03 | ⬜ | Heightfield (cœur) | Steepest/SteepestSlope : précondition absente | `Include/heightfield.h:66` |
| D-H04 | ⬜ | Heightfield (cœur) | Commentaire non Doxygen sur `num` | `Include/heightfield.h:24` |
| D-H05 | ⬜ | Heightfield (cœur) | Classes sans bloc `\class` | `Include/heightfield.h:14` |
| D-H06 | ⬜ | Heightfield (cœur) | TerrainDX::isRiver non documenté | `Include/heightfield.h:347` |
| D-H07 | ⬜ | Heightfield (cœur) | TerrainDX : membres mal décrits | `Include/heightfield.h:326` |
| D-H08 | ⬜ | Heightfield (cœur) | LaplacianSmoothing et SmoothClamp sans doc | `Source/heightfield.cpp:1758` |
| D-H09 | ⬜ | Heightfield (cœur) | LaplacianSmoothing : commentaire faux | `Source/heightfield.cpp:1797` |
| D-H10 | ⬜ | Heightfield (cœur) | WetnessIndex : \param c faux | `Source/heightfield.cpp:720` |
| D-H11 | ⬜ | Heightfield (cœur) | StreamLength : \param steepest absent | `Source/heightfield.cpp:301` |
| D-H12 | ⬜ | Heightfield (cœur) | AverageFlowDirection : brief incomplet, liaison externe | `Source/heightfield.cpp:408` |
| D-H13 | ⬜ | Heightfield (cœur) | Slope(bool) : `\sa ()` vide ; masquage de `Box2::a/b` | `Source/heightfield.cpp:542` |
| D-H14 | ✅ | Heightfield (cœur) | Signed : doc imprécise | `Source/heightfield.cpp:812` |
| D-H15 | ⬜ | Heightfield (cœur) | Draw : \param brush ignoré | `Source/heightfield.cpp:1044` |
| D-H16 | ⬜ | Heightfield (cœur) | Sea : \param height absent | `Source/heightfield.cpp:1191` |
| D-H17 | ⬜ | Heightfield (cœur) | Shade : doc approximative | `Source/heightfield.cpp:1003` |
| D-H18 | ✅ | Heightfield (cœur) | Fautes de frappe | `Source/heightfield.cpp:1712` |
| D-H19 | ⬜ | Heightfield (cœur) | FastProceduralErosion : style `@param` sans `\brief` | `Source/heightfield.cpp:1648` |
| D-H20 | ⬜ | Heightfield (cœur) | TerrainDX : docs copiées-collées | `Source/heightfield-access.cpp:272` |
| D-H21 | ⬜ | Heightfield (cœur) | CompleteBreach : paramètre non documenté, code mort | `Source/heightfield-carve.cpp:45` |
| D-H22 | ⬜ | Heightfield (cœur) | HeightFieldNext8 : doc fausse | `Source/heightfield-next.cpp:12` |
| D-H23 | ⬜ | Heightfield (cœur) | Carve(SmoothDisc2) : « Flatten » | `Source/heightfield-sculpt.cpp:5` |
| D-H24 | ⬜ | Heightfield (cœur) | Référence liée à un temporaire | `Source/heightfield-lighting.cpp:45` |
| D-H25 | ⬜ | Heightfield (cœur) | Subdivide : doc imprécise | `Source/heightfield.cpp:84` |
| D-H26 | ⬜ | Heightfield (cœur) | CreateCubes : conventions non documentées | `Source/heightfield-mesh.cpp:186` |
| D-H27 | ⬜ | Heightfield (cœur) | Faute et nom de fichier trompeur | `Source/heightdune.cpp:33` |
| D-H28 | ⬜ | Heightfield (cœur) | Commentaire faux, pas de garde d'inclusion | `Include/heightfield-qt.h:1` |
| D-H29 | ⬜ | Heightfield (cœur) | Dépendance implicite au masque de VertexIntegerArea | `Source/heightfield-sculpt.cpp:14` |
| D-A01 | ⬜ | Analyse de terrain CPU | hollow/spur convexe/concave inversés | `Source/heightfield-geomorphons.cpp:7` |
| D-A02 | ⬜ | Analyse de terrain CPU | GeomorphonsTangent : paramètres incomplets | `Source/heightfield-geomorphons.cpp:71` |
| D-A03 | ⬜ | Analyse de terrain CPU | « Slope threshold » appliqué sans √2 | `Source/heightfield-geomorphons.cpp:9` |
| D-A04 | ⬜ | Analyse de terrain CPU | LocalVariance renvoie un écart-type | `Source/heightfield-metrics.cpp:471` |
| D-A05 | ⬜ | Analyse de terrain CPU | TRI : « RMS » faux | `Source/heightfield-metrics.cpp:603` |
| D-A06 | ⬜ | Analyse de terrain CPU | Peakedness : « lower or equal » vs `<` | `Source/heightfield-metrics.cpp:689` |
| D-A07 | ⬜ | Analyse de terrain CPU | \param w imprécis, \return absent | `Source/heightfield-metrics.cpp:208` |
| D-A08 | ⬜ | Analyse de terrain CPU | FractLaplacian : doc incomplète | `Source/heightfield-metrics.cpp:937` |
| D-A09 | ⬜ | Analyse de terrain CPU | Code commenté et include inutile | `Source/heightfield-metrics.cpp:334` |
| D-A10 | ⬜ | Analyse de terrain CPU | FillDepressions : \return faux | `Source/heightfield-fill.cpp:130` |
| D-A11 | ⬜ | Analyse de terrain CPU | InitBorders et helpers en `//` | `Source/heightfield-fill.cpp:43` |
| D-A12 | ⬜ | Analyse de terrain CPU | ThermalStep : unité et emplacement | `Source/heightfield-fill.cpp:66` |
| D-A13 | ⬜ | Analyse de terrain CPU | CheckFlowSlope/CheckFlowDirectionsAngle : \return absent | `Source/heightfield-flow.cpp:34` |
| D-T01 | ⬜ | Simulation de terrain CPU | Fonctions sans doc ou \brief vide | `Source/heightfieldsand.cpp:226` |
| D-T02 | ⬜ | Simulation de terrain CPU | Evaporate : \param absents | `Source/heightfieldsand.cpp:549` |
| D-T03 | ⬜ | Simulation de terrain CPU | StabilizeStep : \param absents, commentaire faux | `Source/heightfieldsand.cpp:377` |
| D-T04 | ✅ | Simulation de terrain CPU | Caractères mal encodés | `Source/heightfieldsand.cpp:76` |
| D-T05 | ✅ | Simulation de terrain CPU | Fautes de frappe | `Source/heightfieldsand.cpp:130` |
| D-T06 | ⬜ | Simulation de terrain CPU | C_T_S : commentaire non Doxygen | `Include/heightfieldsediment.h:73` |
| D-T07 | ⬜ | Simulation de terrain CPU | Fonctions sans doc | `Source/terrain-droplet.cpp:5` |
| D-T08 | ✅ | Simulation de terrain CPU | Exemple \code non compilable | `Source/terrain-droplet.cpp:18` |
| D-T09 | ⬜ | Simulation de terrain CPU | Droplet : doc incomplète | `Source/terrain-droplet.cpp:32` |
| D-T10 | ⬜ | Simulation de terrain CPU | HydraulicTransport : doc contredit le code | `Source/terrain-hydraulic.cpp:389` |
| D-T11 | ⬜ | Simulation de terrain CPU | FlowStep : doc vague | `Source/terrain-hydraulic.cpp:62` |
| D-T12 | ✅ | Simulation de terrain CPU | HydraulicEvaporate : brief minimal | `Source/terrain-hydraulic.cpp:12` |
| D-T13 | ✅ | Simulation de terrain CPU | \brief vides | `Source/terrain-hydraulic-dynamic.cpp:3` |
| D-T14 | ⬜ | Simulation de terrain CPU | LightningErosion : doc copiée | `Source/terrain-lightning.cpp:4` |
| D-T15 | ✅ | Simulation de terrain CPU | Lightning : « shielded by bedrock » | `Source/terrain-lightning.cpp:32` |
| D-T16 | ✅ | Simulation de terrain CPU | \brief vides | `Source/terrain-lightning.cpp:52` |
| D-T17 | ⬜ | Simulation de terrain CPU | ThermalImpactSlope : doc fausse | `Source/terrain-thermal.cpp:21` |
| D-T18 | ⬜ | Simulation de terrain CPU | ThermalDropletErosion : nom trompeur | `Source/terrain-thermal.cpp:216` |
| D-T19 | ⬜ | Simulation de terrain CPU | Membre `flowouside` mal nommé et mal décrit | `Include/layersfielderosion.h:14` |
| D-T20 | ⬜ | Simulation de terrain CPU | VegetationGrowingStep : « eight neighbours » | `Source/terrain-vegetation.cpp:15` |
| D-T21 | ⬜ | Simulation de terrain CPU | Commentaires de seuils obsolètes | `Source/terrain-vegetation.cpp:97` |
| D-T22 | ✅ | Simulation de terrain CPU | \brief vides | `Source/terrain-grass.cpp:5` |
| D-T23 | ✅ | Simulation de terrain CPU | Texture : \brief vide | `Source/terrain-image.cpp:6` |
| D-T24 | ✅ | Simulation de terrain CPU | GetStats : \brief vide | `Source/terrain.cpp:75` |
| D-T25 | ⬜ | Simulation de terrain CPU | CARVE, TRYY sans doc | `Source/terrain.cpp:107` |
| D-T26 | ⬜ | Simulation de terrain CPU | HillSlope : signe faux dans la formule | `Source/heightfielderosion.cpp:22` |
| D-T27 | ⬜ | Simulation de terrain CPU | DebrisSlope : ‖∇h‖ annoncé | `Source/heightfielderosion.cpp:37` |
| D-T28 | ⬜ | Simulation de terrain CPU | StreamPowerErosion : SetBorder(0) non documenté | `Source/heightfielderosion.cpp:77` |
| D-T29 | ⬜ | Simulation de terrain CPU | \brief « Erosion. » | `Source/heightfielderosion.cpp:5` |
| D-T30 | ⬜ | Simulation de terrain CPU | Normal : doc ne correspond pas aux triangles | `Source/hexagonheightfield.cpp:47` |
| D-T31 | ⬜ | Simulation de terrain CPU | Section `protected:` vide | `Include/hexagonheightfield.h:9` |
| D-T32 | ⬜ | Simulation de terrain CPU | `//!` au lieu de `//!<` | `Include/analyticfield.h:52` |
| D-T33 | ⬜ | Simulation de terrain CPU | Membres sans doc | `Include/analyticfield.h:74` |
| D-T34 | ⬜ | Simulation de terrain CPU | Défaut `Vector::Null` pour un Vector2 | `Include/analyticfield.h:105` |
| D-T35 | ✅ | Simulation de terrain CPU | ~~Classes sans \class dans l'en-tête~~ (faux positif) | `Include/analyticfield.h:5` |
| D-T36 | ⬜ | Simulation de terrain CPU | Deux epsilons coexistent | `Source/analyticfield.cpp:8` |
| D-T37 | ⬜ | Simulation de terrain CPU | Ordre de la liste d'initialisation | `Source/analyticmusgrave.cpp:21` |
| D-T38 | ⬜ | Simulation de terrain CPU | \param o « Number of signals » | `Source/analyticmusgrave.cpp:16` |
| D-T39 | ✅ | Simulation de terrain CPU | BedrockLevel : doc juste, code faux | `Source/heightfieldsand.cpp:116` |
| D-L01 | ⬜ | Couches de matériaux | GetColorVector : doc copiée | `Source/layer-geometry.cpp:86` |
| D-L02 | ⬜ | Couches de matériaux | Vertex(const Vector2&) : doc copiée de Height | `Source/layer-stack.cpp:113` |
| D-L03 | ⬜ | Couches de matériaux | GetTrees : doc vide | `Source/layer-stack.cpp:320` |
| D-L04 | ✅ | Couches de matériaux | CreateCubes : \param mal ordonnés | `Source/layer-generic.cpp:270` |
| D-L05 | ✅ | Couches de matériaux | UpdateHeightField : \param incomplets | `Source/layer-generic.cpp:36` |
| D-L06 | ⬜ | Couches de matériaux | CreateWaterGeometry présentée comme fonctionnelle | `Source/layer-generic.cpp:105` |
| D-L07 | ✅ | Couches de matériaux | « elevation » pour des épaisseurs | `Include/layers.h:105` |
| D-L08 | ✅ | Couches de matériaux | WaterFlood(const double&) : grass non mentionné | `Source/layer-stack.cpp:345` |
| D-L09 | ⬜ | Couches de matériaux | Variables locales a, b masquant Box2::a/b | `Source/layer-stack.cpp:167` |
| D-L10 | ✅ | Couches de matériaux | Sémantique des couches non documentée | `Include/layers.h:5` |
| D-L11 | ✅ | Couches de matériaux | bedrockflow/sandflow même commentaire | `Include/layersfielderosion.h:9` |
| D-L12 | ⬜ | Couches de matériaux | TRYY dans l'API publique | `Include/layersfielderosion.h:85` |
| D-L13 | ✅ | Couches de matériaux | Accesseurs sans doc | `Include/layersfielderosion.h:62` |
| D-R01 | ⬜ | Rendu (shader de terrain) | BrownShading sans doc | `Source/heightfieldshader.cpp:279` |
| D-R02 | ✅ | Rendu (shader de terrain) | Fonctions libres sans doc et exportées | `Source/heightfieldshader.cpp:141` |
| D-R03 | ⬜ | Rendu (shader de terrain) | Relief : \param enhanced absent | `Source/heightfieldshader.cpp:111` |
| D-R04 | ✅ | Rendu (shader de terrain) | ImageShadeRadianceScaling : \param absents | `Source/heightfieldshader.cpp:233` |
| D-R05 | ✅ | Rendu (shader de terrain) | RadianceScaling : doc mal formée | `Source/heightfieldshader.cpp:173` |
| D-R06 | ✅ | Rendu (shader de terrain) | Eric : nom et doc copiés | `Source/heightfieldshader.cpp:620` |
| D-R07 | ✅ | Rendu (shader de terrain) | ShadedRelief : doc identique à Relief | `Source/heightfieldshader.cpp:558` |
| D-R08 | ⬜ | Rendu (shader de terrain) | LaplacianShading : taille de sortie non documentée | `Source/heightfieldshader.cpp:214` |
| D-R09 | ⬜ | Rendu (shader de terrain) | CartographicGreen : commentaire « 1024 » faux | `Source/heightfieldshader.cpp:585` |
| D-R10 | ⬜ | Rendu (shader de terrain) | Variable locale masquant le membre heightfield | `Source/heightfieldshader.cpp:596` |
| D-R11 | ✅ | Rendu (shader de terrain) | RiverWidth : fautes et unités | `Source/heightfieldshader.cpp:776` |
| D-R12 | ✅ | Rendu (shader de terrain) | CartographicGreenYellow : GPU non mentionné | `Source/heightfieldshader.cpp:772` |
| D-R13 | ⬜ | Rendu (shader de terrain) | Code mort commenté | `Source/heightfieldshader.cpp:398` |
| D-R14 | ✅ | Rendu (shader de terrain) | Includes redondants en milieu de fichier | `Source/heightfieldshader.cpp:285` |
| D-R15 | ✅ | Rendu (shader de terrain) | Durée de vie et méthodes GPU non documentées | `Include/heightfieldshader.h:5` |
| D-G01 | ⬜ | Érosion GPU | `\class GPUHydraulicErosion` inexistante | `Source/gputerrainsimu.cpp:7` |
| D-G02 | ⬜ | Érosion GPU | GetSpeeds « no transfer back » | `Source/gputerrainsimu.cpp:235` |
| D-G03 | ⬜ | Érosion GPU | SetUniforms sans \param | `Source/gputerrainsimu.cpp:202` |
| D-G04 | ⬜ | Érosion GPU | Mode « 3 = Faulting » inexistant | `Source/gputerrainsimu.cpp:312` |
| D-G05 | ⬜ | Érosion GPU | GetData mal documenté | `Source/gputhermal.cpp:205` |
| D-G06 | ⬜ | Érosion GPU | noiseWavelength par défaut faux | `Source/gputhermal.cpp:192` |
| D-G07 | ⬜ | Érosion GPU | Plage d'angle contredite | `Shaders/heightfield_thermal_double.glsl:93` |
| D-G08 | ⬜ | Érosion GPU | Commentaire « doesn't work anymore » | `Source/gputerrainsimuparticle.cpp:16` |
| D-G09 | ⬜ | Érosion GPU | Fonctions sans doc | `Source/gputhermaldouble.cpp:7` |
| D-G10 | ⬜ | Érosion GPU | Classes et membres sans doc | `Include/gputerrainsimu.h:9` |
| D-G11 | ⬜ | Érosion GPU | Ping-pong de GPUMapProcessing::Step non documenté | `Source/gputerrainsimu_test.cpp:530` |
| D-G12 | ⬜ | Érosion GPU | « bedrock set to 0 » non garanti | `Source/gputhermal.cpp:59` |
| D-P01 | ⬜ | Analyse GPU / stream power | InitForTexture : précision surestimée | `Source/gpuanalysis.cpp:80` |
| D-P02 | ⬜ | Analyse GPU / stream power | \param s mal décrits | `Source/gpuanalysis.cpp:471` |
| D-P03 | ✅ | Analyse GPU / stream power | Laplacian « double precision » | `Source/gpuanalysis.cpp:276` |
| D-P04 | ⬜ | Analyse GPU / stream power | Commentaires faux | `Shaders/heightfield_analysis.glsl:283` |
| D-P05 | ⬜ | Analyse GPU / stream power | maxSteps = 2256 suspect (à confirmer) | `Include/gputerrainsimu.h:379` |
| D-P06 | ✅ | Analyse GPU / stream power | `\class GPUStreamPowerImproved` | `Source/gpustreampoweroptimized.cpp:9` |
| D-P07 | ✅ | Analyse GPU / stream power | Erreurs de \param | `Source/gpustreampoweroptimized.cpp:279` |
| D-P08 | ⬜ | Analyse GPU / stream power | Init : doc fausse | `Source/gpustreampoweroptimized.cpp:49` |
| D-P09 | ⬜ | Analyse GPU / stream power | Retargeting et threshold inopérants | `Shaders/heightfield_streampower_optimized.glsl:172` |
| D-P10 | ⬜ | Analyse GPU / stream power | Aire drainée élémentaire = longueur (à confirmer) | `Shaders/heightfield_streampower_optimized.glsl:422` |
| D-P11 | ⬜ | Analyse GPU / stream power | Commentaire et uniform obsolètes | `Shaders/map_processing.glsl:42` |
| D-P12 | ⬜ | Analyse GPU / stream power | GetHardness morte | `Shaders/map_add.glsl:17` |
| D-P13 | ⬜ | Analyse GPU / stream power | Bords et max_spe non documentés | `Shaders/ms_spe_gpu.glsl:167` |
| D-P14 | ⬜ | Analyse GPU / stream power | MS_SPE_GPU / GPUMapAdd sans doc | `Source/ms_spe_gpu.cpp:5` |
| D-P15 | ⬜ | Analyse GPU / stream power | GPU_Drainage sans doc | `Source/gpu_drainage.cpp:4` |
| D-P16 | ⬜ | Analyse GPU / stream power | GPUHeightField : doc incomplète | `Include/gpuheightfield.h:43` |
| D-P17 | ⬜ | Analyse GPU / stream power | Dimensions et division par zéro non documentées | `Shaders/heightfield_procedural_erosion.glsl:17` |

**34 / 138 corrigés**

### O — Optimisations

| ID | ✅ | Domaine | Description courte | Fichier |
|---|:---:|---|---|---|
| O-H01 | ⬜ | Heightfield (cœur) | AverageFlowDirection recalculée n fois | `Source/heightfield.cpp:457` |
| O-H02 | ⬜ | Heightfield (cœur) | StreamAreaLimited : CheckFlowSlope recalculé à chaque itération | `Source/heightfield.cpp:506` |
| O-H03 | ⬜ | Heightfield (cœur) | Shade : GetBox() et K() recalculés à chaque point | `Source/heightfield.cpp:1016` |
| O-H04 | ⬜ | Heightfield (cœur) | TerrainDX : accumulation en O(N·L) | `Source/heightfield-access.cpp:209` |
| O-H05 | ⬜ | Heightfield (cœur) | StabilizeSlope : distances recalculées | `Source/heightfield.cpp:1583` |
| O-H06 | ⬜ | Heightfield (cœur) | FastProceduralErosion : buffer `dataSlope` inutilisé | `Source/heightfield.cpp:1667` |
| O-H07 | ⬜ | Heightfield (cœur) | CreateMeshSide : reserve double | `Source/heightfield-mesh.cpp:21` |
| O-A01 | ⬜ | Analyse de terrain CPU | ReliefSteepness/Jut/Rut : distances recalculées | `Source/heightfield-metrics.cpp:774` |
| O-A02 | ⬜ | Analyse de terrain CPU | FractLaplacian : pow par voisin | `Source/heightfield-metrics.cpp:956` |
| O-A03 | ⬜ | Analyse de terrain CPU | AreaRatio en force brute | `Source/heightfield-metrics.cpp:248` |
| O-A04 | ⬜ | Analyse de terrain CPU | TRI quadratique en force brute | `Source/heightfield-metrics.cpp:630` |
| O-A05 | ✅ | Analyse de terrain CPU | FillDepressions : indexation x*size_y + y | `Source/heightfield-fill.cpp:212` |
| O-T01 | ⬜ | Simulation de terrain CPU | Intersect : pas fixe de 1.0 | `Source/heightfieldsand.cpp:209` |
| O-T02 | ⬜ | Simulation de terrain CPU | WorldmachineSeveralStep duplique WorldmachineStep | `Source/terrain-hydraulic.cpp:214` |
| O-T03 | ✅ | Simulation de terrain CPU | AnalyticPalette construite par pixel | `Source/terrain-image.cpp:87` |
| O-T04 | ⬜ | Simulation de terrain CPU | CARVE en O(N·(r/h)²) | `Source/terrain.cpp:125` |
| O-T05 | ⬜ | Simulation de terrain CPU | AverageSlope : directions comptées deux fois | `Source/analyticfield.cpp:72` |
| O-T06 | ⬜ | Simulation de terrain CPU | pow(frequency, -H) par octave | `Source/analyticmusgrave.cpp:53` |
| O-L01 | ⬜ | Couches de matériaux | QSet<int> pour des indices denses | `Source/layer-stack.cpp:241` |
| O-L02 | ⬜ | Couches de matériaux | Intersect dupliqué de HeightFieldSediment::Intersect | `Source/layer-stack.cpp:192` |
| O-L03 | ⬜ | Couches de matériaux | CreateCubes sans reserve | `Source/layer-generic.cpp:285` |
| O-L04 | ⬜ | Couches de matériaux | Accesseurs renvoyant des champs par valeur | `Include/layersfielderosion.h:62` |
| O-R01 | ✅ | Rendu (shader de terrain) | Slope() calculé et inutilisé | `Source/heightfieldshader.cpp:615` |
| O-R02 | ✅ | Rendu (shader de terrain) | GetRange inutilisé | `Source/heightfieldshader.cpp:463` |
| O-R03 | ✅ | Rendu (shader de terrain) | Deux GPUHeightFieldAnalysis temporaires | `Source/heightfieldshader.cpp:459` |
| O-R04 | ✅ | Rendu (shader de terrain) | Aller-retour QColor → Color → QColor | `Source/heightfieldshader.cpp:772` |
| O-R05 | ✅ | Rendu (shader de terrain) | Boucles i externe sur QImage | `Source/heightfieldshader.cpp:67` |
| O-G01 | ⬜ | Érosion GPU | Uniforms constants repositionnés à chaque pas | `Source/gputerrainsimu.cpp:146` |
| O-G02 | ⬜ | Érosion GPU | 64 gouttes par dispatch | `Source/gputerrainsimuparticle.cpp:175` |
| O-G03 | ⬜ | Érosion GPU | ~256 lectures SSBO par cellule | `Shaders/heightfield_hydraulic_test.glsl:108` |
| O-G04 | ⬜ | Érosion GPU | Poids d'écoulement calculés deux fois | `Shaders/deposition.glsl:124` |
| O-P01 | ⬜ | Analyse GPU / stream power | Recompilation du shader à chaque appel | `Source/gpuanalysis.cpp:45` |
| O-P02 | ⬜ | Analyse GPU / stream power | Dispatch carré max(nx, ny) | `Source/gpuanalysis.cpp:43` |
| O-P03 | ⬜ | Analyse GPU / stream power | VisibilityIndex en N² dispatches | `Source/gpuanalysis.cpp:828` |
| O-P04 | ⬜ | Analyse GPU / stream power | GetRange redondant et séquentiel | `Shaders/heightfield_analysis.glsl:713` |
| O-P05 | ⬜ | Analyse GPU / stream power | Uniforms invariants ré-envoyés | `Source/gpustreampoweroptimized.cpp:150` |
| O-P06 | ⬜ | Analyse GPU / stream power | Écriture inutile de out_terrain | `Shaders/ms_spe_gpu.glsl:169` |
| O-P07 | ⬜ | Analyse GPU / stream power | Buffers liés mais inutilisés | `Source/ms_spe_gpu.cpp:24` |
| O-P08 | ⬜ | Analyse GPU / stream power | GetBox() inutilisé | `Source/gpu_drainage.cpp:68` |
| O-P09 | ⬜ | Analyse GPU / stream power | texture() pour un accès entier | `Shaders/heightfield_analysis.glsl:179` |
| O-P10 | ⬜ | Analyse GPU / stream power | Indices GL_STREAM_READ inadaptés | `Source/gpustreampoweroptimized.cpp:77` |

**7 / 41 corrigés**

---

## Priorités de correction recommandées

### Urgences absolues (crash / UB / boucle infinie / résultat entièrement faux) — 4 / 14 corrigés
1. **B-H01** — Carving inversé (poids appliqué au terrain au lieu de la cible) (`Source/heightfield.cpp:1305`).
2. **B-H02** — Sélection de la k-ième direction fausse (FlowTo(p) entièrement faux) (`Source/heightfield-next.cpp:104`).
3. **B-A01** — Classification inversée (sommet → cuvette, crête → vallée) (`Source/heightfield-geomorphons.cpp:60`).
4. **B-A02** — Classification agrégée inversée (`Source/heightfield-geomorphons.cpp:190`).
5. **B-T01** — Boucle infinie : eau réinitialisée à chaque itération (`Source/heightfieldsand.cpp:524`).
6. **B-T02** — Boucle infinie dès qu'il y a de l'eau (`Source/terrain-hydraulic.cpp:339`).
7. ✅ **B-T03** — `slope` non initialisé et téléportation en (0,0) (`Source/terrain-droplet.cpp:60`).
8. **B-L01** — Boucle j avec les bornes de i : écritures hors bornes (`Source/layer-generic.cpp:51`).
9. **B-L02** — Table de triangulation hors bornes (`Source/layer-generic.cpp:112`).
10. **B-L03** — `abxy` jamais initialisé (écriture dans `aby`) (`Source/layer-generic.cpp:190`).
11. **B-L04** — Format d'indices incohérent ; tableau d'indices de normales faux (`Source/layer-generic.cpp:266`).
12. ✅ **B-G01** — L'érosion par particules ne modifie jamais le terrain (`Shaders/heightfield_hydraulic_particle.glsl:304`).
13. ✅ **B-P01** — Uniform dvec2 affecté en float : Gradient/Laplacian divisent par 0 (`Source/gpuanalysis.cpp:238`).
14. ✅ **B-P02** — Lecture d'un buffer jamais écrit (`Source/gpuanalysis.cpp:299`).

### Corrections à fort impact (résultats faux courants) — 21 / 43 corrigés
15. **B-H03** — Direction d'écoulement écrite dans les cellules aval.
16. ✅ **B-H04** — Dimensions formatées avec 1 chiffre significatif.
17. ✅ **B-H05** — Fuite d'un programme GL à chaque appel, échec de compilation ignoré.
18. ✅ **B-H06** — Dispatch carré sans garde côté shader : bords écrasés de façon concurrente.
19. ✅ **B-A03** — Inverse de la longueur diagonale fausse (×2).
20. ✅ **B-T04** — Flatten appelé avec une altitude comme facteur d'échelle.
21. ✅ **B-T05** — Couche de sable ignorée.
22. **B-T06** — Paramètres de rayon 2D utilisés en 3D.
23. ✅ **B-T07** — Matériau en suspension dupliqué.
24. ✅ **B-T08** — Matériau en suspension dupliqué.
25. ✅ **B-T09** — Pas courant perdu quand la goutte s'évapore.
26. **B-T10** — Transfert d'eau excessif : oscillation sans fin.
27. **B-T11** — Ligne dupliquée : masse de roches doublée.
28. ✅ **B-T12** — Bouclier inversé.
29. **B-T13** — Végétation et herbe toujours nulles.
30. ✅ **B-T14** — Somme renvoyée au lieu de la moyenne.
31. ✅ **B-L05** — Sommets d'arêtes jamais calculés.
32. **B-L06** — Surface de l'eau collée au sol.
33. ✅ **B-L07** — Indices d'arêtes horizontales non échangés.
34. **B-L08** — Cubes de roches posés à l'altitude de leur épaisseur.
35. **B-L09** — Champs propres non mis à jour.
36. **B-L10** — Paramètres de rayon 2D utilisés en 3D.
37. **B-L11** — Sable et roches ignorés.
38. ✅ **B-G02** — Relecture de l'avant-dernier pas.
39. **B-G03** — Dénormalisation par une constante 3000.
40. **B-G04** — Dureté utilisateur écrasée.
41. ✅ **B-G05** — Bedrock/eau d'un pas en retard, sédiment déposé à jour.
42. ✅ **B-G06** — Index 1D calculé avec ny.
43. ✅ **B-G07** — Décalages du pinceau calculés avec ny.
44. ✅ **B-G08** — Dispatch dépendant de nx seul.
45. ✅ **B-G09** — Sédiment relu avec un pas de retard.
46. **B-G10** — Mode local : cellules hors disque non recopiées.
47. **B-G11** — Carte source ignorée.
48. **B-P03** — GetRange réduit des entrées non initialisées si nx < 256.
49. **B-P04** — Compteur atomique jamais remis à zéro.
50. **B-P05** — Accumulation sur buffer non initialisé.
51. ✅ **B-P06** — Relecture de l'état du pas précédent.
52. **B-P07** — Cartes m et n ignorées.
53. **B-P08** — Dureté relue = taux d'érosion.
54. **B-P09** — Lectures hors bornes en bord de grille.
55. ✅ **B-P10** — Dispatch dépendant de nx seul.
56. **B-P11** — Handles remis à 0 : fuite à chaque ré-Init.
57. **B-P12** — Programme GL perdu à chaque appel d'analyse (partiellement corrigé : `InitForTexture` seulement).

### À planifier (robustesse, cas limites)
- **Conservation de masse et terminaison des simulations** (2 / 31 corrigés) : B-H10, B-H12, B-H18, ~~B-A04~~, B-A12, B-T15, B-T17, B-T19, B-T20, B-T22, B-T23, B-T26, B-T31, B-T45, B-T46, B-L12, ~~B-R07~~, B-G21, B-G24, B-G25, B-G27, B-G28, B-G29, B-G30, B-G38, B-G48, B-P22, B-P27, B-P31, B-P33, B-P42.
- **Ressources GL, buffers non initialisés et gestion d'erreur GPU** (0 / 22 corrigés) : B-H27, B-A17, B-G12, B-G14, B-G15, B-G16, B-G20, B-G31, B-G32, B-G35, B-G36, B-G39, B-G47, B-P16, B-P18, B-P26, B-P28, B-P29, B-P30, B-P34, B-P40, B-P41.
- **Entrées dégénérées et tailles non vérifiées** (nx/ny petits, terrain plat, champs de tailles différentes) (4 / 16 corrigés) : B-H09, B-H15, B-H22, ~~B-A05~~, ~~B-A06~~, B-T16, B-T35, B-T39, ~~B-L14~~, B-L22, B-R03, B-R04, ~~B-R15~~, B-G17, B-P23, B-P38.
- **Bords de grille et grilles non carrées** (2 / 22 corrigés) : B-H07, B-H11, B-H20, B-H21, B-H23, B-A08, B-A09, B-A10, B-A13, B-A15, B-A19, B-A20, B-L15, ~~B-R08~~, ~~B-R11~~, B-G19, B-P13, B-P19, B-P21, B-P25, B-P36, B-P37.
- **Code mort, traces de débogage, hygiène** (4 / 18 corrigés) : B-H19, B-A11, ~~B-T24~~, ~~B-T27~~, B-T33, B-T34, B-T37, B-T42, B-T43, B-T47, B-T48, ~~B-R13~~, ~~B-R16~~, B-G40, B-G43, B-G45, B-G46, B-P43.
- **Précision numérique et cohérence des conventions** (9 / 64 corrigés) : B-H08, B-H13, B-H14, B-H16, B-H17, B-H24, B-H25, B-H26, B-H28, B-A07, B-A14, B-A16, B-A18, B-T18, B-T21, B-T25, ~~B-T28~~, ~~B-T29~~, B-T30, B-T32, B-T36, B-T38, B-T40, B-T41, B-T44, B-T49, B-T50, B-T51, B-T52, B-L13, B-L16, B-L17, B-L18, B-L19, B-L20, B-L21, B-R01, ~~B-R02~~, B-R05, ~~B-R06~~, ~~B-R09~~, ~~B-R10~~, ~~B-R12~~, ~~B-R14~~, ~~B-R17~~, B-G13, B-G18, B-G22, B-G23, B-G26, B-G33, B-G34, B-G37, B-G41, B-G42, B-G44, B-P14, B-P15, B-P17, B-P20, B-P24, B-P32, B-P35, B-P39.
