import struct

# FILE GENERATION SETTINGS

# Include Charon?
major_moon = True
# Include Nix, Hydra, Styx, and Kerberos? (requires major_moon)
minor_moons = True
# Where to store the file. Note that changing this requires changing the C++ program as well
filepath = "universe.bin"

######

# The four minor moons don't make sense without the major one
minor_moons = major_moon and minor_moons

positions = []
velocities = []
mus = []
rads = []
num = 0
# Regardless of settings, this is the order we put bodies into the array
names = ["Pluto", "Charon", "Nix", "Hydra", "Styx", "Kerberos"]

# Build the system
if not major_moon:
    positions = [0, 0, 0]
    velocities = [0, 0, 0]
    mus = [8.71e11]
    rads = [1188000]
    num = 1
else:
    # Pluto and Charon
    bodies = [
        {"pos": [-2112909.83, 0, 0], "vel": [0, -23.9824556, 0], "mu": 8.71e11, "rad": 1188000},
        {"pos": [17527090.2, 0, 0], "vel": [0, 198.940179, 0], "mu": 1.05e11, "rad": 606000}
    ]
    num = 2
    if minor_moons:
        num = 6
        # Nix
        bodies.append({"pos": [48694000, 0, 0], "vel": [0, 141.55, 0], "mu": 3003435, "rad": 25000})
        # Hydra
        bodies.append({"pos": [64738000, 0, 0], "vel": [0, 122.77, 0], "mu": 3203664, "rad": 31000})
        # Styx
        bodies.append({"pos": [42656000, 0, 0], "vel": [0, 151.25, 0], "mu": 500572.5, "rad": 5000})
        # Kerberos
        bodies.append({"pos": [57783000, 0, 0], "vel": [0, 129.95, 0], "mu": 1067888, "rad": 8500})
    # Some hastily written code to go from an easy-to-code AoS to the expected SoA format I want in the binary file
    for body in bodies:
        px, py, pz = body["pos"]
        positions.append(px) ; positions.append(py) ; positions.append(pz)
        vx, vy, vz = body["vel"]
        velocities.append(vx) ; velocities.append(vy) ; velocities.append(vz)
        mus.append(body["mu"])
        rads.append(body["rad"])

# Ensure we didn't mess up generating the arrays *too* much
assert len(positions) == num * 3, "Positions has an incorrect length"
assert len(velocities) == num * 3, "Velocities has an incorrect length"
assert len(mus) == num, "Mus has an incorrect length"
# We don't check names since we crop it out later - this will change in time
assert len(rads) == num, "Rads has an incorrect length"

# Shift the reference frame to keep the center of mass and net momentum zero
# This block is AI generated
if num > 0:
    total_mass = sum(mus)
    com_x = 0.0
    com_y = 0.0
    com_z = 0.0
    mom_x = 0.0
    mom_y = 0.0
    mom_z = 0.0
    for i in range(0, len(positions), 3):
        mass = mus[i // 3]
        x = positions[i + 0]
        y = positions[i + 1]
        z = positions[i + 2]
        vx = velocities[i + 0]
        vy = velocities[i + 1]
        vz = velocities[i + 2]
        com_x += x * mass ; com_y += y * mass ; com_z += z * mass
        mom_x += vx * mass ; mom_y += vy * mass ; mom_z += vz * mass
    com_x /= total_mass
    com_y /= total_mass
    com_z /= total_mass
    vcm_x = mom_x / total_mass
    vcm_y = mom_y / total_mass
    vcm_z = mom_z / total_mass
    for i in range(0, len(positions), 3):
        positions[i + 0] -= com_x
        positions[i + 1] -= com_y
        positions[i + 2] -= com_z
        velocities[i + 0] -= vcm_x
        velocities[i + 1] -= vcm_y
        velocities[i + 2] -= vcm_z
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
    # A double[] of len 3*num
    for i in range(num):
        x, y, z = positions[3*i:3*i+3]
        file.write(pack(x,'float'))
        file.write(pack(y,'float'))
        file.write(pack(z,'float'))
    # A double[] of len 2*num
    for i in range(num):
        x, y, z = velocities[3*i:3*i+3]
        file.write(pack(x,'float'))
        file.write(pack(y,'float'))
        file.write(pack(z,'float'))
    # A double[] of len num
    for i in range(num):
        file.write(pack(mus[i],'float'))
    # A str[] of hard-to-predict length
    for name in names[0:num]:
        file.write(pack(name, 'string'))
    # A double[] of len num (radii in meters)
    for i in range(num):
        file.write(pack(rads[i],'float'))