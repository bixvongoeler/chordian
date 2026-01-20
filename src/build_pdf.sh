#!/bin/bash

INPUT_FILE="chordian_v2_report.md"
OUTPUT_FILE="chordian_v2_report.pdf"
MARGIN_TOP="0.7in"
MARGIN_BOTTOM="0.7in"
MARGIN_SIDE="0.7in"
PAPER_SIZE="letterpaper" # letterpaper, a4paper
FONT_SIZE="11pt"         # 10pt, 11pt, 12pt
LINE_SPACING="1.1"       # 1.0 = single, 1.5, 2.0 = double
MAIN_FONT="Palatino"     # Serif font for body
SANS_FONT="Helvetica"    # Sans font for headings
MONO_FONT="Menlo"        # Monospace for code
PDF_ENGINE="xelatex"
HIGHLIGHT_STYLE="tango"
INCLUDE_TOC=false
TOC_DEPTH=2
LINK_COLOR="NavyBlue"
HEADING_COLOR="MidnightBlue"

# ============== BUILD COMMAND ==============
cd "$(dirname "$0")"

cat >_header.tex <<'HEADER'
% Better float handling - keep figures near text
\usepackage{float}
\floatplacement{figure}{H}
\usepackage{placeins}

% Scale images to reasonable size (max 85% text width, max 40% page height)
\usepackage{graphicx}
\makeatletter
\def\maxwidth{\ifdim\Gin@nat@width>0.85\textwidth 0.85\textwidth\else\Gin@nat@width\fi}
\def\maxheight{\ifdim\Gin@nat@height>0.4\textheight 0.4\textheight\else\Gin@nat@height\fi}
\makeatother
\setkeys{Gin}{width=\maxwidth,height=\maxheight,keepaspectratio}

% Colors
\usepackage{xcolor}
\definecolor{headingcolor}{HTML}{191970}
\definecolor{linkcolor}{HTML}{000080}
\definecolor{codebackground}{HTML}{f5f5f5}

% Colored section headings
\usepackage{sectsty}
\allsectionsfont{\color{headingcolor}}

% Better code block styling
\usepackage{mdframed}
\surroundwithmdframed[
  backgroundcolor=codebackground,
  linewidth=0pt,
  innerleftmargin=8pt,
  innerrightmargin=8pt,
  innertopmargin=8pt,
  innerbottommargin=8pt
]{verbatim}

% Reduce figure caption font size and style
\usepackage[font=small,labelfont=bf,labelsep=period]{caption}

% Better spacing
\usepackage{parskip}
\setlength{\parskip}{0.5em}

% Prevent orphans and widows
\widowpenalty=10000
\clubpenalty=10000

% Reduce whitespace around figures
\setlength{\intextsep}{12pt plus 2pt minus 2pt}
\setlength{\floatsep}{12pt plus 2pt minus 2pt}

% Header/footer
\usepackage{fancyhdr}
\pagestyle{fancy}
\fancyhf{}
\fancyhead[L]{\small\textcolor{gray}{EMID Final Project Report: Chordian}}
\fancyhead[R]{\small\textcolor{gray}{\thepage}}
\renewcommand{\headrulewidth}{0.4pt}
\fancyfoot{}

% Title page styling
\usepackage{titling}
\setlength{\droptitle}{-2em}
\pretitle{\begin{center}\LARGE\bfseries\color{headingcolor}}
\posttitle{\end{center}\vskip 0.5em}
\preauthor{\begin{center}\large}
\postauthor{\end{center}}
\predate{\begin{center}\normalsize}
\postdate{\end{center}\vskip 2em}

HEADER

CMD="pandoc \"$INPUT_FILE\" -o \"$OUTPUT_FILE\""
CMD+=" --pdf-engine=$PDF_ENGINE"
CMD+=" -V geometry:top=$MARGIN_TOP"
CMD+=" -V geometry:bottom=$MARGIN_BOTTOM"
CMD+=" -V geometry:left=$MARGIN_SIDE"
CMD+=" -V geometry:right=$MARGIN_SIDE"
CMD+=" -V geometry:paper=$PAPER_SIZE"
CMD+=" -V fontsize=$FONT_SIZE"
CMD+=" -V linestretch=$LINE_SPACING"
CMD+=" -V mainfont=\"$MAIN_FONT\""
CMD+=" -V sansfont=\"$SANS_FONT\""
CMD+=" -V monofont=\"$MONO_FONT\""
CMD+=" -V colorlinks=true"
CMD+=" -V linkcolor=$LINK_COLOR"
CMD+=" -V urlcolor=$LINK_COLOR"
CMD+=" --syntax-highlighting=$HIGHLIGHT_STYLE"
CMD+=" -H _header.tex"
CMD+=" --columns=80"

if [ "$INCLUDE_TOC" = true ]; then
    CMD+=" --toc --toc-depth=$TOC_DEPTH"
fi

echo "Building PDF..."
echo "Command: $CMD"
echo ""

eval $CMD
BUILD_STATUS=$?

rm -f _header.tex

if [ $BUILD_STATUS -eq 0 ]; then
    echo ""
    echo "Success: $OUTPUT_FILE"
else
    echo ""
    echo "Build failed. Check error messages above."
    exit 1
fi
