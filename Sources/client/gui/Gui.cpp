#if defined(__APPLE__)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "Gui.h"
#include "Font.h"
#include "client/Options.h"
#include "platform/input/Keyboard.h"
#include "screens/IngameBlockSelectionScreen.h"
#include "screens/ChatScreen.h"
#include "screens/ConsoleScreen.h"
#include "../Minecraft.h"
#include "../player/LocalPlayer.h"
#include "../renderer/Chunk.h"
#include "../renderer/Tesselator.h"
#include "../renderer/TileRenderer.h"
#include "../renderer/LevelRenderer.h"
#include "../renderer/GameRenderer.h"
#include "../renderer/entity/ItemRenderer.h"
#include "../player/input/IInputHolder.h"
#include "../gamemode/GameMode.h"
#include "../gamemode/CreativeMode.h"
#include "../renderer/Textures.h"
#include "../../AppConstants.h"
#include "../../world/entity/player/Inventory.h"
#include "../../world/level/material/Material.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../../platform/input/Mouse.h"
#include "../../world/level/Level.h"
#include "../../world/PosTranslator.h"
#include "../../platform/mc_time.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <cstdio>

float Gui::InvGuiScale = 1.0f / 3.0f;
float Gui::GuiScale = 1.0f / Gui::InvGuiScale;
const float Gui::DropTicks = 40.0f;

//#include <android/log.h>

Gui::Gui(Minecraft* minecraft)
	:	minecraft(minecraft),
	tickCount(0),
	progress(0),
	overlayMessageTime(0),
	animateOverlayMessageColor(false),
	chatScrollOffset(0),
	tbr(1),
	_inventoryNeedsUpdate(true),
	_flashSlotId(-1),
	_flashSlotStartTime(-1),
	_slotFont(NULL),
	_numSlots(minecraft->useTouchscreen() ? 10 : Inventory::MAX_SELECTION_SIZE),
	_currentDropTicks(-1),
	_currentDropSlot(-1),
	MAX_MESSAGE_WIDTH(240),
	itemNameOverlayTime(2),
	_openInventorySlot(minecraft->useTouchscreen()),
	lastTrackedHealth(20),
	lastTrackedFood(20),
	heartBounceImpact(0.0f),
	lastTrackedLevel(0),
	levelUpFlash(0.0f)
{
	glGenBuffers2(1, &_inventoryRc.vboId);
	glGenBuffers2(1, &rcFeedbackInner.vboId);
	glGenBuffers2(1, &rcFeedbackOuter.vboId);
	//Gui::InvGuiScale = 1.0f / (int) (3 * Minecraft::width / 854);
}

Gui::~Gui()
{
	if (_slotFont)
		delete _slotFont;

	glDeleteBuffers(1, &_inventoryRc.vboId);
}

void Gui::render(float a, bool mouseFree, int xMouse, int yMouse) {

	if (!minecraft->level || !minecraft->player)
		return;

	//minecraft->gameRenderer->setupGuiScreen();
	Font* font = minecraft->font;

	const bool isTouchInterface = minecraft->useTouchscreen();

	const int screenWidth = (int)(minecraft->width * InvGuiScale);
	const int screenHeight = (int)(minecraft->height * InvGuiScale);
	blitOffset = -90;
	renderProgressIndicator(isTouchInterface, screenWidth, screenHeight, a);

	glColor4f2(1, 1, 1, 1);

	// H: 4
	// T: 7
	// L: 6
	// F: 3
	int ySlot = screenHeight - 16 - 3;

	if (!minecraft->options.getBooleanValue(OPTIONS_HIDEGUI) && minecraft->screen == NULL) {
		renderModernStatusCard(font, screenWidth, screenHeight);

		if (minecraft->gameMode->canHurtPlayer()) {
			minecraft->textures->loadAndBindTexture("gui/icons.png");
			Tesselator& t = Tesselator::instance;
			t.beginOverride();
			t.colorABGR(0xffffffff);
			renderHearts();
			renderHunger();
			renderBubbles();
			t.endOverrideAndDraw();

			renderExperienceBar(font, screenWidth, screenHeight);
		}
	}

	// viginette has been fixed, was due to gl_blend not being enabled, my bad
	if (minecraft->options.getBooleanValue(OPTIONS_FANCY_GRAPHICS) && minecraft->options.getBooleanValue(OPTIONS_VIGNETTE)){
			renderVignette(this->minecraft->player->getBrightness(a), screenWidth, screenHeight);
		}
	// shredder end

	if(minecraft->player->getSleepTimer() > 0) {
		glDisable(GL_DEPTH_TEST);
		GLState::setAlphaTestEnabled(false);

		renderSleepAnimation(screenWidth, screenHeight);

		GLState::setAlphaTestEnabled(true);
		glEnable(GL_DEPTH_TEST);
	}
	// Update health and food popup tracking
	updateFloatingPopups(0.016f);
	if (minecraft->player) {
		int curHp = minecraft->player->health;
		if (lastTrackedHealth != curHp && lastTrackedHealth > 0 && curHp > 0) {
			int diff = curHp - lastTrackedHealth;
			int xHearts = screenWidth / 2 - 91 + 40;
			int yHearts = screenHeight - 46;
			if (diff > 0) {
				char buf[32]; sprintf(buf, "+%d HP", diff);
				addFloatingPopup(buf, 0x50ff70, (float)xHearts, (float)yHearts);
			} else if (diff < 0) {
				char buf[32]; sprintf(buf, "%d HP", diff);
				addFloatingPopup(buf, 0xff4545, (float)xHearts, (float)yHearts);
				heartBounceImpact = 1.0f;
			}
		}
		lastTrackedHealth = curHp;

		int curFood = minecraft->player->foodData.getFoodLevel();
		if (lastTrackedFood != curFood && lastTrackedFood > 0 && curFood > 0) {
			int diff = curFood - lastTrackedFood;
			int xFood = screenWidth / 2 + 91 - 40;
			int yFood = screenHeight - 46;
			if (diff > 0) {
				char buf[32]; sprintf(buf, "+%d COMIDA", diff);
				addFloatingPopup(buf, 0xffd030, (float)xFood, (float)yFood);
			} else if (diff < 0) {
				char buf[32]; sprintf(buf, "%d COMIDA", diff);
				addFloatingPopup(buf, 0xff7030, (float)xFood, (float)yFood);
			}
		}
		lastTrackedFood = curFood;
	}

	if (!minecraft->options.getBooleanValue(OPTIONS_HIDEGUI) && minecraft->screen == NULL) {
		renderToolBar(a, ySlot, screenWidth);

		glEnable(GL_BLEND);
		bool isChatting = (minecraft->screen && (dynamic_cast<ChatScreen*>(minecraft->screen) || dynamic_cast<ConsoleScreen*>(minecraft->screen)));
		unsigned int max = 10;
		if (isChatting) {
			int lineHeight = 9;
			max = (screenHeight - 48) / lineHeight;
			if (max < 1) max = 1;
			int maxScroll = (int)guiMessages.size() - (int)max;
			if (maxScroll < 0) maxScroll = 0;
			if (chatScrollOffset > maxScroll) chatScrollOffset = maxScroll;
		} else {
			chatScrollOffset = 0;
		}
		renderChatMessages(screenHeight, max, isChatting, font);
#if !defined(RPI)
		renderOnSelectItemNameText(screenWidth, font, ySlot);
#endif
#if defined(RPI)
		renderDebugInfo();
#endif

		if (Keyboard::isKeyDown(Keyboard::KEY_TAB)) {
			renderPlayerList(font, screenWidth, screenHeight);
		}

		if (minecraft->options.getBooleanValue(OPTIONS_RENDER_DEBUG))
			renderDebugInfo();

		renderFloatingPopups(font);
	}

	glDisable(GL_BLEND);
	GLState::setAlphaTestEnabled(true);
}

