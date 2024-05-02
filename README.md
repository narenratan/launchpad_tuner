Launchpad Tuner
===============

Audio plugin which lets you use a Novation Launchpad as an isomorphic keyboard.

There are three parameters:
- **X step** - the pitch change going one key across
- **Y step** - the pitch change going one key up
- **Transpose** - an overall shift in pitch

The plugin displays the pitch in cents (modulo 1200) for each Launchpad key in
a grid.

Builds for Windows, Mac, and Linux are available on the [releases
page](https://github.com/narenratan/launchpad_tuner/releases).

Steps and transpose can be entered in:

- Cents:

![cents](images/cents.png)

- EDO steps:

![edo_steps](images/edo_steps.png)

- Ratios:

![ratios](images/ratios.png)

The Launchpad needs to be in programmer mode or custom mode four (for the
Launchpad X). See the Launchpad [Programmer's reference
manual](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Launchpad%20X%20-%20Programmers%20Reference%20Manual.pdf)
(pdf).

The plugin acts as an MTS-ESP source so will only work with MTS-ESP compatible
synths like Surge XT.

When using Reaper, Launchpad Tuner can either go on its own track or be added
at the start of an FX chain with a synth already on it (for me adding Launchpad
Tuner to an empty FX chain prevents sound output if I then add a synth).

Build
-----
Assuming all dependencies are installed:
```console
    $ git clone --recurse-submodules https://github.com/narenratan/launchpad_tuner
    $ cd launchpad_tuner
    $ cmake -B build
    $ cmake --build build
```
See the [CI build workflow](.github/workflows/build.yml) for the commands to
install the dependencies on Mac, Windows, and Linux.
