#define NOMINMAX

#include "Game.h"
#include <iostream>
#include <ResourceManager.h>
#include <TextureCache.h>
#include <GLTexture.h>
#include <windows.h>
#include <algorithm>
//#include <Psapi.h>
#include "Entity.h"

Game::Game() {}

Game::~Game() {}

Game* Game::instance = NULL;

void Game::Boot() {
  initSystem();

  _state = GameState::RUNNING;

  _level = new Level();
  _level->load("intro");

  _player = new Player(_textureCache.getTexture("Textures/Cumz4AC.png")._id, 90.0f, 120.0f, glm::vec2(_camera.getViewportSize().x / 2, 100.0f));
  _player->setBaseVelocity(glm::vec2(0.0, this->scrollSpeed));
  _player->setBaseDirection(glm::vec2(0.0, 1.0f));
  _player->setVelocity(_player->getBaseVelocity() * _player->getBaseDirection());
  _player->getBody()->SetLinearVelocity(b2Vec2(0.0f, 0.2f));

  _player->spawn();
}

void Game::Run() {
  SDL_StartTextInput();

  const float DESIRED_FPS = 60;
  const float MS_PER_SECOND = 1000;
  const float DESIRED_FRAMETIME = MS_PER_SECOND / DESIRED_FPS;
  const int MAX_FRAMES_SIMULATED = 6;
  const float MAX_DELTA_TIME = 1.0f;

  float previousTicks = (float) SDL_GetTicks();

  while(_state == GameState::RUNNING) {
    _fpsLimiter.begin();

    float newTicks = (float) SDL_GetTicks();
    float frameTime = newTicks - previousTicks;
    previousTicks = newTicks;
    float totalDeltaTime = frameTime / DESIRED_FRAMETIME;
    
    int i = 0;
    while(totalDeltaTime > 0.0f && i < MAX_FRAMES_SIMULATED) {
      float deltaTime = std::min(totalDeltaTime, MAX_DELTA_TIME);
  
      processInput(deltaTime);
      update(deltaTime);

      totalDeltaTime -= deltaTime;
      i++;
    }

    render();

    _fps = _fpsLimiter.end();

    if(_debugMode == true) {
      static int frameCounter = 0;
      frameCounter++;
      if(frameCounter == 20) {
        std::cout << this->_fps << std::endl;
        frameCounter = 0;
      }
    }
  }

  SDL_StopTextInput();
}

void Game::update(float deltaTime) {
  if(!_isPaused) {
    _camera.setPosition(_camera.getPosition() + glm::vec2(0.0f, this->scrollSpeed * _camera.getZoom()) * deltaTime);
    _level->update(deltaTime);
    updatePlayer(deltaTime);
    updateProjectiles(deltaTime);
    updateObjects(deltaTime);
  }
}

void Game::updateObjects(float deltaTime) {
  Entity* entity;
  std::vector<unsigned int> activeObjects = _level->getActiveObjects();
  for(unsigned int i = 0; i < activeObjects.size(); i++) {
    entity = _entityManager.getEntity(activeObjects[i]);
    if(entity == nullptr) {
      continue;
    }

    _player->collidesWith(entity);
  }
}

void Game::updatePlayer(float deltaTime) {
  //_player->move(glm::vec2(0.0f, 1.0f), this->scrollSpeed, deltaTime);
  _player->update(deltaTime);
}

void Game::updateProjectiles(float deltaTime) {
  _projectileManager.update(deltaTime);
}