int Gui::getSlotIdAt(int x, int y) {
	int screenWidth = (int)(minecraft->width * InvGuiScale);
	int screenHeight = (int)(minecraft->height * InvGuiScale);
	x = (int)(x * InvGuiScale);
	y = (int)(y * InvGuiScale);

	if (y < (screenHeight - 22) || y > screenHeight)
		return -1;

	int xBase = screenWidth / 2 - getNumSlots() * 10;
	int xRel  = (x - xBase);
	if (xRel < 0)
		return -1;

	int slot = xRel / 20;
	return (slot >= 0 && slot < getNumSlots())? slot : -1;
}

bool Gui::isInside(int x, int y) {
	return getSlotIdAt(x, y) != -1;
}

int Gui::getNumSlots() {
	return _numSlots;
}

void Gui::flashSlot(int slotId) {
	_flashSlotId = slotId;
	_flashSlotStartTime = getTimeS();
}

void Gui::getSlotPos(int slot, int& posX, int& posY) {
	int screenWidth = (int)(minecraft->width * InvGuiScale);
	int screenHeight = (int)(minecraft->height * InvGuiScale);
	posX = screenWidth / 2 - getNumSlots() * 10 + slot * 20, 
		posY = screenHeight - 22;
}

RectangleArea Gui::getRectangleArea(int extendSide) {
	const int Spacing = 3;
	const float pCenterX   = 2.0f + (float)(minecraft->width / 2);
	const float pHalfWidth = (1.0f + (getNumSlots() * 10 + Spacing)) * Gui::GuiScale;
	const float pHeight    = (22 + Spacing) * Gui::GuiScale;

	if (extendSide < 0)
		return RectangleArea(0, (float)minecraft->height-pHeight, pCenterX+pHalfWidth+2, (float)minecraft->height);
	if (extendSide > 0)
		return RectangleArea(pCenterX-pHalfWidth, (float)minecraft->height-pHeight, (float)minecraft->width, (float)minecraft->height);

	return RectangleArea(pCenterX-pHalfWidth, (float)minecraft->height-pHeight, pCenterX+pHalfWidth+2, (float)minecraft->height);
}

void Gui::handleClick(int button, int x, int y) {
	if (button != MouseAction::ACTION_LEFT)	return;

	int slot = getSlotIdAt(x, y);
	if (slot != -1) {
		if (_openInventorySlot && slot == (getNumSlots()-1)) {
			minecraft->screenChooser.setScreen(SCREEN_BLOCKSELECTION);
		} else {
			minecraft->player->inventory->selectSlot(slot);
			itemNameOverlayTime = 0;
		}
	}
}

	void Gui::handleKeyPressed(int key)
	{
		bool isChatting = (minecraft->screen && (dynamic_cast<ChatScreen*>(minecraft->screen) || dynamic_cast<ConsoleScreen*>(minecraft->screen)));
		if (isChatting) {
			// Allow scrolling the chat history with the mouse/keyboard when chat is open
			if (key == 38) { // VK_UP
				scrollChat(1);
				return;
			} else if (key == 40) { // VK_DOWN
				scrollChat(-1);
				return;
			} else if (key == 33) { // VK_PRIOR (Page Up)
				// Scroll by a page
				int screenHeight = (int)(minecraft->height * InvGuiScale);
				int maxVisible = (screenHeight - 48) / 9;
				scrollChat(maxVisible);
				return;
			} else if (key == 34) { // VK_NEXT (Page Down)
				int screenHeight = (int)(minecraft->height * InvGuiScale);
				int maxVisible = (screenHeight - 48) / 9;
				scrollChat(-maxVisible);
				return;
			}
		}

		if (key == Keyboard::KEY_F1) {
			minecraft->options.toggle(OPTIONS_HIDEGUI);
		}

		if (key == 99)
		{
			if (minecraft->player->inventory->selected > 0)
			{
				minecraft->player->inventory->selected--;
			}
		}
		else if (key == 4)
		{
			int maxSelectable = _openInventorySlot ? (getNumSlots() - 2) : (getNumSlots() - 1);
			if (minecraft->player->inventory->selected < maxSelectable)
			{
				minecraft->player->inventory->selected++;
			}
		}
		else if (key == 100)
		{
			minecraft->screenChooser.setScreen(SCREEN_BLOCKSELECTION);
		}
		else if (key == minecraft->options.getIntValue(OPTIONS_KEY_DROP)) 
		{
			minecraft->player->inventory->dropSlot(minecraft->player->inventory->selected, false);
		}
	}

void Gui::scrollChat(int delta) {
	if (delta == 0)
		return;

	int screenHeight = (int)(minecraft->height * InvGuiScale);
	int maxVisible = (screenHeight - 48) / 9;
	if (maxVisible <= 0)
		return;

	int maxScroll = (int)guiMessages.size() - maxVisible;
	if (maxScroll < 0) maxScroll = 0;
	int desired = chatScrollOffset + delta;
	if (desired < 0) desired = 0;
	if (desired > maxScroll) desired = maxScroll;
	chatScrollOffset = desired;
}

void Gui::tick() {
	if (overlayMessageTime > 0) overlayMessageTime--;
	tickCount++;
	if(itemNameOverlayTime < 2)
		itemNameOverlayTime += 1.0f / SharedConstants::TicksPerSecond;
	for (unsigned int i = 0; i < guiMessages.size(); i++) {
		guiMessages.at(i).ticks++;
	}

	if (!minecraft->isCreativeMode())
		tickItemDrop();
}

void Gui::addMessage(const std::string& _string) {
	if (!minecraft->font)
		return;

	std::string string = _string;
	while (minecraft->font->width(string) > MAX_MESSAGE_WIDTH) {
		unsigned int i = 1;
		while (i < string.length() && minecraft->font->width(string.substr(0, i + 1)) <= MAX_MESSAGE_WIDTH) {
			i++;
		}
		addMessage(string.substr(0, i));
		string = string.substr(i);
	}
	GuiMessage message;
	message.message = string;
	message.ticks = 0;
	guiMessages.insert(guiMessages.begin(), message);

	// Keep a larger history so users can scroll through the full chat
	const unsigned int MaxHistoryLines = 200;
	while (guiMessages.size() > MaxHistoryLines) {
		guiMessages.pop_back();
	}

	// If the user has scrolled up, keep their window fixed (new messages shift older ones down)
	if (chatScrollOffset > 0) {
		chatScrollOffset++;
	}
}

void Gui::clearMessages() {
	guiMessages.clear();
	chatScrollOffset = 0;
}

void Gui::setNowPlaying(const std::string& string) {
	overlayMessageString = "Now playing: " + string;
	overlayMessageTime = 20 * 3;
	animateOverlayMessageColor = true;
}

