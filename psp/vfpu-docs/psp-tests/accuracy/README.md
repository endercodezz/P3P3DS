
This program calculates error figures for approximate functions (such as
trigonometric primitives). It takes a while to run.

The error is calculated by taking the difference between the VFPU outputs
and the C aproximations (which have a much smaller error). Since we do not
know what _shape_ the error function has (it depends a lot on the function
and how the approximation works) the program tries random inputs (within a
range that makes sense for the function under testing) hoping to find the
worst case and give a good error approximation.

