#include "openglwindow.hpp"

#include <imgui.h>

#include <cppitertools/itertools.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "imfilebrowser.h"
#include "glm/gtx/fast_trigonometry.hpp"

void OpenGLWindow::handleEvent(SDL_Event& event) {
  glm::ivec2 mousePosition;
  SDL_GetMouseState(&mousePosition.x, &mousePosition.y);
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)
            m_gameData.m_input.set(static_cast<size_t>(Input::Up));
        if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s)
            m_gameData.m_input.set(static_cast<size_t>(Input::Down));
        if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a)
            m_gameData.m_input.set(static_cast<size_t>(Input::Left));
        if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d)
            m_gameData.m_input.set(static_cast<size_t>(Input::Right));
    }
    if (event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w)
            m_gameData.m_input.reset(static_cast<size_t>(Input::Up));
        if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s)
            m_gameData.m_input.reset(static_cast<size_t>(Input::Down));
        if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a)
            m_gameData.m_input.reset(static_cast<size_t>(Input::Left));
        if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d)
            m_gameData.m_input.reset(static_cast<size_t>(Input::Right));
    }
  if (event.type == SDL_MOUSEMOTION) {
    m_trackBallModel.mouseMove(mousePosition);
    m_trackBallLight.mouseMove(mousePosition);
  }
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_trackBallModel.mousePress(mousePosition);
    }
    if (event.button.button == SDL_BUTTON_RIGHT) {
      m_trackBallLight.mousePress(mousePosition);
    }
  }
  if (event.type == SDL_MOUSEBUTTONUP) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_trackBallModel.mouseRelease(mousePosition);
    }
    if (event.button.button == SDL_BUTTON_RIGHT) {
      m_trackBallLight.mouseRelease(mousePosition);
    }
  }
  if (event.type == SDL_MOUSEWHEEL) {
    m_zoom += (event.wheel.y > 0 ? 1.0f : -1.0f) / 5.0f;
    m_zoom = glm::clamp(m_zoom, -1.5f, 1.0f);
  }
}

void OpenGLWindow::initializeGL() {
  abcg::glClearColor(0, 0, 0, 1);
  abcg::glEnable(GL_DEPTH_TEST);

  // Create programs
  for (const auto& name : m_shaderNames) {
    const auto path{getAssetsPath() + "shaders/" + name};
    const auto program{createProgramFromFile(path + ".vert", path + ".frag")};
    m_programs.push_back(program);
  }

  // Load default model
  loadModel(getAssetsPath() + "whale.obj");
  m_mappingMode = 2;  // "From mesh" option

  // Initial trackball spin
  m_trackBallModel.setAxis(glm::normalize(glm::vec3(1, 1, 1)));
  m_trackBallModel.setVelocity(0.0001f);
}

void OpenGLWindow::loadModel(std::string_view path) {
  m_model.terminateGL();

  m_model.loadDiffuseTexture(getAssetsPath() + "maps/whale.jpg");
  m_model.loadObj(path);
  m_model.setupVAO(m_programs.at(m_currentProgramIndex));
  m_trianglesToDraw = m_model.getNumTriangles();

  // Use material properties from the loaded model
  m_Ka = m_model.getKa();
  m_Kd = m_model.getKd();
  m_Ks = m_model.getKs();
  m_shininess = m_model.getShininess();
}

