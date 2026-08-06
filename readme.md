# Description

One of a million n-body simulations on the internet. The end goal for this is to be able to design a mission for a rocket in the solar system (probably by modifying a plaintext file, haven't decided on the format yet). That's a long ways away.

Do note that this is my third time simulating the solar system, once in a Python/C hybrid (abandoned due to lag), and once in C (abandoned due to unscalable code). For this one, I'll use C++.

# Support

I have no means to test this on other operating systems than my own, so I intentionally only code for Windows. It requires the SDL3 library installed on the computer in `C:/User/{USERNAME}/vendored`.

# Progress so far

Right now, I've got a window opening and I can draw hardcoded circles. That circle drawing function will last through the entire project, if the last version of the project ends up being similar.

# Planned goals of note

I want to have two files (read at runtime) that determine two properties:
1. What planets are loaded in (e.g. Earth/Moon or Earth/Moon/Sun/Mars, etc.).
2. A mission profile, with an optional `NO PROFILE` as to let you just view the solar system.

I might write a program to make editing these files easier, but haven't decided yet. If I do, I'll most likely have a Python program where you check boxes for planets, set the epoch, and plan the mission.

I want to support up to ~100 bodies, although odds are I land around ~70. The planets are a definite yes, although I want to allow you to plan an asteroid mission if you want to.

Ideally, I want to be able to have the program store each point of your mission. Luckily, C++ is much easier to do dynamic allocation in than C.