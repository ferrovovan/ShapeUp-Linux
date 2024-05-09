#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>


#define print(...) TraceLog(LOG_ERROR, __VA_ARGS__)

const int sidebar_width = 210;

typedef enum {
	CONTROL_NONE,
	CONTROL_POS_X,
	CONTROL_POS_Y,
	CONTROL_POS_Z,
	CONTROL_SCALE_X,
	CONTROL_SCALE_Y,
	CONTROL_SCALE_Z,
	CONTROL_ANGLE_X,
	CONTROL_ANGLE_Y,
	CONTROL_ANGLE_Z,
	CONTROL_COLOR_R,
	CONTROL_COLOR_G,
	CONTROL_COLOR_B,
	CONTROL_TRANSLATE,
	CONTROL_ROTATE,
	CONTROL_SCALE,
	CONTROL_CORNER_RADIUS,
	CONTROL_ROTATE_CAMERA,
	CONTROL_BLOB_AMOUNT,
} Control;

Control focusedControl;
Control mouseAction;

Vector2 scroll_offset;

union {
	struct {
		bool x : 1;
		bool y : 1;
		bool z : 1;
	};
	int mask;
} controlled_axis = {.mask = 0x7};
double last_axis_set = 0;

typedef struct {
	Vector3 pos;
	Vector3 size;
	Vector3 angle;
	float corner_radius;
	float blob_amount;
	struct {
		uint8_t r,g,b;
	} color;
	struct {
		bool x,y,z;
	} mirror;
	bool subtract;
} Sphere;

bool needs_rebuild = true;

#if DEMO_VIDEO_FEATURES
bool false_color_mode = false;
#endif

enum {
	VISUALS_NONE,
	VISUALS_SDF,
} visuals_mode;

Shader main_shader;
struct {
	int viewEye;
	int viewCenter;
	int runTime;
	int resolution;
	int selectedParams;
	int visualizer;
} main_locations;

int num_spheres = 1;
#define MAX_SPHERES 100
Sphere spheres[MAX_SPHERES];
int selected_sphere = 0;

Camera camera = { 0 };

Color last_color_set = {
	(uint8_t)(0.941*255),
	(uint8_t)(0.631*255),
	(uint8_t)(0.361*255),
	0
};

double lastSave;

void append(char **str1, const char *str2){
	assert(str1);
	assert(str2);
	size_t len = (*str1 ? strlen(*str1) : 0) + strlen(str2);

	char *concatenated = (char *)malloc(len + 1);
	concatenated[0] = 0;
	assert(concatenated);

	if (*str1) {
		strcpy(concatenated, *str1);
		free(*str1);
	}
	strcat(concatenated, str2);

	*str1 = concatenated;
}

int RayPlaneIntersection(const Vector3 RayOrigin, const Vector3 RayDirection, const Vector3 PlanePoint, const Vector3 PlaneNormal, Vector3 *IntersectionPoint) {
	float dotProduct = (PlaneNormal.x * RayDirection.x) + (PlaneNormal.y * RayDirection.y) + (PlaneNormal.z * RayDirection.z);

	// Check if the ray is parallel to the plane
	if (dotProduct == 0.0f) {
		return 0;
	}

	float t = ((PlanePoint.x - RayOrigin.x) * PlaneNormal.x + (PlanePoint.y - RayOrigin.y) * PlaneNormal.y + (PlanePoint.z - RayOrigin.z) * PlaneNormal.z) / dotProduct;

	IntersectionPoint->x = RayOrigin.x + t * RayDirection.x;
	IntersectionPoint->y = RayOrigin.y + t * RayDirection.y;
	IntersectionPoint->z = RayOrigin.z + t * RayDirection.z;

	return 1;
}

Vector3 WorldToCamera(Vector3 worldPos, Matrix cameraMatrix) {
	return  Vector3Transform(worldPos, cameraMatrix);
}

Vector3 CameraToWorld(Vector3 worldPos, Matrix cameraMatrix) {
	return Vector3Transform(worldPos, MatrixInvert(cameraMatrix));
}

