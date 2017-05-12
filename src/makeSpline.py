# Make splines
# Run in Maya script editor (Python)
# Change radius to something reasonable (between 0.01 and 1.0)
# it’s easier to determine radii in the .lit file
# Scale a curve to an appropriate size (should fit within a 2x2x2 box)
# Freeze transformations before running script

import pymel.core as pymel
import random

def makeSpline(radius, r, g, b):
    cvs = pymel.getAttr('curveShape1.spans')+1
    
    print "* " + s tr(r) + " " + str(g) + " " + str(b)
    for i in range(0, cvs):
        line = pymel.getAttr( 'curve1.cv['+ str(i) +']')
        newline = ""
        for float in line:
            trunc = format(float, '.2f')
            newline += trunc
            newline += " "
            
        newline += str(radius) # str(format(random.random(), '.2f'))
    
        #string = string[1:-1]
        #string = string.replace(",", "")
        print newline
    print "#"

makeSpline(0.1, 1.0, 0.0, 0.0)