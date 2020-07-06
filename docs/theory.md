---
title: Theory
---

# {{page.title}}

The essence of GPA is the comparison of the phase of a set of perfect planes (defined by a <script type="math/tex">g</script>-vector) to the planes measured from an image (defined by a mask at the <script type="math/tex">g</script>-vector).

If we consider and image to be formed from a Fourier series

<script type="math/tex; mode=display">I \left( \pmb r \right) = \sum_g A_g e^{i P_g + 2 \pi i \pmb g \cdot \pmb r}</script>

Where <script type="math/tex">I</script> is the image intensity, <script type="math/tex">\pmb r</script> is the position in the image and <script type="math/tex">\pmb g</script> are the periodicities in the image (i.e. the position in reciprocal space). <script type="math/tex">A_g</script> and <script type="math/tex">P_g</script> give the amplitude and phase of the periodicity given by <script type="math/tex">\pmb g</script>. Using a simple mask, it is easy to extract one of (or a select few) of these Fourier components by masking the FFT. Inversing this FFT then produces the complex image <script type="math/tex">H'_g \left( \pmb r \right)</script>. From this, the phase difference can be calculated as

<script type="math/tex; mode=display">P_g \left( \pmb r \right) = \text{Phase} \left[ H'_g \left( \pmb r \right) \right] - 2 \pi \pmb g \cdot \pmb r</script>

where the first term is the phase from the masked FFT and the second term in the phase calculated from the <script type="math/tex">g</script>-vector the FFT was masked at. At this point, the <script type="math/tex">g</script>-vector can be refined to an area of homogeneous strain. If the <script type="math/tex">g</script>-vector is incorrect (even if only by a little bit!) then the phase in the uniform strain region will have a gradient. By fitting this gradient, it is possible to correct the <script type="math/tex">g</script>-vector using

<script type="math/tex; mode=display">\Delta \pmb g = \frac{1}{2 \pi} \nabla P_g.</script>

Each phase can be used to calculate the displacements in the direction of the lattice place. To get the full strain field, the phase needs to be calculated for two non-colinear <script type="math/tex">g</script>-vectors. The phases are related to the displacement field, <script type="math/tex">\pmb u</script>, by

<script type="math/tex; mode=display">% <![CDATA[
\begin{pmatrix}
P_{g1} \\ P_{g2}
\end{pmatrix}
= - 2 \pi \begin{pmatrix}
g_{1x} & g_{1y} \\
g_{2x} & g_{2y}
\end{pmatrix} \begin{pmatrix}
u_x \\
u_y
\end{pmatrix} %]]></script>

where <script type="math/tex">g_{1x}</script> and <script type="math/tex">g_1y</script> are the <script type="math/tex">x</script> and <script type="math/tex">y</script> components of the <script type="math/tex">g</script>-vector using to calculate the phase, <script type="math/tex">P_{g1}</script>. Inverting this gives the displacements in terms of the phases,

<script type="math/tex; mode=display">% <![CDATA[
\begin{pmatrix}
u_x \\ u_y
\end{pmatrix}
= - \frac{1}{2 \pi} \begin{pmatrix}
a_{1x} & a_{2x} \\
a_{1y} & a_{2y}
\end{pmatrix} \begin{pmatrix}
P_{g1} \\
P_{g2}
\end{pmatrix} %]]></script>

where we have used

<script type="math/tex; mode=display">% <![CDATA[
\begin{pmatrix}
g_{1x} & g_{1y} \\
g_{2x} & g_{2y}
\end{pmatrix}^T = \begin{pmatrix}
a_{1x} & a_{2x} \\
a_{1y} & a_{2y}
\end{pmatrix}^{-1}. %]]></script>

Finally, the distortion is calulcated by differentiating:

<script type="math/tex; mode=display">% <![CDATA[
e = \begin{pmatrix}
e_{xx} & e_{xy} \\
e_{yx} & e_{yy}
\end{pmatrix}
= \begin{pmatrix}
\frac{\partial u_x}{\partial x} & \frac{\partial u_x}{\partial y} \\
\frac{\partial u_y}{\partial x} & \frac{\partial u_y}{\partial y}
\end{pmatrix}. %]]></script>

From this matrix, the strain, <script type="math/tex">\varepsilon</script>, rotation, <script type="math/tex">\omega</script> and dilitation, <script type="math/tex">\Delta</script>, are easily calculated from

<script type="math/tex; mode=display">\varepsilon = \frac{1}{2} \left( e + e^T \right)</script>

<script type="math/tex; mode=display">\omega = \frac{1}{2} \left( e - e^T \right)</script>

<script type="math/tex; mode=display">\Delta = \text{Trace} \left[ e \right]</script>