Vector3 VectorProjection(const Vector3 vectorToProject, const Vector3 targetVector) {
	float dotProduct = (vectorToProject.x * targetVector.x) +
					  (vectorToProject.y * targetVector.y) +
					  (vectorToProject.z * targetVector.z);
	
	float targetVectorLengthSquared = (targetVector.x * targetVector.x) +
									  (targetVector.y * targetVector.y) +
									  (targetVector.z * targetVector.z);
	
	float scale = dotProduct / targetVectorLengthSquared;

	Vector3 projection;
	projection.x = targetVector.x * scale;
	projection.y = targetVector.y * scale;
	projection.z = targetVector.z * scale;

	return projection;
}

// Find the point on line p1 to p2 nearest to line p2 to p4
Vector3 NearestPointOnLine(Vector3 p1,
						   Vector3 p2,
						   Vector3 p3,
						   Vector3 p4)
{
	float mua;

	Vector3 p13,p43,p21;
	float d1343,d4321,d1321,d4343,d2121;
	float numer,denom;

	const float EPS = 0.001;

	p13.x = p1.x - p3.x;
	p13.y = p1.y - p3.y;
	p13.z = p1.z - p3.z;
	p43.x = p4.x - p3.x;
	p43.y = p4.y - p3.y;
	p43.z = p4.z - p3.z;
	if (fabs(p43.x) < EPS && fabs(p43.y) < EPS && fabs(p43.z) < EPS)
		return(Vector3){};
	p21.x = p2.x - p1.x;
	p21.y = p2.y - p1.y;
	p21.z = p2.z - p1.z;
	if (fabs(p21.x) < EPS && fabs(p21.y) < EPS && fabs(p21.z) < EPS)
		return(Vector3){};

	d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
	d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
	d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
	d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
	d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

	denom = d2121 * d4343 - d4321 * d4321;
	if (fabs(denom) < EPS)
		return (Vector3){};
	numer = d1343 * d4321 - d1321 * d4343;

	mua = numer / denom;

	return (Vector3){ 
		p1.x + mua * p21.x,
		p1.y + mua * p21.y,
		p1.z + mua * p21.z};

}

BoundingBox boundingBoxSized(Vector3 center, float size) {
	return (BoundingBox){
		Vector3SubtractValue(center, size/2),
		Vector3AddValue(center, size/2),
	};
}

BoundingBox shapeBoundingBox(Sphere s) {
	// const float radius = sqrtf(powf(s.size.x, 2) + powf(s.size.y, 2) + powf(s.size.z, 2));
	return (BoundingBox){
		Vector3Subtract(s.pos, s.size),
		Vector3Add(s.pos, s.size),
	};
}

