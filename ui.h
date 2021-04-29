#pragma once

#include <SDL.h>
#include <stdio.h>

SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* texture;

int initUI();
int closeUI();

void drawFramebuffer();