# Fog

Unlike OpenGL ES 1.1, fog is not limited to linear, exp, exp2 modes. The PICA200 allows one to setup a lookup table for fog factors (denoted by $f$), which are indexed by depth values in window coordinates. There are 128 LUT entries which encode values + difference with the next element, thus depth space is divided into 129 chunks ($\frac{i}{128}$ from 0 to 128, inclusive).

This also implies that the fog implementation deviates from the standard, where fog factors depend instead on $Z_{eye}$. Thus, mapping standard fog effects requires delving in a bit of math.

## LUT creation

All calculations are carried assuming a right hand projection, but can be easily adjusted for left hand projections.

First, we transform depth values into normalized device coordinates:

$$
depth = A_0 * Z_{ndc} + B_0
$$

$$
\rightarrow Z_{ndc} = \frac{depth - B_0}{A_0}
$$

where $A_0$, $B_0$ are values computed from the depth range:

$$
A_0 = {maxDepth} - {minDepth}
$$

$$
B_0 = {maxDepth}
$$

Then, we calculate $Z_{eye}$ from $Z_{ndc}$:

$$
Z_{ndc} = \frac{Z_c}{W_c} = \frac{A_1 * Z_{eye} + B_1}{-Z_{eye}}
$$

$$
\rightarrow Z_{eye} = \frac{-B_1}{Z_{ndc} + A_1}
$$

where $A_1$, $B_1$ come from the perspective projection matrix.

Finally, we can populate the LUT:

$$
{LUT}_i = F(Z(i))
$$

where $i$ is the index of the LUT entry (and thus of the depth chunk it refers to), $Z(i)$ is a function that maps $i$ to the relative $Z_{eye}$ value:

$$
Z(i) = \frac{B_1}{\frac{\frac{i}{128} - B_0}{A_0} + A_1}
$$

and $F(z)$ is the fog factor formula, eg.:

$$
F(z) = \frac{{end} - z}{{end} - {start}}
$$

or

$$
F(z) = e^{-density * z}
$$

or

$$
F(z) = e^{-(density * z)^2}
$$

Note that, if we plug the standard values for $A_1$, $B_1$:

$$
A_1 = \frac{{near}}{{near} - {far}}
$$

$$
B_1 = \frac{-{near} * {far}}{{near} - {far}}
$$

we derive the algorithm used by [citro3d](https://github.com/devkitPro/citro3d/blob/9d51a9445dff7521f5f3959f7003c86a19f7c09b/include/c3d/fog.h#L18):

$$
Z_{eye} = \frac{\frac{n * f}{n - f}}{Z_{ndc} + \frac{n}{n - f}}
$$

$$
\rightarrow Z_{eye} = \frac{\frac{n * f}{n - f}}{\frac{Z_{ndc}(n - f) + n}{n - f}}
$$

$$
\rightarrow Z_{eye} = \frac{n * f}{Z_{ndc}(n - f) + n}
$$

$$
\rightarrow Z_{eye} = \frac{n * f}{-Z_{ndc}(f - n) + n}
$$

with ${depth} = -Z_{ndc}$, since citro3d defaults $({minDepth}, {maxDepth})$ to $(1.0, 0.0)$.