#!/bin/sh

python devtools/qpc/qpc.py -b "qpc_scripts/default.qgc" -a everything -g visual_studio -m SDK2013CE -mf Everything

read -s -n 1 -p "Press any key to continue . . ."
echo ""
