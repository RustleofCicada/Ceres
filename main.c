// ==========================
/*
 *	Name: Ceres
 *	Description: Planet simulator with 3D experience
 *	Author: Damian Legutko
 *  Version: 1.0
 *	Date: 21.01.2021
*/
// ==========================


#include "winbgi2.h"
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>


// Vector3
typedef struct
{
    double x;
    double y;
    double z;
} Vector3;
const Vector3 v3_zero = { 0, 0, 0 };
const Vector3 v3_unit = { 1, 1, 1 };
double distance(Vector3 point1, Vector3 point2)
{
    return sqrt(pow(point1.x - point2.x, 2) + pow(point1.y - point2.y, 2) + pow(point1.z - point2.z, 2));
}
double v3_length(Vector3 v)
{
    return distance(v3_zero, v);
}
Vector3 v3_add(Vector3 v1, Vector3 v2)
{
    v1.x += v2.x;
    v1.y += v2.y;
    v1.z += v2.z;
    return v1;
}
Vector3 v3_subtract(Vector3 v1, Vector3 v2)
{
    v1.x -= v2.x;
    v1.y -= v2.y;
    v1.z -= v2.z;
    return v1;
}
Vector3 v3_scale(Vector3 v, double a)
{
    v.x *= a;
    v.y *= a;
    v.z *= a;
    return v;
}
Vector3 azimuth(Vector3 point2, Vector3 point1)
{
    point1.x -= point2.x;
    point1.y -= point2.y;
    point1.z -= point2.z;
    point1 = v3_scale(point1, 1 / v3_length(point1));
    return point1;
}
bool v3_equal(Vector3 v1, Vector3 v2)
{
    return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);
}


// Physics
typedef struct
{
    Vector3 position;
    Vector3 velocity;
    Vector3 force;

    double mass;
} Planet;
double gravity_constant = 1;
double time_warp = 1;
double last_time_warp = 1;
SYSTEMTIME sys_time;
LONG millis = 0;
LONG last_millis = 0;
int planet_count = 3;
Planet* planets;
void attract(Planet* system, int count)
{
    double dist = 0;
    double pull = 0;
    for (int i = 0; i < count; i++)
    {
        system[i].force.x = 0;
        system[i].force.y = 0;
        system[i].force.z = 0;
        for (int j = 0; j < count; j++)
        {
            if (j != i)
            {
                dist = distance(system[i].position, system[j].position);
                pull = gravity_constant * system[i].mass * system[j].mass / pow(dist, 2);
                
                system[i].force = v3_add(system[i].force, v3_scale(azimuth(system[i].position, system[j].position), pull));
            }
        }
    }
}
void move(Planet* system, int count)
{
    for (int i = 0; i < count; i++)
    {
        system[i].velocity = v3_add(system[i].velocity, v3_scale(system[i].force, time_warp / system[i].mass));
        system[i].position = v3_add(system[i].position, v3_scale(system[i].velocity, time_warp));
    }
}
void update_millis()
{
    last_millis = millis;
    GetSystemTime(&sys_time);
    millis = (sys_time.wSecond * 1000) + sys_time.wMilliseconds;
}


