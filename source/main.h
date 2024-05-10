#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define DEMO_VIDEO_FEATURES 0

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "dark.h"

Vector3 GetCameraRight(Camera *camera);
Vector3 GetCameraForward(Camera *camera);
void CameraMoveForward(Camera *camera, float distance, bool moveInWorldPlane);
void CameraMoveUp(Camera *camera, float distance);
void CameraMoveRight(Camera *camera, float distance, bool moveInWorldPlane);
