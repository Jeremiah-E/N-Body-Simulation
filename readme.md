# Description

One of a million n-body simulations on the internet. The end goal for this is to be able to design a mission for a rocket in the solar system (probably by modifying a plaintext file, haven't decided on the format yet). That's a long ways away.

Do note that this is my third time simulating the solar system, once in a Python/C hybrid (abandoned due to lag), and once in C (abandoned due to unscalable code). For this one, I'm using C++.

# Support

I have no means to test this on other operating systems than my own, so I intentionally only code for Windows. Huge thanks to Weffuls for making `setup.ps1` more portable.

# Progress so far

I have a general structure for how bodies will be stored, as five different `double` arrays, alongside a `string` vector:
1. Position
2. Velocity
3. Acceleration
4. Gravitational parameter<br>(Mass times a constant $G$, roughly $6.67\cdot 10^{-11} \frac{\text{m}^3}{\text{kg}\thinspace{}\text{s}^2}$. This is easier to measure than just mass, and it means we don't need to store $G$ anywhere)
5. Radius
6. Name

A Python script (`universe.py`) generates a `universe.bin` file describing the system (body count, positions, velocities, gravitational parameters, radii, and names), which `main.cpp` reads at startup. `universe.py` currently supports toggling various aspects of the solar system. At maximum settings, 47 bodies are simulated. At minimum settings, only the Earth/Sun/Moon are simulated.

Right now, I draw the loaded bodies' positions on screen using perspective projection and 3D camera controls, then perform velocity-verlet integration to find their position for the next frame.

# Planned goals of note

Given the scope creep that will forever permiate the project, here's some goals for me to work towards.

Well-defined goals:
- Decouple physics and rendering.<br>Decouple the computation loop from the draw loop. The physics engine will run without visuals to precompute and record the entire solar system and mission state over a specified timeframe. The simulation will save this timeline, and the draw loop will load it and display the prerecorded data. (High priority)
- Introduce Planet Textures.<br>Every body, even Sedna, has either a real image of its surface or an artists rendition out there somewhere. If I want to make this simulation less jarring and easier to follow, having bodies use real imagery would go a long way. Note that Saturn's rings will accompany this step. (Medium priority)
- Planet Lighting.<br>The sun is largely the main source of light in the solar system. The math to make the solar system lit up will be relatively simple compared to traditional lighting engines. (Low priority)

Not well-defined goals:
- Mission planning.<br>I want to be able to set up a base mission and target parameters (Δv budget, target orbit, etc.) and have the program brute force more optimal solutions, either quicker or more Δv efficient. Have yet to decide how any of this will work. This will entail a Python program for the manual creation of an initial mission profile (similar to tweaking maneuver nodes in KSP, somewhere between vanilla and [Principia](https://github.com/mockingbirdnest/Principia)) using patched conics terminology to describe n-body motion (i.e. "Burn +0.03m/s2 prograde rel. to ECI at T+30hr for 20s"). Once this crude but working baseline is established, along with restrictions and goals (i.e. "Orbit Mars at these orbital parameters" and acceleration budgets based on staging) the C++ engine will take over to optimize and brute-force a better solution. (Medium priority)