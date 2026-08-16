import struct
from astroquery.jplhorizons import Horizons
from astropy.time import Time
from dateutil import parser as dateparser

AU_TO_M = 149597870700.0
DAY_TO_S = 86400.0

# FILE GENERATION SETTINGS

epoch = "January 1 2000 0:00:00" # UTC
epoch = Time(dateparser.parse(epoch)).jd

# Inner planets, barring Earth. Earth is on by default
terrestrial = True
# Phobos, Deimos
mars_moons = True
# Jupiter, Saturn
gas = True
# Moons of Jupiter/Saturn
gas_moons = True
# Neptune, Uranus
ice = True
# Moons of Neptune/Uranus
ice_moons = True
# Asteroid belt objects, like Ceres and Vesta
asteroid_minors = True
# Kuiper belt objects, like Pluto
kuiper_minors = True
# Where to store the file. Note that changing this requires changing the C++ program as well
filepath = "universe.bin"

######

# Some settings don't make sense without their parent setting
mars_moons = mars_moons and terrestrial
if gas_moons and not gas:
    print("Omitting gas giant moons: no gas giants")
gas_moons = gas_moons and gas
if ice and not gas:
    print("Omitting ice giants: no gas giants")
ice = ice and gas
if ice_moons and not ice:
    print("Omitting ice giant moons: no ice giants")
ice_moons = ice_moons and ice
if kuiper_minors and not ice:
    print("Omitting kuiper minors: ice giants not included")
kuiper_minors = kuiper_minors and ice
if asteroid_minors and not (terrestrial and gas):
    print("Omitting asteroid minors: not enough planets")
asteroid_minors = asteroid_minors and terrestrial and gas

# JPL Horizons target IDs for planets and their named moons (NAIF IDs - unambiguous)
HORIZONS_MAJOR_IDS = {
    "Sun": "10",
    "Mercury": "199", "Venus": "299", "Earth": "399", "Moon": "301", "Mars": "499",
    "Phobos": "401", "Deimos": "402",
    "Jupiter": "599", "Io": "501", "Europa": "502", "Ganymede": "503", "Callisto": "504",
    "Saturn": "699", "Mimas": "601", "Enceladus": "602", "Tethys": "603", "Dione": "604",
    "Rhea": "605", "Titan": "606", "Iapetus": "608",
    "Uranus": "799", "Miranda": "705", "Ariel": "701", "Umbriel": "702", "Titania": "703", "Oberon": "704",
    "Neptune": "899", "Proteus": "808", "Triton": "801", "Nereid": "802",
    "Pluto": "999", "Charon": "901",
}
# NAIF ids as well
BARYCENTER_IDS = {
    "Earth": "3", "Mars": "4", "Jupiter": "5", "Saturn": "6", "Uranus": "7", "Neptune": "8",
}
# Standard gravitational parameter (mu = G * M) in m^3/s^2
mus_dict = {
    # Star & Terrestrial Planets + Moon
    "Sun": 1.32712440042e20,
    "Mercury": 2.2032e13,
    "Venus": 3.24859e14,
    "Earth": 3.986004418e14,
    "Moon": 4.9028e12,
    "Mars": 4.282837e13,
    "Phobos": 7.11e5,
    "Deimos": 9.8e4,
    
    # Gas Giants & Ice Giants
    "Jupiter": 1.26686534e17,
    "Saturn": 3.7931187e16,
    "Uranus": 5.793939e15,
    "Neptune": 6.836529e15,
    
    # Jovian Moons
    "Io": 5.9599e12,
    "Europa": 3.2027e12,
    "Ganymede": 9.8878e12,
    "Callisto": 7.1792e12,
    
    # Saturnian Moons
    "Mimas": 2.5e9,
    "Enceladus": 7.21e9,
    "Tethys": 4.12e10,
    "Dione": 7.31e10,
    "Rhea": 1.54e11,
    "Titan": 8.9781e12,
    "Iapetus": 1.205e11,
    
    # Uranian Moons
    "Miranda": 4.4e9,
    "Ariel": 8.92e10,
    "Umbriel": 8.51e10,
    "Titania": 2.269e11,
    "Oberon": 2.053e11,
    
    # Neptunian Moons
    "Proteus": 3.3e9,
    "Triton": 1.4279e12,
    "Nereid": 2.06e9,
    
    # Main-Belt Asteroids
    "Ceres": 6.263e10,
    "Vesta": 1.728e10,
    "Pallas": 1.35e10,
    "Hygiea": 5.8e9,
    "Juno": 1.8e9,
    "Euphrosyne": 1.1e9,
    "Interamnia": 2.2e9,
    "Herculina": 2.0e9,
    
    # Kuiper Belt & TNOs
    "Pluto": 8.719e11,
    "Charon": 1.0588e11,
    "Eris": 1.108e12,
    "Haumea": 2.67e11,
    "Makemake": 2.07e11,
    "Sedna": 6.7e10,
    "Quaoar": 9.3e10,
    "Orcus": 4.2e10
}

