# Source provenance and license boundaries

PSPRecomp separates independently written framework/profile code from third-party components with their own licenses.

## Framework

The reusable PSPRecomp runtime, decoder, analyzer and code generator in the repository root are distributed under the MIT License. Game-specific addresses and implementations are not accepted in the reusable core.

The project may use public hardware documentation, observable program behavior and other implementations as technical references. Reference material is used to understand behavior and architecture; source code from incompatible copyleft projects is not imported into the MIT framework.

## Decryption

The repository does not contain an EBOOT/PRX decryption implementation. A profile may document the input format it expects, but users are responsible for preparing their own legally obtained executable outside PSPRecomp.

## Profile code

A profile owns its generated AOT corpus, address-specific lowering, HLE behavior and native fast paths. Those files remain isolated under `profiles/<id>` so they do not become hidden dependencies of the generic framework.

## Third-party components

Third-party source, binary dependencies, shader code and notices stay beside the profile that needs them. Their original copyright and license notices must be preserved. The VCS profile summarizes its bundled components in `profiles/vcs/THIRD_PARTY.md`.

## Contribution rule

Do not paste or adapt source from a project whose license is incompatible with the destination file. Reimplement required behavior from specifications, observations or independently documented semantics, and record the source of third-party material when it is intentionally included under a compatible license.
