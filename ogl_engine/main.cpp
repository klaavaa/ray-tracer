#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>

#include "rendering/framebuffer.h"
#include "rendering/shader.h"
#include "rendering/vao.h"
#include "rendering/vbo.h"
#include "rendering/ebo.h"

#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/mat3x2.hpp>
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "objparser.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Object.h"
#include <filesystem>


#define MAX_VERTEX_COUNT 5000
#define MAX_TRIMESH_COUNT 5
#define MAX_INDICES_COUNT 5000
#define MAX_SPHERE_COUNT 100

bool is_key_pressed(GLFWwindow* window, int key)
{
    int state = glfwGetKey(window, key);
    return state == GLFW_PRESS;
}

glm::mat3 rotate_x(const glm::mat3& m, float angle)
{
    glm::mat3 rotx;
    rotx[0] = glm::vec3(1, 0, 0);
    rotx[1] = glm::vec3(0, cosf(angle), -sinf(angle));
    rotx[2] = glm::vec3(0, sin(angle), cos(angle));
    return rotx;
}

glm::mat3 rotate_y(const glm::mat3& m, float angle)
{
    glm::mat3 roty;
    roty[0] = glm::vec3(cosf(angle), 0, sinf(angle));
    roty[1] = glm::vec3(0, 1, 0);
    roty[2] = glm::vec3(-sinf(angle), 0, cosf(angle));
    return roty;
}

static glm::vec2 last_mouse_pos = {1080.f / 2, 1920.f / 2};
static glm::vec2 delta_mouse_pos = {0.f, 0.f};
constexpr float sensitivity = 0.2f;
static bool lock_cursor = false;


static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!lock_cursor) return;
    
    delta_mouse_pos = last_mouse_pos - glm::vec2{ypos, xpos};
    last_mouse_pos = { ypos, xpos };
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {


        lock_cursor = !lock_cursor;
    if (!lock_cursor)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
    { 
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPos(window, last_mouse_pos.y, last_mouse_pos.x);
    }
    }
}



void updateTriMeshes(GLuint trimeshBufferID, std::vector<TriMesh>& trimeshes)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, trimeshBufferID);



    int trimesh_size = sizeof(glm::vec3) * MAX_VERTEX_COUNT
        + sizeof(glm::ivec3) * MAX_INDICES_COUNT
        + sizeof(glm::vec3)
        + sizeof(glm::vec4)
        + sizeof(float)
        + sizeof(AxisAllignedBox);
       

    int padding = trimesh_size % 16;
    if (padding)
    {
        padding = 16 - padding;
    }
    padding = 0;
    trimesh_size += padding;
        
    
    for (int i = 0; i < trimeshes.size(); i++)
    {
 
        glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), trimeshes[i].scale);
        glm::mat4 rotation = glm::rotate(glm::identity<glm::mat4>(), trimeshes[i].rotation.x, { 1.0, 0.0, 0.0 })
                * glm::rotate(glm::identity<glm::mat4>(), trimeshes[i].rotation.y, { 0.0, 1.0, 0.0 })
                * glm::rotate(glm::identity<glm::mat4>(), trimeshes[i].rotation.z, { 1.0, 0.0, 1.0 });

        glm::mat4 translate = glm::translate(glm::identity<glm::mat4>(), trimeshes[i].translation);
        trimeshes[i].transform = translate * rotation * scale;

        glm::vec3 near = { INFINITY, INFINITY, INFINITY };
        glm::vec3 far = {-INFINITY, -INFINITY, -INFINITY };

        trimeshes[i].transformed_vertices.clear();

        for (auto& vertex : trimeshes[i].vertices)
        {
            glm::vec3 transformed_vertex = trimeshes[i].transform * glm::vec4(vertex, 1.0f);

            trimeshes[i].transformed_vertices.push_back(transformed_vertex);

            near.x = std::min(transformed_vertex.x, near.x);
            near.y = std::min(transformed_vertex.y, near.y);
            near.z = std::min(transformed_vertex.z, near.z);

            far.x = std::max(transformed_vertex.x, far.x);
            far.y = std::max(transformed_vertex.y, far.y);
            far.z = std::max(transformed_vertex.z, far.z);


        }
      

        trimeshes[i].box.p1 = near;
        trimeshes[i].box.p2 = far;


        

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * trimesh_size,
            sizeof(glm::vec3) * trimeshes[i].transformed_vertices.size(), trimeshes[i].transformed_vertices.data());

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * trimesh_size + sizeof(glm::vec3) * MAX_VERTEX_COUNT,
            sizeof(glm::ivec3) * trimeshes[i].indices.size(), trimeshes[i].indices.data());


        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * trimesh_size + sizeof(glm::vec3) * MAX_VERTEX_COUNT
            + sizeof(glm::ivec3) * MAX_INDICES_COUNT, sizeof(glm::vec3), &trimeshes[i].material.color);

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * trimesh_size + sizeof(glm::vec3) * MAX_VERTEX_COUNT
            + sizeof(glm::ivec3) * MAX_INDICES_COUNT
            + sizeof(glm::vec3),

            sizeof(glm::vec4), &trimeshes[i].material.emission);

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * trimesh_size + sizeof(glm::vec3) * MAX_VERTEX_COUNT
            + sizeof(glm::ivec3) * MAX_INDICES_COUNT
            + sizeof(glm::vec3)
            + sizeof(glm::vec4),
            sizeof(float), &trimeshes[i].material.reflection);
        /*
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * trimesh_size + sizeof(glm::vec3) * MAX_VERTEX_COUNT
            + sizeof(glm::ivec3) * MAX_INDICES_COUNT
            + sizeof(glm::vec3)
            + sizeof(glm::vec4)
            + sizeof(float),
            sizeof(float[4][4]), &trimeshes[i].transform);*/

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * trimesh_size + sizeof(glm::vec3) * MAX_VERTEX_COUNT
            + sizeof(glm::ivec3) * MAX_INDICES_COUNT
            + sizeof(glm::vec3)
            + sizeof(glm::vec4)
            + sizeof(float),
            sizeof(AxisAllignedBox), &trimeshes[i].box);
   
    }
}