# Mean volumetric radius in meters
rads_dict = {
    # Star & Terrestrial Planets + Moon
    "Sun": 6.957e8,
    "Mercury": 2.4397e6,
    "Venus": 6.0518e6,
    "Earth": 6.371e6,
    "Moon": 1.7374e6,
    "Mars": 3.3895e6,
    "Phobos": 1.126e4,
    "Deimos": 6.2e3,
    
    # Gas Giants & Ice Giants
    "Jupiter": 6.9911e7,
    "Saturn": 5.8232e7,
    "Uranus": 2.5362e7,
    "Neptune": 2.4622e7,
    
    # Jovian Moons
    "Io": 1.8216e6,
    "Europa": 1.5608e6,
    "Ganymede": 2.6341e6,
    "Callisto": 2.4103e6,
    
    # Saturnian Moons
    "Mimas": 1.982e5,
    "Enceladus": 2.521e5,
    "Tethys": 5.311e5,
    "Dione": 5.614e5,
    "Rhea": 7.638e5,
    "Titan": 2.5747e6,
    "Iapetus": 7.345e5,
    
    # Uranian Moons
    "Miranda": 2.358e5,
    "Ariel": 5.789e5,
    "Umbriel": 5.847e5,
    "Titania": 7.884e5,
    "Oberon": 7.614e5,
    
    # Neptunian Moons
    "Proteus": 2.10e5,
    "Triton": 1.3534e6,
    "Nereid": 1.70e5,
    
    # Main-Belt Asteroids
    "Ceres": 4.73e5,
    "Vesta": 2.627e5,
    "Pallas": 2.56e5,
    "Hygiea": 2.17e5,
    "Juno": 1.23e5,
    "Euphrosyne": 1.34e5,
    "Interamnia": 1.66e5,
    "Herculina": 1.10e5,
    
    # Kuiper Belt & TNOs
    "Pluto": 1.1883e6,
    "Charon": 6.06e5,
    "Eris": 1.163e6,
    "Haumea": 8.16e5,
    "Makemake": 7.15e5,
    "Sedna": 4.98e5,
    "Quaoar": 5.55e5,
    "Orcus": 4.59e5
}
names = []
# Build the system
names = ["Sun", "Earth", "Moon"]
if terrestrial:
    names.append("Mercury")
    names.append("Venus")
    names.append("Mars")
if mars_moons:
    names.append("Phobos")
    names.append("Deimos")
if gas:
    names.append("Jupiter")
    names.append("Saturn")
if gas_moons:
    names.extend([
        # Jupiter
        "Io", "Europa", "Ganymede", "Callisto",
        # Saturn
        "Mimas", "Enceladus", "Tethys", "Dione", "Rhea", "Titan", "Iapetus"
    ])
if ice:
    names.append("Neptune")
    names.append("Uranus")
if ice_moons:
    names.extend([
        # Uranus
        "Miranda", "Ariel", "Umbriel", "Titania", "Oberon",
        # Neptune
        "Proteus", "Triton", "Nereid"
    ])
if asteroid_minors:
    names.extend([
        "Ceres", "Vesta", "Pallas", "Hygiea", 
        "Juno", "Euphrosyne", "Interamnia", "Herculina"
    ])
if kuiper_minors:
    names.extend([
        "Pluto", "Charon", "Eris", "Haumea", 
        "Makemake", "Sedna", "Quaoar", "Orcus"
    ])
