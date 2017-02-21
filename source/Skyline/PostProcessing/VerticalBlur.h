#pragma once
#include <Shader.h>
#include "../FBORenderer.h"
#include <glew.h>
#include "../Game.h"

class VerticalBlur : public FBORenderer {
  public:
    VerticalBlur();
    VerticalBlur(int downScaling);
    ~VerticalBlur();

    virtual void initShader();
    virtual void render(Ess2D::FrameBufferObject* fbo);
    Ess2D::FrameBufferObject* getResult();

  protected:
    virtual void initVertexAttributeObject();

  private:
    Ess2D::FrameBufferObject* _targetFBO;
    Game* _game;

};