void updateSpheres(GLuint sphereBufferID, std::vector<Sphere>& spheres)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereBufferID);

    int sphere_size = sizeof(glm::vec3)
        + sizeof(float)
        + sizeof(glm::vec3)
        + sizeof(glm::vec4)
        + sizeof(float);

    for (int i = 0; i < spheres.size(); i++)
    {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sphere_size,
            sizeof(glm::vec3), &spheres[i].center);

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sphere_size + sizeof(glm::vec3),
            sizeof(float), &spheres[i].radius);

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sphere_size + sizeof(glm::vec3)
            + sizeof(float), sizeof(glm::vec3), &spheres[i].material.color);

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sphere_size + sizeof(glm::vec3)
            + sizeof(float) + sizeof(glm::vec3), sizeof(glm::vec4), &spheres[i].material.emission);

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sphere_size + sizeof(glm::vec3)
            + sizeof(float) + sizeof(glm::vec3) + sizeof(glm::vec4), sizeof(float), &spheres[i].material.reflection);
    }
}


void add_vec4_to_save(std::stringstream& ss, glm::vec4 v)
{
    ss << v.x << " " << v.y << " " << v.z << " " << v.w;
}

void add_vec3_to_save(std::stringstream& ss, glm::vec3 v)
{
    ss << v.x << " " << v.y << " " << v.z;
}

void add_vec2_to_save(std::stringstream& ss, glm::vec2 v)
{
    ss << v.x << " " << v.y;
}

void add_material_to_save(std::stringstream& ss, Material m)
{
    add_vec3_to_save(ss, m.color);
    ss << " ";
    add_vec4_to_save(ss, m.emission);
    ss << " " << m.reflection;
    
}