num = len(names)
# Filter out non-included bodies
mus = {name: val for (name, val) in mus_dict.items() if name in names}
rads = {name: val for (name, val) in rads_dict.items() if name in names}
# Rearrange the data
mus = [mus[name] for name in names]
rads = [rads[name] for name in names]

# Create inverse mapping of names to indices
name_dict = {name: i for i, name in enumerate(names)}
# Replacements for the position data
replacements = []

# Corrections if not everything's in
# Corrects the mus so the rough mass is the same, and adds the names to `replacements`, which tracks which bodies are to be replaced with their system barycenters
if not mars_moons and terrestrial:
    mus[name_dict["Mars"]] += mus_dict["Phobos"]
    mus[name_dict["Mars"]] += mus_dict["Deimos"]
    replacements.append("Mars")
if not gas_moons and gas:
    mus[name_dict["Jupiter"]] += mus_dict["Io"]
    mus[name_dict["Jupiter"]] += mus_dict["Europa"]
    mus[name_dict["Jupiter"]] += mus_dict["Ganymede"]
    mus[name_dict["Jupiter"]] += mus_dict["Callisto"]
    replacements.append("Jupiter")

    mus[name_dict["Saturn"]] += mus_dict["Mimas"]
    mus[name_dict["Saturn"]] += mus_dict["Enceladus"]
    mus[name_dict["Saturn"]] += mus_dict["Tethys"]
    mus[name_dict["Saturn"]] += mus_dict["Dione"]
    mus[name_dict["Saturn"]] += mus_dict["Rhea"]
    mus[name_dict["Saturn"]] += mus_dict["Titan"]
    mus[name_dict["Saturn"]] += mus_dict["Iapetus"]
    replacements.append("Saturn")
if not ice_moons and ice:
    mus[name_dict["Uranus"]] += mus_dict["Miranda"]
    mus[name_dict["Uranus"]] += mus_dict["Ariel"]
    mus[name_dict["Uranus"]] += mus_dict["Umbriel"]
    mus[name_dict["Uranus"]] += mus_dict["Titania"]
    mus[name_dict["Uranus"]] += mus_dict["Oberon"]
    replacements.append("Uranus")

    mus[name_dict["Neptune"]] += mus_dict["Proteus"]
    mus[name_dict["Neptune"]] += mus_dict["Triton"]
    mus[name_dict["Neptune"]] += mus_dict["Nereid"]
    replacements.append("Neptune")

# Returns needed information for asking JPL for a body
def horizons_target(name):
    if name in replacements:
        return BARYCENTER_IDS[name], None
    if name in HORIZONS_MAJOR_IDS:
        return HORIZONS_MAJOR_IDS[name], None
    # Small bodies have to be told to go through a different database
    return name, "smallbody"

positions = []
velocities = []
for name in names:
    target_id, id_type = horizons_target(name)
    obj = Horizons(id=target_id, id_type=id_type, location='@0', epochs=epoch)
    vec = obj.vectors()[0]
    positions.extend([
        float(vec['x']) * AU_TO_M,
        float(vec['y']) * AU_TO_M,
        float(vec['z']) * AU_TO_M,
    ])
    velocities.extend([
        float(vec['vx']) * AU_TO_M / DAY_TO_S,
        float(vec['vy']) * AU_TO_M / DAY_TO_S,
        float(vec['vz']) * AU_TO_M / DAY_TO_S,
    ])

# Ensure we didn't mess up generating the arrays *too* much
assert len(positions) == num * 3, "Positions has an incorrect length"
assert len(velocities) == num * 3, "Velocities has an incorrect length"
assert len(mus) == num, "Mus has an incorrect length"
assert len(names) == num, "Rads has an incorrect length"
assert len(rads) == num, "Rads has an incorrect length"

# Shift the reference frame to keep the center of mass and net momentum zero
if num > 0:
    total_mass = sum(mus)
    com = [0, 0, 0]
    mom = [0, 0, 0]
    for i in range(0, len(positions), 3):
        mass = mus[i // 3]
        for j in range(3):
            com[j] += positions[i + j] * mass
            mom[j] += velocities[i + j] * mass
    for i in range(3):
        com[i] /= total_mass
        mom[i] /= total_mass
    for i in range(0, len(positions), 3):
        for j in range(3):
            positions[i + j] -= com[j]
            velocities[i + j] -= mom[j]

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