# RhodoSim++

This is the web version of RhodoSim with a number of extended/improved features:

* Full compatibility with RhodoSim but works in the user's web browser.
* No dependencies to ROOT.
* RF cavity design module that is integrated with tracking simulation.
* Turkish language support.

## Getting started

Open [`rhodosim.html`](https://erkcan.github.io/RhodoSimWeb/Web/rhodosim.html) in a recent web browser; there is nothing to install. If you are new to rhodotrons, the guided
tour (header button) is a good start. The header also switches between English and Turkish, and day and night themes.

## What it does

* Tracks electrons through the cavity and the deflecting magnets, showing their trajectories, the energy gained on
  every pass, and how the result depends on the RF phase.
* Designs the RF cavity: shape its cross-section, solve its resonant mode, see its frequency, quality factor and
  fields, and save its 3D shape as STL.
* Connects the two: tune the cavity to your RF frequency, then track electrons in the field of the cavity you
  designed. Inconsistent or non-physical settings are flagged.
* Exchanges files with RhodoSim (settings and RF field maps) and CST.

## Credits

Designed by veö, built upon C++ coding work by F. Er & G. Unel. Code translation and further development assisted by
Claude. If you use this software, please cite: Er, Muhammet Furkan. "Design and simulation software for rhodotron
type electron accelerators." MSc thesis, Boğaziçi University, 2024.

We would like to acknowledge support from TÜBİTAK under the grant 122F304.
