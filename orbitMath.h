#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "sgp4/libsgp4/Tle.h"
#include "sgp4/libsgp4/SGP4.h"
#include "sgp4/libsgp4/CoordTopocentric.h"
#include "sgp4/libsgp4/CoordGeodetic.h"
#include "sgp4/libsgp4/Eci.h"

using namespace std;


//prototypes:
struct GeoData;
vector<libsgp4::Tle> parse_TLE_file(const string& filename);
void propagateOrbits(const vector<libsgp4::Tle>& satellites);
GeoData toGeodetic(const libsgp4::Eci& posVel, const double& refTime);

vector<string> extractNames(const vector<libsgp4::Tle>& satellites);
void displayTle(const vector<libsgp4::Tle>& v1);
void displayGeoData(const vector<GeoData> g1);


struct GeoData{
    /*A structure to store the Geodetic data
      Tle member ftns return the position in ECI co-ordinates
      While converting to Geodetic, this struct is useful
    */
    short satID;
    double time;
    double lat;
    double lon;
    double alt;

    GeoData(double time, double lat, double lon, double alt): time(time), lat(lat),lon(lon),alt(alt) {}
};
struct OrbitResults{
    /*Now that we have to use this code in python we need a single 
      object type that we can return, OrbitResult is that type*/
    vector<string> names;
    vector<GeoData> shortOrbits;
    vector<GeoData> longOrbits;
};

vector<string> extractNames(const vector<libsgp4::Tle>& satellites){
    vector<string> names;
    for(const auto& sat : satellites){
        names.push_back(sat.Name());
    }
    return names;
}
void fixLength(string& s1){
    const int lineLength = 69;
    string newString;
    for(int i = 0; i < lineLength; ++i){
        newString.push_back(s1[i]);
    }
    s1 = newString;
}
void displayTle(const vector<libsgp4::Tle>& v1){
    for(const auto& sat : v1){
        cout<<sat;
        cout << "-------------------------------------\n";
    }
}
void displayGeoData(const vector<GeoData> g1){
    for(const auto& sat : g1){
        cout<<"ID: "<<sat.satID<<endl;
        cout<<"Time: "<<sat.time<<endl;
        cout<<"Latitude: "<<sat.lat<<endl;
        cout<<"Longitude: "<<sat.lon<<endl;
        cout<<"Altitude: "<<sat.alt<<endl;
        cout<<"----------------------------------"<<endl;
    }
}

vector<libsgp4::Tle> parse_TLE_file(const string& filename) {
    /*Opens the text file and parses it.
      First line contains the name
      Second line contains vital data

      A vector of ligsgp4::Tle is made to store Tle objects.
    */
    ifstream file(filename);
    if (!file) {
        cout << "Error! Unable to open TLE file." << endl;
        return {};
    }

    vector<libsgp4::Tle> TLElist;
    string l1, l2, name, line;

    while (!file.eof()) {
        getline(file, name);
        if (name.empty()) continue;  

        if (!getline(file, l1) || !getline(file, l2)) {
            cout << "Error: Incomplete TLE data for satellite: " << name << endl;
            break;
        }
        fixLength(l1);fixLength(l2);
        TLElist.push_back(libsgp4::Tle(name, l1, l2));
    }
    
    return TLElist;
}
GeoData toGeodetic(const libsgp4::Eci& posVel, const double& refTime){
    /*Convertes ECI(Earth Centered Initial) Co-ordinates to Lat/Long
      Constants:
      1- Earth's Rotation Rate(deg/s): The Earth Rotates 360deg in one day(86164s) 23hrs56m4s
      2- correctionAngle: Greenwich Mean Time correction: ESI assumes the earth is stationary
              after T + x minutes the earth roatates z degrees. If this is not accounted for
              in calculations then we get a wrong value. We calculate rotation from a fixed 
              ref point (Prime Meridian long = 0 deg). 
      3- radialDistance: radial distance in EQ plane (sqrt(x^2 + y^2))
    */

   /*Calculation:
     Longitude: take arctan(y,x). This gives us the counterclockwise angle from +x to (y,x)
              use the correctionAngle to account for rotation
     Latitude: tan^-1(z/r)
     Altitude: distance from earth's center: sqrt(x^2 + y^2 + z^2)
              subtract equatorial radius from this value to obtain satellite's height

                                ******Note******* 
    latitude is geocentric NOT geodetic resulting in some inaccuracy but
                        it can be used in this case.
   */
    const double X = posVel.Position().x;
    const double Y = posVel.Position().y;
    const double Z = posVel.Position().z;
    const double eqRadius = 6378.137;
    const double rotationRate = 360.0 / 86164.0;
    const double correctionAngle = fmod(rotationRate * refTime * 60, 360);
    const double radialDistance = sqrt(X * X + Y * Y);

    double lon_deg = atan2(Y,X) * (180/M_PI);   
    lon_deg = fmod((lon_deg - correctionAngle)+ 360, 360);

    double lat_deg = atan2(Z,radialDistance) * (180/M_PI);

    double altitude = posVel.Position().Magnitude() - eqRadius;

    return(GeoData(refTime, lat_deg, lon_deg, altitude));

    
}
void propagateOrbits(vector<libsgp4::Tle>& satellites, vector<GeoData>& shortOrbits, vector<GeoData>& longOrbits) {
    /*As we are animating the data and providing near real time AND future path display
      we need to compute two sets of data, one with v high precision and one with moderate
      precision. This ensures displayed paths are always accurate and both features can
      be implemented*
    */

    short ID = 0 ;
    for (const auto& sat : satellites) {
        //cout << "\n Propagating: " << sat.Name() << endl;
        libsgp4::SGP4 sgp4(sat);
        
        for (double minutes = 0; minutes <= 90; minutes += 1) {
            libsgp4::Eci posVel = sgp4.FindPosition(minutes);
            GeoData geo = toGeodetic(posVel, minutes);
            geo.satID = ID;
            shortOrbits.push_back(geo);
            
            //cout << "T+" << minutes << " min | Lat: " << geo.lat << " Lon: " << geo.lon << " Alt: " << geo.alt << " km\n";

        }
        for (double minutes = 0; minutes <= 1440; minutes += 10) {
            libsgp4::Eci posVel = sgp4.FindPosition(minutes);
            GeoData geo = toGeodetic(posVel, minutes);
            geo.satID = ID;
            longOrbits.push_back(geo);
            
            //cout << "T+" << minutes << " min | Lat: " << geo.lat << " Lon: " << geo.lon << " Alt: " << geo.alt << " km\n";
        }
        ++ID;
    }

}

OrbitResults runPropagation(const string& filename){
    OrbitResults result;

    vector<libsgp4::Tle> satellites = parse_TLE_file(filename);
    if(satellites.empty()){
        cout<<"No data found\n";
        return{};
    }

    result.names = extractNames(satellites);
    propagateOrbits(satellites, result.shortOrbits, result.longOrbits);

    return result;
}
