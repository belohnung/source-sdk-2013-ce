#!/bin/sh

python devtools/qpc/qpc.py -b "qpc_scripts/default.qgc" -a game -g visual_studio -m SDK2013CE -mf Game

read -s -n 1 -p "Press any key to continue . . ."
echo ""