int GuiFloatValueBox(Rectangle bounds, const char *text, float *value, float minValue, float maxValue, bool editMode)
{
	#if !defined(RAYGUI_VALUEBOX_MAX_CHARS)
		#define RAYGUI_VALUEBOX_MAX_CHARS  32
	#endif

	int result = 0;
	GuiState state = guiState;

	static char editingTextValue[RAYGUI_VALUEBOX_MAX_CHARS + 1] = "\0";
	char textValue[RAYGUI_VALUEBOX_MAX_CHARS + 1] = "\0";
	static float original_value;
	static int key_delay = 0;
	snprintf(textValue, RAYGUI_VALUEBOX_MAX_CHARS, "%g", *value);

	Rectangle textBounds = { 0 };
	if (text != NULL)
	{
		textBounds.width = (float)(GetTextWidth(text) + 2);
		textBounds.height = (float)(GuiGetStyle(DEFAULT, TEXT_SIZE));
		textBounds.x = bounds.x + bounds.width + GuiGetStyle(VALUEBOX, TEXT_PADDING);
		textBounds.y = bounds.y + bounds.height/2 - GuiGetStyle(DEFAULT, TEXT_SIZE)/2;
		if (GuiGetStyle(VALUEBOX, TEXT_ALIGNMENT) == TEXT_ALIGN_LEFT) textBounds.x = bounds.x - textBounds.width - GuiGetStyle(VALUEBOX, TEXT_PADDING);
	}

	char *text_to_display = textValue;

	// Update control
	//--------------------------------------------------------------------
	if ((state != STATE_DISABLED) && !guiLocked && !guiSliderDragging)
	{
		Vector2 mousePoint = GetMousePosition();

		bool valueHasChanged = false;

		if (editMode)
		{   
			state = STATE_PRESSED;

			int keyCount = (int)strlen(editingTextValue);

			// Only allow keys in range [48..57]
			if (keyCount < RAYGUI_VALUEBOX_MAX_CHARS)
			{
				int key = GetCharPressed();
				if (((key >= 48) && (key <= 57)) || key == 46)
				{
					editingTextValue[keyCount] = (char)key;
					keyCount++;
					valueHasChanged = true;
				}
			}

			// Delete text
			if (keyCount > 0)
			{
				if (IsKeyPressed(KEY_BACKSPACE))
				{
					keyCount--;
					editingTextValue[keyCount] = '\0';
					valueHasChanged = true;
				}
			}

			if (!valueHasChanged && IsKeyDown(KEY_UP) && key_delay <= 0) {
				*value+=fabs(*value*0.1) + 0.1;
				if (*value > maxValue) *value = maxValue;
				key_delay=9;
				sprintf(editingTextValue, "%g", *value);
			}else if (!valueHasChanged && IsKeyDown(KEY_DOWN) && key_delay <= 0) {
				*value-=fabs(*value*0.1) + 0.1;
				if (*value < minValue) *value = minValue;
				key_delay=9;
				sprintf(editingTextValue, "%g", *value);
			}

			if (key_delay > 0) key_delay--;
			if (!IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN))key_delay = 0;
			text_to_display = editingTextValue;
			if (IsKeyPressed(KEY_ESCAPE)) {
				*value = original_value;
				result = 1;
			} else {
				if (valueHasChanged) *value = (float)strtod(editingTextValue, NULL);
				if (IsKeyPressed(KEY_ENTER) || (!CheckCollisionPointRec(mousePoint, bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) result = 1;
			}
		}
		else
		{
			if (*value > maxValue) *value = maxValue;
			else if (*value < minValue) *value = minValue;

			if (CheckCollisionPointRec(mousePoint, bounds))
			{
				state = STATE_FOCUSED;
				if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
					original_value = *value;
					memcpy(editingTextValue, textValue, RAYGUI_VALUEBOX_MAX_CHARS + 1);
					result = 1;
				}
			}
		}
	}
	//--------------------------------------------------------------------

	// Draw control
	//--------------------------------------------------------------------
	Color baseColor = BLANK;
	if (state == STATE_PRESSED) baseColor = GetColor(GuiGetStyle(VALUEBOX, BASE_COLOR_PRESSED));
	else if (state == STATE_DISABLED)  baseColor = GetColor(GuiGetStyle(VALUEBOX, BASE_COLOR_DISABLED));
	else if (state == STATE_FOCUSED) baseColor = WHITE;

	GuiDrawRectangle(bounds, 0, GetColor(GuiGetStyle(VALUEBOX, BORDER + (state*3))), baseColor);
	GuiDrawText(text_to_display, GetTextBounds(VALUEBOX, bounds), TEXT_ALIGN_LEFT, GetColor(GuiGetStyle(VALUEBOX, TEXT)));

	// Draw cursor
	if (editMode)
	{
		Rectangle cursor = { bounds.x + GetTextWidth(text_to_display) + 1, bounds.y + 2*GuiGetStyle(VALUEBOX, BORDER_WIDTH), 4, bounds.height - 4*GuiGetStyle(VALUEBOX, BORDER_WIDTH) };
		GuiDrawRectangle(cursor, 0, BLANK, GetColor(GuiGetStyle(LABEL, TEXT + (state*3))));
	}

	// Draw text label if provided
	GuiDrawText(text, textBounds, (GuiGetStyle(VALUEBOX, TEXT_ALIGNMENT) == TEXT_ALIGN_RIGHT)? TEXT_ALIGN_LEFT : TEXT_ALIGN_RIGHT, GetColor(GuiGetStyle(LABEL, TEXT)));
	//--------------------------------------------------------------------

	return result;
}

