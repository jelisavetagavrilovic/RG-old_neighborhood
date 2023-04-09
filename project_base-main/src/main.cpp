#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(char const* path);
unsigned int loadCubemap(vector<std::string> faces);


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// day or night
bool isDay = true;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
void setPointLight(Shader shader, PointLight pointLight1, PointLight pointLight2, PointLight pointLight3,
                    PointLight pointLight4, PointLight pointLight5, PointLight pointLight6);

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
void setDirLight(Shader shader);

void setSpotLight(Shader shader, PointLight pointLight1, PointLight pointLight2, PointLight pointLight3,
                   PointLight pointLight4, PointLight pointLight5, PointLight pointLight6);


struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackRotate = 0.0f;
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef _APPLE_
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader lightCubeShader("resources/shaders/light_source.vs", "resources/shaders/light_source.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");


    // load models
    // -----------
    // farm house
    Model ourModelHouse("resources/objects/house/Farm_house.obj");
    ourModelHouse.SetShaderTextureNamePrefix("material.");
    // old company
    Model oldCompany("resources/objects/oldHouse/house_01.obj");
    oldCompany.SetShaderTextureNamePrefix("material.");
    // brick house
    Model brickHouse("resources/objects/BrickHouse/Brick_House.obj");
    brickHouse.SetShaderTextureNamePrefix("material.");
    // blue house
    Model blueHouse("resources/objects/blueHouse/HouseSuburban.obj");
    blueHouse.SetShaderTextureNamePrefix("material.");
    // pol house
    Model polHouse("resources/objects/polHouse1/polHouse1.obj");
    polHouse.SetShaderTextureNamePrefix("material.");

//    // tree
//    Model ourModelTree("resources/objects/tree/prunus_persica.obj");
//    ourModelTree.SetShaderTextureNamePrefix("material.");
//    // oldTree
//    Model oldTree1("resources/objects/oldTree/01/Col_1_tree_1.obj");
//    oldTree1.SetShaderTextureNamePrefix("material.");
//    Model oldTree2("resources/objects/oldTree/02/Col_1_tree_2.obj");
//    oldTree2.SetShaderTextureNamePrefix("material.");

    // road
    Model road("resources/objects/road/untitled.obj");
    road.SetShaderTextureNamePrefix("material.");

    //street lamp
    Model streetLamp("resources/objects/streetLamp/Street Lamp.obj");
    streetLamp.SetShaderTextureNamePrefix("material.");


    // pointlight
    PointLight& pointLight1 = programState->pointLight;
    pointLight1.position = glm::vec3(-7.0f, 7.6, -35.0);
    pointLight1.ambient = glm::vec3(0.1, 0.06, 0.0); // 0.1, 0.1, 0.1
    pointLight1.diffuse = glm::vec3(1.0, 0.6, 0.0);
    pointLight1.specular = glm::vec3(1.0, 0.6, 0.0);
    pointLight1.constant = 1.0f;
    pointLight1.linear = 0.09f; //0.09f
    pointLight1.quadratic = 0.032f; // 0.32f

    PointLight pointLight2;
    pointLight2.position = glm::vec3(-7.0f, 7.6, -3.0);
    pointLight2.ambient = glm::vec3(0.1, 0.06, 0.0); // 0.1, 0.1, 0.1
    pointLight2.diffuse = glm::vec3(1.0, 0.6, 0.0);
    pointLight2.specular = glm::vec3(1.0, 0.6, 0.0);
    pointLight2.constant = 1.0f;
    pointLight2.linear = 0.09f; //0.09f
    pointLight2.quadratic = 0.032f; // 0.32f

    PointLight pointLight3;
    pointLight3.position = glm::vec3(-7.0f, 7.6, 29.0);
    pointLight3.ambient = glm::vec3(0.1, 0.06, 0.0); // 0.1, 0.1, 0.1
    pointLight3.diffuse = glm::vec3(1.0, 0.6, 0.0);
    pointLight3.specular = glm::vec3(1.0, 0.6, 0.0);
    pointLight3.constant = 1.0f;
    pointLight3.linear = 0.09f; //0.09f
    pointLight3.quadratic = 0.032f; // 0.32f

    PointLight pointLight4;
    pointLight4.position = glm::vec3(4.5f, 7.6, -20.0);
    pointLight4.ambient = glm::vec3(0.1, 0.06, 0.0); // 0.1, 0.1, 0.1
    pointLight4.diffuse = glm::vec3(1.0, 0.6, 0.0);
    pointLight4.specular = glm::vec3(1.0, 0.6, 0.0);
    pointLight4.constant = 1.0f;
    pointLight4.linear = 0.09f; //0.09f
    pointLight4.quadratic = 0.032f; // 0.32f

    PointLight pointLight5;
    pointLight5.position = glm::vec3(4.5f, 7.6, 12.0);
    pointLight5.ambient = glm::vec3(0.1, 0.06, 0.0); // 0.1, 0.1, 0.1
    pointLight5.diffuse = glm::vec3(1.0, 0.6, 0.0);
    pointLight5.specular = glm::vec3(1.0, 0.6, 0.0);
    pointLight5.constant = 1.0f;
    pointLight5.linear = 0.09f; //0.09f
    pointLight5.quadratic = 0.032f; // 0.32f

    PointLight pointLight6;
    pointLight6.position = glm::vec3(4.5f,7.6, 44.0);
    pointLight6.ambient = glm::vec3(0.1, 0.06, 0.0); // 0.1, 0.1, 0.1
    pointLight6.diffuse = glm::vec3(1.0, 0.6, 0.0);
    pointLight6.specular = glm::vec3(1.0, 0.6, 0.0);
    pointLight6.constant = 1.0f;
    pointLight6.linear = 0.09f; //0.09f
    pointLight6.quadratic = 0.032f; // 0.32f




    // vertices
    float planeVertices[] = {
            // positions         //normals         // texture Coords
            5.0f,  -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 16.0f, 0.0f,
            -5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 16.0f,

            5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 16.0f, 0.0f,
            -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 16.0f,
            5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 16.0f, 16.0f
    };

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

