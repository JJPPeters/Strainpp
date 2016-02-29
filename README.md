# Strain++
## About
Strain++ is a program written in C++ used to measure strain from high resolution transmission electron microscope (TEM) images. To measure the strain, the geometric phase analysis (GPA) algorithm is implemented as detailed in Martin Hÿtch's paper:

> Hÿtch, M. J., Snoeck, E. & Kilaas, R. *Quantitative measurement of displacement and strain fields from HREM micrographs*. Ultramicroscopy **74**, 131–146 (1998)

----
## Usage
1. Open an image (.dm3, .dm4 and .tif supported) using the 'File' menu
2. Apply a Hann window from the 'Window' menu if needed
3. Select 'GPA' from the 'Strain' menu
4. A circle is shown on the FFT with the radius of the smallest g-vector. If this is incorrect you can manually select it (this determines the mask size)
5. Select a Bragg spot on the FFT
6. If you want to, the selection can be refined by clicking two corners of a rectangle on the displayed phase
7. Repeat steps 5 and 6 for a second, non-colinear Bragg spot
8. The strain is now calculated and shown in the results tab
9. In the results tab the rotation of the x and y axis can be changed relative to the image axes.
10. The other tab can be used to show intermediate steps

Data can be exported using the 'File' then 'Export' menus or by right clicking individual images. There are 3 options to choose:

1. 'RGB image' saves a TIFF with RGB channels (8-bit per channel)
2. 'Data image' saves the raw image data as a TIFF with single floating point precision
3. 'Binary' saves the raw image data as a basic (no shape information etc.) binary file with double floating point precision

----
## External libraries
The external libraries used are listed here with their uses and licenses

1. [Eigen](eigen.tuxfamily.org/) - matrices and linear regression (MPL2)
2. [QCustomPlot](www.qcustomplot.com/) - plotting (GPLv3)
3. [Qt](www.qt.io/) - gui (GPLv3)
4. [FFTW](www.fftw.org/) - Fourier transforms (GPLv2)
5. [LibTIFF](www.libtiff.org/) - TIFF input/output (BSD-like)

