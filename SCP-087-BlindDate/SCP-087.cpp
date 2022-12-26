#include "raylib.h"
#include "raymath.h"
#define RLIGHTS_IMPLEMENTATION
#include "extras/rlights.h"
#include <vector>
#include <iostream>

float Map(float n, float start1, float stop1, float start2, float stop2)
{
    return ((n - start1) / (stop1 - start1)) * (stop2 - start2) + start2;
}

Texture2D brick_texture;
Texture2D concrete_texture;
Texture2D black_rock_texture;
Shader light_shader;
Shader pixelizer_shader;
Shader bloom_shader;
Shader open_ai_shader;

float last_x = 0;

void RenderChunk(const float& camera_x, float offset, Model &floor, Model &wall, Model &ceiling)
{
    // Floor
    for (int i = 0; i < 10; i++)
    {
        // Floor
        DrawModel(floor, { 0.0f + 0.5f * i + offset, (0.0f - 0.5f * i) + camera_x - offset, 0.0f }, 1.0f, WHITE);

        // Ceiling
        DrawModel(ceiling, { 0.0f + 0.25f * i + offset, (7.0f - 0.25f * i) + camera_x - offset, 0.0f }, 1.0f, WHITE);
        DrawModel(ceiling, { 2.5f + 0.25f * i + offset, (4.5f - 0.25f * i) + camera_x - offset, 0.0f }, 1.0f, WHITE);
    }
    // Walls

    // Left wall
    DrawModel(wall, { 0.0f + 2.5f + offset, 0.0f + camera_x - offset, -2.0f }, 1.0f, WHITE);
    DrawModel(wall, { 0.0f + 2.5f + offset, 5.0f + camera_x - offset, -2.0f }, 1.0f, WHITE);
    DrawModel(wall, { 0.0f + 2.5f + offset, -5.0f + camera_x - offset, -2.0f }, 1.0f, WHITE);

    // Right wall
    DrawModel(wall, { 0.0f + 2.5f + offset, 0.0f + camera_x - offset, 2.0f }, 1.0f, WHITE);
    DrawModel(wall, { 0.0f + 2.5f + offset, 0.0f + 5.0f + camera_x - offset, 2.0f }, 1.0f, WHITE);
    DrawModel(wall, { 0.0f + 2.5f + offset, 0.0f - 5.0f + camera_x - offset, 2.0f }, 1.0f, WHITE);
}

void RenderMultipleChunks(const float& camera_x, Model& floor, Model& wall, Model& ceiling)
{
    float chunk_pos = (int)camera_x - (int)camera_x % 5;
    RenderChunk(camera_x, chunk_pos, floor, wall, ceiling);
    RenderChunk(camera_x, chunk_pos + 5.0f, floor, wall, ceiling);
    RenderChunk(camera_x, chunk_pos + 10.0f, floor, wall, ceiling);
    RenderChunk(camera_x, chunk_pos - 5.0f, floor, wall, ceiling);
    RenderChunk(camera_x, chunk_pos - 10.0f, floor, wall, ceiling);
}

