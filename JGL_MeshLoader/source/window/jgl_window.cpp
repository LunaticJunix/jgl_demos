#include "pch.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "jgl_window.h"
#include "elems/input.h"
#include "application.h"



namespace nwindow
{
  bool GLWindow::init(int width, int height, const std::string& title)
  {
    Width = width;
    Height = height;
    Title = title;

    mRenderCtx->init(this);

    mFrameBuffer->create_buffers(Width, Height);

    mUICtx->init(this);

    auto aspect = (float)width / (float)height;
    mShader = std::make_unique<Shader>();
    mShader->load("shaders/vs.shader", "shaders/fs.shader");

    mCamera = std::make_unique<Camera>(glm::vec3(0, 0, 3), 45.0f, aspect, 0.1f, 100.0f);

    mLight = std::make_unique<Light>();

    mPropertyPanel = std::make_unique<UI_Property_Panel>(mLight.get());

    load_mesh();

    mShader->use();

    return mIsRunning;
  }

  GLWindow::~GLWindow()
  {
    mUICtx->end();

    mShader->unload();

    mRenderCtx->end();

    if (mShader)
    {
      mShader->unload();
    }
  }

  void GLWindow::on_resize(int width, int height)
  {
    Width = width;
    Height = height;

    mFrameBuffer->create_buffers(Width, Height);

    render();
  }

  void GLWindow::on_key(int key, int scancode, int action, int mods)
  {
    if (action == GLFW_PRESS)
    {
    }
  }

  void GLWindow::on_close()
  {
    mIsRunning = false;
  }

  void GLWindow::render()
  {

    mLight->update(mShader.get());

    // Render to scene to framebuffer
    mRenderCtx->pre_render();

    mFrameBuffer->bind();

    // TODO: render meshes in render ctx
    if (mMesh)
    {
      mMesh->render();
    }

    mFrameBuffer->unbind();

    // Render UI components
    mUICtx->pre_render();

    mPropertyPanel->render();

    ImGui::Begin("Content");
      
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    glm::vec2 v = { viewportPanelSize.x, viewportPanelSize.y };

    mCamera->set_aspect(v.x / v.y);
    mCamera->update(mShader.get());

    uint64_t textureID = mFrameBuffer->get_texture();
    ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ v.x, v.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

    ImGui::End();

    mUICtx->post_render();

    handle_input();

    // Render end, swap buffers
    mRenderCtx->post_render();

  }

  void GLWindow::handle_input()
  {
    // TODO: move this and camera to scene UI component

    if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
      mCamera->set_distance(-0.1f);
    }

    if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
      mCamera->set_distance(0.1f);
    }

    double x, y;
    glfwGetCursorPos(mWindow, &x, &y);

    mCamera->on_mouse_move(x, y, Input::GetPressedButton(mWindow));

  }

  bool GLWindow::load_mesh()
  {
    Assimp::Importer Importer;

    const aiScene* pScene = Importer.ReadFile(mModel.c_str(),
      aiProcess_Triangulate |
      aiProcess_JoinIdenticalVertices);

    if (pScene->HasMeshes())
    {
      auto* mesh = pScene->mMeshes[0];

      mMesh = std::make_unique<Mesh>();

      for (uint32_t i = 0; i < mesh->mNumVertices; i++)
      {
        VertexHolder vh;
        vh.mPos = { mesh->mVertices[i].x, mesh->mVertices[i].y ,mesh->mVertices[i].z };
        vh.mNormal = { mesh->mNormals[i].x, mesh->mNormals[i].y ,mesh->mNormals[i].z };

        mMesh->add_vertex(vh);
      }

      for (size_t i = 0; i < mesh->mNumFaces; i++)
      {
        aiFace face = mesh->mFaces[i];

        for (size_t j = 0; j < face.mNumIndices; j++)
          mMesh->add_vertex_index(face.mIndices[j]);
      }
    }

    mMesh->init();
    return true;
  }
}
