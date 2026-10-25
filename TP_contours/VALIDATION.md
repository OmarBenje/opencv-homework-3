# Vérification du rendu

Contrôle effectué le 24 octobre 2026 sur macOS avec Apple Clang 17 et OpenCV 5.0.0.

## Code et résultats

- Compilation neuve avec `bash build.sh` : réussie.
- Tests CTest : **3/3 réussis** (`algorithms_and_matching`, `cli_help`, `reject_bad_detector`). Les tests d'appariement comprennent une comparaison à une recherche exhaustive sur 150 petits graphes.
- `scripts/validate_results.py` : vérification réussie des 80 fichiers sources, des 240 mesures, des 120 cartes binaires, des 72 configurations de réglage, des paramètres sélectionnés et des erreurs de ligne de commande.
- Nouvelle exécution du lot avec chacun des trois détecteurs et les paramètres de `results/selected_parameters.json` : les **240 lignes de mesures sont identiques** aux CSV livrés, en excluant la durée de calcul, qui dépend de l'exécution.
- `./build/contours --gui-check --output tmp/submission_gui_check` : contrôle réussi de la création de l'interface, des curseurs et des trois détecteurs. Ce contrôle automatique ne remplace pas une démonstration manuelle de toutes les interactions.

## Rapport - mise à jour du 25 octobre 2026

- Mise en page adaptée du modèle de rapport d'analyse d'images de l'Université de Fribourg ; attribution et licence dans `report/latex/README.md`.
- Compilation de la source LaTeX : réussie, **5 pages**.
- Conservation vérifiée des huit sections, des équations, des trois tableaux et de la bibliographie par comparaison des sources avant/après (hors espaces et sauts de page). Les empreintes des fichiers images, dont les deux logos, sont identiques.
- Tableaux confrontés aux CSV et aux fichiers de synthèse.
- Présentation de Sobel, Laplace et Canny, fonctions OpenCV et paramètres, protocole de comparaison, résultats et limites présents.
- Les cinq pages rendues ont été inspectées visuellement : logos et figures présents, texte et tableaux lisibles. Le résumé et le sommaire ont été ajoutés ; aucune date n'est affichée.
- Le rapport indique que la vidéo est transmise séparément par courriel.

## Périmètre

Le dépôt contient uniquement le Homework 3. Les fichiers temporaires, les exécutables locaux et la vidéo sont exclus. Les données et les résultats nécessaires pour reproduire les mesures sont inclus.

La consigne demande une vidéo de **3 à 5 minutes**. La conformité de sa durée doit être vérifiée sur la version envoyée séparément ; les contrôles ci-dessus portent sur le code, les résultats et le rapport de ce dépôt.

Les instructions de compilation Linux sont fournies, mais cette vérification a été exécutée sur macOS.