void Gui::displayClientMessage(const std::string& messageId) {
	//Language language = Language.getInstance();
	//std::string languageString = language.getElement(messageId);
	addMessage(messageId);
}


// @todo - shredder: Function seems to be completely fine and ported over from java beta, but renders opaque??? need to investigate
void Gui::renderVignette(float br, int w, int h) {
	br = 1 - br;
	if (br < 0) br = 0;
	if (br > 1) br = 1;
	tbr += (br - tbr) * 0.01f;

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendFunc2(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glColor4f2(tbr, tbr, tbr, 1);

	minecraft->textures->loadAndBindTexture("misc/vignette.png");
	glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri2(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	Tesselator& t = Tesselator::instance;
	t.begin();
	t.vertexUV(0, (float)h, -90, 0, 1);
	t.vertexUV((float)w, (float)h, -90, 1, 1);
	t.vertexUV((float)w, 0, -90, 1, 0);
	t.vertexUV(0, 0, -90, 0, 0);
	t.draw();
	glDepthMask(true);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glColor4f2(1, 1, 1, 1);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Gui::renderSlot(int slot, int x, int y, float a) {
	ItemInstance* item = minecraft->player->inventory->getItem(slot);
	if (!item) {
		//LOGW("Warning: item @ Gui::renderSlot is NULL\n");
		return;
	}

	const bool fancy = true;
	ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, item, (float)x, (float)y, fancy);
}

void Gui::renderSlotText( const ItemInstance* item, float x, float y, bool hasFinite, bool shadow )
{
	//if (!item || item->getItem()->getMaxStackSize() <= 1) {
	if (item->count <= 1) {
		return;
	}

	int c = item->count;

	char buffer[4] = {0,0,0,0};
	if (hasFinite)
		itemCountItoa(buffer, c);
	else
		buffer[0] = (char)157;

	//LOGI("slot: %d - %s\n", slot, buffer);
	if (shadow)
		minecraft->font->drawShadow(buffer, x, y, item->count>0?0xffcccccc:0x60cccccc);
	else
		minecraft->font->draw(buffer, x, y, item->count>0?0xffcccccc:0x60cccccc);
}

void Gui::inventoryUpdated() {
	_inventoryNeedsUpdate = true;
}

void Gui::onGraphicsReset() {
	inventoryUpdated();
}

void Gui::texturesLoaded( Textures* textures ) {
	//_slotFont = new Font(&minecraft->options, "gui/gui_blocks.png", textures, 0, 504, 10, 1, '0');
}

void Gui::onConfigChanged( const Config& c ) {
	Tesselator& t = Tesselator::instance;
	t.begin();

	//
	// Create outer feedback circle
	//
#ifdef ANDROID
	const float mm = 50; //20
#else
	const float mm = 50; //20
#endif
	const float maxRadius = minecraft->pixelCalcUi.millimetersToPixels(mm);
	const float radius = Mth::Min(80.0f/2, maxRadius);
	//LOGI("radius, maxradius: %f, %f\n", radius, maxRadius);
	const float radiusInner = radius * 0.95f;

	const int steps = 24;
	const float fstep = Mth::TWO_PI / steps;
	for (int i = 0; i < steps; ++i) {
		float a = i * fstep;;
		float b = a + fstep;

		float aCos = Mth::cos(a);
		float bCos = Mth::cos(b);
		float aSin = Mth::sin(a);
		float bSin = Mth::sin(b);
		float x00 = radius * aCos;
		float x01 = radiusInner * aCos;
		float x10 = radius * bCos;
		float x11 = radiusInner * bCos;
		float y00 = radius * aSin;
		float y01 = radiusInner * aSin;
		float y10 = radius * bSin;
		float y11 = radiusInner * bSin;

		t.vertexUV(x01, y01, 0, 0, 1);
		t.vertexUV(x11, y11, 0, 1, 1);
		t.vertexUV(x10, y10, 0, 1, 0);
		t.vertexUV(x00, y00, 0, 0, 0);
	}
	rcFeedbackOuter = t.end(true, rcFeedbackOuter.vboId);

	//
	// Create the inner feedback ring
	//
	t.begin(GL_TRIANGLE_FAN);
	t.vertex(0, 0, 0);
	for (int i = 0; i < steps + 1; ++i) {
		float a = -i * fstep;
		float xx = radiusInner * Mth::cos(a);
		float yy = radiusInner * Mth::sin(a);
		t.vertex(xx, yy, 0);
		//LOGI("x,y: %f, %f\n", xx, yy);
	}
	rcFeedbackInner = t.end(true, rcFeedbackInner.vboId);

	_openInventorySlot = c.minecraft->useTouchscreen();
	if (c.minecraft->useTouchscreen()) {
		_numSlots = 10;
	} else {
		_numSlots = Inventory::MAX_SELECTION_SIZE; // 9
	}
	MAX_MESSAGE_WIDTH = c.guiWidth;
}

float Gui::floorAlignToScreenPixel(float v) {
	return (int)(v * Gui::GuiScale) * Gui::InvGuiScale;
}

int Gui::itemCountItoa( char* buffer, int count )
{
	if (count < 0)
		return 0;

	if (count < 10) { // 1 digit
		buffer[0] = '0' + count;
		buffer[1] = 0;
		return 1;
	} else if (count < 100) { // 2 digits
		int digit = count/10;
		buffer[0] = '0' + digit;
		buffer[1] = '0' + count - digit*10;
		buffer[2] = 0;
	} else { // 3 digits -> "99+"
		buffer[0] = buffer[1] = '9';
		buffer[2] = '+';
		buffer[3] = 0;
		return 3;
	}
	return 2;
}

void Gui::tickItemDrop()
{
	// Handle item drop
	static bool isCurrentlyActive = false;
	isCurrentlyActive = false;

	int slots = getNumSlots() - _openInventorySlot;

	if (Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
		int slot = getSlotIdAt(Mouse::getX(), Mouse::getY());
		if (slot >= 0 && slot < slots) {
			if (slot != _currentDropSlot) {
				_currentDropTicks = 0;
				_currentDropSlot = slot;
			}
			isCurrentlyActive = true;
			if ((_currentDropTicks += 1.0f) >= DropTicks) {
				minecraft->player->inventory->dropSlot(slot, false);
				minecraft->level->playSound(minecraft->player, "random.pop", 0.3f, 1);
				isCurrentlyActive = false;
			}
		}
	}
	if (!isCurrentlyActive) {
		_currentDropSlot = -1;
		_currentDropTicks = -1;
	}
}

void Gui::postError( int errCode )
{
	static std::set<int> posted;
	if (posted.find(errCode) != posted.end())
		return;

	posted.insert(errCode);

	std::stringstream s;
	s << "Something went wrong! (errcode " << errCode << ")\n";
	addMessage(s.str());
}

void Gui::setScissorRect( const IntRectangle& bbox )
{
	GLuint x = (GLuint)(GuiScale * bbox.x);
	GLuint y = minecraft->height - (GLuint)(GuiScale * (bbox.y + bbox.h));
	GLuint w = (GLuint)(GuiScale * bbox.w);
	GLuint h = (GLuint)(GuiScale * bbox.h);
	glScissor(x, y, w, h);
}

float Gui::cubeSmoothStep(float percentage, float min, float max) {
	//percentage = percentage * percentage;
	//return (min * percentage) + (max * (1 - percentage));
	return (percentage) * (percentage) * (3 - 2 * (percentage));
}

void Gui::renderProgressIndicator( const bool isTouchInterface, const int screenWidth, const int screenHeight, float a ) {
	ItemInstance* currentItem = minecraft->player->inventory->getSelected();
	bool bowEquipped = currentItem != NULL ? currentItem->getItem() == Item::bow : false;
	bool itemInUse = currentItem != NULL ? currentItem->getItem() == minecraft->player->getUseItem()->getItem() : false;
	if ((!isTouchInterface || minecraft->options.getBooleanValue(OPTIONS_IS_JOY_TOUCH_AREA) 
		|| (bowEquipped && itemInUse)) && !minecraft->options.getBooleanValue(OPTIONS_HIDEGUI))
	{
			minecraft->textures->loadAndBindTexture("gui/icons.png");
			glEnable(GL_BLEND);
			glBlendFunc2(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
			blit(screenWidth/2 - 8, screenHeight/2 - 8, 0, 0, 16, 16);
			glDisable(GL_BLEND);
	} else if(!bowEquipped) {
		const float tprogress = minecraft->gameMode->destroyProgress;
		const float alpha = Mth::clamp(minecraft->inputHolder->alpha, 0.0f, 1.0f);
		//LOGI("alpha: %f\n", alpha);

		if (tprogress <= 0 && minecraft->inputHolder->alpha >= 0) {
			GLState::setTextureEnabled(false);
			glEnable2(GL_BLEND);
			glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			if (minecraft->hitResult.isHit())
				glColor4f2(1, 1, 1, 0.8f * alpha);
			else
				glColor4f2(1, 1, 1, Mth::Min(0.4f, alpha*0.4f));

			//LOGI("alpha2: %f\n", alpha);
			const float x = InvGuiScale * minecraft->inputHolder->mousex;
			const float y = InvGuiScale * minecraft->inputHolder->mousey;
			glTranslatef2(x, y, 0);
			drawArrayVT(rcFeedbackOuter.vboId, rcFeedbackOuter.vertexCount, 36);
			glTranslatef2(-x, -y, 0);

			GLState::setTextureEnabled(true);
			glDisable(GL_BLEND);
		} else if (tprogress > 0) {
			const float oProgress = minecraft->gameMode->oDestroyProgress;
			const float progress = 0.5f * (oProgress + (tprogress - oProgress) * a);

			//static Stopwatch w;
			//w.start();

			GLState::setTextureEnabled(false);
			glColor4f2(1, 1, 1, 0.8f * alpha);
			glEnable(GL_BLEND);
			glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			const float x = InvGuiScale * minecraft->inputHolder->mousex;
			const float y = InvGuiScale * minecraft->inputHolder->mousey;
			glPushMatrix2();
			glTranslatef2(x, y, 0);
			drawArrayVT(rcFeedbackOuter.vboId, rcFeedbackOuter.vertexCount, 36);
			glScalef2(0.5f + progress, 0.5f + progress, 1);
			//glDisable2(GL_CULL_FACE);
			glColor4f2(1, 1, 1, 1);
			glBlendFunc2(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
			drawArrayVT(rcFeedbackInner.vboId, rcFeedbackInner.vertexCount, 36, GL_TRIANGLE_FAN);
			glPopMatrix2();

			glDisable(GL_BLEND);
			GLState::setTextureEnabled(true);

			//w.stop();
			//w.printEvery(100, "feedback-r ");
		}
	}
}

static OffsetPosTranslator posTranslator;
void Gui::onLevelGenerated() {
	if (Level* level = minecraft->level) {
		Pos p = level->getSharedSpawnPos();
		posTranslator = OffsetPosTranslator((float)-p.x, (float)-p.y, (float)-p.z);
	}
}

void Gui::renderHearts() {
	bool blink = (minecraft->player->invulnerableTime / 3) % 2 == 1;
	if (minecraft->player->invulnerableTime < 10) blink = false;
	int h = minecraft->player->health;
	int oh = minecraft->player->lastHealth;
	random.setSeed(tickCount * 312871);

	int screenWidth = (int)(minecraft->width * InvGuiScale);
	int screenHeight = (int)(minecraft->height * InvGuiScale);

	int xx = (minecraft->options.getBooleanValue(OPTIONS_BAR_ON_TOP)) ? screenWidth / 2 - getNumSlots() * 10 - 1 : (screenWidth / 2 - 91);
	int baseYo = (minecraft->options.getBooleanValue(OPTIONS_BAR_ON_TOP)) ? (screenHeight - 32) : (screenHeight - 37);

	int armor = minecraft->player->getArmorValue();
	bool isHealingWave = (minecraft->player->foodData.getFoodLevel() >= SimpleFoodData::HEAL_LEVEL && h < Player::MAX_HEALTH);
	int waveIdx = isHealingWave ? ((tickCount / 3) % 10) : -1;

	for (int i = 0; i < Player::MAX_HEALTH / 2; i++) {
		int yo = baseYo;
		int ip2 = i + i + 1;

		if (armor > 0) {
			int xo = xx + i * 8;
			int armorYo = yo - 10;
			if (ip2 < armor) blit(xo, armorYo, 16 + 2 * 9, 9 * 1, 9, 9);
			else if (ip2 == armor) blit(xo, armorYo, 16 + 4 * 9, 9 * 1, 9, 9);
			else if (ip2 > armor) blit(xo, armorYo, 16 + 0 * 9, 9 * 1, 9, 9);
		}

		int bg = 0;
		if (blink) bg = 1;
		int xo = xx + i * 8;

		// 1. Elastic damage impact bounce
		if (heartBounceImpact > 0.001f && (ip2 >= h - 1 && ip2 <= h + 1)) {
			yo -= (int)(heartBounceImpact * 4.0f);
		}
		// 2. Low HP heartbeat pulsation & jitter
		if (h <= 4) {
			yo += random.nextInt(2) - 1 + (int)(sinf((float)tickCount * 0.35f) * 1.5f);
		}
		// 3. Natural healing wave
		if (i == waveIdx) {
			yo -= 2;
		}

		blit(xo, yo, 16 + bg * 9, 9 * 0, 9, 9);
		if (blink) {
			if (ip2 < oh) blit(xo, yo, 16 + 6 * 9, 9 * 0, 9, 9);
			else if (ip2 == oh) blit(xo, yo, 16 + 7 * 9, 9 * 0, 9, 9);
		}
		if (ip2 < h) blit(xo, yo, 16 + 4 * 9, 9 * 0, 9, 9);
		else if (ip2 == h) blit(xo, yo, 16 + 5 * 9, 9 * 0, 9, 9);
	}
}

void Gui::renderHunger() {
	int screenWidth = (int)(minecraft->width * InvGuiScale);
	int screenHeight = (int)(minecraft->height * InvGuiScale);

	int xx = (minecraft->options.getBooleanValue(OPTIONS_BAR_ON_TOP)) ? (screenWidth / 2 + getNumSlots() * 10 + 1 - 9) : (screenWidth / 2 + 91 - 9);
	int baseYo = (minecraft->options.getBooleanValue(OPTIONS_BAR_ON_TOP)) ? (screenHeight - 32) : (screenHeight - 37);

	int food = minecraft->player->foodData.getFoodLevel();

	for (int i = 0; i < 10; i++) {
		int xo = xx - i * 8;
		int yo = baseYo;
		if (food <= 4) {
			yo += random.nextInt(2) - 1;
		}
		int ip2 = i * 2 + 1;

		// Empty food shank background
		blit(xo, yo, 16 + 0 * 9, 9 * 3, 9, 9);

		// Full or half food shank
		if (ip2 < food) {
			blit(xo, yo, 16 + 4 * 9, 9 * 3, 9, 9);
		} else if (ip2 == food) {
			blit(xo, yo, 16 + 5 * 9, 9 * 3, 9, 9);
		}
	}
}

void Gui::renderBubbles() {
	if (minecraft->player->isUnderLiquid(Material::water)) {
		int screenWidth = (int)(minecraft->width * InvGuiScale);
		int screenHeight = (int)(minecraft->height * InvGuiScale);

		int xx = (minecraft->options.getBooleanValue(OPTIONS_BAR_ON_TOP)) ? (screenWidth / 2 + getNumSlots() * 10 + 1 - 9) : (screenWidth / 2 + 91 - 9);
		int yo = (minecraft->options.getBooleanValue(OPTIONS_BAR_ON_TOP)) ? (screenHeight - 42) : (screenHeight - 47);
		int count = (int) std::ceil((minecraft->player->airSupply - 2) * 10.0f / Player::TOTAL_AIR_SUPPLY);
		int extra = (int) std::ceil((minecraft->player->airSupply) * 10.0f / Player::TOTAL_AIR_SUPPLY) - count;

		bool lowAirFlash = (minecraft->player->airSupply < 90) && ((tickCount / 4) % 2 == 0);

		for (int i = 0; i < count + extra; i++) {
			int xo = xx - i * 8;
			int wobbleY = yo + (int)(sinf((float)tickCount * 0.25f + i * 0.8f) * 1.5f);

			if (i < count) {
				if (i == count - 1 && lowAirFlash) blit(xo, wobbleY, 16 + 9, 9 * 2, 9, 9);
				else blit(xo, wobbleY, 16, 9 * 2, 9, 9);
			}
			else blit(xo, wobbleY, 16 + 9, 9 * 2, 9, 9);
		}
	}
}

void Gui::addFloatingPopup(const std::string& text, int color, float x, float y) {
	FloatingPopup pop;
	pop.text = text;
	pop.color = color;
	pop.x = x;
	pop.y = y;
	pop.offsetY = 0.0f;
	pop.alpha = 1.0f;
	pop.life = 1.5f;
	floatingPopups.push_back(pop);
}

void Gui::updateFloatingPopups(float dt) {
	if (heartBounceImpact > 0.001f) {
		heartBounceImpact = Mth::Max(heartBounceImpact - dt * 6.0f, 0.0f);
	}
	if (levelUpFlash > 0.001f) {
		levelUpFlash = Mth::Max(levelUpFlash - dt * 3.0f, 0.0f);
	}

	for (size_t i = 0; i < floatingPopups.size(); ) {
		floatingPopups[i].offsetY -= dt * 28.0f;
		floatingPopups[i].life -= dt;
		floatingPopups[i].alpha = floatingPopups[i].life / 1.5f;
		if (floatingPopups[i].life <= 0.0f) {
			floatingPopups.erase(floatingPopups.begin() + i);
		} else {
			++i;
		}
	}
}

void Gui::renderFloatingPopups(Font* font) {
	if (!font || floatingPopups.empty())
		return;

	for (size_t i = 0; i < floatingPopups.size(); ++i) {
		const FloatingPopup& pop = floatingPopups[i];
		int a = (int)(pop.alpha * 255.0f);
		if (a <= 0) continue;
		if (a > 255) a = 255;

		int rgb = pop.color & 0x00ffffff;
		int col = (a << 24) | rgb;

		float tw = (float)font->width(pop.text);
		float tx = pop.x - tw / 2.0f;
		float ty = pop.y + pop.offsetY;

		font->drawShadow(pop.text, tx, ty, col);
	}
}

void Gui::renderExperienceBar(Font* font, int screenWidth, int screenHeight) {
	int xExp = screenWidth / 2 - 91;
	int yExp = screenHeight - 26;
	const int wExp = 182;

	// Dark border background
	fill(xExp, yExp, xExp + wExp, yExp + 3, 0xa0000000);
	fill(xExp + 1, yExp + 1, xExp + wExp - 1, yExp + 2, 0x60002000);

	int score = minecraft->player->getScore();
	int level = score / 10;
	float progress = (float)(score % 10) / 10.0f;
	if (progress <= 0.0f && score == 0) progress = 0.0f;

	int fillW = (int)(progress * (wExp - 2));
	if (fillW > 0) {
		fill(xExp + 1, yExp + 1, xExp + 1 + fillW, yExp + 2, 0xff00ff55);
	}

	if (level > 0) {
		char lvlBuf[16];
		sprintf(lvlBuf, "%d", level);
		float lvlX = (float)(screenWidth / 2 - font->width(lvlBuf) / 2);
		float lvlY = (float)(yExp - 6);
		font->drawShadow(lvlBuf, lvlX, lvlY, 0xff80ff20);
	}
}

void Gui::renderModernStatusCard(Font* font, int screenWidth, int screenHeight) {
	if (!minecraft->player || !minecraft->level)
		return;

	// System real time (Hora Real)
	time_t rawtime;
	time(&rawtime);
	struct tm* timeinfo = localtime(&rawtime);
	char realTimeStr[16] = "--:--";
	if (timeinfo) {
		strftime(realTimeStr, sizeof(realTimeStr), "%H:%M", timeinfo);
	}

	// Game world time (Hora del Juego)
	long timeVal = minecraft->level->getTime();
	const int ticksPerDay = Level::TICKS_PER_DAY;
	long dayIndex = timeVal >= 0 ? timeVal / ticksPerDay
		: -(((-timeVal) + ticksPerDay - 1) / ticksPerDay);
	int day = (int)dayIndex + 1;
	int timeInDay = (int)(timeVal - dayIndex * ticksPerDay);

	int totalMinutes = (timeInDay * 24 * 60) / ticksPerDay;
	int hours = (totalMinutes / 60 + 6) % 24;
	int minutes = totalMinutes % 60;
	const char* ampm = (hours >= 12) ? "PM" : "AM";
	int displayHours = hours % 12;
	if (displayHours == 0) displayHours = 12;

	const char* phaseName = "Dia";
	int dayColor = 0xffffd530; // Solar gold
	// The engine's day length is configurable (currently 19,200 ticks), while
	// the vanilla phase boundaries are expressed in Java ticks.
	int dayTick = ticksPerDay * 1000 / 24000;
	int sunsetTick = ticksPerDay * 11500 / 24000;
	int nightTick = ticksPerDay * 13000 / 24000;
	if (timeInDay < dayTick) {
		phaseName = "Amanecer";
		dayColor = 0xffff7f40;
	} else if (timeInDay < sunsetTick) {
		phaseName = "Dia";
		dayColor = 0xffffd530;
	} else if (timeInDay < nightTick) {
		phaseName = "Atardecer";
		dayColor = 0xffff5520;
	} else {
		phaseName = "Noche";
		dayColor = 0xff7b5ce8;
	}

	// Player coordinates & Biome
	LocalPlayer* p = minecraft->player;
	float px = p->x, py = p->y - p->heightOffset, pz = p->z;

	int bx = (int)floorf(px);
	int bz = (int)floorf(pz);
	Biome* biome = minecraft->level->getBiome(bx, bz);
	std::string biomeName = biome ? biome->name : "Desconocido";

	// This engine has no world weather state, so show the biome climate instead
	// of presenting a biome-derived value as current rain or thunder.
	char climateBuf[64];
	if (biome) {
		const char* temperature = biome->getTemperature() < 0.35f ? "Frio"
			: biome->getTemperature() > 1.0f ? "Calido" : "Templado";
		const char* humidity = biome->getDownfall() > 0.65f ? "Humedo"
			: biome->getDownfall() < 0.2f ? "Seco" : "Normal";
		sprintf(climateBuf, "%s / %s", temperature, humidity);
	} else {
		sprintf(climateBuf, "Desconocido");
	}

	// Modern Status Card Layout with Responsive Adaptive Scaling
	// Senses available horizontal space before center touch buttons (Chat/Camera/Pause)
	float chatButtonLeft = (float)screenWidth * 0.5f - 46.0f;
	if (!minecraft->useTouchscreen()) {
		chatButtonLeft = (float)screenWidth * 0.45f;
	}

	const float baseCardW = 190.0f;
	const float baseCardH = 58.0f;

	// Calculate scale factor so the card never collides with center buttons on small screens (iPhone 4/5/SE)
	float cardScale = 1.0f;
	float maxAllowedW = chatButtonLeft - 6.0f; // 6px safety margin
	if (maxAllowedW < 90.0f) maxAllowedW = 90.0f;

	if (maxAllowedW < baseCardW) {
		cardScale = maxAllowedW / baseCardW;
		if (cardScale < 0.65f) cardScale = 0.65f; // Min readable scale
	} else if (screenWidth < 360) {
		cardScale = 0.75f;
	}

	const int cardX = 3;
	const int cardY = 3;

	glPushMatrix2();
	glTranslatef2((float)cardX, (float)cardY, 0.0f);
	glScalef2(cardScale, cardScale, 1.0f);

	const int cardW = (int)baseCardW;
	const int cardH = (int)baseCardH;

	// Dark semi-transparent glass panel with glowing border
	fill(0, 0, cardW, cardH, 0xc00c1420);
	fill(0, 0, cardW, 1, 0x504090e0);
	fill(0, cardH - 1, cardW, cardH, 0x504090e0);
	fill(0, 0, 1, cardH, 0x504090e0);
	fill(cardW - 1, 0, cardW, cardH, 0x504090e0);

	// Fila 1: Modo de Juego Badge (Izquierda) & Coordenadas Y (Derecha)
	const char* modeText = minecraft->isCreativeMode() ? "[CREATIVO]" : "[SUPERVIVENCIA]";
	if (cardScale < 0.80f) {
		modeText = minecraft->isCreativeMode() ? "[CREAT]" : "[SUPERV]";
	}
	int modeCol = minecraft->isCreativeMode() ? 0xff50e0ff : 0xff50f080;
	font->drawShadow(modeText, 4.0f, 3.0f, modeCol);

	char coordBuf[64];
	sprintf(coordBuf, "Y:%.0f (%.0f, %.0f)", py, px, pz);
	float coordX = (float)(cardW - font->width(coordBuf) - 4);
	if (coordX < 4.0f + font->width(modeText) + 2.0f) {
		sprintf(coordBuf, "Y:%.0f", py);
		coordX = (float)(cardW - font->width(coordBuf) - 4);
	}
	font->drawShadow(coordBuf, coordX, 3.0f, 0xffbdd4ea);

	// Fila 2: Hora del Juego & Hora Real
	char timeBuf[128];
	sprintf(timeBuf, "%s  %02d:%02d %s  |  Real %s", phaseName, displayHours, minutes, ampm, realTimeStr);
	font->drawShadow(timeBuf, 4.0f, 13.0f, 0xffffe8a0);

	// Fila 3: Barra animada del ciclo solar día/noche
	int barX = 4;
	int barY = 23;
	int barW = cardW - 8;
	fill(barX, barY, barX + barW, barY + 2, 0x800a1018);
	float dayProgress = (float)timeInDay / (float)ticksPerDay;
	int fillDayW = (int)(dayProgress * barW);
	if (fillDayW > 0) {
		fill(barX, barY, barX + fillDayW, barY + 2, dayColor);
	}

	// Fila 4: Clima del bioma (no es lluvia/tormenta; el motor no simula esos estados todavía)
	char envBuf[128];
	sprintf(envBuf, "Clima: %s  |  Dia %d", climateBuf, day);
	font->drawShadow(envBuf, 4.0f, 27.0f, 0xff90d0ff);

	// Fila 5: Bioma actual, truncated to keep the card readable on narrow screens.
	std::string visibleBiome = biomeName;
	while (font->width("Bioma: " + visibleBiome) > cardW - 8 && visibleBiome.size() > 3) {
		visibleBiome.resize(visibleBiome.size() - 1);
	}
	if (visibleBiome != biomeName) visibleBiome += "...";
	font->drawShadow("Bioma: " + visibleBiome, 4.0f, 36.0f, 0xffb8c8d8);

	char tickBuf[64];
	sprintf(tickBuf, "Ticks: %d / %d", timeInDay, ticksPerDay);
	font->drawShadow(tickBuf, 4.0f, 46.0f, 0xff819bb5);

	glPopMatrix2();
}

void Gui::renderDebugInfo() {
	// FPS counter (updates once per second)
	static int fps = 0;
	static int fpsLastTime = 0;
	static int   fpsFrames = 0;
	static int   displayChunkUpdates = 0;
	float now = getTimeS();
	fpsFrames++;
	if (now - fpsLastTime >= 1) {
		fps = fpsFrames / (now - fpsLastTime);

		displayChunkUpdates = Chunk::updates;
    
		Chunk::updates = 0;

		fpsFrames = 0;
		fpsLastTime = now;

		FILE* fp = fopen("debug_profiling.txt", "a");
		if (fp) {
			fprintf(fp, "[%.2fs] FPS: %d | ChunkUpdates: %d | %s | %s | %s | %s\n",
				now, fps, displayChunkUpdates,
				minecraft->gatherStats1().c_str(), minecraft->gatherStats2().c_str(),
				minecraft->gatherStats3().c_str(), minecraft->gatherStats4().c_str());
			fclose(fp);
		}
	}

	LocalPlayer* p   = minecraft->player;
	Level*       lvl = minecraft->level;

	// Position
	float px = p->x, py = p->y - p->heightOffset, pz = p->z;
	posTranslator.to(px, py, pz);
	int bx = (int)floorf(px), by = (int)floorf(py), bz = (int)floorf(pz);
	int cx = bx >> 4, cz = bz >> 4;

	// Facing direction
	float yMod = fmodf(p->yRot, 360.0f);
	if (yMod < 0) yMod += 360.0f;
	const char* facing;
	const char* axis;
	if      (yMod < 45  || yMod >= 315) { facing = "South"; axis = "+Z"; }
	else if (yMod < 135)                 { facing = "West";  axis = "-X"; }
	else if (yMod < 225)                 { facing = "North"; axis = "-Z"; }
	else                                 { facing = "East";  axis = "+X"; }

	// Biome
	const char* biomeName = "unknown";
	if (lvl) {
		Biome* biome = lvl->getBiome(bx, bz);
		if (biome) biomeName = biome->name.c_str();
	}

	// Block looking at
	std::string CurrentTile = "Air";
	if (minecraft->hitResult.type == TILE) {
		int LookingX = minecraft->hitResult.x;
		int LookingY = minecraft->hitResult.y;
		int LookingZ = minecraft->hitResult.z;

		int tileID = minecraft->level->getTile(LookingX, LookingY, LookingZ);
		if (tileID > 0 && tileID < 256){
			Tile* LookingTile = Tile::tiles[tileID];
			if (LookingTile != NULL) {
				CurrentTile = LookingTile->getDescriptionId();
			} else {
				CurrentTile = "Unknown Tile";
			}
		} else {
			CurrentTile = "Air";
		}
	}

	Font* font = minecraft->font;
	char buf[256];

	sprintf(buf, "FPS: %d | XYZ: %.1f %.1f %.1f", fps, px, py, pz);
	font->drawShadow(buf, 2, 2, 0xff00FF00); 

	sprintf(buf, "Chunk: %d %d | Facing: %s | Biome: %s", cx, cz, facing, biomeName);
	font->drawShadow(buf, 2, 12, 0xffDDDDDD);

	sprintf(buf, "Target: %s", CurrentTile.c_str());
	font->drawShadow(buf, 2, 22, 0xffDDDDDD);
}

void Gui::renderPlayerList(Font* font, int screenWidth, int screenHeight) {
	// only show when in game, no other screen
	// if (!minecraft->level) return;

	// only show the overlay while connected to a multiplayer server
	Level* level = minecraft->level;
	if (!level) return;
	if (!level->isClientSide) return;

	std::vector<std::string> playerNames;
	playerNames.reserve(level->players.size());

	for (Player* player : level->players) {
		if (!player) continue;
		playerNames.push_back(player->name);
	}

	// is this check needed? if there are no players, the box won't render at all since height will be 0, 
	// but maybe we want to skip rendering entirely in that case
	// if (playerNames.empty())
	// 	return;

	std::sort(playerNames.begin(), playerNames.end());

	float maxNameWidth = 0.0f;
	// find the longest name so we can size the box accordingly
	for (const std::string& name : playerNames) {
		float nameWidth = font->width(name);
		if (nameWidth > maxNameWidth)
			maxNameWidth = nameWidth;
	}

	// player count title
	std::ostringstream titleStream;
	titleStream << "Players (" << playerNames.size() << ")";
	std::string titleText = titleStream.str();
	float titleWidth = font->width(titleText);

	if (titleWidth > maxNameWidth)
		maxNameWidth = titleWidth;

	const float padding = 4.0f;
	const float lineHeight = (float)Font::DefaultLineHeight;

	const float boxWidth = maxNameWidth + padding * 2;
	const float boxHeight = (playerNames.size() + 1) * lineHeight + padding * 2;

	const float boxLeft = (screenWidth - boxWidth) / 2.0f;
	const float boxTop = 10.0f;
	const float boxRight = boxLeft + boxWidth;
	const float boxBottom = boxTop + boxHeight;

	fill(boxLeft, boxTop, boxRight, boxBottom, 0x90000000);

	float titleX = (screenWidth - titleWidth) / 2.0f;
	float titleY = boxTop + padding;

	// scale the text down slightly
	// i think the gl scaling is the best for this
	// oh my god this looks really bad OH GOD
	//const float textScale = 0.8f;
	//const float invTextScale = 1.0f / textScale;
	//glPushMatrix2();
	//glScalef2(textScale, textScale, 1);

	// draw title
	//font->draw(titleText, titleX * invTextScale, titleY * invTextScale, 0xFFFFFFFF);
	font->draw(titleText, titleX, titleY, 0xFFFFFFFF);

	// draw player names
	// we should add ping icons here eventually, but for now just show names
	float currentY = boxTop + padding + lineHeight;
	for (const std::string& name : playerNames) {
		font->draw(name, (boxLeft + padding), currentY, 0xFFDDDDDD);
		currentY += lineHeight;
	}
	//glPopMatrix2();
}

void Gui::renderSleepAnimation( const int screenWidth, const int screenHeight ) {
	int timer = minecraft->player->getSleepTimer();
	float amount = (float) timer / (float) Player::SLEEP_DURATION;
	if (amount > 1) {
		// waking up
		amount = 1.0f - ((float) (timer - Player::SLEEP_DURATION) / (float) Player::WAKE_UP_DURATION);
	}

	int color = (int) (220.0f * amount) << 24 | (0x101020);
	fill(0, 0, screenWidth, screenHeight, color);
}

void Gui::renderOnSelectItemNameText( const int screenWidth, Font* font, int ySlot ) {
	if(itemNameOverlayTime < 1.0f) {
		ItemInstance* item = minecraft->player->inventory->getSelected();
		if(item != NULL) {
			float x = float(screenWidth / 2 - font->width(item->getName()) / 2);
			float y = float(ySlot - 22);
			int alpha = 255;
			if(itemNameOverlayTime > 0.75) {
				float time = 0.25f - (itemNameOverlayTime - 0.75f);
				float percentage = cubeSmoothStep(time *  4, 0.0f, 1.0f);
				alpha = int(percentage * 255);
			}
			if(alpha != 0)
				font->drawShadow(item->getName(), x, y, 0x00ffffff + (alpha << 24));
		}
	}
}



// helper structure used by drawColoredString
struct ColorSegment {
	std::string text;
	uint32_t color;
};

// parse [tag] and [/tag] markers; tags may contain a color name (gold, green, etc.)
static void parseColorTags(const std::string& in, std::vector<ColorSegment>& out) {
	uint32_t curColor = 0xffffff;
	size_t pos = 0;
	while (pos < in.size()) {
		size_t open = in.find('[', pos);
		if (open == std::string::npos) {
			out.push_back({in.substr(pos), curColor});
			break;
		}
		if (open > pos) {
			out.push_back({in.substr(pos, open - pos), curColor});
		}
		size_t close = in.find(']', open);
		if (close == std::string::npos) {
			out.push_back({in.substr(open), curColor});
			break;
		}
		std::string tag = in.substr(open + 1, close - open - 1);
		if (!tag.empty() && tag[0] == '/') {
			curColor = 0xffffff;
		} else {
			std::string lower;
			lower.resize(tag.size());
			std::transform(tag.begin(), tag.end(), lower.begin(), ::tolower);
			if (lower.find("gold") != std::string::npos) curColor = 0xffd700;
			else if (lower.find("green") != std::string::npos) curColor = 0x00ff00;
			else if (lower.find("yellow") != std::string::npos) curColor = 0xffff00;
			else if (lower.find("red") != std::string::npos) curColor = 0xff0000;
			else if (lower.find("blue") != std::string::npos) curColor = 0x0000ff;
		}
		pos = close + 1;
	}
}

void Gui::drawColoredString(Font* font, const std::string& text, float x, float y, int alpha) {
	std::vector<ColorSegment> segs;
	parseColorTags(text, segs);
	float cx = x;
	for (auto &s : segs) {
		int color = s.color + (alpha << 24);
		font->drawShadow(s.text, cx, y, color);
		cx += font->width(s.text);
	}
}

float Gui::getColoredWidth(Font* font, const std::string& text) {
	std::vector<ColorSegment> segs;
	parseColorTags(text, segs);
	float w = 0;
	for (auto &s : segs) {
		w += font->width(s.text);
	}
	return w;
}

void Gui::renderChatMessages( const int screenHeight, unsigned int max, bool isChatting, Font* font ) {
	//        if (minecraft.screen instanceof ChatScreen) {
	//            max = 20;
	//            isChatting = true;
	//        }
	//
	//        glEnable(GL_BLEND);
	//        glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//        GLState::setAlphaTestEnabled(false);
	//
	//        glPushMatrix2();
	//        glTranslatef2(0, screenHeight - 48, 0);
	//        // glScalef2(1.0f / ssc.scale, 1.0f / ssc.scale, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int offsetY = (isChatting && minecraft->useTouchscreen()) ? (screenHeight / 2) : 0;
	int baseY = screenHeight - 48 - offsetY;
	int start = chatScrollOffset;
	if (start < 0) start = 0;
	for (unsigned int i = 0; i < max; i++) {
		unsigned int msgIdx = (unsigned int)start + i;
		if (msgIdx >= guiMessages.size())
			break;

		GuiMessage& message = guiMessages.at(msgIdx);
		if (message.ticks < 20 * 10 || isChatting) {
			float t = message.ticks / (20 * 10.0f);
			t = 1 - t;
			t = t * 10;
			if (t < 0) t = 0;
			if (t > 1) t = 1;
			t = t * t;
			int alpha = (int) (255 * t);
			if (isChatting) alpha = 255;

			if (alpha > 0) {
				const float x = 2;
				const float y = (float)(baseY - i * 9);
				std::string msg = message.message;
				this->fill(x, y - 1, x + MAX_MESSAGE_WIDTH, y + 8, (alpha / 2) << 24);
				glEnable(GL_BLEND);

				// special-case join/leave announcements
				int baseColor = 0xffffff;
				if (msg.find(" joined the game") != std::string::npos ||
					msg.find(" left the game") != std::string::npos) {
						baseColor = 0xffff00; // yellow
				}
				// replace previous logic; allow full colour tags now
				Gui::drawColoredString(font, msg, x, y, alpha);
			}
		}
	}
}

void Gui::renderToolBar( float a, int ySlot, const int screenWidth ) {
	glColor4f2(1, 1, 1, 1.0f);
	
	// Ensure the toolbar (background + slots) is drawn on top of other UI
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Inventory* inventory = minecraft->player->inventory;

	int xBase, yBase;
	getSlotPos(0, xBase, yBase);
	const float baseItemX = (float)xBase + 3;

	// Render the hotbar background
	if (minecraft->useTouchscreen()) {
		minecraft->textures->loadAndBindTexture("gui/hotbar-android.png");
		blit(xBase - 1, yBase, 0, 0, 202, 22, 256, 256);
	} else {
		minecraft->textures->loadAndBindTexture("gui/gui.png");
		blit(xBase - 1, yBase, 0, 0, 182, 22);
	}

	if (_currentDropSlot >= 0 && inventory->getItem(_currentDropSlot)) {
		int x = xBase + 3 +  _currentDropSlot * 20;
		int color = 0x8000ff00;
		int yy = (int)(17.0f * (_currentDropTicks + a) / DropTicks);

		if (_currentDropTicks >= 3) {
			glColor4f2(0, 1, 0, 0.5f);
		}
		fill(x, ySlot+16-yy, x+16, ySlot+16, color);
	}
	
	// Render selection overlay
	int maxSelectable = _openInventorySlot ? (getNumSlots() - 1) : getNumSlots();
	if (inventory->selected >= 0 && inventory->selected < maxSelectable) {
		minecraft->textures->loadAndBindTexture("gui/gui.png");
		blit(xBase - 1 + 20 * inventory->selected, yBase - 1, 0, 22, 24, 24);
	}
	glColor4f2(1, 1, 1, 1);

	// Flash a slot background
	if (_flashSlotId >= 0) {
		const float since = getTimeS() - _flashSlotStartTime;
		if (since > 0.2f) _flashSlotId = -1;
		else {
			int x = xBase + 3 + _flashSlotId * 20;
			int color = 0xffffff + (((int)(0x51 - 0x50 * Mth::cos(10 * 6.28f * since))) << 24);
			fill(x, ySlot, x+16, ySlot+16, color);
		}
	}
	glColor4f2(1, 1, 1, 1);

	Tesselator& t = Tesselator::instance;

	float x = baseItemX;

	int slots = getNumSlots() - _openInventorySlot;

	for (int i = 0; i < slots; i++) {
		ItemInstance* item = inventory->getItem(i);
		if (item)
			ItemRenderer::renderGuiItemCorrect(minecraft->font, minecraft->textures, item, (int)x, ySlot);
		x += 20;
	}
	_inventoryNeedsUpdate = false;

	// Render damaged items (@todo: investigate if it's faster by drawing in same batch)
	glDisable2(GL_DEPTH_TEST);
	GLState::setTextureEnabled(false);
	t.beginOverride();
	x = baseItemX;
	for (int i = 0; i < slots; i++) {
		ItemRenderer::renderGuiItemDecorations(minecraft->player->inventory->getItem(i), x, (float)ySlot);
		x += 20;
	}
	t.endOverrideAndDraw();
	glEnable(GL_DEPTH_TEST);
	GLState::setTextureEnabled(true);

	// Draw count
	const float k = 0.5f * GuiScale;
	if (minecraft->options.getBooleanValue(OPTIONS_JAVA_HUD)) // if true enables the java beta item count size and color and calls the java items decorations
	{
		t.beginOverride();
		if (minecraft->gameMode->isSurvivalType()) {
			x = baseItemX;
			for (int i = 0; i < slots; i++) {
				ItemInstance* item = minecraft->player->inventory->getItem(i);
				if (item && item->count >= 0)
					ItemRenderer::renderGuiItemDecorations(minecraft->font, minecraft->textures, minecraft->player->inventory->getItem(i), x, (float)ySlot);
				x += 20;
			}
		}
		minecraft->textures->loadAndBindTexture("font/default8.png");
		t.endOverrideAndDraw();
	}
	else { // otherwise uses the normal pocket edition one
		glPushMatrix2();
		glScalef2(InvGuiScale + InvGuiScale, InvGuiScale + InvGuiScale, 1);
		t.beginOverride();
		if (minecraft->gameMode->isSurvivalType()) {
			x = baseItemX;
			for (int i = 0; i < slots; i++) {
				ItemInstance* item = minecraft->player->inventory->getItem(i);
				if (item && item->count >= 0)
					renderSlotText(item, k*x, k*ySlot, true, true);
				x += 20;
			}
		}

		minecraft->textures->loadAndBindTexture("font/default8.png");
		t.endOverrideAndDraw();

		glPopMatrix2();
	}

	if (_openInventorySlot) {
		glColor4f2(1, 1, 1, 1);
		minecraft->textures->loadAndBindTexture("gui/inventory_menu_button.png");
		blit(xBase + 20 * (getNumSlots() - 1) + 3, yBase + 9, 0, 0, 14, 4, 256, 256);
	}
	glDisable(GL_BLEND);
}
