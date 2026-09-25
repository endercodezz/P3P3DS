# CloudWorks attribution

The volumetric-cloud density and noise model in the VCS DirectX 12 presenter is
adapted from **CloudWorks Alpha 4.0** by Brian Tu (RTU), dated 2021-07-25.

Original project: <https://github.com/keroroxzz>

The original shader identifies its license as Creative Commons
Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0):
<https://creativecommons.org/licenses/by-nc-sa/3.0/>

This adaptation replaces RenderHook/timecycle inputs with fixed parameters from
`ProperShaders.ini`, uses the PSP GE camera captured by VCSNative, and composites
the result in the native DirectX 12 presentation pass.
