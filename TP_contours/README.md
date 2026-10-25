# TP_contours - Homework 3

Programme C++17/OpenCV comparant Sobel, Laplace et Canny sur 40 images de Groningen, avec interface à curseurs, traitement par lot et mesures quantitatives.

## Fichiers du rendu

- `src/` : détecteurs, calcul des mesures, appariement, interface et mode batch.
- `tests/` : vérification des détecteurs, des cas limites et de l'appariement.
- `data/images/` : 40 images et 40 références originales, sans modification.
- `data/provenance.json` : source, citation et empreintes SHA-256.
- `data/tuning.txt` et `data/test.txt` : répartition 10 images de réglage / 30 de test.
- `results/all_metrics.csv` : 240 lignes = 40 images × 3 détecteurs × 2 tolérances.
- `results/{sobel,laplace,canny}/` : cartes binaires, superpositions et CSV.
- `results/search/`, `search.json`, `selected_parameters.json`, `summary.json` : recherche et résultats synthétiques.
- `report/rapport.pdf` : rapport de cinq pages.
- `report/latex/` et `report/rapport-latex.zip` : sources LaTeX, images et logos.
- `presentation/lancer_demo.command` et `executer_lot.command` : raccourcis de démonstration sur Mac.
- `video/README.md` : modalités de transmission de la vidéo, fournie séparément par lien dans le courriel.

## Compiler et tester

Prérequis : compilateur C++17, CMake, OpenCV (core, imgproc, imgcodecs, highgui).

```bash
# Mac
brew install cmake opencv
# Debian / Ubuntu
sudo apt install build-essential cmake libopencv-dev
```

Depuis `TP_contours` :

```bash
bash build.sh
```

Cette commande compile et exécute les tests CTest. Si OpenCV n'est pas trouvé :

```bash
bash build.sh -DOpenCV_DIR=/chemin/vers/lib/cmake/opencv4
```

La compilation et l'exécution sont vérifiées sur macOS arm64. La procédure Linux est fournie, mais n'a pas été exécutée pour ce rendu.

## Interface avec curseurs

```bash
./build/contours --gui --detector canny --sigma 2 --low 40 --high 120
```

Les curseurs choisissent le détecteur (0=Sobel, 1=Laplace, 2=Canny), l'image, sigma, le noyau de dérivation (3/5/7), les seuils et la tolérance. Le seuil Sobel/Laplace est ignoré par Canny et inversement. Les quatre vues montrent l'original, la référence, les contours détectés et les erreurs.

| Touche | Action |
|---|---|
| `n` / `p` | Image suivante / précédente |
| `s` | Sauvegarder les contours, la vue et les mesures de l'image courante |
| `b` (minuscule) | Traiter le lot avec le détecteur et les réglages courants |
| `q` / Échap | Quitter |

La fenêtre OpenCV doit avoir le focus pour recevoir les touches. Les sorties sont dans `results/manual/` par défaut ; `--output` permet de choisir un autre dossier. Réutiliser le même dossier remplace ses fichiers de même nom.

Couleurs : vert = détection correcte, rouge = faux positif, bleu = faux négatif, violet = FP et FN superposés. Les mesures utilisent les 512 × 512 pixels originaux.

Pour la démonstration sur les 30 images de test :

```bash
bash presentation/lancer_demo.command
```

Choisir `Image=15` pour le gnou. Le lancement fixe Canny à 40/120, sigma=2, noyau=3. Le traitement par lot produit 60 lignes de mesures, une par image et par tolérance.

## Traitement par lot

Paramètres retenus, appliqués aux 40 images :

```bash
./build/contours --batch --detector sobel --sigma 1 --threshold 160 --save-images --output results/custom_sobel
./build/contours --batch --detector laplace --sigma 2 --threshold 20 --save-images --output results/custom_laplace
./build/contours --batch --detector canny --sigma 2 --low 40 --high 120 --save-images --output results/custom_canny
```

Ajouter `--list data/test.txt` pour limiter le lot aux 30 images de test. `--detector all` lance les trois détecteurs avec les mêmes paramètres de ligne de commande ; les trois commandes ci-dessus utilisent leurs réglages propres.

Pour une autre base :

```bash
./build/contours --batch --images mes_images --references mes_references --gt-white --output results/other
```

La référence d'une image `nom` est recherchée parmi `nom_gt_binary.pgm`, `nom_gt_binary.png`, `nom.png`, `nom.pgm`. Les dimensions doivent coïncider. Les références de Groningen ont des contours noirs sur fond blanc ; utiliser `--gt-white` pour la convention inverse. Une référence manquante ou un nom dupliqué provoque une erreur.

## Mesures et comparaison

Le CSV contient les cinq comptages `detected` (D), `reference` (R), `correct` (C), `fp` et `fn`, les paramètres, la tolérance `radius`, le temps de détection et les trois taux :

```text
FP = D - C                  FN = R - C
Z = C + FP + FN
P = C / Z                   TFP = FP / Z                  TFN = FN / Z
```

`radius=0` impose la même position ; `radius=1` autorise un voisinage 3 × 3 avec appariement maximum un à un, sans double comptage. Les deux modes sont toujours calculés. `--tolerance` choisit celui montré dans l'interface et les superpositions. Pour deux cartes vides, P=1 et TFP=TFN=0. Le TFP du sujet ne correspond pas au taux classique FP/(FP+VN).

Sobel utilise la norme des dérivées premières. Laplace utilise le module du Laplacien puis un seuil, sans recherche des passages par zéro. Canny utilise la norme L2, la suppression des non-maxima et l'hystérésis. Un lissage gaussien commun est réglable et désactivable avec sigma=0.

Le protocole compare 72 configurations sur 10 images, puis fige les paramètres sur les 30 autres. Voir le rapport et `results/summary.json` pour les moyennes macro, micro, les comptages et les limites de la méthode.

## Reproduire et vérifier les résultats

Le programme C++ n'a aucune dépendance Python. Les trois commandes de la section « Traitement par lot » recalculent les cartes et les mesures avec les paramètres retenus. Les données sont déjà incluses.

Les scripts auxiliaires de la campagne d'expérimentation et de préparation du rapport sont conservés localement et ne sont pas distribués dans ce dépôt. Leurs mentions dans le rapport et le compte rendu de validation décrivent les vérifications déjà réalisées.

Pour compiler le rapport avec Tectonic :

```bash
cd report/latex
tectonic main.tex
```

Cette commande produit `main.pdf` dans le dossier LaTeX. Le dossier `report/latex` peut également être importé dans Overleaf.

Contrôle des curseurs avec une session graphique :

```bash
./build/contours --gui-check --output tmp/gui_check
```

Exécuter cette commande depuis `TP_contours`. Ce test contrôle les curseurs, les trois détecteurs et les exports. Le compte rendu des vérifications de la campagne fournie est dans `VALIDATION.md`.

## Sources

- [Base de contours de Groningen](https://www.cs.rug.nl/~imaging/databases/contour_database/contour_database.html).
- [Sobel - documentation OpenCV](https://docs.opencv.org/4.x/d2/d2c/tutorial_sobel_derivatives.html).
- [Laplace - documentation OpenCV](https://docs.opencv.org/4.x/d5/db5/tutorial_laplace_operator.html).
- [Canny - documentation OpenCV](https://docs.opencv.org/4.x/da/d22/tutorial_py_canny.html).

Les images et annotations restent attribuées à leurs auteurs. Référence : C. Grigorescu, N. Petkov et M. A. Westenberg, *Contour detection based on non-classical receptive field inhibition*, IEEE TIP, 12(7), 729-739, 2003.