// Rendering
#define M_PI 3.14159265358979323846
Vector3 camera_pos = { 0, 0, 500 };
double camera_zoom = 100;
double camera_fov = 25;
double camera_ax = 0;
double camera_ay = 0;
double last_camera_ax = 0;
double last_camera_ay = 0;
bool saving_tracks = false;
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 700
int get_screen_width() { return getmaxx() + 1; }
int get_screen_height() { return getmaxy() + 1; }
Vector3 project(Vector3 v, double size)
{
    Vector3 temp = { 0 };

    v = v3_subtract(v, camera_pos); // Applying translation origin

    // Rotation about OY axis
    temp.x = cos(camera_ay) * v.x - sin(camera_ay) * v.z;
    temp.y = v.y;
    temp.z = sin(camera_ay) * v.x + cos(camera_ay) * v.z;

    // Rotation about OX axis
    v.x = temp.x;
    v.y = cos(camera_ax) * temp.y + sin(camera_ax) * temp.z;
    v.z = cos(camera_ax) * temp.z - sin(camera_ax) * temp.y;

    v = v3_add(v, camera_pos);  // De-applying translation origin

    // Calculating depth
    double depth = (camera_zoom / tan((camera_fov / 2.0) * (M_PI / 180.0))) / (camera_pos.z - v.z);
    if (v.z > camera_pos.z) { v.x *= -1; v.y *= -1; }       // Ummm, it works somehow (solves line "overflow")

    // Projecting on screen
    v.x = depth * (v.x - camera_pos.x) + (get_screen_width() / 2.);
    v.y = depth * (v.y - camera_pos.y) * (-1.0) + (get_screen_height() / 2.);
    if (v.x < 0 || v.x > get_screen_width() ||
        v.y < 0 || v.y > get_screen_height()) v.z = 0;       // Object behind the camera or outside the screen, returning 0 size
    else
    {
        v.z = depth * 1.5 * pow(size, 1. / 3.);
        v.z = max(1, v.z);
    }

    return v;
}
void render(Planet* system, int count)
{
    // Planets
    for (int i = 0; i < count; i++)
    {
        Vector3 projection = project(system[i].position, system[i].mass);
        if (projection.z > 0)
        {
            circle((int)projection.x, (int)projection.y, (int)projection.z);
            /* // Colored planets
            setfillstyle(1, i * 16 + 8);
            fillellipse((int)projection.x, (int)projection.y, (int)projection.z, (int)projection.z);
            setfillstyle(0, 0);
            */
        }
    }

    // Floor
    Vector3 corner = { -200, -200, -200 };
    Vector3 corner2 = { -200, -200, 200 };
    for (int i = 0; i <= 5; i++)
    {
        Vector3 proj = project(corner, 1);
        Vector3 proj2 = project(corner2, 1);
        line((int)proj.x, (int)proj.y, (int)proj2.x, (int)proj2.y);
        corner.x += 80;
        corner2.x += 80;
    }
    Vector3 corner3 = { -200, -200, -200 };
    Vector3 corner4 = { 200, -200, -200 };
    for (int i = 0; i <= 5; i++)
    {
        Vector3 proj3 = project(corner3, 1);
        Vector3 proj4 = project(corner4, 1);
        line((int)proj3.x, (int)proj3.y, (int)proj4.x, (int)proj4.y);
        corner3.z += 80;
        corner4.z += 80;
    }

    // GUI
    // GUI - Cross
    setlinestyle(1, 1, 1);
    int x_c = get_screen_width() / 2;
    int y_c = get_screen_height() / 2;
    line(x_c - 7, y_c, x_c + 7, y_c);
    line(x_c, y_c - 7, x_c, y_c + 7);
    setlinestyle(0, 1, 1);
    // GUI - Info
    char str[256] = { 0 };
    sprintf_s(str, 255, "Pos: %.1f \t %.1f \t %.1f\r\n", camera_pos.x, camera_pos.y, camera_pos.z);
    outtextxy(30, 30, str);
    sprintf_s(str, 255, "Rot: %.3f* \t %.3f*\r\n", camera_ax / (M_PI / 180.0), camera_ay / (M_PI / 180.0));
    outtextxy(30, 50, str);
    sprintf_s(str, 255, "View: %.0f%% \t %.2f*\r\n", camera_zoom, camera_fov);
    outtextxy(30, 70, str);
    sprintf_s(str, 255, "Physics: %.3f \t %.0f%%\r\n", gravity_constant, time_warp * 100.0);
    outtextxy(30, 90, str);
    sprintf_s(str, 255, saving_tracks ? "track saving active" : "track saving not active" );
    outtextxy(30, 110, str);
}

