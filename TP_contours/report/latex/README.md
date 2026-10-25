# Rapport LaTeX - modèle de Fribourg

Source principale : `main.tex`. Les huit sections sont dans `sections/`, la bibliographie dans
`references.tex`, et les logos et figures dans `images/`. Ce dossier est autonome.
Le rapport comporte cinq pages : titre, logos, résumé et sommaire en première page, puis les quatre
pages de contenu scientifique du TP.

## Compiler

Depuis ce dossier, avec Tectonic :

```sh
tectonic main.tex
```

Ou avec TeX Live :

```sh
latexmk -pdf main.tex
```

Le projet est aussi compatible avec Overleaf : importer `rapport-latex.zip`, choisir `main.tex`
comme document principal et compiler avec pdfLaTeX.

Depuis la racine de TP_contours, `python3 scripts/make_report.py` compile cette source et met à jour
`report/rapport.pdf` ainsi que l'archive des sources `report/rapport-latex.zip`.
Les tableaux et commentaires correspondent aux résultats vérifiés de la campagne fournie.
Après une nouvelle campagne différente, mettre à jour les chiffres du texte et les figures ensemble.

## Modèle et attribution

Cette mise en page est adaptée du modèle **Technical Report for Document Image Analysis (UniFR)**,
créé par **Paul Märgner et Lars Vögtlin** pour l'Université de Fribourg.

- [Modèle et sources sur Overleaf](https://www.overleaf.com/latex/templates/technical-report-for-document-image-analysis-unifr/xhdkbqhhzmyw)
- Licence du modèle : [Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/)

Éléments repris : classe `article` en 10 points, format A4, dimensions du texte, titre centré,
résumé, sommaire, sections numérotées en noir et pagination simple.

Adaptations : français, typographie Latin Modern, logos Polytech Marseille et AMU, absence de date,
conservation des huit sections du TP, tableaux `booktabs` et figures existantes. Un résumé a été
ajouté à partir du contenu du rapport. Le texte scientifique, les équations, les trois tableaux,
les images et la bibliographie de la version précédente ont été conservés intégralement.
Il ne s'agit pas d'un modèle officiel de Polytech Marseille.

## Logos

- Polytech Marseille : logo couleur déjà présent dans le rapport du projet Intern, issu de la charte
  graphique de Polytech. La [page de documentation officielle](https://polytech.univ-amu.fr/fr/documentation)
  renvoie à la charte du réseau. La [version blanche du site officiel](https://polytech.univ-amu.fr/themes/amu/amu_polytech/logo.svg)
  a également été consultée ; la version couleur est utilisée ici sur fond blanc.
- Aix-Marseille Université : logo amU bleu déjà présent dans le projet Intern, vérifié visuellement
  avec le [logo utilisé sur le site officiel](https://www.univ-amu.fr/themes/amu/amu_www/logo.svg).

Les proportions et les couleurs des fichiers fournis sont conservées. Les logos appartiennent à leurs
établissements respectifs. Toutes les figures d'analyse proviennent des résultats du programme.
