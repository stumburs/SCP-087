#include "raylib.h"

#include "raymath.h"

#define RLIGHTS_IMPLEMENTATION
#include "extras/rlights.h"

#include <vector>

Texture2D brick_texture;
Shader light_shader;
Shader pixelizer_shader;

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    InitWindow(screenWidth, screenHeight, "SCP-087 Blind Date");

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = { 0.0f, 2.0f, 0.0f };    // Camera position
    camera.target = { 0.0f, 0.5f, 0.0f };      // Camera looking at point
    camera.up = { 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 70.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    // init textures and shaders
    brick_texture = LoadTexture("textures/brick.png");
    light_shader = LoadShader("shaders/lighting.vs", "shaders/fog.fs");
    pixelizer_shader = LoadShader(0, "shaders/pixelizer.fs");
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    light_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(light_shader, "matModel");
    light_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(light_shader, "viewPos");

    // Init walls
    std::vector<Model> walls;
    for (int i = 0; i < 4; i++)
    {
        Model wall = LoadModelFromMesh(GenMeshCube(4.0f, 4.0f, 4.0f));
        wall.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = brick_texture;
        wall.materials[0].shader = light_shader;
        walls.push_back(wall);
    }
    

    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(light_shader, "ambient");
    float values[4] = { 0.05f, 0.05f, 0.05f, 1.0f };
    SetShaderValue(light_shader, ambientLoc, (const void*)(&values), SHADER_UNIFORM_VEC4);

    //float fogDensity = 0.0f;
    float fogDensity = 0.15f;
    int fogDensityLoc = GetShaderLocation(light_shader, "fogDensity");
    SetShaderValue(light_shader, fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);

    // Create lights
    Light lights[MAX_LIGHTS] = { 0 };
    lights[0] = CreateLight(LIGHT_POINT, camera.position, Vector3Zero(), {70, 70, 70, 255}, light_shader);

    SetCameraMode(camera, CAMERA_FIRST_PERSON);  // Set an orbital camera mode

    SetTargetFPS(0);                       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())            // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera);

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(light_shader, light_shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        // Check key inputs to enable/disable lights
        if (IsKeyPressed(KEY_F))
        {
            lights[0].enabled = !lights[0].enabled;
        }

        // Update light values (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            lights[i].position = camera.position;
            lights[i].position.y -= 0.5f;
            UpdateLightValues(light_shader, lights[i]);
        }
        //----------------------------------------------------------------------------------

        // Rendertexture
        BeginTextureMode(target);       // Enable drawing to texture
            ClearBackground(BLACK);  // Clear texture background

            BeginMode3D(camera);        // Begin 3d mode drawing
            for (int i = 0; i < walls.size(); i++)
            {
                DrawModel(walls[i], { 0.0f, 2.0f, 0.0f + 4.0f * i }, 1.0f, WHITE);
            }
            EndMode3D();                // End 3d mode drawing, returns to orthographic 2d mode
        EndTextureMode();

        // Draw final image
        BeginDrawing();
        {
            ClearBackground(BLACK);
            BeginShaderMode(pixelizer_shader);
            DrawTextureRec(target.texture, { 0, 0, (float)target.texture.width, (float)-target.texture.height }, { 0, 0 }, WHITE);
            EndShaderMode();
            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    CloseWindow();
}
