# Description

One of a million n-body simulations on the internet. The end goal for this is to be able to design a mission for a rocket in the solar system (probably by modifying a plaintext file, haven't decided on the format yet). That's a long ways away.

Do note that this is my third time simulating the solar system, once in a Python/C hybrid (abandoned due to lag), and once in C (abandoned due to unscalable code). For this one, I'll use C++.

# Support

I have no means to test this on other operating systems than my own, so I intentionally only code for Windows. Huge thanks to Weffuls for making `setup.ps1` more portable.

# Progress so far

I have a general structure for how bodies will be stored, as four different `double` arrays, alongside a `string` vector:
1. Position
2. Velocity
3. Acceleration
4. Gravitational parameter
5. Name

Bodies are no longer hardcoded in `main.cpp`. Instead, a Python script (`universe.py`) generates a `universe.bin` file describing the system (body count, positions, velocities, gravitational parameters, and names), which `main.cpp` reads at startup. `universe.py` currently supports toggling Pluto/Charon and the four minor moons (Nix, Hydra, Styx, Kerberos) on and off, and automatically re-centers the system so the center of mass stays fixed with zero net momentum.

Right now, I draw the loaded bodies' positions and then perform velocity-verlet integration to find their position for the next frame.

# Planned goals of note

Given the scope creep that will forever permiate the project, here's some goals for me to work towards.

Well-defined goals:
- Allow for rotation of the screen.<br>3D coordinates exist but are unused. (High priority)
- Expand the simulation to the solar system.<br>Right now the universe consists of Pluto and five moons. (High priority)
- Introduce a new rendering system for the planets.<br>There's a million texture files online of the major planets, and I assume I can figure it out for the moons. I'm looking to have the simulation look something akin to Celestia. Do note that this goal will likely wait until after the mission planning is in, as I'd like to pre-compute where everything is before starting to draw, instead of doing both expensive(?) rendering and integration at the same time. (Low priority)

Not well-defined goals:
- Mission planning.<br>I want to be able to set up a base mission and target parameters (Δv budget, target orbit, etc.) and have the program brute force more optimal solutions, either quicker or more Δv efficient. Have yet to decide how any of this will work. (Medium priority)