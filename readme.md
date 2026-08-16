# Description

One of a million n-body simulations on the internet. The end goal for this is to be able to design a mission for a rocket in the solar system (probably by modifying a plaintext file, haven't decided on the format yet). That's a long ways away.

Do note that this is my third time simulating the solar system, once in a Python/C hybrid (abandoned due to lag), and once in C (abandoned due to unscalable code). For this one, I'm using C++.

# Support

I have no means to test the code on machines other than my own (a Windows machine). However, I have tried to keep the code portable.

It will most likely work on your machine if it's Windows, beyond that, I'm not sure if it will.

# Progress

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

Goals:
1. Switch to SDL3's 3D tools
2. Find an alternative for `SDL_RenderDebugText`
3. Figure out what data is needed for rendering. I suspect this to be the needed information \[TBD means IDK if I'll use it\]:<br>Images (lit, Earth's night, rings \[RGB*A*\])<br>3D models (for the minor bodies like Phobos or Pallas)<br>Rotation data (Rotation at the epoch \[+ verification, somehow? Might reduce `FACTOR` to 0 and see which side's day on Earth\], axis of rotation, rate of rotation)<br>Height maps (TBD, the terminus does not apply to landmasses above/below their surroundings)<br>Atmospheric data (TBD)
4. Test particles<br>This will come with splitting integration from the draw loop, having the rendering interpolate various points in some timespan
5. Mission planning<br>A file will have a handwritten mission for a rocket (test particle w/ thrust), and be ran through a Python file to generate a data file for the program
6. Mission optimization<br>Have the C++ program optimize the mission