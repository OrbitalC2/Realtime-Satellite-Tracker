import tkinter as tk
from tkinter import ttk

import requests
import numpy as np
import plotly.graph_objects as go

import my_sgp4_module

FETCH_INTERVAL_MINUTES = 90

# ---------------------------------------
# Animated Orbit with Accumulating Path
# ---------------------------------------
def animate_satellite_orbit_accumulating(lat_vals, lon_vals, sat_name="My Satellite"):
    """
    Create an animated Plotly figure showing a satellite's orbit with
    an accumulating path. Each frame shows [0..k] of the lat/lon data,
    so the red line grows as time progresses.

    lat_vals, lon_vals: lists (or arrays) of lat/lon.
    """

    # If no data, just return
    if len(lat_vals) < 2:
        print("Not enough points to animate!")
        return

    # Wrap longitude to -180..+180 for continuity, if needed
    lon_vals = [lon - 360 if lon > 180 else lon for lon in lon_vals]

    # 1) Initial figure with the first point
    fig = go.Figure(
        data=[
            go.Scattergeo(
                lat=[lat_vals[0]],
                lon=[lon_vals[0]],
                mode="markers",
                marker=dict(color="red", size=6),
                name=sat_name,
            )
        ],
        layout=go.Layout(
            title_text=f"Animated Orbit (Accumulating): {sat_name}",
            geo=dict(
                projection_type="orthographic",
                showland=True, landcolor="rgb(200, 200, 200)",
                showocean=True, oceancolor="rgb(150, 180, 255)",
                showcountries=True
            ),
            updatemenus=[
                dict(
                    type="buttons",
                    buttons=[
                        dict(
                            label="Play (Slow)",
                            method="animate",
                            args=[None, dict(frame=dict(duration=800), mode="immediate")],
                        ),
                        dict(
                            label="Play (Normal)",
                            method="animate",
                            args=[None, dict(frame=dict(duration=400), mode="immediate")],
                        ),
                        dict(
                            label="Play (Fast)",
                            method="animate",
                            args=[None, dict(frame=dict(duration=100), mode="immediate")],
                        ),
                        dict(
                            label="Pause",
                            method="animate",
                            args=[[None], dict(frame=dict(duration=0, redraw=False), mode="immediate")],
                        ),
                    ]
                )
            ]
        ),
        frames=[
            go.Frame(
                data=[
                    go.Scattergeo(
                        lat=lat_vals[:k+1],
                        lon=lon_vals[:k+1],
                        mode="lines+markers",
                        marker=dict(color="red", size=6),
                        line=dict(color="red", width=2)
                    )
                ],
                name=f"frame{k}",
            )
            for k in range(len(lat_vals))
        ],
    )

    # Show the figure (opens in browser or interactive window)
    fig.show()

# ---------------------------------------
# Fetch & Propagate
# ---------------------------------------
def fetch_tle_file():
    """Fetch the TLE file from Celestrak and write to stations.txt."""
    tle_url = "https://celestrak.org/NORAD/elements/stations.txt"
    response = requests.get(tle_url)

    if response.status_code == 200:
        with open("stations.txt", "w") as file:
            file.write(response.text)
        print("Successfully downloaded TLE data.")
    else:
        print("Failed to fetch TLE data!")

def update_tle_and_propagation():
    """
    1) Fetch updated TLE data.
    2) Re-run the C++ orbit propagation.
    3) Update our orbit_data and the dropdown with new names.
    """
    global orbit_data, satellite_names

    # 1) Fetch new TLE
    fetch_tle_file()

    # 2) Re-run the C++ code to get fresh orbit data
    orbit_data = my_sgp4_module.runOrbitPropagation("stations.txt")

    # 3) Rebuild the satellite name list
    satellite_names = [name.strip() for name in orbit_data.names]

    # Update the combobox values so the user sees new satellites if any
    combo["values"] = satellite_names
    combo.set("Select Satellite")

    print(f"🔄 Updated orbit data with new TLE. Next update in {FETCH_INTERVAL_MINUTES} minutes.\n")

    # Schedule the next fetch
    mainMenu.after(FETCH_INTERVAL_MINUTES * 60 * 1000, update_tle_and_propagation)

# ---------------------------------------
# GUI Callback
# ---------------------------------------
def on_select(_event):
    """Callback when user selects a satellite from the dropdown."""
    selected_sat = combo.get().strip()
    if not selected_sat or selected_sat == "Select Satellite":
        return

    # Find the satellite index
    try:
        sat_index = satellite_names.index(selected_sat)
        print(f"Selected Satellite: {selected_sat}, Index: {sat_index}")
    except ValueError:
        print("Selected satellite not found in names.")
        return

    # Filter out orbit data for this specific satellite's ID
    short_orbits_for_sat = [pt for pt in orbit_data.shortOrbit if pt.satID == sat_index]

    if not short_orbits_for_sat:
        print("No short orbit data found for this satellite.")
        return

    # Collect lat/lon
    lat_vals = [pt.lat for pt in short_orbits_for_sat]
    lon_vals = [pt.lon for pt in short_orbits_for_sat]

    # Animate with an accumulating path
    animate_satellite_orbit_accumulating(lat_vals, lon_vals, sat_name=selected_sat)

# ---------------------------------------
# Main GUI Code
# ---------------------------------------
mainMenu = tk.Tk()
mainMenu.title("Satellite Menu")
mainMenu.geometry("400x200")

# 1) Fetch new TLE + run propagation the FIRST time
fetch_tle_file()
orbit_data = my_sgp4_module.runOrbitPropagation("stations.txt")
satellite_names = [name.strip() for name in orbit_data.names]

# 2) Create a Combobox with the satellite names
combo = ttk.Combobox(mainMenu, values=satellite_names, state="readonly")
combo.set("Select Satellite")
combo.pack(pady=20)

# 3) Bind a callback to handle selection
combo.bind("<<ComboboxSelected>>", on_select)

# 4) Schedule the TLE update + re-propagation in X minutes
mainMenu.after(FETCH_INTERVAL_MINUTES * 60 * 1000, update_tle_and_propagation)

# 5) Start the event loop
mainMenu.mainloop()