void Game::render() {
  glClearDepth(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  _baseProgram->use();
  _camera.update();
  /*
  PROCESS_MEMORY_COUNTERS memCounter;
  bool result = GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
  std::cout << "Start " << memCounter.WorkingSetSize / (1014) << " kb" << std::endl;
  */
  glActiveTexture(GL_TEXTURE0);
  GLint textureLocation = _baseProgram->getUniformLocation("textureSampler");
  glUniform1i(textureLocation, 0);

  GLint pLocation = _baseProgram->getUniformLocation("P");
  glm::mat4 cameraMatrix = _camera.getCameraMatrix();

  glUniformMatrix4fv(pLocation, 1, GL_FALSE, &(cameraMatrix[0][0]));

  _spriteBatch.begin();
  /*
  glm::vec4 position(80.0f, 23.0f, 100.0f, 100.0f);
  glm::vec4 uv(0.0f, 0.0f, 1.0f, 1.0f);
  Essengine::ColorRGBA8 color(255, 255, 255, 255);
  
  Essengine::GLTexture tex = _textureCache.getTexture("Textures/grass_tile.png");

  //result = GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
  //std::cout << "End " << memCounter.WorkingSetSize / (1014) << " kb" << std::endl << std::endl;

  _spriteBatch.draw(position, uv, _textureCache.getTexture("Textures/grass_tile.png")._id, color, 0);
  //_spriteBatch.draw(position, uv, tex._id, color, 0);
  */

  _level->draw();
  _player->draw();
  _projectileManager.draw();

  _spriteBatch.end();
  _spriteBatch.render();

  _baseProgram->unuse();

  _window->SwapBuffer();
}

void Game::processInput(float deltaTime) {
  SDL_Event event;

  const float CAMERA_SPEED = 0.5f;
  const float SCALE_SPEED = 1.0f;

  while(SDL_PollEvent(&event) != 0) {
    switch(event.type) {
      case SDL_QUIT:
        _state = GameState::EXIT;
        break;
      case SDL_KEYDOWN:
        _inputManager.pressKey(event.key.keysym.sym);
        break;
      case SDL_KEYUP:
        _inputManager.releaseKey(event.key.keysym.sym);
    }
  }

  if(_inputManager.isKeyDown(SDLK_w)) {
    _camera.setPosition(_camera.getPosition() + glm::vec2(0.0f, CAMERA_SPEED * deltaTime));
  }
  if(_inputManager.isKeyDown(SDLK_s)) {
    _camera.setPosition(_camera.getPosition() + glm::vec2(0.0f, -CAMERA_SPEED * deltaTime));
  }
  if(_inputManager.isKeyDown(SDLK_d)) {
    _camera.setPosition(_camera.getPosition() + glm::vec2(CAMERA_SPEED * deltaTime, 0.0f));
  }
  if(_inputManager.isKeyDown(SDLK_a)) {
    _camera.setPosition(_camera.getPosition() + glm::vec2(-CAMERA_SPEED * deltaTime, 0.0f));
  }
  if(_inputManager.isKeyDown(SDLK_q)) {
    _camera.setScale(_camera.getScale() + SCALE_SPEED * deltaTime);
    std::cout << "Scale: " << _camera.getScale() << std::endl;
  }
  if(_inputManager.isKeyDown(SDLK_e)) {
    _camera.setScale(_camera.getScale() - SCALE_SPEED * deltaTime);
    std::cout << "Scale: " << _camera.getScale() << std::endl;
  }
  if(_inputManager.isKeyDown(SDLK_TAB)) {
    if(_canPause == true) {
      _isPaused = !_isPaused;
      _canPause = false;
    }
  } else {
    _canPause = true;
  }

  float playerSpeed = 0.5f;

  b2Vec2 direction(0,0);
  b2Vec2 maxVelocity(0.5f, 0.55f);
  b2Vec2 minVelocity(0.0f, 0.2f);
  b2Vec2 velocity = _player->getBody()->GetLinearVelocity();

  if(_inputManager.isKeyDown(SDLK_LEFT)) {
    direction += b2Vec2(-1.0f, 0.0f);
  }

  if(_inputManager.isKeyDown(SDLK_RIGHT)) {
    direction += b2Vec2(1.0f, 0.0f);
  }

  if(_inputManager.isKeyDown(SDLK_UP)) {
    direction += b2Vec2(0.0f, 1.0f);
  }

  if(_inputManager.isKeyDown(SDLK_DOWN)) {
    direction += b2Vec2(0.0f, -1.0f);
  }

  b2Vec2 force(0.0f, 0.0f), acceleration(0.0f, 0.0f);
  
  if(direction.x != 0) {
    acceleration.x = (direction.x * maxVelocity.x - velocity.x) * 2.0f;
  } else if (velocity.x != 0) {
    acceleration.x = (0.0f - velocity.x) * 5.0f;
  }

  if(direction.y != 0) {
    acceleration.y = (direction.y * maxVelocity.y - velocity.y) * (direction.y > 0 ? 2.0f : 0.5f);
  } else {
    acceleration.y = (0.2f - velocity.y) * 3.0f;
  }

  force = _player->getBody()->GetMass() * deltaTime * acceleration;

  _player->getBody()->ApplyForce(force, _player->getBody()->GetWorldCenter(), true);

  if(_inputManager.isKeyDown(SDLK_SPACE)) {
    _player->setIsFiring(true);
  } else {
    _player->setIsFiring(false);
  }
}

void Game::initSystem() {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  _window = new Essengine::Window(this->_title, (int) this->_width, (int) this->_height, 0);

  initShaders();

  _camera.init((int) this->_width, (int) this->_height);
  _camera.setScale(32.0f);
  _camera.setZoom(this->getWidth() / 1024.0f);
  _camera.setPosition(_camera.getWorldCoordinates(glm::vec2(this->_width / 2, this->_height / 2)));

  _spriteBatch.init();
  _fpsLimiter.init(_maxFPS, _limitFPS);
}

void Game::initShaders() {
  _baseProgram = new Essengine::GLProgram(true);
  _baseProgram->loadShader(Essengine::ShaderType::VERTEX, "Shaders/Vertex.shader");
  _baseProgram->loadShader(Essengine::ShaderType::FRAGMENT, "Shaders/Fragment.shader");
  _baseProgram->compileShaders();

  _baseProgram->addAttribute("vertexPosition");
  _baseProgram->addAttribute("vertexColor");
  _baseProgram->addAttribute("vertexUV");

  _baseProgram->linkShaders();
}

void Game::Destroy() {
  delete _window;
  delete _baseProgram;
  delete _player;
  delete _level;

  delete this;
}

Game* Game::GetInstance() {
  if (instance == NULL) {
    instance = new Game();
  }
  return instance;
}

Essengine::SpriteBatch* Game::getSpriteBatch() {
  return &_spriteBatch;
}

Essengine::Camera2D* Game::getMainCamera() {
  return &_camera;
}

float Game::getWidth() {
  return _width;
}

float Game::getHeight() {
  return _height;
}

EntityManager* Game::getEntityManager() {
  return &_entityManager;
}

Essengine::TextureCache* Game::getTextureCache() {
  return &_textureCache;
}