int main(void)
{
    GLFWwindow* window;

    int width = 1920;
    int height = 1080;
    std::string model_name = "models/box.obj";

    /* Initialize the library */
    if (!glfwInit())
        return -1;
    // using opengl version 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(width, height, "Hello World", glfwGetPrimaryMonitor(), NULL);

    if (!window)
    {
        std::cout << "failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glViewport(0, 0, width, height);

    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();



    float vertices[] = {
        //         X Y Z   |         RGBA          |  UV         // alpha ei vielä toimi kait
         1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // top right
         1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // bottom left
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f   // top left 
    };
    unsigned int indices[] = {
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);




    Framebuffer fbo(width, height);
    Framebuffer fbo2(width, height);
    Shader shader("shaders/default.vert", "shaders/default.frag");
    Shader shader_inverse("shaders/default.vert", "shaders/modified.frag");

    VAO vao;
    vao.Bind();
    VBO vbo(&vertices, sizeof(vertices));
    EBO ebo(&indices, sizeof(indices));
    // muuta linkattrip (kaikki) muotoon
    vao.LinkAttrib(vbo);

    /////////////////////////////////////////////////////////////////////////////////////////
    uint32_t previousFrameTex;
    glGenTextures(1, &previousFrameTex);
    glBindTexture(GL_TEXTURE_2D, previousFrameTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    Shader rayShader("shaders/rayVert.vert", "shaders/rayFrag.frag");
    Shader raySecondPass("shaders/rayVert2.vert", "shaders/rayFrag2.frag");

    double prevTime = 0.0;
    /* Loop until the user closes the window */
    uint32_t frameCounter = 0;


    glm::vec3 camera = { -20, 0, 0 };
    glm::vec2 camera_rot{ 0, 0 };

    glm::mat3 rotx;
    glm::mat3 roty;

    float focal_length = 1.0;



    std::vector<TriMesh> trimeshes;

    std::vector<Sphere> spheres;

    Sphere sphere;
    sphere.center = { 0.f, 0.f, -.5f };
    sphere.radius = 0.4f;
    sphere.material.color.b = 1.0f;
    spheres.push_back(sphere);

    Sphere sphere2;
    sphere2.center = { 4.f, 0.f, -.5f };
    sphere2.radius = 1.4f;
    sphere2.material.color.g = 1.0f;
    sphere2.material.reflection = 1.0f;
    spheres.push_back(sphere2);

    TriMesh trimesh = parse_obj(model_name);
    trimesh.material.color.r = 0.76f;
  
    trimeshes.push_back(trimesh);

    TriMesh trimesh2 = parse_obj("models/cylinder.obj");
    trimesh2.material.color.g = 1.0f;

    trimeshes.push_back(trimesh2);
     
    
    GLuint trimeshBufferID;
    glGenBuffers(1, &trimeshBufferID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, trimeshBufferID);
    
    int trimesh_size = sizeof(glm::vec3) * MAX_VERTEX_COUNT
        + sizeof(glm::ivec3) * MAX_INDICES_COUNT
        + sizeof(glm::vec3)
        + sizeof(glm::vec4)
        + sizeof(float)
        + sizeof(AxisAllignedBox);

    int padding = trimesh_size % 16;
    if (padding)
    {
        padding = 16 - padding;
    }
    padding = 0;
    trimesh_size += padding;
        
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_TRIMESH_COUNT * trimesh_size,
                                          nullptr, GL_STATIC_DRAW);




    glm::mat4 transform = glm::identity<glm::mat4>();

 
    updateTriMeshes(trimeshBufferID, trimeshes);
  
   

    GLuint sphereBufferID;
    glGenBuffers(1, &sphereBufferID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereBufferID);

    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_SPHERE_COUNT * (sizeof(glm::vec3)
                        + sizeof(float)
                        + sizeof(glm::vec3)
                        + sizeof(glm::vec4)
                        + sizeof(float)), nullptr, GL_STATIC_DRAW);

    updateSpheres(sphereBufferID, spheres);


    
    int sample_per_pixel = 1;
    int bounce_count = 4;
    int fraction_pixel_per_frame = 2;

    unsigned int time = 0;

    char save_name[128] = "save";


    glm::vec3 sky_color = { 0.529f, 0.808f, 0.922f };
    glm::vec3 horizont = { 0.8f, .8f, .8f };

    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();
        frameCounter++;
        time++;
        double curTime = glfwGetTime();
        double deltaTime = curTime - prevTime;
        prevTime = curTime;

       

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        //ImGui::ShowDemoWindow();

        ImGui::InputText("save file name", save_name, 128);

        if (ImGui::Button("save"))
        {
            std::stringstream ss;
            ss << "cam_pos " << camera.x << " " << camera.y << " " << camera.z << "\n";
            ss << "cam_rot " << camera_rot.x << " " << camera_rot.y << "\n";
            ss << "sky_color " << sky_color.x << " " << sky_color.y << " " << sky_color.z << "\n";
            ss << "horizont_color " << horizont.x << " " << horizont.y << " " << horizont.z << "\n";
            for (auto& sphere : spheres)
            {
                ss << "sphere ";
                add_vec3_to_save(ss, sphere.center);
                ss << " " << sphere.radius << " ";
                add_material_to_save(ss, sphere.material);
                ss << "\n";
            }

            for (auto& trimesh : trimeshes)
            {
                ss << "trimesh " << trimesh.filename << " ";
                add_vec3_to_save(ss, trimesh.translation);
                ss << " ";
                add_vec3_to_save(ss, trimesh.rotation);
                ss << " ";
                add_vec3_to_save(ss, trimesh.scale);
                ss << " ";
                add_material_to_save(ss, trimesh.material);
                ss << "\n";
            }


            save_scene(save_name, ss);
        }


        const std::string path_to_dir = "saves\\";

        std::vector<std::string> paths_to_models;
        paths_to_models.push_back("");
        for (const auto& entry : std::filesystem::directory_iterator(path_to_dir))
            paths_to_models.push_back(entry.path().string());

        static int path_index = 0;

        if (ImGui::Button("load"))
        {
            load_scene(paths_to_models[path_index], camera, camera_rot, sky_color, horizont, spheres, trimeshes);
            updateSpheres(sphereBufferID, spheres);
            updateTriMeshes(trimeshBufferID, trimeshes);
            frameCounter = 1;
        }

   
        ImGui::SameLine();


        if (ImGui::BeginCombo("scenes", paths_to_models[path_index].c_str()))
        {
            for (int n = 0; n < paths_to_models.size(); n++)
            {
                const bool is_selected = (path_index == n);
                if (ImGui::Selectable(paths_to_models[n].c_str(), is_selected))
                    path_index = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }


        if (ImGui::SliderInt("Sample per pixel", &sample_per_pixel, 1, 10))
        {
            frameCounter = 1;
        }
        if (ImGui::SliderInt("Bounce count", &bounce_count, 1, 10))
        {
            frameCounter = 1;
        }
        if (ImGui::SliderInt("Fraction pixel per frame", &fraction_pixel_per_frame, 1, 100))
        {
            frameCounter = 1;
        }


        ImGui::InputInt("width", &width);
        ImGui::InputInt("height", &height);

        if (ImGui::Button("resize"))
        {
            glfwSetWindowSize(window, width, height);
            frameCounter = 1;
        }

        if (ImGui::ColorEdit3("sky colour", &sky_color.x))
        {
            frameCounter = 1;
        }
        if (ImGui::ColorEdit3("horizont colour", &horizont.x))
        {
            frameCounter = 1;
        }

        ImGui::NewLine();
        int id = 0;

       
        if (ImGui::Button("add##0"))
        {
            TriMesh trimesh;
            trimeshes.push_back(trimesh);
            updateTriMeshes(trimeshBufferID, trimeshes);
            frameCounter = 1;
        }
        ImGui::SameLine();
        ImGui::Indent(40);

        if (ImGui::CollapsingHeader("trimeshes"))
        {
            ImGui::Indent();
            
            for (auto it = trimeshes.begin(); it != trimeshes.end();)
            {

                auto& trimesh = *it;
                ImGui::PushID(id);

                bool updated = false;

                if (ImGui::Button("remove"))
                {
                    it = trimeshes.erase(it);
                    updated = true;
                }
                else
                    it++;

                ImGui::SameLine();


                if (ImGui::CollapsingHeader(trimesh.name.c_str()))
                {
                    
                    ImGui::Indent();
                    
                    if (ImGui::DragFloat3("translation", &trimesh.translation.x, 0.1f))
                        updated = true;

                    if (ImGui::DragFloat3("rotation", &trimesh.rotation.x, 0.1f))
                        updated = true;

                    if (ImGui::DragFloat3("scale", &trimesh.scale.x, 0.1f))
                        updated = true;
                    
                    if (ImGui::ColorEdit3("color", &trimesh.material.color.r))
                        updated = true;

                    if (ImGui::ColorEdit3("emission", &trimesh.material.emission.r))
                        updated = true;

                    if (ImGui::SliderFloat("emission strength", &trimesh.material.emission.a, 0.0f, 100.f))
                        updated = true;

                    if (ImGui::SliderFloat("reflectivity", &trimesh.material.reflection, 0.0f, 1.0f))
                        updated = true;

                    
                    
                          
                    
                    const std::string path_to_dir = "models\\";

                    std::vector<std::string> paths_to_models;
                    paths_to_models.push_back("");
                    for (const auto& entry : std::filesystem::directory_iterator(path_to_dir))
                        paths_to_models.push_back(entry.path().string());

                    static int path_index = 0;

                    if (ImGui::Button("load"))
                    {
                        trimesh.vertices.clear();
                        trimesh.indices.clear();

                        parse_obj(paths_to_models[path_index], trimesh.vertices, trimesh.indices);
                        trimesh.filename = paths_to_models[path_index];
                        updated = true;
                    }

                    
                    ImGui::SameLine();

                    

                    if (ImGui::BeginCombo("models", paths_to_models[path_index].c_str()))
                    {
                        for (int n = 0; n < paths_to_models.size(); n++)
                        {
                            const bool is_selected = (path_index == n);
                            if (ImGui::Selectable(paths_to_models[n].c_str(), is_selected))
                                path_index = n;

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        
                        

                        ImGui::EndCombo();
                    }

                    
                    ImGui::Unindent();
                }
                if (updated)
                {
                    updateTriMeshes(trimeshBufferID, trimeshes);
                    frameCounter = 1;
                }
                id++;
                ImGui::PopID();
            }
            ImGui::Unindent();
        }
        ImGui::Unindent(40);
        
        if (ImGui::Button("add##1"))
        {
            Sphere sphere;
            spheres.push_back(sphere);
            updateSpheres(sphereBufferID, spheres);
            frameCounter = 1;
        }
        ImGui::SameLine();
        ImGui::Indent(40);
        if (ImGui::CollapsingHeader("spheres"))
        {
            ImGui::Indent();
            for (auto it = spheres.begin(); it != spheres.end();)
            {

                auto& sphere = *it;
                

                ImGui::PushID(id);

               
                bool updated = false;
                
                if (ImGui::Button("remove"))
                {
                    it = spheres.erase(it);
                    updated = true;
                }
                else
                    it++;

                ImGui::SameLine();

                if (ImGui::CollapsingHeader(sphere.name.c_str()))
                {
                    
                    ImGui::Indent();
                    
                 
                    if (ImGui::DragFloat3("center", &sphere.center.x, 0.1f))
                        updated = true;

                    if (ImGui::DragFloat("radius", &sphere.radius, 0.01f))
                        updated = true;
                    
                    if (ImGui::ColorEdit3("color", &sphere.material.color.r))
                        updated = true;

                    if (ImGui::ColorEdit3("emission", &sphere.material.emission.r))
                        updated = true;

                    if (ImGui::SliderFloat("emission strength", &sphere.material.emission.a, 0.0f, 100.f))
                        updated = true;

                    if (ImGui::SliderFloat("reflectivity", &sphere.material.reflection, 0.0f, 1.0f))
                        updated = true;

                 
                    ImGui::Unindent();
                }
               
                

                if (updated)
                {
                    updateSpheres(sphereBufferID, spheres);
                    frameCounter = 1;
                }
                id++;
                ImGui::PopID();
            }
            ImGui::Unindent();
        }
        ImGui::Unindent(40);


        if (delta_mouse_pos.x || delta_mouse_pos.y)
        {
            camera_rot += delta_mouse_pos * (float)deltaTime * sensitivity;
            delta_mouse_pos = { 0, 0 };
            frameCounter = 1;
        }

        if (is_key_pressed(window, GLFW_KEY_W))
        {
            rotx = rotate_x(rotx, camera_rot.x);
            roty = rotate_y(roty, camera_rot.y);
            glm::vec3 dir = { 0, 0, 1 };
            camera -= dir * rotx * roty * (float)deltaTime;
            frameCounter = 1;
        }
        if (is_key_pressed(window, GLFW_KEY_S))
        {
            rotx = rotate_x(rotx, camera_rot.x);
            roty = rotate_y(roty, camera_rot.y);
            glm::vec3 dir = { 0, 0, -1 };
            camera -= dir * rotx * roty * (float)deltaTime;
            frameCounter = 1;
        }
        if (is_key_pressed(window, GLFW_KEY_A))
        {
            rotx = rotate_x(rotx, camera_rot.x);
            roty = rotate_y(roty, camera_rot.y);
            glm::vec3 dir = { 1, 0, 0 };
            camera -= dir * rotx * roty * (float)deltaTime;
            frameCounter = 1;
        }
        if (is_key_pressed(window, GLFW_KEY_D))
        {
            rotx = rotate_x(rotx, camera_rot.x);
            roty = rotate_y(roty, camera_rot.y);
            glm::vec3 dir = { -1, 0, 0 };
            camera -= dir * rotx * roty * (float)deltaTime;
            frameCounter = 1;
        }

        if (is_key_pressed(window, GLFW_KEY_UP))
        {
            camera_rot.x += deltaTime;
            frameCounter = 1;
        }
        if (is_key_pressed(window, GLFW_KEY_DOWN))
        {
            camera_rot.x -= deltaTime;
            frameCounter = 1;
        }
        if (is_key_pressed(window, GLFW_KEY_LEFT))
        {
            camera_rot.y += deltaTime;
            frameCounter = 1;
        }
        if (is_key_pressed(window, GLFW_KEY_RIGHT))
        {
            camera_rot.y -= deltaTime;
            frameCounter = 1;
        }

        if (is_key_pressed(window, GLFW_KEY_SPACE))
        {

      
            glm::vec3 dir = { 0, -1, 0 };
            camera -= dir * (float)deltaTime;
            frameCounter = 1;
            
        }

        if (is_key_pressed(window, GLFW_KEY_LEFT_SHIFT))
        {

      
            glm::vec3 dir = { 0, 1, 0 };
            camera -= dir * (float)deltaTime;
            frameCounter = 1;

        }

        if (is_key_pressed(window, GLFW_KEY_COMMA))
        {
            focal_length -= deltaTime;
            frameCounter = 1;
        }

        if (is_key_pressed(window, GLFW_KEY_PERIOD))
        {
            focal_length += deltaTime;
            frameCounter = 1;
        }

        if (is_key_pressed(window, GLFW_KEY_P))
        {
            break;
        }


        // first pass
        
        fbo.Bind();
       
        rayShader.Bind();
        //rayShader.SetUInt("time", frameCounter);
        rayShader.SetUInt("time", time);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        rayShader.SetInt2("resolution", glm::ivec2(windowWidth, windowHeight));
        rayShader.SetFloat3("camera", camera);
        rayShader.SetFloat2("camera_rotation", camera_rot);
        rayShader.SetFloat("focal_length", focal_length);
        rayShader.SetInt("samples_per_pixel", sample_per_pixel);
        rayShader.SetInt("bounces", bounce_count);
        rayShader.SetInt("fraction_pixel_per_frame", fraction_pixel_per_frame);
        rayShader.SetInt("trimesh_count", trimeshes.size());
        rayShader.SetInt("sphere_count", spheres.size());

        rayShader.SetFloat3("sky_color", sky_color);
        rayShader.SetFloat3("horizont_color", horizont);


        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, trimeshBufferID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sphereBufferID);
        

       // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indicesBufferID);
         // TODODO
        for (size_t i = 0; i < trimeshes.size(); i++)
        {
            rayShader.SetInt("triangle_count[" + std::to_string(i) + "]", trimeshes[i].indices.size());
        }
        
 
       
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        // second pass
        glCopyImageSubData(fbo2.fbTex, GL_TEXTURE_2D, 0, 0, 0, 0, previousFrameTex, GL_TEXTURE_2D, 0, 0, 0, 0, width, height, 1);
        fbo2.Bind();
        raySecondPass.Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, previousFrameTex);
        glActiveTexture(GL_TEXTURE1); 
        glBindTexture(GL_TEXTURE_2D, fbo.fbTex);
        raySecondPass.SetInt("old_texture", 0);
        raySecondPass.SetInt("new_texture", 1);
        raySecondPass.SetInt("rendered_frames_count", frameCounter);
        raySecondPass.SetInt("fraction_pixel_per_frame", fraction_pixel_per_frame);
        raySecondPass.SetUInt("time", time);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // third pass
        fbo2.Unbind();
        shader.Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbo2.fbTex);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
 
    
  

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