void append_map_function(char **result, bool use_color_as_index, int dynamic_index) {

	char *map = NULL;

	append(&map, "uniform vec3 selectionValues[5];\n\n");

	#if DEMO_VIDEO_FEATURES
	if (false_color_mode) {
		append(&map, "#define FALSE_COLOR_MODE 1\n\n");
	}
	#endif

	append(&map,
		"vec4 signed_distance_field( in vec3 pos ){\n"
		"\tvec4 distance = vec4(999999.,0,0,0);\n");

	for (int i=0; i < num_spheres; i++) {
		Sphere s = spheres[i];
		float used_radius = fmaxf(0.01,fminf(s.corner_radius, fminf(s.size.x,fminf(s.size.y, s.size.z))));

		char *symmetry[8] = {
		"",
		"opSymX",
		"opSymY",
		"opSymXY", 
		"opSymZ",
		"opSymXZ",
		"opSymYZ",
		"opSymXYZ"};

		int mirror_index = (s.mirror.z << 2) | (s.mirror.y << 1) | s.mirror.x;
		if (i == dynamic_index) {
			append(&map, TextFormat("\tdistance = %s(\n"
							  "\t\tvec4(RoundBox(\n"
							  "\t\t\t\topRotateXYZ(\n"
							  "\t\t\t\t\t%s(pos) - selectionValues[0], // position\n"
							  "\t\t\t\t\tselectionValues[1]), // angle\n"
							  "\t\t\t\tselectionValues[2],  // size\n"
							  "\t\t\t\tselectionValues[4].x), // corner radius\n"
							  "\t\t\tselectionValues[3]), // color\n"
							  "\t\tdistance,\n\t\tselectionValues[4].y); // blobbyness\n",
							  s.subtract ? "opSmoothSubtraction" : "BlobbyMin",
							  symmetry[mirror_index]));
		} else {

			uint8_t r = use_color_as_index ?  (uint8_t)(i+1) : s.color.r;
			uint8_t g = use_color_as_index ?  0 : s.color.g;
			uint8_t b = use_color_as_index ?  0 : s.color.b;

#           if DEMO_VIDEO_FEATURES
			if (false_color_mode) {
				Color c = ColorFromHSV((i*97) % 360, 1, 0.5);
				r = c.r;
				g = c.g;
				b = c.b;
			}
#           endif


			char *opName = s.subtract ? "opSmoothSubtraction" : ((use_color_as_index
																#if DEMO_VIDEO_FEATURES
																  || false_color_mode
																#endif
																  ) ? "opSmoothUnionSteppedColor" : (s.blob_amount > 0 ? "BlobbyMin" : "Min"));
			append(&map,  TextFormat("\tdistance = %s(\n\t\tvec4(RoundBox(\n", opName));

			const bool rotated = fabsf(s.angle.x) > .01 || fabsf(s.angle.y) > 0.01 || fabsf(s.angle.z) > .01;
			if (rotated) {
					float cz = cos(s.angle.z);
					float sz = sin(s.angle.z);
					float cy = cos(s.angle.y);
					float sy = sin(s.angle.y);
					float cx = cos(s.angle.x);
					float sx = sin(s.angle.x);

					append(&map, TextFormat("\t\t\tmat3(%g, %g, %g,"
											"%g, %g, %g,"
											"%g, %g, %g)*\n",
											cz*cy,
											cz*sy*sx - cx*sz,
											sz*sx + cz*cx*sy,

											cy*sz,
											cz*cx + sz*sy*sx,
											cx*sz*sy - cz*sx,

											-sy,
											cy*sx,
											cy*cx
											));
			}

			append(&map, TextFormat("\t\t\t\t(%s(pos) - vec3(%g,%g,%g)), // position\n",
								  symmetry[mirror_index],
								  s.pos.x, s.pos.y, s.pos.z));



			append(&map, TextFormat("\t\t\tvec3(%g,%g,%g),// size\n"
							  "\t\t\t%g), // corner radius\n"
							  "\t\t\t%g,%g,%g), // color\n"
							  "\t\tdistance",
							  s.size.x-used_radius, s.size.y-used_radius, s.size.z-used_radius,
							  used_radius,
							  r/255.f, g/255.f, b/255.f));

			if (!strcmp(opName, "Min")) {
				append(&map,  TextFormat(");\n"));
			} else {
				append(&map,  TextFormat(",\n\t\t%g);  // blobbyness\n", fmaxf(s.blob_amount, 0.0001)));
			}

		}


	}

	append(&map, "\treturn distance;\n}\n");

#   if DEMO_VIDEO_FEATURES
	SaveFileText("map.glsl", map);
#   endif

	append(result, map);
	free(map);
}

