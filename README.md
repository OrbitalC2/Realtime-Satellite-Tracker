Real-Time Satellite Tracker
===========================

A small C++ ✕ Python utility that

1. downloads up-to-date TLE data from Celestrak,
2. propagates each orbit with the SGP4 algorithm in C++,
3. returns positions in geodetic form (lat / lon / alt),
4. sends the data to Python through a thin wrapper, and
5. lets you pick a satellite in a Tkinter window and watch its ground-track animate with Plotly.

------------------------------------------------------------
Folder layout
------------------------------------------------------------
src/               C++ propagation code (uses libsgp4)
bindings/          pybind11 wrapper (my_sgp4_module)
realtime_gui.py    Python GUI + animation
stations.txt       TLE file (auto-refreshed every 90 min)

------------------------------------------------------------
Quick start
------------------------------------------------------------
# clone with submodules
git clone --recursive https://github.com/<your-user>/sat-tracker.git
cd sat-tracker

# install Python deps
python -m pip install -r requirements.txt   # numpy, requests, plotly, pybind11, etc.

# build C++ engine + wrapper
cmake -S . -B build && cmake --build build --config Release
python -m pip install build/python          # installs my_sgp4_module

# run the GUI
python realtime_gui.py

------------------------------------------------------------
Usage notes
------------------------------------------------------------
* The program refreshes the file _stations.txt_ every 90 minutes; change  
  `FETCH_INTERVAL_MINUTES` in `realtime_gui.py` if you want a different cadence.
* To track other catalogs, just edit the URL inside `fetch_tle_file()`.
* Orbit resolution: 1-min steps for the first 90 min, 10-min steps for 24 h.  
  Adjust the loops in `propagateOrbits()` for finer or coarser output.

------------------------------------------------------------
License
------------------------------------------------------------
MIT