//    float roadVertices[] = {
//            // positions         //normals         // texture Coords
//            5.0f,  0.05f,  5.0f, 0.0f, 1.0f, 0.0f, 20.0f, 0.0f,
//            -5.0f, 0.05f,  5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
//            -5.0f, 0.05f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 20.0f,
//
//            5.0f, 0.05f,  5.0f, 0.0f, 1.0f, 0.0f, 20.0f, 0.0f,
//            -5.0f, 0.05f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 20.0f,
//            5.0f, 0.05f, -5.0f, 0.0f, 1.0f, 0.0f, 20.0f, 20.0f
//    };

    float cubeVertices[] = {
            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f
    };

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };


    // VAO, VBO
    //----------
    // skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // plane
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

//    // road
//    unsigned int roadVAO, roadVBO;
//    glGenVertexArrays(1, &roadVAO);
//    glGenBuffers(1, &roadVBO);
//    glBindVertexArray(roadVAO);
//    glBindBuffer(GL_ARRAY_BUFFER, roadVBO);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(roadVertices), &roadVertices, GL_STATIC_DRAW);
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//    glEnableVertexAttribArray(2);
//    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // light
    unsigned int lightCubeVAO, lightCubeVBO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &lightCubeVBO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glBindVertexArray(lightCubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // load textures
    unsigned int grassTexture = loadTexture(FileSystem::getPath("resources/textures/grass.jpeg").c_str());
    //unsigned int roadTexture = loadTexture(FileSystem::getPath("resources/textures/road/cobblestone_large_01_diff_4k.jpg").c_str());
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/bush.png").c_str());

    vector<std::string> day
          {
                    FileSystem::getPath("resources/textures/skyboxDay/right.png"),
                    FileSystem::getPath("resources/textures/skyboxDay/left.png"),
                    FileSystem::getPath("resources/textures/skyboxDay/top.png"),
                    FileSystem::getPath("resources/textures/skyboxDay/bottom.png"),
                    FileSystem::getPath("resources/textures/skyboxDay/front.png"),
                    FileSystem::getPath("resources/textures/skyboxDay/back.png")
            };
    unsigned int cubemapTextureDay = loadCubemap(day);

    vector<std::string> night
            {
                    FileSystem::getPath("resources/textures/skyboxNight/right.jpg"),
                    FileSystem::getPath("resources/textures/skyboxNight/left.jpg"),
                    FileSystem::getPath("resources/textures/skyboxNight/top.jpg"),
                    FileSystem::getPath("resources/textures/skyboxNight/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skyboxNight/front.jpg"),
                    FileSystem::getPath("resources/textures/skyboxNight/back.jpg")
            };
    unsigned int cubemapTextureNight = loadCubemap(night);


    // skybox shader configuration
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    blendingShader.use();
    blendingShader.setInt("texture1", 0);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        ourShader.setInt("material.texture_diffuse1", 0);
        //ourShader.setInt("material.texture_specular1", 1);
        //pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));

        setDirLight(ourShader);
        setPointLight(ourShader, pointLight1, pointLight2, pointLight3, pointLight4, pointLight5, pointLight6);
        setSpotLight(ourShader, pointLight1, pointLight2, pointLight3, pointLight4, pointLight5, pointLight6);



        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 3000.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);


        // render the loaded model backpack
        //glm::mat4 model = glm::mat4(1.0f); //inicijalizacija
        //model = glm::translate(model, programState->backpackPosition); // translate it down so it's at the center of the scene
        //model = glm::scale(model, glm::vec3(programState->backpackScale));    // it's a bit too big for our scene, so scale it down
        //ourShader.setMat4("model", model);
        //ourMOdel.Draw(ourShader);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f); //inicijalizacija

        // house
        model = glm::mat4(1.0f); //inicijalizacija
        model = glm::translate(model,glm::vec3(22.0f, 9.8f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2, 0.2, 0.2));
        ourShader.setMat4("model", model);
        ourModelHouse.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(22.0f, 0.0f, -26.0f));
        model = glm::rotate(model, 1.60f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.125f, 0.125f, 0.125f));
        ourShader.setMat4("model", model);
        oldCompany.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = model = glm::translate(model, glm::vec3(-22.0f, 0, -22.0));
        //model = glm::translate(model, programState->backpackPosition); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
        //model = glm::scale(model, glm::vec3(programState->backpackScale));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        blueHouse.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = model = glm::translate(model, glm::vec3(-22.0f, 0, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.3f, 1.3f, 1.3f));
        ourShader.setMat4("model", model);
        brickHouse.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = model = glm::translate(model, glm::vec3(-18.0f, 0, 17.0));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        ourShader.setMat4("model", model);
        polHouse.Draw(ourShader);

        // trees