#ifdef PLATFORM_WEB
#define SHADER_VERSION_PREFIX "#version 300 es\nprecision highp float;"
#else
#define SHADER_VERSION_PREFIX "#version 330\n" 
#endif

const char *vshader =     
	SHADER_VERSION_PREFIX
	"in vec3 vertexPosition;            \n"
	"in vec2 vertexTexCoord;            \n"
	"out vec2 fragTexCoord;             \n"
	"uniform mat4 mvp;                  \n"
	"void main()                        \n"
	"{                                  \n"
	"    fragTexCoord = vertexTexCoord; \n"
	"    gl_Position = mvp*vec4(vertexPosition, 1.0); \n"
	"}                                  \n";

void rebuild_shaders(void ) {
	needs_rebuild = false;
	UnloadShader(main_shader); 

	char *map_function = NULL;
	append_map_function(&map_function, false, selected_sphere);

	char *result = NULL;
	append(&result, 
		SHADER_VERSION_PREFIX
		   "out vec4 finalColor; \
		   uniform vec3 viewEye; \
		   uniform vec3 viewCenter; \
		   uniform float runTime; \
		   uniform float visualizer; \
		   uniform vec2 resolution;");
	append(&result, shader_prefix_fs);
	append(&result, map_function);
	append(&result, shader_base_fs);
	main_shader = LoadShaderFromMemory(vshader, (char *)result);
	free(result);
	result = NULL;

	main_locations.viewEye = GetShaderLocation(main_shader, "viewEye");
	main_locations.viewCenter = GetShaderLocation(main_shader, "viewCenter");
	main_locations.runTime = GetShaderLocation(main_shader, "runTime");
	main_locations.resolution = GetShaderLocation(main_shader, "resolution");
	main_locations.selectedParams = GetShaderLocation(main_shader, "selectionValues");
	main_locations.visualizer = GetShaderLocation(main_shader, "visualizer");


	free(map_function);
}

void delete_sphere(int index) {
	memmove(&spheres[index], &spheres[index+1], sizeof(Sphere)*(num_spheres-index));
	num_spheres--;

	if (selected_sphere == index) {
		selected_sphere = num_spheres-1;
	} else if (selected_sphere > index) {
		selected_sphere--;
	}

	needs_rebuild = true;
}

void add_shape(void) {
	if (num_spheres >= MAX_SPHERES) return;

	spheres[num_spheres] = (Sphere){
		.size = { 1, 1, 1 },
		.color = {
			last_color_set.r,
			last_color_set.g,
			last_color_set.b,
		},
	};
	selected_sphere = num_spheres;
	num_spheres++;
	needs_rebuild = true;
}

#define MIN(x,y) ({ \
		__typeof(x) xv = (x);\
		__typeof(y) yv = (y); \
		xv < yv ? xv : yv;\
	})

__attribute__((format(printf, 4, 5)))
void append_format(char **data, int *size, int *capacity, const char *format, ...) {

	va_list arg_ptr;
	va_start(arg_ptr, format);
	int added = vsnprintf(*data+*size, *capacity-*size,format, arg_ptr); 

	*size+=MIN(added, *capacity - *size);
	assert(*size < *capacity);

	va_end(arg_ptr);
}

Vector3 VertexInterp(Vector4 p1,Vector4 p2, float threshold) {

   if (fabsf(threshold-p1.w) < 0.00001)
	  return *(Vector3 *)&p1;
   if (fabsf(threshold-p2.w) < 0.00001)
	  return *(Vector3 *)&p2;
   if (fabsf(p1.w-p2.w) < 0.00001)
	  return *(Vector3 *)&p1;

   float mu = (threshold - p1.w) / (p2.w - p1.w);
   Vector3 r = {
	p1.x + mu * (p2.x - p1.x),
	p1.y + mu * (p2.y - p1.y),
	p1.z + mu * (p2.z - p1.z),
   };

   return r;
}

uint64_t FNV1a_64_hash(uint8_t *data, int len) {
	uint64_t hash = 0xcbf29ce484222325;
	for (int i=0; i < len; i++) {
		hash = (hash ^ data[i]) * 0x00000100000001B3;
	}

	return hash;
}

