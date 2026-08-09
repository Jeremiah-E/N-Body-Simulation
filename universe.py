import struct

# FILE GENERATION SETTINGS

# Include Charon?
major_moon = True
# Include Nix, Hydra, Styx, and Kerberos? (requires major_moon)
minor_moons = False
# Where to store the file. Note that changing this requires changing the C++ program as well
filepath = "universe.bin"

######

# The four minor moons don't make sense without the major one
minor_moons = major_moon and minor_moons

positions = []
velocities = []
mus = []
num = 0
# Regardless of settings, this is the order we put bodies into the array
names = ["Pluto", "Charon", "Nix", "Hydra", "Styx", "Kerberos"]

# Build the system
if not major_moon:
    positions = [0, 0]
    velocities = [0, 0]
    mus = [8.71e11]
    num = 1
else:
    # Pluto and Charon
    bodies = [{"pos": [-2112909.83, 0], "vel": [0, -23.9824556], "mu": 8.71e11}, {"pos": [17527090.2, 0], "vel": [0, 198.940179], "mu": 1.05e11}]
    num = 2
    if minor_moons:
        num = 6
        # Nix
        bodies.append({"pos": [48694000, 0], "vel": [0, 141.55], "mu": 3003435})
        # Hydra
        bodies.append({"pos": [64738000, 0], "vel": [0, 122.77], "mu": 3203664})
        # Styx
        bodies.append({"pos": [42656000, 0], "vel": [0, 151.25], "mu": 500572.5})
        # Kerberos
        bodies.append({"pos": [57783000, 0], "vel": [0, 129.95], "mu": 1067888})
    # Some hastily written code to go from an easy-to-code AoS to the expected SoA format I want in the binary file
    for body in bodies:
        px, py = body["pos"]
        positions.append(px) ; positions.append(py)
        vx, vy = body["vel"]
        velocities.append(vx) ; velocities.append(vy)
        mus.append(body["mu"])

# Ensure we didn't mess up generating the arrays *too* much
assert len(positions) == num * 2, "Positions has an incorrect length"
assert len(velocities) == num * 2, "Velocities has an incorrect length"
assert len(mus) == num, "Mus has an incorrect length"

# Shift the reference frame to keep the center of mass and net momentum zero
# This block is AI generated
if num > 0:
    total_mass = sum(mus)
    com_x = 0.0
    com_y = 0.0
    mom_x = 0.0
    mom_y = 0.0
    for i in range(0, len(positions), 2):
        mass = mus[i // 2]
        x = positions[i]
        y = positions[i + 1]
        vx = velocities[i]
        vy = velocities[i + 1]
        com_x += x * mass ; com_y += y * mass
        mom_x += vx * mass ; mom_y += vy * mass
    com_x /= total_mass
    com_y /= total_mass
    vcm_x = mom_x / total_mass
    vcm_y = mom_y / total_mass
    for i in range(0, len(positions), 2):
        positions[i] -= com_x
        positions[i + 1] -= com_y
        velocities[i] -= vcm_x
        velocities[i + 1] -= vcm_y
# And back to handwritten code

# Pack a piece of data into a type
def pack(data, type):
    # Python floats are equivalent to C doubles
    if type == "float":
        return struct.pack('<d', data)
    if type == "string":
        return (data + '\0').encode('utf-8')
    if type == "int":
        return struct.pack('<i', data)

# Now we generate the file
with open(filepath, 'wb') as file:
    # An int
    file.write(pack(num, 'int'))
    # A double[] of len 2*num
    for i in range(num):
        x, y = positions[2*i], positions[2*i+1]
        file.write(pack(x,'float'))
        file.write(pack(y,'float'))
    # A double[] of len 2*num
    for i in range(num):
        x, y = velocities[2*i], velocities[2*i+1]
        file.write(pack(x,'float'))
        file.write(pack(y,'float'))
    # A double[] of len num
    for i in range(num):
        file.write(pack(mus[i],'float'))
    # A str[] of hard-to-predict length
    for name in names[0:num]:
        file.write(pack(name, 'string'))