// File management
FILE* track_file = 0;
void ask_for_file(char* str, bool to_save)
{
    cleardevice();
    outtextxy(get_screen_width() / 2 - 64, get_screen_height() / 2 - 10, to_save ? "======SAVE======" : "======LOAD======");
    outtextxy(get_screen_width() / 2 - 104, get_screen_height() / 2 + 10, "WRITE FILE PATH IN CONSOLE");
    printf("Write file path here:\n"); scanf_s("%s", str, 255);
}
bool save_state(char* filepath)
{
    printf("Saving to: %s\r\n", filepath);
    cleardevice();
    outtextxy(get_screen_width() / 2 - 25, get_screen_height() / 2, "SAVING...");
    delay(1000);

    FILE* state_file = 0;
    if (fopen_s(&state_file, filepath, "w")) { printf("File: [%s] could not be opened!\r\n", filepath); return false; }

    fprintf_s(state_file, "%d\n", saving_tracks);
    fprintf_s(state_file, "%.3f %.3f\n", gravity_constant, time_warp);
    fprintf_s(state_file, "%d\n", planet_count);
    
    for (int i = 0; i < planet_count; i++)
    {
        fprintf_s(state_file, "%.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
            planets[i].position.x, planets[i].position.y, planets[i].position.z,
            planets[i].velocity.x, planets[i].velocity.y, planets[i].velocity.z, planets[i].mass);
    }

    fprintf_s(state_file, "%.2f %.2f %.2f\n", camera_pos.x, camera_pos.y, camera_pos.z);
    fprintf_s(state_file, "%.3f %.3f\n", camera_ax, camera_ay);
    fprintf_s(state_file, "%.3f %.3f\n", camera_zoom, camera_fov);

    fclose(state_file);
    return true;
}
bool load_state(char* filepath)
{
    printf("Loading: %s\r\n", filepath);
    cleardevice();
    outtextxy(get_screen_width() / 2 - 25, get_screen_height() / 2, "LOADING...");
    delay(1000);

    char file_line[256] = { 0 };
    FILE* state_file = 0;
    if (fopen_s(&state_file, filepath, "r")) { printf("File: [%s] could not be opened!\r\n", filepath); return false; }

    fscanf_s(state_file, "%d", &saving_tracks);
    fscanf_s(state_file, "%lf %lf", &gravity_constant, &time_warp);
    fscanf_s(state_file, "%d", &planet_count);
    
    // Planet array allocation
    planet_count = min(15, planet_count);
    free(planets);
    planets = (Planet*)malloc(sizeof(Planet) * planet_count);
    
    for (int i = 0; i < planet_count; i++)
    {
        fscanf_s(state_file, "%lf %lf %lf %lf %lf %lf %lf",
            &planets[i].position.x, &planets[i].position.y, &planets[i].position.z,
            &planets[i].velocity.x, &planets[i].velocity.y, &planets[i].velocity.z, &planets[i].mass);
    }

    fscanf_s(state_file, "%lf %lf %lf", &camera_pos.x, &camera_pos.y, &camera_pos.z);
    fscanf_s(state_file, "%lf %lf", &camera_ax, &camera_ay);
    fscanf_s(state_file, "%lf %lf", &camera_zoom, &camera_fov);

    // Final touches
    last_time_warp = time_warp;
    last_camera_ax = camera_ax;
    last_camera_ay = camera_ay;

    fclose(state_file);
    return true;
}
bool start_track_save()
{
    if (!saving_tracks)
    {
        // Ask for file
        printf("Type file path for tracks to be saved to\r\n");
        char filepath[256] = { 0 };
        ask_for_file(filepath, true);

        printf("Saving track to: %s\r\n", filepath);
        cleardevice();
        outtextxy(get_screen_width() / 2 - 25, get_screen_height() / 2, "SAVING...");
        delay(1000);

        if (fopen_s(&track_file, filepath, "w"))
        {
            track_file = 0;
            printf("File: [%s] could not be opened!\r\n", filepath);
            return false;
        }
        saving_tracks = true;
    }
    return true;
}
void save_tracks()
{
    if (track_file != 0 && saving_tracks)
    {
        fprintf_s(track_file, "@%ld\n", millis);
        for (int i = 0; i < planet_count; i++)
        {
            fprintf_s(track_file, "%.2f %.2f %.2f\n", planets[i].position.x, planets[i].position.y, planets[i].position.z);
        }
    }
}
void end_track_save()
{
    if (track_file != 0)
    {
        fclose(track_file);
    }
    saving_tracks = false;
}