//        int xt = -5, yt = 0, zt = 17;
//        for(int i = 0; i < 3; i++)
//        {
//            model = glm::mat4(1.0f);
//            model = glm::translate(model,glm::vec3(xt, yt, zt));
//            model = glm::scale(model, glm::vec3(0.2, 0.2, 0.2));
//            ourShader.setMat4("model", model);
//            ourModelTree.Draw(ourShader);
//            xt += 12;
//        }

//        model = glm::mat4(1.0f);
//        model = model = glm::translate(model, glm::vec3(-36.0f, 0, -32.0));
//        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//        model = glm::scale(model, glm::vec3(0.75f, 0.75f, 0.75f));
//        ourShader.setMat4("model", model);
//        oldTree1.Draw(ourShader);
//
//        model = glm::mat4(1.0f);
//        model = glm::translate(model, programState->backpackPosition);
//        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//        model = glm::scale(model, glm::vec3(programState->backpackScale));
//        //model = model = glm::translate(model, glm::vec3(-34.0f, 0, -32.0));
//        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//        //model = glm::scale(model, glm::vec3(0.75f, 0.75f, 0.75f));
//        ourShader.setMat4("model", model);
//        oldTree2.Draw(ourShader);

        // road
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0, -1.0f, 2.2f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(3.1f, 1.0f, 2.5f));
        ourShader.setMat4("model", model);
        road.Draw(ourShader);

        // street lamp
        float zl = -35.0f;
        for(int i = 0; i < 3; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-7.0f, 2.2f, zl));
            model = glm::scale(model, glm::vec3(0.005f, 0.005f, 0.005f));
            ourShader.setMat4("model", model);
            streetLamp.Draw(ourShader);

            zl += 32;
        }
        zl = -20.0f;
        for(int i = 0; i < 3; i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(4.5f, 2.2f, zl));
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.005f, 0.005f, 0.005f));
            ourShader.setMat4("model", model);
            streetLamp.Draw(ourShader);

            zl += 32;
        }

        // grass
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(10, 0, 10));
        ourShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // vegetation
        vector<glm::vec3> vegetationPositions = {
                glm::vec3(4.1f, 0.82f, -19.7f),
                glm::vec3(-10.0f, 0.82f, -6.0f),
                glm::vec3(-14.0f, 0.8f, -10.0f),
                glm::vec3(14.0f, 0.8f, -11.0f),
                glm::vec3(15.2f, 0.8f, -11.5f),
                glm::vec3(6.2f, 0.8f, 38.0f),
                glm::vec3(-14.0f, 0.8f, 27.0f),
                glm::vec3(-8.2f, 0.8f, 31.37f),
        };
        blendingShader.use();
        setDirLight(blendingShader);
        setPointLight(blendingShader, pointLight1, pointLight2, pointLight3, pointLight4, pointLight5, pointLight6);
        setSpotLight(blendingShader, pointLight1, pointLight2, pointLight3, pointLight4, pointLight5, pointLight6);
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (unsigned int i = 0; i < vegetationPositions.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetationPositions[i]);
            model = glm::scale(model, glm::vec3(1.7f));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // light
        glm::vec3 pointLightPositions[] = {
                glm::vec3(-4.85f, 7.6f, -35.0f),
                glm::vec3(-4.85f, 7.6f, -3.0f),
                glm::vec3(-4.85f, 7.6f, 29.0f),
                glm::vec3(2.35f, 7.6f, -20.0f),
                glm::vec3(2.35f, 7.6f, 12.0f),
                glm::vec3(2.35f, 7.6f, 44.0f)
        };
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        glBindVertexArray(lightCubeVAO);
        for(int i = 0; i < 6; i++)
        {
            if(!isDay)
            {
                model = glm::mat4(1.0f);
                model = glm::translate(model, pointLightPositions[i]);
                model = glm::scale(model, glm::vec3(0.25f, 0.01f, 0.08f));
                lightCubeShader.setVec3("lightColor", glm::vec3(0.9f, 0.8f, 0.6f));
                lightCubeShader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }



//        // road
//        glBindVertexArray(roadVAO);
//        glBindTexture(GL_TEXTURE_2D, roadTexture);
//        model = glm::mat4(1.0f);
//        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
//        model = glm::scale(model, glm::vec3(9.8f, 1.0f, 1.0f));
//        ourShader.setMat4("model", model);
//        glDrawArrays(GL_TRIANGLES, 0, 6);



        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        skyboxShader.setInt("skybox", 0);
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        if(isDay)
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureDay);
        else
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureNight);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &planeVAO);
    //glDeleteVertexArrays(1, &roadVAO);
    glDeleteVertexArrays(1, &transparentVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &skyboxVAO);
    glDeleteBuffers(1, &planeVAO);
    //glDeleteBuffers(1, &roadVAO);
    glDeleteBuffers(1, &lightCubeVAO);
    glDeleteBuffers(1, &transparentVAO);

    glfwTerminate();
    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);
        ImGui::DragFloat("rotate", &programState->backpackRotate, 0.05, 0.0, 6.5);
        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
        isDay = !isDay;
}
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_MIRRORED_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void setDirLight(Shader shader)
{
    if(isDay)
    {
        // light on day
        shader.setVec3("dirLight.direction", 0.7f, -0.5f, -0.5f);
        shader.setVec3("dirLight.ambient", 0.23f, 0.24f, 0.14f);
        shader.setVec3("dirLight.diffuse", 0.65f, 0.42f, 0.26f);
        shader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
    }
    else
    {
        // Light on night
        shader.setVec3("dirLight.direction", -0.5f, 0.9f, -0.2f);
        shader.setVec3("dirLight.ambient", 0.05f, 0.034f, 0.024f);
        shader.setVec3("dirLight.diffuse", 0.08f, 0.052f, 0.036f);
        shader.setVec3("dirLight.specular", 0.05f, 0.05f, 0.05f);
    }
}