float step_pitch, step_volume;
bool pixelizer_shader_enabled = true;

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    int screen_width = 640;
    int screen_height = 360;

    SetConfigFlags(FLAG_VSYNC_HINT);
    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    InitWindow(screen_width, screen_height, "SCP-087");
    int current_monitor = GetCurrentMonitor();
    screen_width = GetMonitorWidth(current_monitor);
    screen_height = GetMonitorHeight(current_monitor);
    SetWindowSize(screen_width, screen_height);
    ToggleFullscreen();

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = { 0.0f, 2.0f, 0.0f };    // Camera position
    camera.target = { 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = { 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    // init textures and shaders
    brick_texture = LoadTexture("textures/brick.png");
    concrete_texture = LoadTexture("textures/concrete.png");
    black_rock_texture = LoadTexture("textures/black_rock.png");
    light_shader = LoadShader("shaders/lighting.vs", "shaders/fog.fs");
    pixelizer_shader = LoadShader(0, "shaders/pixelizer.fs");
    bloom_shader = LoadShader(0, "shaders/overdraw.fs");
    //open_ai_shader = LoadShader(0, "shaders/openai.fs");
    RenderTexture2D target = LoadRenderTexture(screen_width, screen_height);
    light_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(light_shader, "matModel");
    light_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(light_shader, "viewPos");

    // Init sounds
    InitAudioDevice();
    Sound step = LoadSound("sounds/step_reverb.wav");
    Music bg = LoadMusicStream("sounds/bg.mp3");

    // Floor piece
    Model floor = LoadModelFromMesh(GenMeshCube(0.5f, 0.5f, 3.0f));
    floor.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = concrete_texture;
    floor.materials[0].shader = light_shader;

    Model ceiling = LoadModelFromMesh(GenMeshCube(0.25f, 0.25f, 3.0f));
    ceiling.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = black_rock_texture;
    ceiling.materials[0].shader = light_shader;

    Model wall = LoadModelFromMesh(GenMeshCube(5.0f, 5.0f, 1.0f));
    wall.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = brick_texture;
    wall.materials[0].shader = light_shader;

    Model back_wall = LoadModelFromMesh(GenMeshCube(1.0f, 10.0f, 10.0f));
    back_wall.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = brick_texture;
    back_wall.materials[0].shader = light_shader;

    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(light_shader, "ambient");
    float values[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    //float values[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    //float values[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    SetShaderValue(light_shader, ambientLoc, (const void*)(&values), SHADER_UNIFORM_VEC4);

    float fogDensity = 0.2f;
    int fogDensityLoc = GetShaderLocation(light_shader, "fogDensity");
    SetShaderValue(light_shader, fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);

    // Create lights
    Light lights[MAX_LIGHTS] = { 0 };
    lights[0] = CreateLight(LIGHT_POINT, camera.position, Vector3Zero(), {20, 20, 20, 255}, light_shader);

    SetCameraMode(camera, CAMERA_FIRST_PERSON);  // Set an orbital camera mode

    SetTargetFPS(60);                       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())            // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera);

        // Lock player movement
        // Left
        if (camera.position.z < -1.4)
            camera.position.z = -1.4;
        // Right
        if (camera.position.z > 1.4)
            camera.position.z = 1.4;
        // Back
        if (camera.position.x < 0)
            camera.position.x = 0;

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(light_shader, light_shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        // Check key inputs to enable/disable lights
        if (IsKeyPressed(KEY_F))
            lights[0].enabled = !lights[0].enabled;
        if (IsKeyPressed(KEY_G))
            pixelizer_shader_enabled = !pixelizer_shader_enabled;

        // Walking sounds
        if ((IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D)) && !IsSoundPlaying(step))
        {
            step_pitch = (float)GetRandomValue(60, 70) / 150;
            step_volume = (float)GetRandomValue(20, 40) / 150;
            SetSoundPitch(step, step_pitch);
            SetSoundVolume(step, step_volume);
            PlaySound(step);
        }

        // Background sound
        UpdateMusicStream(bg);
        if (!IsMusicStreamPlaying(bg))
        {
            SetMusicVolume(bg, 0.2f);
            PlayMusicStream(bg);
        }

        // Update light values (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            lights[i].position = camera.position;
            lights[i].position.y -= 0.5f;
            UpdateLightValues(light_shader, lights[i]);
        }
        //----------------------------------------------------------------------------------

        // Rendertexture pass 1
        BeginTextureMode(target);       // Enable drawing to texture
            ClearBackground(BLACK);  // Clear texture background
                BeginMode3D(camera);        // Begin 3d mode drawing

                    RenderMultipleChunks(camera.position.x, floor, wall, ceiling);

                    // Back wall
                    DrawModel(back_wall, { -5.75f, 10.0f + camera.position.x, 0.0f }, 1.0f, WHITE);
            
                EndMode3D();
        EndTextureMode();

        // Draw final image
        BeginDrawing();
        {
            ClearBackground(BLACK);
            if (pixelizer_shader_enabled)
                BeginShaderMode(pixelizer_shader);
                DrawTextureRec(target.texture, { 0, 0, (float)target.texture.width, (float)-target.texture.height }, { 0, 0 }, WHITE);
            EndShaderMode();
            DrawFPS(10, 10);
            DrawText(TextFormat("x%f y%f z%f", camera.position.x, camera.position.y, camera.position.z), 20, 40, 20, GREEN);
            DrawText(TextFormat("F - Enable/disable lighting shader"), 20, 60, 20, GREEN);
            DrawText(TextFormat("G - Enable/disable pixelizer shader"), 20, 80, 20, GREEN);
        }
        EndDrawing();
    }

    CloseWindow();
}