// User
bool closed = false;
bool mouse_is_down = false;
#define KEY_SPACE 0x20
#define KEY_SHIFT 0x10
#define KEY_ESC 0x1B
#define KEY_LEFT 0x25
#define KEY_UP 0x26
#define KEY_RIGHT 0x27
#define KEY_DOWN 0x28
void handle_keys()
{
    if (kbhit())
    {
        Vector3 forward = { sin(camera_ay) * 10 * cos(camera_ax), (-10.0) * sin(camera_ax), cos(camera_ay) * 10 * cos(camera_ax) };
        Vector3 rightward = { cos(camera_ay) * 10, 0 , sin(camera_ay) * (-10.0) };
        char input_path[256] = { 0 };
        switch ((char)getkey())
        {
            case '=': camera_zoom += 10; break;
            case '-': camera_zoom -= 10; break;
            case ']': camera_fov += 1; break;
            case '[': camera_fov -= 1; break;
            case 'w': camera_pos = v3_subtract(camera_pos, forward); break;
            case 's': camera_pos = v3_add(camera_pos, forward); break;
            case 'a': camera_pos = v3_subtract(camera_pos, rightward); break;
            case 'd': camera_pos = v3_add(camera_pos, rightward); break;
            case KEY_SPACE: camera_pos.y += 10; break;
            case KEY_SHIFT: camera_pos.y -= 10; break;
            case KEY_LEFT: camera_ay += 0.04; last_camera_ay = camera_ay; break;
            case KEY_RIGHT: camera_ay -= 0.04; last_camera_ay = camera_ay; break;
            case KEY_UP: camera_ax += 0.04; last_camera_ax = camera_ax; break;
            case KEY_DOWN: camera_ax -= 0.04; last_camera_ax = camera_ax; break;
            case 'v': time_warp -= 0.05; break;
            case 'b': time_warp += 0.05; break;
            case 'g': gravity_constant += 0.01; break;
            case 'f': gravity_constant -= 0.01; break;
            case KEY_ESC: closed = true; break;
            case '1': ask_for_file(input_path, true); save_state(input_path); break;
            case '2': ask_for_file(input_path, false); load_state(input_path); break;
            case '3': start_track_save(); break;
            case '4': end_track_save(); break;
            default: break;
        }
    }

    if (mousedown()) mouse_is_down = true;
    else if (mouseup()) { mouse_is_down = false; last_camera_ax = camera_ax; last_camera_ay = camera_ay; }

    if (mouse_is_down)
    {
        camera_ax = last_camera_ax + (mouseclicky() - mousecurrenty()) * (-0.1) / 100.0;
        camera_ay = last_camera_ay + (mouseclickx() - mousecurrentx()) * (-0.1) / 100.0;
    }
}

// Main
void loop()
{
    animate(60);
    handle_keys();
    if (time_warp != 0)
    {
        update_millis();
        attract(planets, planet_count);
        move(planets, planet_count);
        save_tracks();
    }
    cleardevice();
    render(planets, planet_count);
}

int main()
{
    graphics(SCREEN_WIDTH + 8, SCREEN_HEIGHT + 12);

    if (load_state("basic_state.txt") && saving_tracks)
    {
        char yn = 'n';
        saving_tracks = false;
        printf("Do you want to save planet tracks? [y / n]: "); scanf_s("%c", &yn);
        if (yn == 'y') start_track_save();
    }

    while (!closed) { loop(); }

    end_track_save();

    return 0;
}