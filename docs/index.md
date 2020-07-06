---
title: Home
---

# {{page.title}}
## About

<div class="image-figure">
    <img style="width:300px;" src="{{'/assets/images/example-image.jpg' | relative_url}}" alt="ADF image" />
    <img style="width:300px;" src="{{'/assets/images/example-strain.jpg' | relative_url}}" alt="Distortion" />
    <p>
        <span class="figure-title">Figure</span> Example ADF image of PbZr<sub>0.2</sub>Ti<sub>0.8</sub>O<sub>3</sub> and a corresponding \(e_{xx}\) distortion map.
    </p>
</div> 

Strain++ is an open source program used to measure strain from high resolution transmission <script type="math/tex">test</script> electron microscope (TEM) images. Strain is measured using the geometric phase analysis (GPA) algorithm as detailed in Martin Hÿtch’s paper:

> [Hÿtch, M. J., Snoeck, E. & Kilaas, R. _Quantitative measurement of displacement and strain fields from HREM micrographs_. Ultramicroscopy **74**, 131–146 (1998)](http://dx.doi.org/10.1016/S0304-3991(98)00035-7)

GPA is a Fourier space technique and is therefore more robust to noise than real space techniques, making it a great tool for quick and easy strain analysis.

## Libraries

The external libraries used are listed here with their uses and licenses

 - [Eigen](http://eigen.tuxfamily.org/) - matrices and linear regression (MPL2)
 - [QCustomPlot](http://qcustomplot.com/) - plotting (GPLv3)
 - [Qt](http://www.qt.io/) - GUI (GPLv3)
 - [FFTW](http://www.fftw.org/) - Fourier transforms (GPLv2)
 - [LibTIFF](http://www.remotesensing.org/libtiff/) - TIFF input/output (BSD-like)