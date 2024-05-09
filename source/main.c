#include "main.h"
#include "additions.c"

#include "shaders.h"


int main(void){
	const int FPS = 60;
	const int WINDOW_W = 1940 / 2, WINDOW_H = 1100 / 2;
	// SetTraceLogLevel(LOG_ERROR);
	lastSave = GetTime();
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(WINDOW_W, WINDOW_H, "ShapeUp!");
	SetExitKey(0);

	GuiLoadStyleDark();
	// Font gfont = GuiGetFont();
	// GenTextureMipmaps(&guiFont.texture);
	// SetTextureFilter(guiFont.texture, TEXTURE_FILTER_POINT);
	// GuiSetFont(gfont);
	GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);

	camera.position = (Vector3){ 2.5f, 2.5f, 3.0f };
	camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };  
	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };      
	camera.fovy = 55.0f;                            
	camera.projection = CAMERA_PERSPECTIVE;

	spheres[0] = (Sphere){
		.size = {1,1,1}, 
	.color={
		last_color_set.r,
			last_color_set.g,
			last_color_set.b
		}};

	float runTime = 0.0f;
	SetTargetFPS(FPS);

	void swizzleWindow(void);
	swizzleWindow();
	void makeWindowKey(void);
	makeWindowKey();

	const int gamepad = 0;

	bool ui_mode_gamepad = false;

	while (!WindowShouldClose()) {

		if (fabsf(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X)) > 0 || 
			fabsf(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y)) > 0 ||
			fabsf(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X)) > 0 ||
			fabsf(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y)) > 0) {
			ui_mode_gamepad = true;
		} 

		if (Vector2Length(GetMouseDelta()) > 0) {
			ui_mode_gamepad = false;
		}

		#if DEMO_VIDEO_FEATURES
		if (IsKeyPressed(KEY_F)) {
			false_color_mode = !false_color_mode;
			needs_rebuild = true;
		}
		#endif


		if (GetTime() - lastSave > 60) {
			save("snapshot");
		}

		if (IsFileDropped()) {
			FilePathList droppedFiles = LoadDroppedFiles();
			if (droppedFiles.count > 0) {

				openSnapshot(droppedFiles.paths[0]);
			}
			UnloadDroppedFiles(droppedFiles);
		}


		Ray ray = GetMouseRay(GetMousePosition(), camera);

		static Sphere before_edit;
		if (focusedControl == CONTROL_NONE) {
			if (mouseAction == CONTROL_NONE && selected_sphere >= 0 && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)|| IsKeyPressed(KEY_X))) {
				delete_sphere(selected_sphere);
			}

			if (selected_sphere >= 0 && IsKeyPressed(KEY_D) && (IsKeyDown(KEY_RIGHT_SUPER) || IsKeyDown(KEY_LEFT_SUPER)) && num_spheres < MAX_SPHERES) {
				spheres[num_spheres] = spheres[selected_sphere];
				selected_sphere = num_spheres;
				num_spheres++;
				needs_rebuild = true;
			}

			if (IsKeyPressed(KEY_A)) {
				add_shape();
			}

			
			if (selected_sphere >= 0 && IsKeyPressed(KEY_G)) {
				mouseAction = CONTROL_TRANSLATE;
				last_axis_set = 0;
				controlled_axis.mask = 0x7;
				before_edit = spheres[selected_sphere];
			}
			if (selected_sphere >= 0 && IsKeyPressed(KEY_R)) {
				mouseAction = CONTROL_ROTATE;
				last_axis_set = 0;
				controlled_axis.mask = 0x7;
				before_edit = spheres[selected_sphere];
			}
			if (selected_sphere >= 0 && IsKeyPressed(KEY_S)) {
				mouseAction = CONTROL_SCALE;
				last_axis_set = 0;
				controlled_axis.mask = 0x7;
				before_edit = spheres[selected_sphere];
			}
		}

		if (selected_sphere >= 0 && (mouseAction == CONTROL_TRANSLATE || mouseAction == CONTROL_ROTATE || mouseAction == CONTROL_SCALE)) {
			bool should_set = GetTime() - last_axis_set > 1;
			if (IsKeyPressed(KEY_X)) {
				controlled_axis.mask =  (should_set || !(controlled_axis.mask ^ 1)) ? 1 : (controlled_axis.mask ^ 1);
				last_axis_set = GetTime();
				spheres[selected_sphere] = before_edit;
			}
			if (IsKeyPressed(KEY_Y)) {
				controlled_axis.mask = (should_set || !(controlled_axis.mask ^ 2)) ? 2 : (controlled_axis.mask ^ 2);
				last_axis_set = GetTime();
				spheres[selected_sphere] = before_edit;
			}
			if (IsKeyPressed(KEY_Z)) {
				controlled_axis.mask = (should_set || !(controlled_axis.mask ^ 4)) ? 4 : (controlled_axis.mask ^ 4);
				last_axis_set = GetTime();
				spheres[selected_sphere] = before_edit;
			}

			if (mouseAction == CONTROL_TRANSLATE) {
			
				if (controlled_axis.x + controlled_axis.y + controlled_axis.z == 1) {
					Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos, 
															   Vector3Add(spheres[selected_sphere].pos, 
																		  (Vector3){controlled_axis.x ,
																		  controlled_axis.y ,
																	  controlled_axis.z }),
															   ray.position, 
															   Vector3Add(ray.position, ray.direction));

					spheres[selected_sphere].pos = nearest;
				} else {
					Vector3 plane_normal;
					Vector3 intersection;
					if(controlled_axis.x + controlled_axis.y + controlled_axis.z == 2) {
						plane_normal = (Vector3){!controlled_axis.x, !controlled_axis.y, !controlled_axis.z};
					} else {
						plane_normal = Vector3Subtract(camera.position, camera.target);
					}

					if(RayPlaneIntersection(ray.position, ray.direction, spheres[selected_sphere].pos, plane_normal, &intersection)) {
						spheres[selected_sphere].pos = intersection;
					}
				}

			} else if (mouseAction == CONTROL_ROTATE) {
				if (controlled_axis.x) spheres[selected_sphere].angle.x += GetMouseDelta().x/10.f;
				if (controlled_axis.y) spheres[selected_sphere].angle.y += GetMouseDelta().x/10.f;
				if (controlled_axis.z) spheres[selected_sphere].angle.z += GetMouseDelta().x/10.f;
			} else if (mouseAction == CONTROL_SCALE) {
				if (controlled_axis.x) spheres[selected_sphere].size.x *= powf(2, GetMouseDelta().x/10.f);
				if (controlled_axis.y) spheres[selected_sphere].size.y *= powf(2, GetMouseDelta().x/10.f);
				if (controlled_axis.z) spheres[selected_sphere].size.z *= powf(2, GetMouseDelta().x/10.f);
			}

			if (IsKeyPressed(KEY_ESCAPE)) {
				mouseAction = CONTROL_NONE;
				spheres[selected_sphere] = before_edit;
			}
			if (IsKeyPressed(KEY_ENTER)) mouseAction = CONTROL_NONE;
		}

		static float drag_offset;
		
		if (GetMousePosition().x > sidebar_width) {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouseAction == CONTROL_NONE && selected_sphere >= 0) {
				if (GetRayCollisionSphere(ray, Vector3Add(spheres[selected_sphere].pos, (Vector3){0.6,0,0}), .1).hit) {
					Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos,
														 Vector3Add(spheres[selected_sphere].pos, (Vector3){1,0,0}),
														 ray.position,
														 Vector3Add(ray.position, ray.direction));

					drag_offset = spheres[selected_sphere].pos.x - nearest.x;
					mouseAction = CONTROL_POS_X;
				} else if (GetRayCollisionSphere(ray, Vector3Add(spheres[selected_sphere].pos, (Vector3){0,0.6,0}), .1).hit) {
					Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos,
														 Vector3Add(spheres[selected_sphere].pos, (Vector3){0,1,0}),
														 ray.position,
														 Vector3Add(ray.position, ray.direction));

					drag_offset = spheres[selected_sphere].pos.y - nearest.y;
					mouseAction = CONTROL_POS_Y;
				} else if (GetRayCollisionSphere(ray, Vector3Add(spheres[selected_sphere].pos, (Vector3){0,0,0.6}), .1).hit) {
					Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos,
														 Vector3Add(spheres[selected_sphere].pos, (Vector3){0,0,1}),
														 ray.position,
														 Vector3Add(ray.position, ray.direction));

					drag_offset = spheres[selected_sphere].pos.z - nearest.z;
					mouseAction = CONTROL_POS_Z;
				}
				else if (GetRayCollisionBox(ray, boundingBoxSized(Vector3Add(spheres[selected_sphere].pos,
																			 (Vector3){spheres[selected_sphere].size.x,0,0}), 0.2)).hit) {
					Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos,
														 Vector3Add(spheres[selected_sphere].pos, (Vector3){1,0,0}),
														 ray.position,
														 Vector3Add(ray.position, ray.direction));

				drag_offset = nearest.x - spheres[selected_sphere].size.x;
				mouseAction = CONTROL_SCALE_X;
			} else if (GetRayCollisionBox(ray, boundingBoxSized(Vector3Add(spheres[selected_sphere].pos,
																		   (Vector3){0,spheres[selected_sphere].size.y,0}), 0.2)).hit) {
				Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos,
													 Vector3Add(spheres[selected_sphere].pos, (Vector3){0,1,0}),
													 ray.position,
													 Vector3Add(ray.position, ray.direction));

				drag_offset = nearest.y - spheres[selected_sphere].size.y;
				mouseAction = CONTROL_SCALE_Y;
			} else if (GetRayCollisionBox(ray, boundingBoxSized(Vector3Add(spheres[selected_sphere].pos,
																		   (Vector3){0,0,spheres[selected_sphere].size.z}), 0.2)).hit) {
				Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos,
													 Vector3Add(spheres[selected_sphere].pos, (Vector3){0,0,1}),
													 ray.position,
													 Vector3Add(ray.position, ray.direction));

				drag_offset = nearest.z - spheres[selected_sphere].size.z;
				mouseAction = CONTROL_SCALE_Z;
			}
			}

			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && mouseAction == CONTROL_NONE) {
				int new_selection = object_at_pixel((int)GetMousePosition().x, (int)GetMousePosition().y);
				if (new_selection != selected_sphere) {
					selected_sphere = new_selection;
					needs_rebuild = true;
				}
			}

			static Vector2 mouseDownPosition;
			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) mouseDownPosition = GetMousePosition();

			if (fabsf(GetMouseWheelMove()) > 0.01 && mouseAction == CONTROL_NONE ) {
				Vector2 delta = GetMouseWheelMoveV();

				if (IsKeyDown(KEY_LEFT_ALT)) {
					CameraMoveForward(&camera, delta.y, false);
				} else {
					Vector3 shift = Vector3Scale(camera.up, delta.y/10);
					camera.position = Vector3Add(camera.position, shift);
					camera.target = Vector3Add(camera.target,shift );
				
					UpdateCameraPro(&camera, (Vector3){0, -delta.x/10, 0}, Vector3Zero(), 0);
				}
			} else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !guiSliderDragging && mouseAction == CONTROL_NONE && Vector2Distance(mouseDownPosition, GetMousePosition()) > 1) {
				mouseAction = CONTROL_ROTATE_CAMERA;
			}

			if (mouseAction == CONTROL_ROTATE_CAMERA) {
				if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) mouseAction = CONTROL_NONE;

				Vector2 delta = GetMouseDelta();
				if (IsKeyDown(KEY_LEFT_ALT)) {
					UpdateCameraPro(&camera, (Vector3){0, -delta.x/80, delta.y/80}, Vector3Zero(), 0);
				} else {
					extern void CameraYaw(Camera *camera, float angle, bool rotateAroundTarget);
					extern void CameraPitch(Camera *camera, float angle, bool lockView, bool rotateAroundTarget, bool rotateUp);
					CameraYaw(&camera, -delta.x*0.003f, true);
					CameraPitch(&camera, -delta.y*0.003f, true, true, false);
				}
			}

			extern float magnification;
			if (mouseAction == CONTROL_NONE ) {
				CameraMoveForward(&camera, 8*magnification, false);
			}
			magnification = 0;
		}

		
		const float movement_scale = 0.1;
		const float rotation_scale = 0.04;
		const float rotation_radius = 4;
		

		if (!IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_THUMB)) {
			Vector3 offset = Vector3Scale(GetCameraForward(&camera), -rotation_radius);
			offset = Vector3RotateByAxisAngle(offset, (Vector3){0,1,0}, -rotation_scale*GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X));
			offset = Vector3RotateByAxisAngle(offset, GetCameraRight(&camera), -rotation_scale*GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y));
			camera.position = Vector3Add(offset, camera.target);
		}

		if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
			Vector3 up = Vector3Normalize(Vector3CrossProduct(GetCameraForward(&camera), GetCameraRight(&camera)));
			up = Vector3Scale(up, movement_scale*GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y));
			camera.position = Vector3Add(camera.position, up);
			camera.target = Vector3Add(camera.target, up);
		} else {
			CameraMoveForward(&camera, -GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y)*movement_scale, false);
		}

		CameraMoveRight(&camera, GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X)*movement_scale, false);

		Matrix camera_matrix = GetCameraMatrix(camera);
		static Vector3 camera_space_offset;
		if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
			selected_sphere = object_at_pixel(sidebar_width + (GetScreenWidth()-sidebar_width)/2, GetScreenHeight()/2);
			needs_rebuild = true;
			if (selected_sphere>= 0){
				camera_space_offset = WorldToCamera(spheres[selected_sphere].pos, camera_matrix);
			}
		}

		if (selected_sphere >= 0) {
			if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
				spheres[selected_sphere].pos = CameraToWorld(camera_space_offset, camera_matrix);
				// spheres[selected_sphere].pos = Vector3Add(camera.position, Vector3Scale(GetCameraForward(&camera),distance));
			}

			if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
				spheres[selected_sphere].size = Vector3Scale(spheres[selected_sphere].size, 1.05);
				spheres[selected_sphere].corner_radius *= 1.05;
			}
			if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
				spheres[selected_sphere].size = Vector3Scale(spheres[selected_sphere].size, 0.95);
				spheres[selected_sphere].corner_radius *= 0.95;
			}


			if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
				if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
					spheres[selected_sphere].blob_amount *= 0.95;
				}
				if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
					spheres[selected_sphere].blob_amount = (0.01 + spheres[selected_sphere].blob_amount*1.05);
				}
			} else {
				if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) {
					spheres[selected_sphere].corner_radius *= 0.95;
				}
				if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) {
					Vector3 size = spheres[selected_sphere].size;
					spheres[selected_sphere].corner_radius = fminf(0.01 + spheres[selected_sphere].corner_radius*1.05, fminf(size.x, fminf(size.y,size.z)));
				}
			}

			if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_THUMB)) {
				spheres[selected_sphere].angle = Vector3Add(spheres[selected_sphere].angle, (Vector3){
					rotation_scale*GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y),
					rotation_scale*GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X),
					0,
				});
			}
		}
		
		if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
			if (IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
				add_shape();
			}

			if (selected_sphere>=0) spheres[selected_sphere].pos = Vector3Add(camera.position, Vector3Scale(GetCameraForward(&camera),8));
		}

		if (mouseAction == CONTROL_POS_X) {
			Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos, 
													   Vector3Add(spheres[selected_sphere].pos, (Vector3){1,0,0}),
													   ray.position, 
													   Vector3Add(ray.position, ray.direction));

			spheres[selected_sphere].pos.x = nearest.x + drag_offset;
		} else if (mouseAction == CONTROL_POS_Y) {
			Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos, 
													   Vector3Add(spheres[selected_sphere].pos, (Vector3){0,1,0}),
													   ray.position, 
													   Vector3Add(ray.position, ray.direction));
			spheres[selected_sphere].pos.y = nearest.y + drag_offset;
		} else if (mouseAction == CONTROL_POS_Z) {
			Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos, 
													   Vector3Add(spheres[selected_sphere].pos, (Vector3){0,0,1}),
													   ray.position, 
													   Vector3Add(ray.position, ray.direction));
			spheres[selected_sphere].pos.z = nearest.z + drag_offset;
		} else if (mouseAction == CONTROL_SCALE_X) {
			Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos, 
													   Vector3Add(spheres[selected_sphere].pos, (Vector3){1,0,0}),
													   ray.position, 
													   Vector3Add(ray.position, ray.direction));

			spheres[selected_sphere].size.x = fmaxf(0,nearest.x-drag_offset);
		} else if (mouseAction == CONTROL_SCALE_Y) {
			Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos, 
													   Vector3Add(spheres[selected_sphere].pos, (Vector3){0,1,0}),
													   ray.position, 
													   Vector3Add(ray.position, ray.direction));

			spheres[selected_sphere].size.y = fmaxf(0,nearest.y-drag_offset);
		} else if (mouseAction == CONTROL_SCALE_Z) {
			Vector3 nearest = NearestPointOnLine(spheres[selected_sphere].pos, 
													   Vector3Add(spheres[selected_sphere].pos, (Vector3){0,0,1}),
													   ray.position, 
													   Vector3Add(ray.position, ray.direction));

			spheres[selected_sphere].size.z = fmaxf(0,nearest.z-drag_offset);
		} 
		  
		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			mouseAction = CONTROL_NONE;
		}

		//
		// Mark: Drawing!
		//

		float deltaTime = GetFrameTime();
		runTime += deltaTime;

		if ( needs_rebuild ) {
			rebuild_shaders();
		}
		SetShaderValue(main_shader, main_locations.viewEye, &camera.position, SHADER_UNIFORM_VEC3);
		SetShaderValue(main_shader, main_locations.viewCenter, &camera.target, SHADER_UNIFORM_VEC3);
		SetShaderValue(main_shader, main_locations.resolution, (float[2]){ (float)GetScreenWidth()*GetWindowScaleDPI().x, (float)GetScreenHeight()*GetWindowScaleDPI().y }, SHADER_UNIFORM_VEC2);
		SetShaderValue(main_shader, main_locations.runTime, &runTime, SHADER_UNIFORM_FLOAT);
		float mode = visuals_mode;
		SetShaderValue(main_shader, main_locations.visualizer, &mode, SHADER_UNIFORM_FLOAT);
		if (selected_sphere >= 0) {
			Sphere *s = &spheres[selected_sphere];
			float used_radius = fmaxf(0.01,fminf(s->corner_radius, fminf(s->size.x,fminf(s->size.y, s->size.z))));
			float data[15] = {
				s->pos.x,
				s->pos.y,
				s->pos.z,

				s->angle.x,
				s->angle.y,
				s->angle.z,

				s->size.x - used_radius,
				s->size.y - used_radius,
				s->size.z - used_radius,

				s->color.r / 255.f,
				s->color.g / 255.f,
				s->color.b / 255.f,

				used_radius,
				fmaxf(s->blob_amount, 0.0001),
				0,
			};

			SetShaderValueV(main_shader, main_locations.selectedParams, data, SHADER_UNIFORM_VEC3, 5);
			
		}

		BeginDrawing(); {
			
			ClearBackground(RAYWHITE);
			BeginShaderMode(main_shader); {
				DrawRectangle(sidebar_width, 0, GetScreenWidth()-sidebar_width, GetScreenHeight(), WHITE);
			} EndShaderMode();

			BeginMode3D(camera); {
				if (selected_sphere >= 0 && selected_sphere < MAX_SPHERES) {
					Sphere s = spheres[selected_sphere];

					if (mouseAction == CONTROL_TRANSLATE || mouseAction == CONTROL_ROTATE || mouseAction == CONTROL_SCALE) {
						if (controlled_axis.x) DrawRay((Ray){Vector3Add(s.pos, (Vector3){.x=-1000}), (Vector3){.x=1}} , RED);
						if (controlled_axis.y) DrawRay((Ray){Vector3Add(s.pos, (Vector3){.y=-1000}), (Vector3){.y=1}} , GREEN);
						if (controlled_axis.z) DrawRay((Ray){Vector3Add(s.pos, (Vector3){.z=-1000}), (Vector3){.z=1}} , BLUE);
					} else {

						if (mouseAction == CONTROL_NONE || mouseAction == CONTROL_POS_X)
							DrawLine3D(s.pos, Vector3Add(s.pos, (Vector3){0.5,0,0}),  RED);
						if (mouseAction == CONTROL_NONE || mouseAction == CONTROL_SCALE_X)  
							DrawCube(Vector3Add(s.pos, (Vector3){s.size.x,0,0}), .1,.1,.1, RED);
						if ((mouseAction == CONTROL_NONE || mouseAction == CONTROL_POS_X) && !ui_mode_gamepad) 
							DrawCylinderEx(Vector3Add(s.pos, (Vector3){0.5,0,0}),
										   Vector3Add(s.pos, (Vector3){.7,0,0}), .1, 0, 12, RED);

						if (mouseAction == CONTROL_NONE || mouseAction == CONTROL_POS_Y)
							DrawLine3D(s.pos, Vector3Add(s.pos, (Vector3){0,0.5,0}),  GREEN);
						if (mouseAction == CONTROL_NONE || mouseAction == CONTROL_SCALE_Y)  
							DrawCube(Vector3Add(s.pos, (Vector3){0,s.size.y,0}), .1,.1,.1, GREEN);
						if ((mouseAction == CONTROL_NONE || mouseAction == CONTROL_POS_Y) && !ui_mode_gamepad) 
							DrawCylinderEx(Vector3Add(s.pos, (Vector3){0,0.5,0}),
										   Vector3Add(s.pos, (Vector3){0,.7,0}), .1, 0, 12, GREEN);

						if (mouseAction == CONTROL_NONE || mouseAction == CONTROL_POS_Z)
							DrawLine3D(s.pos, Vector3Add(s.pos, (Vector3){0,0,0.5}),  BLUE);
						if (mouseAction == CONTROL_NONE || mouseAction == CONTROL_SCALE_Z)  
							DrawCube(Vector3Add(s.pos, (Vector3){0,0,s.size.z}), .1,.1,.1, BLUE);
						if ((mouseAction == CONTROL_NONE || mouseAction == CONTROL_POS_Z) && !ui_mode_gamepad) 
							DrawCylinderEx(Vector3Add(s.pos, (Vector3){0,0,0.5}),
										   Vector3Add(s.pos, (Vector3){0,0,0.7}), .1, 0, 12, BLUE);

					}
				}
			} EndMode3D();

			if (ui_mode_gamepad) {
				DrawCircle(sidebar_width + (GetScreenWidth()-sidebar_width)/2, GetScreenHeight()/2, 5, WHITE);
			}
			
			int default_color = GuiGetStyle(LABEL, TEXT);
			DrawRectangle(0, 0, sidebar_width, GetScreenHeight(), (Color){61, 61, 61,255});

			int y = 20;

			if (GuiButton((Rectangle){20,y,80,20}, "Save")) save("save");
			if (GuiButton((Rectangle){105,y,80,20}, "Export")) export();
			y+=30;

			GuiCheckBox((Rectangle){ 20, y+0.5, 20, 20 }, "Show Field", (bool *)&visuals_mode);
			y+=30;

			if (selected_sphere >= 0 ){
				Sphere old = spheres[selected_sphere];


				GuiLabel((Rectangle){ 20, y, 92, 24 }, "Position");
				y+=18;
				GuiSetStyle(LABEL, TEXT, 0xff0000ff);
				if(GuiFloatValueBox((Rectangle){ 20, y, 50, 20 }, "X", &spheres[selected_sphere].pos.x, -50, 50, focusedControl == CONTROL_POS_X)) focusedControl = (focusedControl == CONTROL_POS_X) ? CONTROL_NONE : CONTROL_POS_X;
				GuiSetStyle(LABEL, TEXT, 0x00ff00ff);
				if(GuiFloatValueBox((Rectangle){ 85, y, 50, 20 }, "Y", &spheres[selected_sphere].pos.y, -50, 50, focusedControl == CONTROL_POS_Y)) focusedControl = (focusedControl == CONTROL_POS_Y) ? CONTROL_NONE : CONTROL_POS_Y;
				GuiSetStyle(LABEL, TEXT, 0x0000ffff);
				if(GuiFloatValueBox((Rectangle){ 150, y, 50, 20 }, "Z", &spheres[selected_sphere].pos.z, -50, 50, focusedControl == CONTROL_POS_Z)) focusedControl = (focusedControl == CONTROL_POS_Z) ? CONTROL_NONE : CONTROL_POS_Z;
				GuiSetStyle(LABEL, TEXT, default_color);
				
				y+=23;
				GuiLabel((Rectangle){ 20, y, 92, 24 }, "Scale");
				y+=18;
				GuiSetStyle(LABEL, TEXT, 0xff0000ff);
				if(GuiFloatValueBox((Rectangle){ 20, y, 50, 20 }, "X", &spheres[selected_sphere].size.x, 0, 50, focusedControl == CONTROL_SCALE_X)) focusedControl = (focusedControl == CONTROL_SCALE_X) ? CONTROL_NONE : CONTROL_SCALE_X;
				GuiSetStyle(LABEL, TEXT, 0x00ff00ff);
				if(GuiFloatValueBox((Rectangle){ 85, y, 50, 20 }, "Y", &spheres[selected_sphere].size.y, 0, 50, focusedControl == CONTROL_SCALE_Y)) focusedControl = (focusedControl == CONTROL_SCALE_Y) ? CONTROL_NONE : CONTROL_SCALE_Y;
				GuiSetStyle(LABEL, TEXT, 0x0000ffff);
				if(GuiFloatValueBox((Rectangle){ 150, y, 50, 20 }, "Z", &spheres[selected_sphere].size.z, 0, 50, focusedControl == CONTROL_SCALE_Z)) focusedControl = (focusedControl == CONTROL_SCALE_Z) ? CONTROL_NONE : CONTROL_SCALE_Z;
				GuiSetStyle(LABEL, TEXT, default_color);

				y+=23;
				GuiLabel((Rectangle){ 20, y, 92, 24 }, "Rotation");
				y+=18;
				GuiSetStyle(LABEL, TEXT, 0xff0000ff);
				if(GuiFloatValueBox((Rectangle){ 20, y, 50, 20 }, "X", &spheres[selected_sphere].angle.x, -360, 360, focusedControl == CONTROL_ANGLE_X)) focusedControl = (focusedControl == CONTROL_ANGLE_X) ? CONTROL_NONE : CONTROL_ANGLE_X;
				GuiSetStyle(LABEL, TEXT, 0x00ff00ff);
				if(GuiFloatValueBox((Rectangle){ 85, y, 50, 20 }, "Y", &spheres[selected_sphere].angle.y, -360, 360, focusedControl == CONTROL_ANGLE_Y)) focusedControl = (focusedControl == CONTROL_ANGLE_Y) ? CONTROL_NONE : CONTROL_ANGLE_Y;
				GuiSetStyle(LABEL, TEXT, 0x0000ffff);
				if(GuiFloatValueBox((Rectangle){ 150, y, 50, 20 }, "Z", &spheres[selected_sphere].angle.z, -360, 360, focusedControl == CONTROL_ANGLE_Z)) focusedControl = (focusedControl == CONTROL_ANGLE_Z) ? CONTROL_NONE : CONTROL_ANGLE_Z;
				GuiSetStyle(LABEL, TEXT, default_color);

				Vector3 hsv = ColorToHSV((Color){spheres[selected_sphere].color.r, spheres[selected_sphere].color.g, spheres[selected_sphere].color.b, 255});
				Vector3 original_hsv = hsv;

				y+=23;
				GuiLabel((Rectangle){ 20, y, 92, 24 }, "Color");
				y+=20;
				GuiSetStyle(LABEL, TEXT, 0xff0000ff);
				if(GuiSlider((Rectangle){ 30, y, 155, 16 }, "H", "", &hsv.x, 0, 360)) focusedControl = (focusedControl == CONTROL_COLOR_R) ? CONTROL_NONE : CONTROL_COLOR_R;
				GuiSetStyle(LABEL, TEXT, 0x00ff00ff);

				y+=19;
				if(GuiSlider((Rectangle){ 30, y, 155, 16 }, "S", "", &hsv.y, 0.01, 1)) focusedControl = (focusedControl == CONTROL_COLOR_G) ? CONTROL_NONE : CONTROL_COLOR_G;
				GuiSetStyle(LABEL, TEXT, 0x0000ffff);

				y+=19;
				if(GuiSlider((Rectangle){ 30, y, 155, 16 }, "B", "", &hsv.z, 0.01, 1)) focusedControl = (focusedControl == CONTROL_COLOR_B) ? CONTROL_NONE : CONTROL_COLOR_B;
				GuiSetStyle(LABEL, TEXT, default_color);

				if (memcmp(&original_hsv, &hsv, sizeof(hsv))) {
					Color new = ColorFromHSV(hsv.x,hsv.y,hsv.z);
					spheres[selected_sphere].color.r = new.r;
					spheres[selected_sphere].color.g = new.g;
					spheres[selected_sphere].color.b = new.b;
				}

				y+=23;
				if (GuiFloatValueBox((Rectangle){ 40, y, 40, 20 }, "blob", &spheres[selected_sphere].blob_amount, 0, 10, focusedControl == CONTROL_BLOB_AMOUNT)) focusedControl = (focusedControl == CONTROL_BLOB_AMOUNT) ? CONTROL_NONE : CONTROL_BLOB_AMOUNT;
				if (GuiFloatValueBox((Rectangle){ 140, y, 70, 20 }, "Roundness", &spheres[selected_sphere].corner_radius, 0, 9999, focusedControl == CONTROL_CORNER_RADIUS)) focusedControl = (focusedControl == CONTROL_CORNER_RADIUS) ? CONTROL_NONE : CONTROL_CORNER_RADIUS;

				GuiCheckBox((Rectangle){ 120, y+=23, 20, 20 }, "cut out", &spheres[selected_sphere].subtract);

				GuiLabel((Rectangle){ 20, y+=23, 92, 24 }, "Mirror");
				GuiSetStyle(LABEL, TEXT, 0xff0000ff);
				GuiCheckBox((Rectangle){ 20, y+=23, 20, 20 }, "x", &spheres[selected_sphere].mirror.x);
				GuiSetStyle(LABEL, TEXT, 0x00ff00ff);
				GuiCheckBox((Rectangle){ 70, y, 20, 20 }, "y", &spheres[selected_sphere].mirror.y);
				GuiSetStyle(LABEL, TEXT, 0x0000ffff);
				GuiCheckBox((Rectangle){ 120, y, 20, 20 }, "z", &spheres[selected_sphere].mirror.z);
				GuiSetStyle(LABEL, TEXT, default_color);

				if (memcmp(&old.mirror, &spheres[selected_sphere].mirror, sizeof(old.mirror)) ||
				 old.subtract != spheres[selected_sphere].subtract) {
					needs_rebuild = true;

					BoundingBox bb = shapeBoundingBox(spheres[selected_sphere]);
					if (spheres[selected_sphere].mirror.x && bb.max.x <= 0) {
						spheres[selected_sphere].pos.x *= -1;
						spheres[selected_sphere].angle.y *= -1;
						spheres[selected_sphere].angle.z *= -1;
					}

					if (spheres[selected_sphere].mirror.y && bb.max.y <= 0) {
						spheres[selected_sphere].pos.y *= -1;
						spheres[selected_sphere].angle.x *= -1;
						spheres[selected_sphere].angle.z *= -1;
					}

					if (spheres[selected_sphere].mirror.z && bb.max.z <= 0) {
						spheres[selected_sphere].pos.z *= -1;
						spheres[selected_sphere].angle.y *= -1;
						spheres[selected_sphere].angle.x *= -1;
					}
				}

				if (memcmp(&old.color, &spheres[selected_sphere].color, sizeof(old.color))) {
					last_color_set = (Color){
						spheres[selected_sphere].color.r,
						spheres[selected_sphere].color.g,
						spheres[selected_sphere].color.b,
						0
					};
				}

				// if (memcmp(&old, &spheres[selected_sphere], sizeof(Sphere))) needs_rebuild = true;

				GuiSetState(STATE_NORMAL);
			}
			y+=30;


			const int row_height = 30;
			Rectangle scrollArea = (Rectangle){0, y, sidebar_width, GetScreenHeight() - y};

			int tempTextAlignment = GuiGetStyle(BUTTON, TEXT_ALIGNMENT);
			int border_width = GuiGetStyle(BUTTON, BORDER_WIDTH);
			int text_padding = GuiGetStyle(BUTTON, BORDER_WIDTH);
			GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
			GuiSetStyle(DEFAULT, BORDER_WIDTH, 0);
			GuiSetStyle(BUTTON, TEXT_PADDING, 8);
			default_color = GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL);

			GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0);
			Rectangle view_area;
			GuiScrollPanel(scrollArea, NULL, (Rectangle){0,0,scrollArea.width-15, num_spheres*row_height}, &scroll_offset, &view_area); // Scroll Panel control
			BeginScissorMode((int)view_area.x, (int)view_area.y, (int)view_area.width, (int)view_area.height); {
				const int first_visible = (int)floorf(-scroll_offset.y / row_height);
				const int last_visible = first_visible + (int)ceilf(view_area.height / row_height);
				for (int i=first_visible; i < MIN(last_visible+1,num_spheres); i++) {
					Sphere *s  = &spheres[i];
					const char *text = TextFormat("%c Shape %i", s->subtract ? '-' : '+', i+1);

					if (selected_sphere == i) GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x444444ff);

					if (GuiButton((Rectangle){10, scrollArea.y+i*row_height+scroll_offset.y, sidebar_width+10,row_height }, text)) {
						selected_sphere = i;
						needs_rebuild = true;
					}

					GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0);
				}
			} EndScissorMode();
			GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, default_color);
			GuiSetStyle(BUTTON, TEXT_ALIGNMENT, tempTextAlignment);
			GuiSetStyle(DEFAULT, BORDER_WIDTH, border_width);
			GuiSetStyle(BUTTON, TEXT_PADDING, text_padding);


			if (focusedControl != CONTROL_NONE) {
				DrawText("Nudge Value: Up & Down Arrows    Cancel: Escape    Done: Enter", sidebar_width + 8, 11, 10, WHITE);
			} else if (mouseAction == CONTROL_NONE) {
				DrawText("Add Shape: A    Delete: X    Grab: G    Rotate: R    Scale: S    Camera: Click+Drag", sidebar_width + 8, 11, 10, WHITE);
			} else if (mouseAction == CONTROL_TRANSLATE || mouseAction == CONTROL_ROTATE || mouseAction == CONTROL_SCALE) {
				DrawText("Change axis: X Y Z    Cancel: Escape    Done: Enter", sidebar_width + 8, 11, 10, WHITE);
			} else if (mouseAction == CONTROL_ROTATE_CAMERA) {
				DrawText("Pan: Alt+Drag", sidebar_width + 8, 11, 10, WHITE);
			} 

		} EndDrawing();
	}

	return 0;
}
