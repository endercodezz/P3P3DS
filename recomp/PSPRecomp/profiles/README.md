# Profiles

Profiles contain everything PSPRecomp needs for a particular title without contaminating the reusable framework with game-specific addresses or behavior.

A profile is selected at configure time:

```bash
cmake -S . -B out/<profile> -DPSPRECOMP_PROFILE=<profile>
```

The current repository includes `vcs`. See `docs/PROFILE_GUIDE.md` before adding another title.