void save(char *name) {
	const int size = sizeof(int) + sizeof(Sphere)*num_spheres;
	char *data = malloc(size);
	*(int *)(void *)data = num_spheres;
	memcpy(data + sizeof(int), spheres, num_spheres*sizeof(Sphere));

	SaveFileData(TextFormat("build/%s_%"PRIu64".shapeup", name, FNV1a_64_hash(data, size)), data, size);

	free(data);
	lastSave = GetTime();
}

void openSnapshot(char *path) {
	print("opening ========= %s", path);

	unsigned int size;
	unsigned char *data = LoadFileData(path, &size);

	assert(data);

	num_spheres = *(int *)(void *)data;
	memcpy(spheres, data + sizeof(int), sizeof(Sphere)*num_spheres);

	UnloadFileData(data);

	selected_sphere = -1;
	needs_rebuild = true;
	lastSave = GetTime();
}

RenderTexture2D LoadFloatRenderTexture(int width, int height)
{
	RenderTexture2D target = { 0 };

	target.id = rlLoadFramebuffer(width, height);   // Load an empty framebuffer

	if (target.id > 0)
	{
		rlEnableFramebuffer(target.id);

		target.texture.format = PIXELFORMAT_UNCOMPRESSED_R32;
		target.texture.id = rlLoadTexture(NULL, width, height, target.texture.format, 1);
		target.texture.width = width;
		target.texture.height = height;
		target.texture.mipmaps = 1;

		// Attach color texture and depth renderbuffer/texture to FBO
		rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);

		if (rlFramebufferComplete(target.id)) {
			print("FBO: [ID %i] Framebuffer object created successfully", target.id);
		}
		else {
			print("FBO: [ID %i] Framebuffer object FAILED", target.id);
		}

		rlDisableFramebuffer();
	}
	else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

	return target;
}
#include "table_constants.c"

