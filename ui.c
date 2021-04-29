#include "ui.h"
#include <SDL.h>
#include <stdio.h>

#include "mmu.h"

int initUI() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;

	window = SDL_CreateWindow("Tixty4", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
	if (window == NULL)
		return -1;

	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL)
		return -1;

	return 0;
}

int closeUI() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	if (texture)
		SDL_DestroyTexture(texture);
	SDL_Quit();
	return 0;
}

void drawFramebuffer() {
	if ((getu32(VIreg) & 0b11) != 3)
		return;

	if (texture)
		SDL_DestroyTexture(texture);

	u32 width = getu32(VIreg + 0x8) & 0xFFF;
	u32 y_scale = getu32(VIreg + 0x34);
	u32 height = ((15 * y_scale) / 64);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	void* origin = RDRAM + (getu32(VIreg + 0x4) & 0x3FFFFF);
	SDL_UpdateTexture(texture, NULL, origin, (getu32(VIreg + 0x8) & 0xFFF) * 4);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}