void OpenGLWindow::paintGL() {
  update();

  abcg::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  abcg::glViewport(0, 0, m_viewportWidth, m_viewportHeight);

  // Use currently selected program
  const auto program{m_programs.at(m_currentProgramIndex)};
  abcg::glUseProgram(program);

    m_modelMatrix = glm::rotate(m_modelMatrix, (float)M_PI, {0, 0, 1});

    m_modelMatrix = glm::translate(m_modelMatrix, m_model.m_position);
    m_modelMatrix = glm::rotate(m_modelMatrix,
                                (float) (-M_PI / 12 * cos(m_model.m_beta) - sin(m_model.m_phi) / 4),
                                {1, 0, 0});
    m_modelMatrix = glm::rotate(m_modelMatrix,
                                (float) (sin(m_model.m_theta) / 3),
                                {0, 1, 0});

    // Get location of uniform variables
  const GLint viewMatrixLoc{abcg::glGetUniformLocation(program, "viewMatrix")};
  const GLint projMatrixLoc{abcg::glGetUniformLocation(program, "projMatrix")};
  const GLint modelMatrixLoc{
      abcg::glGetUniformLocation(program, "modelMatrix")};
  const GLint normalMatrixLoc{
      abcg::glGetUniformLocation(program, "normalMatrix")};
  const GLint lightDirLoc{
      abcg::glGetUniformLocation(program, "lightDirWorldSpace")};
  const GLint shininessLoc{abcg::glGetUniformLocation(program, "shininess")};
  const GLint IaLoc{abcg::glGetUniformLocation(program, "Ia")};
  const GLint IdLoc{abcg::glGetUniformLocation(program, "Id")};
  const GLint IsLoc{abcg::glGetUniformLocation(program, "Is")};
  const GLint KaLoc{abcg::glGetUniformLocation(program, "Ka")};
  const GLint KdLoc{abcg::glGetUniformLocation(program, "Kd")};
  const GLint KsLoc{abcg::glGetUniformLocation(program, "Ks")};
  const GLint diffuseTexLoc{abcg::glGetUniformLocation(program, "diffuseTex")};
  const GLint mappingModeLoc{
      abcg::glGetUniformLocation(program, "mappingMode")};

  // Set uniform variables used by every scene object
  abcg::glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, &m_viewMatrix[0][0]);
  abcg::glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);
  abcg::glUniform1i(diffuseTexLoc, 0);
  abcg::glUniform1i(mappingModeLoc, m_mappingMode);

  const auto lightDirRotated{m_trackBallLight.getRotation() * m_lightDir};
  abcg::glUniform4fv(lightDirLoc, 1, &lightDirRotated.x);
  abcg::glUniform4fv(IaLoc, 1, &m_Ia.x);
  abcg::glUniform4fv(IdLoc, 1, &m_Id.x);
  abcg::glUniform4fv(IsLoc, 1, &m_Is.x);

  // Set uniform variables of the current object
  abcg::glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_modelMatrix[0][0]);

  const auto modelViewMatrix{glm::mat3(m_viewMatrix * m_modelMatrix)};
  glm::mat3 normalMatrix{glm::inverseTranspose(modelViewMatrix)};
  abcg::glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

  abcg::glUniform1f(shininessLoc, m_shininess);
  abcg::glUniform4fv(KaLoc, 1, &m_Ka.x);
  abcg::glUniform4fv(KdLoc, 1, &m_Kd.x);
  abcg::glUniform4fv(KsLoc, 1, &m_Ks.x);

  m_model.render(m_trianglesToDraw);

  abcg::glUseProgram(0);
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();

  // File browser for models
  static ImGui::FileBrowser fileDialogModel;
  fileDialogModel.SetTitle("Load 3D Model");
  fileDialogModel.SetTypeFilters({".obj"});
  fileDialogModel.SetWindowSize(m_viewportWidth * 0.8f,
                                m_viewportHeight * 0.8f);

  // File browser for textures
  static ImGui::FileBrowser fileDialogTex;
  fileDialogTex.SetTitle("Load Texture");
  fileDialogTex.SetTypeFilters({".jpg", ".png"});
  fileDialogTex.SetWindowSize(m_viewportWidth * 0.8f, m_viewportHeight * 0.8f);

// Only in WebGL
#if defined(__EMSCRIPTEN__)
  fileDialogModel.SetPwd(getAssetsPath());
  fileDialogTex.SetPwd(getAssetsPath() + "/maps");
