# Rapport court en LaTeX

Source principale : `main.tex`. Les logos et figures sont inclus dans `images/`, afin que ce dossier
soit autonome. Le rapport reprend les mesures du projet dans un format de quatre pages.

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

## Modèles consultés

- [Report Template, Mazen Amria, Overleaf](https://www.overleaf.com/latex/templates/report-template/bdbvpdswhjcg) : rapport universitaire fondé sur la classe `article`.
- [Simple LaTeX Assignment/Report Template, Blake Rowden](https://github.com/blakerowden/university-latex-report-template) : organisation des informations du rapport.

La mise en page a été écrite pour ce TP : typographie Latin Modern, titres bleus, tableaux booktabs,
logos institutionnels en tête et pagination simple. Il ne s'agit pas d'un modèle officiel de l'école.

## Logos

- Polytech Marseille : logo couleur déjà présent dans le rapport du projet Intern, issu de la charte
  graphique de Polytech. La [page de documentation officielle](https://polytech.univ-amu.fr/fr/documentation)
  renvoie à la charte du réseau. La [version blanche du site officiel](https://polytech.univ-amu.fr/themes/amu/amu_polytech/logo.svg)
  a également été consultée ; la version couleur est utilisée ici sur fond blanc.
- Aix-Marseille Université : logo amU bleu déjà présent dans le projet Intern, vérifié visuellement
  avec le [logo utilisé sur le site officiel](https://www.univ-amu.fr/themes/amu/amu_www/logo.svg).

Les proportions et les couleurs des fichiers fournis sont conservées. Les logos appartiennent à leurs
établissements respectifs. Toutes les figures d'analyse proviennent des résultats du programme.
