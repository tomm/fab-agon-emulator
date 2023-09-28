#ifndef SPRITES_H
#define SPRITES_H

#include <fabgl.h>

#include "agon.h"
#include "agon_screen.h"

uint8_t			numsprites = 0;					// Number of sprites on stage
uint8_t			current_sprite = 0;				// Current sprite number
uint8_t			current_bitmap = 0;				// Current bitmap number
Bitmap			bitmaps[MAX_BITMAPS];			// Bitmap object storage
Sprite			sprites[MAX_SPRITES];			// Sprite object storage

Bitmap * getBitmap(uint8_t b = current_bitmap) {
	return &bitmaps[b];
}

inline void setCurrentBitmap(uint8_t b) {
	current_bitmap = b;
}

inline uint8_t getCurrentBitmap() {
	return current_bitmap;
}

void clearBitmap(uint8_t b = current_bitmap) {
	auto bitmap = getBitmap(b);
	if (bitmap->data) {
		free(bitmap->data);
		bitmap->data = nullptr;
	}
	bitmap->dataAllocated = false;
}

void createBitmap(uint16_t width, uint16_t height, void * data, PixelFormat format = PixelFormat::RGBA8888) {
	clearBitmap();
	bitmaps[current_bitmap] = Bitmap(width, height, (uint8_t *)data, format);
	bitmaps[current_bitmap].dataAllocated = false;
}

void drawBitmap(uint16_t x, uint16_t y) {
	auto bitmap = getBitmap();
	if (bitmap->data) {
		canvas->drawBitmap(x, y, bitmap);
		waitPlotCompletion();
	}
}

void resetBitmaps() {
	for (auto n = 0; n < MAX_BITMAPS; n++) {
		clearBitmap(n);
	}
	waitPlotCompletion();
}

Sprite * getSprite(uint8_t sprite = current_sprite) {
	return &sprites[sprite];
}

inline void setCurrentSprite(uint8_t s) {
	current_sprite = s;
}

inline uint8_t getCurrentSprite() {
	return current_sprite;
}

void clearSpriteFrames(uint8_t s = current_sprite) {
	auto sprite = getSprite(s);
	sprite->visible = false;
	sprite->setFrame(0);
	sprite->clearBitmaps();
}

void addSpriteFrame(uint8_t bitmap) {
	auto sprite = getSprite();
	sprite->addBitmap(getBitmap(bitmap));
}

void activateSprites(uint8_t n) {
	/*
	* Sprites 0-(numsprites-1) will be activated on-screen
	* Make sure all sprites have at least one frame attached to them
	*/
	numsprites = n;

	if (numsprites) {
		_VGAController->setSprites(sprites, numsprites);
	} else {
		_VGAController->removeSprites();
	}
	waitPlotCompletion();
}

inline bool hasActiveSprites() {
	return numsprites > 0;
}

void nextSpriteFrame() {
	auto sprite = getSprite();
	sprite->nextFrame();
}

void previousSpriteFrame() {
	auto sprite = getSprite();
	auto frame = sprite->currentFrame;
	sprite->setFrame(frame ? frame - 1 : sprite->framesCount - 1);
}

void setSpriteFrame(uint8_t n) {
	auto sprite = getSprite();
	if (n < sprite->framesCount) {
		sprite->setFrame(n);
	}
}

void showSprite() {
	auto sprite = getSprite();
	sprite->visible = 1;
}

void hideSprite() {
	auto sprite = getSprite();
	sprite->visible = 0;
}

void moveSprite(int x, int y) {
	auto sprite = getSprite();
	sprite->moveTo(x, y);
}

void moveSpriteBy(int x, int y) {
	auto sprite = getSprite();
	sprite->moveBy(x, y);
}

void refreshSprites() {
	if (numsprites) {
		_VGAController->refreshSprites();
	}
}

void resetSprites() {
	for (auto n = 0; n < MAX_SPRITES; n++) {
		clearSpriteFrames(n);
	}
	waitPlotCompletion();
}

#endif // SPRITES_H