void setPointLight(Shader shader, PointLight pointLight1, PointLight pointLight2, PointLight pointLight3,
              PointLight pointLight4, PointLight pointLight5, PointLight pointLight6)
{
    if(isDay)
    {
        // light on day
        shader.setVec3("pointLight[0].position", pointLight1.position);
        shader.setVec3("pointLight[0].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[0].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[0].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("pointLight[0].constant", pointLight1.constant);
        shader.setFloat("pointLight[0].linear", pointLight1.linear);
        shader.setFloat("pointLight[0].quadratic", pointLight1.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[1].position", pointLight2.position);
        shader.setVec3("pointLight[1].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[1].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[1].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("pointLight[1].constant", pointLight2.constant);
        shader.setFloat("pointLight[1].linear", pointLight2.linear);
        shader.setFloat("pointLight[1].quadratic", pointLight2.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[2].position", pointLight3.position);
        shader.setVec3("pointLight[2].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[2].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[2].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("pointLight[2].constant", pointLight3.constant);
        shader.setFloat("pointLight[2].linear", pointLight3.linear);
        shader.setFloat("pointLight[2].quadratic", pointLight3.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[3].position", pointLight4.position);
        shader.setVec3("pointLight[3].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[3].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[3].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("pointLight[3].constant", pointLight4.constant);
        shader.setFloat("pointLight[3].linear", pointLight4.linear);
        shader.setFloat("pointLight[3].quadratic", pointLight4.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[4].position", pointLight5.position);
        shader.setVec3("pointLight[4].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[4].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[4].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("pointLight[4].constant", pointLight5.constant);
        shader.setFloat("pointLight[4].linear", pointLight5.linear);
        shader.setFloat("pointLight[4].quadratic", pointLight5.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[5].position", pointLight6.position);
        shader.setVec3("pointLight[5].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[5].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("pointLight[5].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("pointLight[5].constant", pointLight6.constant);
        shader.setFloat("pointLight[5].linear", pointLight6.linear);
        shader.setFloat("pointLight[5].quadratic", pointLight6.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);
    }
    else
    {
        shader.setVec3("pointLight[0].position", pointLight1.position);
        shader.setVec3("pointLight[0].ambient", pointLight1.ambient);
        shader.setVec3("pointLight[0].diffuse", pointLight1.diffuse);
        shader.setVec3("pointLight[0].specular", pointLight1.specular);
        shader.setFloat("pointLight[0].constant", pointLight1.constant);
        shader.setFloat("pointLight[0].linear", pointLight1.linear);
        shader.setFloat("pointLight[0].quadratic", pointLight1.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[1].position", pointLight2.position);
        shader.setVec3("pointLight[1].ambient", pointLight2.ambient);
        shader.setVec3("pointLig[1].diffuse", pointLight2.diffuse);
        shader.setVec3("pointLight[1].specular", pointLight2.specular);
        shader.setFloat("pointLight[1].constant", pointLight2.constant);
        shader.setFloat("pointLight[1].linear", pointLight2.linear);
        shader.setFloat("pointLight[1].quadratic", pointLight2.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[2].position", pointLight3.position);
        shader.setVec3("pointLight[2].ambient", pointLight3.ambient);
        shader.setVec3("pointLight[2].diffuse", pointLight3.diffuse);
        shader.setVec3("pointLight[2].specular", pointLight3.specular);
        shader.setFloat("pointLight[2].constant", pointLight3.constant);
        shader.setFloat("pointLight[2].linear", pointLight3.linear);
        shader.setFloat("pointLight[2].quadratic", pointLight3.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[3].position", pointLight4.position);
        shader.setVec3("pointLight[3].ambient", pointLight4.ambient);
        shader.setVec3("pointLight[3].diffuse", pointLight4.diffuse);
        shader.setVec3("pointLight[3].specular", pointLight4.specular);
        shader.setFloat("pointLight[3].constant", pointLight4.constant);
        shader.setFloat("pointLight[3].linear", pointLight4.linear);
        shader.setFloat("pointLight[3].quadratic", pointLight4.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[4].position", pointLight5.position);
        shader.setVec3("pointLight[4].ambient", pointLight5.ambient);
        shader.setVec3("pointLight[4].diffuse", pointLight5.diffuse);
        shader.setVec3("pointLight[4].specular", pointLight5.specular);
        shader.setFloat("pointLight[4].constant", pointLight5.constant);
        shader.setFloat("pointLight[4].linear", pointLight5.linear);
        shader.setFloat("pointLight[4].quadratic", pointLight5.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);

        shader.setVec3("pointLight[5].position", pointLight6.position);
        shader.setVec3("pointLight[5].ambient", pointLight6.ambient);
        shader.setVec3("pointLight[5].diffuse", pointLight6.diffuse);
        shader.setVec3("pointLight[5].specular", pointLight6.specular);
        shader.setFloat("pointLight[5].constant", pointLight6.constant);
        shader.setFloat("pointLigh[5].linear", pointLight6.linear);
        shader.setFloat("pointLight[5].quadratic", pointLight6.quadratic);
        shader.setVec3("viewPosition", programState->camera.Position);
        shader.setFloat("material.shininess", 32.0f);
    }
}

void setSpotLight(Shader shader, PointLight pointLight1, PointLight pointLight2, PointLight pointLight3,
                  PointLight pointLight4, PointLight pointLight5, PointLight pointLight6)
{
    if(isDay)
    {
        shader.setVec3("spotLight[0].position", pointLight1.position);
        shader.setVec3("spotLight[0].direction", glm::vec3(0, 0.0, 0));
        shader.setVec3("spotLight[0].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[0].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[0].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("spotLight[0].constant", pointLight1.constant);
        shader.setFloat("spotLight[0].linear", pointLight1.linear);
        shader.setFloat("spotLight[0].quadratic", pointLight1.quadratic);
        shader.setFloat("spotLight[0].cutOff", glm::cos(glm::radians(2.0f)));
        shader.setFloat("spotLight[0].outerCutOff", glm::cos(glm::radians(24.0f)));

        shader.setVec3("spotLight[1].position", pointLight2.position);
        shader.setVec3("spotLight[1].direction", glm::vec3(0, 0.0, 0));
        shader.setVec3("spotLight[1].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[1].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[1].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("spotLight[1].constant", pointLight2.constant);
        shader.setFloat("spotLight[1].linear", pointLight2.linear);
        shader.setFloat("spotLight[1].quadratic", pointLight2.quadratic);
        shader.setFloat("spotLight[1].cutOff", glm::cos(glm::radians(12.0f)));
        shader.setFloat("spotLight[1].outerCutOff", glm::cos(glm::radians(13.5f)));

        shader.setVec3("spotLight[2].position", pointLight3.position);
        shader.setVec3("spotLight[2].direction", glm::vec3(0, 0.0, 0));
        shader.setVec3("spotLight[2].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[2].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[2].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("spotLight[2].constant", pointLight3.constant);
        shader.setFloat("spotLight[2].linear", pointLight3.linear);
        shader.setFloat("spotLight[2].quadratic", pointLight3.quadratic);
        shader.setFloat("spotLight[2].cutOff", glm::cos(glm::radians(12.0f)));
        shader.setFloat("spotLight[2].outerCutOff", glm::cos(glm::radians(13.5f)));

        shader.setVec3("spotLight[3].position", pointLight4.position);
        shader.setVec3("spotLight[3].direction", glm::vec3(0, 0.0, 0));
        shader.setVec3("spotLight[3].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[3].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[3].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("spotLight[3].constant", pointLight4.constant);
        shader.setFloat("spotLight[3].linear", pointLight4.linear);
        shader.setFloat("spotLight[3].quadratic", pointLight4.quadratic);
        shader.setFloat("spotLight[3].cutOff", glm::cos(glm::radians(12.0f)));
        shader.setFloat("spotLight[3].outerCutOff", glm::cos(glm::radians(13.5f)));

        shader.setVec3("spotLight[4].position", pointLight5.position);
        shader.setVec3("spotLight[4].direction", glm::vec3(0, 0.0, 0));
        shader.setVec3("spotLight[4].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[4].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[4].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("spotLight[4].constant", pointLight5.constant);
        shader.setFloat("spotLight[4].linear", pointLight5.linear);
        shader.setFloat("spotLight[4].quadratic", pointLight5.quadratic);
        shader.setFloat("spotLight[4].cutOff", glm::cos(glm::radians(12.0f)));
        shader.setFloat("spotLight[4].outerCutOff", glm::cos(glm::radians(13.5f)));

        shader.setVec3("spotLight[5].position", pointLight6.position);
        shader.setVec3("spotLight[5].direction", glm::vec3(0, 0.0, 0));
        shader.setVec3("spotLight[5].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[5].diffuse", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[5].specular", glm::vec3(0.0, 0.0, 0.0));
        shader.setFloat("spotLight[5].constant", pointLight6.constant);
        shader.setFloat("spotLight[5].linear", pointLight6.linear);
        shader.setFloat("spotLight[5].quadratic", pointLight6.quadratic);
        shader.setFloat("spotLight[5].cutOff", glm::cos(glm::radians(2.0f)));
        shader.setFloat("spotLight[5].outerCutOff", glm::cos(glm::radians(13.5f)));
    }
    else
    {
        shader.setVec3("spotLight[0].position", pointLight1.position);
        shader.setVec3("spotLight[0].direction", glm::vec3(0.2, -1.0, 0));
        shader.setVec3("spotLight[0].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[0].diffuse", glm::vec3(0.7, 0.7, 0.0));
        shader.setVec3("spotLight[0].specular", glm::vec3(0.5, 0.5, 0.0));
        shader.setFloat("spotLight[0].constant", pointLight1.constant);
        shader.setFloat("spotLight[0].linear", pointLight1.linear);
        shader.setFloat("spotLight[0].quadratic", pointLight1.quadratic);
        shader.setFloat("spotLight[0].cutOff", glm::cos(glm::radians(8.0f)));
        shader.setFloat("spotLight[0].outerCutOff", glm::cos(glm::radians(24.0f)));

        shader.setVec3("spotLight[1].position", pointLight2.position);
        shader.setVec3("spotLight[1].direction", glm::vec3(0.2, -1.0, 0));
        shader.setVec3("spotLight[1].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[1].diffuse", glm::vec3(0.7, 0.7, 0.0));
        shader.setVec3("spotLight[1].specular", glm::vec3(0.5, 0.5, 0.0));
        shader.setFloat("spotLight[1].constant", pointLight2.constant);
        shader.setFloat("spotLight[1].linear", pointLight2.linear);
        shader.setFloat("spotLight[1].quadratic", pointLight2.quadratic);
        shader.setFloat("spotLight[1].cutOff", glm::cos(glm::radians(8.0f)));
        shader.setFloat("spotLight[1].outerCutOff", glm::cos(glm::radians(24.0f)));

        shader.setVec3("spotLight[2].position", pointLight3.position);
        shader.setVec3("spotLight[2].direction", glm::vec3(0.2, -1.0, 0));
        shader.setVec3("spotLight[2].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[2].diffuse", glm::vec3(0.7, 0.7, 0.0));
        shader.setVec3("spotLight[2].specular", glm::vec3(0.5, 0.5, 0.0));
        shader.setFloat("spotLight[2].constant", pointLight3.constant);
        shader.setFloat("spotLight[2].linear", pointLight3.linear);
        shader.setFloat("spotLight[2].quadratic", pointLight3.quadratic);
        shader.setFloat("spotLight[2].cutOff", glm::cos(glm::radians(8.0f)));
        shader.setFloat("spotLight[2].outerCutOff", glm::cos(glm::radians(24.0f)));

        shader.setVec3("spotLight[3].position", pointLight4.position);
        shader.setVec3("spotLight[3].direction", glm::vec3(-0.2, -1.0, 0));
        shader.setVec3("spotLight[3].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[3].diffuse", glm::vec3(0.7, 0.7, 0.0));
        shader.setVec3("spotLight[3].specular", glm::vec3(0.5, 0.5, 0.0));
        shader.setFloat("spotLight[3].constant", pointLight4.constant);
        shader.setFloat("spotLight[3].linear", pointLight4.linear);
        shader.setFloat("spotLight[3].quadratic", pointLight4.quadratic);
        shader.setFloat("spotLight[3].cutOff", glm::cos(glm::radians(8.0f)));
        shader.setFloat("spotLight[3].outerCutOff", glm::cos(glm::radians(24.0f)));

        shader.setVec3("spotLight[4].position", pointLight5.position);
        shader.setVec3("spotLight[4].direction", glm::vec3(-0.2, -1.0, 0));
        shader.setVec3("spotLight[4].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[4].diffuse", glm::vec3(0.7, 0.7, 0.0));
        shader.setVec3("spotLight[4].specular", glm::vec3(0.5, 0.5, 0.0));
        shader.setFloat("spotLight[4].constant", pointLight5.constant);
        shader.setFloat("spotLight[4].linear", pointLight5.linear);
        shader.setFloat("spotLight[4].quadratic", pointLight5.quadratic);
        shader.setFloat("spotLight[4].cutOff", glm::cos(glm::radians(8.0f)));
        shader.setFloat("spotLight[4].outerCutOff", glm::cos(glm::radians(24.0f)));

        shader.setVec3("spotLight[5].position", pointLight6.position);
        shader.setVec3("spotLight[5].direction", glm::vec3(-0.2, -1.0, 0));
        shader.setVec3("spotLight[5].ambient", glm::vec3(0.0, 0.0, 0.0));
        shader.setVec3("spotLight[5].diffuse", glm::vec3(0.7, 0.7, 0.0));
        shader.setVec3("spotLight[5].specular", glm::vec3(0.5, 0.5, 0.0));
        shader.setFloat("spotLight[5].constant", pointLight6.constant);
        shader.setFloat("spotLight[5].linear", pointLight6.linear);
        shader.setFloat("spotLight[5].quadratic", pointLight6.quadratic);
        shader.setFloat("spotLight[5].cutOff", glm::cos(glm::radians(8.0f)));
        shader.setFloat("spotLight[5].outerCutOff", glm::cos(glm::radians(24.0f)));
    }
}