void export(void) {

	char *shader_source = NULL;
	append(&shader_source, SHADER_VERSION_PREFIX);
	append(&shader_source, shader_prefix_fs);
	append_map_function(&shader_source, false, -1);
	append(&shader_source, slicer_body_fs);

	Shader slicer_shader = LoadShaderFromMemory(vshader, (char *)shader_source);
	free(shader_source);
	shader_source = NULL;

	int slicer_z_loc = GetShaderLocation(slicer_shader, "z");

	double startTime = GetTime();
	const float cube_resolution = 0.03;

	BoundingBox bounds = {
		{FLT_MAX,FLT_MAX,FLT_MAX},
		{-FLT_MAX,-FLT_MAX,-FLT_MAX}
	};

	for(int i=0; i < num_spheres; i++) {
		const float radius = sqrtf(powf(spheres[i].size.x, 2) + powf(spheres[i].size.y, 2) + powf(spheres[i].size.z, 2));
		bounds.min.x = fminf(bounds.min.x, spheres[i].pos.x - radius); 
		bounds.min.y = fminf(bounds.min.y, spheres[i].pos.y - radius);
		bounds.min.z = fminf(bounds.min.z, spheres[i].pos.z - radius);

		bounds.max.x = fmaxf(bounds.max.x, spheres[i].pos.x + radius); 
		bounds.max.y = fmaxf(bounds.max.y, spheres[i].pos.y + radius);
		bounds.max.z = fmaxf(bounds.max.z, spheres[i].pos.z + radius);
	}

	// the marching cube sampling lattace must extend beyond the objects you want it to represent
	bounds.min.x -= 1;
	bounds.min.y -= 1;
	bounds.min.z -= 1;

	bounds.max.x += 1;
	bounds.max.y += 1;
	bounds.max.z += 1;

	const int slice_count_x = (int)((bounds.max.x - bounds.min.x) / cube_resolution + 1.5);
	const int slice_count_y = (int)((bounds.max.y - bounds.min.y) / cube_resolution + 1.5);
	const int slice_count_z = (int)((bounds.max.z - bounds.min.z) / cube_resolution + 1.5);

	const float x_step = (bounds.max.x - bounds.min.x) / (slice_count_x-1);
	const float y_step = (bounds.max.y - bounds.min.y) / (slice_count_y-1);
	const float z_step = (bounds.max.z - bounds.min.z) / (slice_count_z-1);

	int data_capacity = 400000000;
	char *data = malloc(data_capacity);
	int data_size = 0;

	RenderTexture2D sliceTexture[2];
	sliceTexture[0] = LoadFloatRenderTexture(slice_count_x, slice_count_y);
	sliceTexture[1] = LoadFloatRenderTexture(slice_count_x, slice_count_y);
	
	for (int z_index = 0; z_index < slice_count_z-1; z_index++) {
		for (int side =0; side < 2; side++) {
			float z = bounds.min.z + (z_index+side)*z_step;
			SetShaderValue(slicer_shader, slicer_z_loc, &z, SHADER_UNIFORM_FLOAT);
			BeginTextureMode(sliceTexture[side]); {
				BeginShaderMode(slicer_shader); {
					rlBegin(RL_QUADS);
					rlTexCoord2f(bounds.max.x, bounds.min.y);
					rlVertex2f(0, 0);

					rlTexCoord2f(bounds.max.x, bounds.max.y);
					rlVertex2f(0, slice_count_y);

					rlTexCoord2f(bounds.min.x, bounds.max.y);
					rlVertex2f(slice_count_x, slice_count_y);

					rlTexCoord2f(bounds.min.x, bounds.min.y);
					rlVertex2f(slice_count_x, 0);
					rlEnd();
				} EndShaderMode();
			} EndTextureMode();
		}

		float *pixels = rlReadTexturePixels(sliceTexture[0].texture.id, sliceTexture[0].texture.width, sliceTexture[0].texture.height, sliceTexture[0].texture.format);
		float *pixels2 = rlReadTexturePixels(sliceTexture[1].texture.id, sliceTexture[1].texture.width, sliceTexture[1].texture.height, sliceTexture[1].texture.format);

		#define SDF_THRESHOLD (0)
		for (int y_index=0; y_index < slice_count_y-1; y_index++) { 
			for (int x_index=0; x_index < slice_count_x-1; x_index++) {

				float val0 = pixels [(x_index  + y_index   *slice_count_x)*1] ;
				float val1 = pixels [(x_index+1+ y_index   *slice_count_x)*1] ;
				float val2 = pixels [(x_index+1 +(y_index+1)*slice_count_x)*1];
				float val3 = pixels [(x_index   +(y_index+1)*slice_count_x)*1];
				float val4 = pixels2[(x_index  + y_index   *slice_count_x)*1] ;
				float val5 = pixels2[(x_index+1+ y_index   *slice_count_x)*1] ;
				float val6 = pixels2[(x_index+1+(y_index+1)*slice_count_x)*1] ;
				float val7 = pixels2[(x_index  +(y_index+1)*slice_count_x)*1] ;

/*
				Vector4 *vectors4_points;
				int vectors4_len = 0;
			#define add_vector(x, y, z, k) \
*/
				Vector4 v0 = {
					bounds.min.x + x_index*x_step,
					bounds.min.y + y_index*y_step,
					bounds.min.z + z_index*z_step,
					val0
				};
				Vector4 v1 = {v0.x+x_step,  v0.y,        v0.z, val1};
				Vector4 v2 = {v0.x+x_step,  v0.y+y_step, v0.z, val2};
				Vector4 v3 = {v0.x,         v0.y+y_step, v0.z, val3};

				Vector4 v4 = {v0.x,         v0.y,        v0.z+z_step, val4};
				Vector4 v5 = {v0.x+x_step,  v0.y,        v0.z+z_step, val5};
				Vector4 v6 = {v0.x+x_step,  v0.y+y_step, v0.z+z_step, val6};
				Vector4 v7 = {v0.x,         v0.y+y_step, v0.z+z_step, val7};

				int cubeindex = (val0 < SDF_THRESHOLD) << 0 | 
							(val1 < SDF_THRESHOLD) << 1 |
							(val2 < SDF_THRESHOLD) << 2 |
							(val3 < SDF_THRESHOLD) << 3 |
							(val4 < SDF_THRESHOLD) << 4 |
							(val5 < SDF_THRESHOLD) << 5 |
							(val6 < SDF_THRESHOLD) << 6 |
							(val7 < SDF_THRESHOLD) << 7;

				/* Cube is entirely in/out of the surface */
				if (edgeTable[cubeindex] == 0) continue;

				Vector3 vertlist[12];
/*
				mapping_index = {
				for (int i=1 ;i <= 12; ++i){
					if (edgeTable[cubeindex] & (1 << i))
						vertlist[i] = VertexInterp(v0,v1, SDF_THRESHOLD);
				}
*/
				if (edgeTable[cubeindex] & 1) vertlist[0] = VertexInterp(v0,v1, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 2) vertlist[1] = VertexInterp(v1,v2, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 4) vertlist[2] = VertexInterp(v2,v3, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 8) vertlist[3] = VertexInterp(v3,v0, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 16) vertlist[4] = VertexInterp(v4,v5, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 32) vertlist[5] = VertexInterp(v5,v6, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 64) vertlist[6] = VertexInterp(v6,v7, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 128) vertlist[7] = VertexInterp(v7,v4, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 256) vertlist[8] = VertexInterp(v0,v4, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 512) vertlist[9] = VertexInterp(v1,v5, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 1024) vertlist[10] = VertexInterp(v2,v6, SDF_THRESHOLD);
				if (edgeTable[cubeindex] & 2048) vertlist[11] = VertexInterp(v3,v7, SDF_THRESHOLD);

				for (int i=0; triTable[cubeindex][i] != -1; i+=3) {
					for (int v=0; v < 3; v++) {
						Vector3 pt = vertlist[triTable[cubeindex][i + v]];
						append_format(&data, &data_size, &data_capacity,"v %g %g %g\n", pt.x, -pt.y, pt.z); 
					}

					append_format(&data, &data_size, &data_capacity,"f -2  -1 -3\n"); 
				}

			}
		}

		free(pixels);
		free(pixels2);
	}

	UnloadRenderTexture(sliceTexture[0]);
	UnloadRenderTexture(sliceTexture[1]);


	SaveFileData("build/export.obj", data, data_size);

	free(data);

	UnloadShader(slicer_shader);

	double duration = GetTime() - startTime;
	print("export time %gms. size: %.2fMB", duration*1000, data_size/1000000.f);
}