#endif

  // Create main window widget
  {
    auto widgetSize{ImVec2(222, 50)};

    if (!m_model.isUVMapped()) {
      // Add extra space for static text
      widgetSize.y += 26;
    }

    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth - widgetSize.x - 5, 5));
    ImGui::SetNextWindowSize(widgetSize);
    const auto flags{ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration};
    ImGui::Begin("Widget window", nullptr, flags);

    abcg::glDisable(GL_CULL_FACE);
    const auto aspect{static_cast<float>(m_viewportWidth) /
                        static_cast<float>(m_viewportHeight)};
    abcg::glFrontFace(GL_CCW);
    
    m_projMatrix =
            glm::ortho(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f, 0.1f, 5.0f);

   //m_projMatrix =
   //         glm::perspective(glm::radians(45.0f), aspect, 0.1f, 5.0f);
    // Projection combo box


    // Shader combo box
    {
      static std::size_t currentIndex{};

      ImGui::PushItemWidth(120);
      if (ImGui::BeginCombo("Shader", m_shaderNames.at(currentIndex))) {
        for (auto index : iter::range(m_shaderNames.size())) {
          const bool isSelected{currentIndex == index};
          if (ImGui::Selectable(m_shaderNames.at(index), isSelected))
            currentIndex = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      // Set up VAO if shader program has changed
      if (static_cast<int>(currentIndex) != m_currentProgramIndex) {
        m_currentProgramIndex = currentIndex;
        m_model.setupVAO(m_programs.at(m_currentProgramIndex));
      }
    }
    //Whale has no material, so there's no UV mapping
    /*
    // UV mapping box
    {
      std::vector<std::string> comboItems{"Triplanar", "Cylindrical",
                                          "Spherical"};

      if (m_model.isUVMapped()) comboItems.emplace_back("From mesh");

      ImGui::PushItemWidth(120);
      if (ImGui::BeginCombo("UV mapping",
                            comboItems.at(m_mappingMode).c_str())) {
        for (auto index : iter::range(comboItems.size())) {
          const bool isSelected{m_mappingMode == static_cast<int>(index)};
          if (ImGui::Selectable(comboItems.at(index).c_str(), isSelected))
            m_mappingMode = index;
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();
    }*/

    ImGui::End();
  }

  // Create window for light sources
  if (m_currentProgramIndex < 4) {
    const auto widgetSize{ImVec2(222, 244)};
    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth - widgetSize.x - 5,
                                   m_viewportHeight - widgetSize.y - 5));
    ImGui::SetNextWindowSize(widgetSize);
    ImGui::Begin(" ", nullptr, ImGuiWindowFlags_NoDecoration);

    ImGui::Text("Light properties");

    // Slider to control light properties
    ImGui::PushItemWidth(widgetSize.x - 36);
    ImGui::ColorEdit3("Ia", &m_Ia.x, ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Id", &m_Id.x, ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Is", &m_Is.x, ImGuiColorEditFlags_Float);
    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::Text("Material properties");

    // Slider to control material properties
    ImGui::PushItemWidth(widgetSize.x - 36);
    ImGui::ColorEdit3("Ka", &m_Ka.x, ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Kd", &m_Kd.x, ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Ks", &m_Ks.x, ImGuiColorEditFlags_Float);
    ImGui::PopItemWidth();

    // Slider to control the specular shininess
    ImGui::PushItemWidth(widgetSize.x - 16);
    ImGui::SliderFloat("", &m_shininess, 0.0f, 500.0f, "shininess: %.1f");
    ImGui::PopItemWidth();

    ImGui::End();
  }

  fileDialogModel.Display();
  if (fileDialogModel.HasSelected()) {
    loadModel(fileDialogModel.GetSelected().string());
    fileDialogModel.ClearSelected();

    if (m_model.isUVMapped()) {
      // Use mesh texture coordinates if available...
      m_mappingMode = 3;
    } else {
      // ...or triplanar mapping otherwise
      m_mappingMode = 0;
    }
  }

  fileDialogTex.Display();
  if (fileDialogTex.HasSelected()) {
    m_model.loadDiffuseTexture(fileDialogTex.GetSelected().string());
    fileDialogTex.ClearSelected();
  }
}

void OpenGLWindow::resizeGL(int width, int height) {
  m_viewportWidth = width;
  m_viewportHeight = height;

  m_trackBallModel.resizeViewport(width, height);
  m_trackBallLight.resizeViewport(width, height);
}

void OpenGLWindow::terminateGL() {
  m_model.terminateGL();
  for (const auto& program : m_programs) {
    abcg::glDeleteProgram(program);
  }
}

void OpenGLWindow::update() {
    float deltaTime{static_cast<float>(getDeltaTime())};
    m_modelMatrix = m_trackBallModel.getRotation();
    m_angle = glm::wrapAngle(m_angle + glm::radians(90.0f) * 3 * deltaTime);

  m_viewMatrix =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f + m_zoom),
                  glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

  m_model.update(deltaTime, m_gameData);
}