int object_at_pixel(int x, int y) {
	const float start = GetTime();
	char *shader_source = NULL;
	append(&shader_source, SHADER_VERSION_PREFIX);
	append(&shader_source, shader_prefix_fs);
	append_map_function(&shader_source, true, -1);
	append(&shader_source, selection_fs);

	Shader shader = LoadShaderFromMemory(vshader, (char *)shader_source);
	free(shader_source);
	shader_source = NULL;

	int eye_loc = GetShaderLocation(shader, "viewEye");
	int center_loc = GetShaderLocation(shader, "viewCenter");
	int resolution_loc = GetShaderLocation(shader, "resolution");

	SetShaderValue(shader, eye_loc, &camera.position, SHADER_UNIFORM_VEC3);
	SetShaderValue(shader, center_loc, &camera.target, SHADER_UNIFORM_VEC3);
	SetShaderValue(shader, resolution_loc, (float[2]){ (float)GetScreenWidth(), (float)GetScreenHeight() }, SHADER_UNIFORM_VEC2);

	RenderTexture2D target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

#define rl_tex_coord_macro(new_x, new_y) \
	rlTexCoord2f(new_x, new_y); \
	rlVertex2f(new_x, new_y);

	BeginTextureMode(target); {
		BeginShaderMode(shader); {
			rlBegin(RL_QUADS);
			rl_tex_coord_macro(x-1, y-1)
			rl_tex_coord_macro(x-1, y+1)
			rl_tex_coord_macro(x+1, y+1)
			rl_tex_coord_macro(x+1, y-1)			
			rlEnd();
		} EndShaderMode();
	} EndTextureMode();

	uint8_t *pixels = rlReadTexturePixels(target.texture.id, target.texture.width, target.texture.height, target.texture.format);

	int object_index = ((int)pixels[(x + target.texture.width*(target.texture.height-y))*4]) - 1;

	free(pixels);

	UnloadRenderTexture(target);
	UnloadShader(shader);

	print("picking object took %ims", (int)((-start + GetTime())*1000));

	return object_index;
}

