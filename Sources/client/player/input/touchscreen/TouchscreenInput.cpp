#include "TouchscreenInput.h"
#include "../../../Options.h"
#include "../../../../platform/input/Multitouch.h"
#include "../../../gui/Gui.h"
#include "../../../renderer/Tesselator.h"
#include "../../../../world/entity/player/Player.h"

#include "../../../Minecraft.h"
#include "../../../../platform/log.h"
#include "../../../renderer/Textures.h"
#include "../../../sound/SoundEngine.h"
#include "client/gui/screens/ScreenChooser.h"


// ARGHHHHHH WHY NOT FUCKING ENUM
static const int AREA_DPAD_FIRST = 100;
static const int AREA_DPAD_N = 100;
static const int AREA_DPAD_S = 101;
static const int AREA_DPAD_W = 102;
static const int AREA_DPAD_E = 103;
static const int AREA_DPAD_C = 104; // Jump / Fly Up
static const int AREA_PAUSE = 105;
static const int AREA_CHAT = 106;
static const int AREA_THIRD = 107;
static const int AREA_SNEAK = 108; // Shift / Sneak in middle of D-pad
static const int AREA_FLY_DOWN = 109; // Fly Down
static const int AREA_FLIGHT_TOGGLE = 110; // Flight toggle on right side

static int cPressed = 0;
static int cReleased = 0;
static int cDiscreet = 0;
static int cPressedPause = 0;
static int cReleasedPause = 0;
//static const int AREA_DPAD_N_JUMP = 105;

//
// TouchscreenInput_TestFps
//

static void Copy(int n, float* x, float* y, float* dx, float* dy) {
	for (int i = 0; i < n; ++i) {
		dx[i] = x[i];
		dy[i] = y[i];
	}
}

static void Translate(int n, float* x, float* y, float xt, float yt) {
	for (int i = 0; i < n; ++i) {
		x[i] += xt;
		y[i] += yt;
	}
}

static void Scale(int n, float* x, float* y, float xt, float yt) {
	for (int i = 0; i < n; ++i) {
		x[i] *= xt;
		y[i] *= yt;
	}
}

static void Transformed(int n, float* x, float* y, float* dx, float* dy, float xt, float yt, float sx=1.0f, float sy=1.0f) {
	Copy(n, x, y, dx, dy);
	Scale(n, dx, dy, sx, sy);
	Translate(n, dx, dy, xt, yt);

	//for (int i = 0; i < n; ++i) {
	//	LOGI("%d. (%f, %f)\n", i, dx[i], dy[i]);
	//}
}

TouchscreenInput_TestFps::TouchscreenInput_TestFps( Minecraft* mc, Options* options )
:	_minecraft(mc),
	_options(options),
	_northJump(false),
	_forward(false),
	_boundingRectangle(0, 0, 1, 1),
	_pressedJump(false),
	_pauseIsDown(false),
	_sneakTapTime(-999),
	aLeft(0),
	aRight(0),
	aUp(0),
	aDown(0),
	aJump(0),
	aUpLeft(0),
	aUpRight(0),
	_allowHeightChange(false)
{
	releaseAllKeys();
	onConfigChanged( createConfig(mc) );

	Tesselator& t = Tesselator::instance;
	const int alpha = 128;
	t.color( 0xc0c0c0, alpha); cPressed  = t.getColor();
	t.color( 0xffffff, alpha); cReleased = t.getColor();
	t.color( 0xffffff, alpha / 4); cDiscreet = t.getColor();
    t.color( 0xc0c0c0, 80); cPressedPause=t.getColor();
    t.color( 0xffffff, 80); cReleasedPause=t.getColor();
}

TouchscreenInput_TestFps::~TouchscreenInput_TestFps() {
	clear();
}

void TouchscreenInput_TestFps::clear() {
	_model.clear();

	delete aUpLeft; aUpLeft = NULL; // @todo: SAFEDEL
	delete aUpRight; aUpRight = NULL;
}

bool TouchscreenInput_TestFps::isButtonDown(int areaId) {
	return _buttons[areaId - AREA_DPAD_FIRST];
}


void TouchscreenInput_TestFps::onConfigChanged(const Config& c) {
	clear();

	const float w = (float)c.width;
	const float h = (float)c.height;

	/*
	// Code for "Move when touching left side of the screen"
	float x0[] = {  0,  w * 0.3f,  w * 0.3f,     0 };
	float y0[] = {	0,	       0,      h-32,  h-32 };

	_model.addArea(AREA_MOVE, new RectangleArea(0, 0, w*0.3f, h-32));
	*/

	// Code for "D-pad with shift in center"
	float Bw = w * 0.08f;
	float Bh = Bw;
	// temp data
	float xx;
	float yy;

	const float BaseY = -8 + h - 3.0f * Bh;
	const float BaseX = _options->getBooleanValue(OPTIONS_IS_LEFT_HANDED)? -8 + w - 3 * Bw
											:	8 + 0;
	// Setup the bounding rectangle for D-pad (keeping standard size for touch/drag calculation)
	_boundingRectangle = RectangleArea(BaseX, BaseY, BaseX + 3 * Bw, BaseY + 3 * Bh);

	xx = BaseX + Bw; yy = BaseY;
	_model.addArea(AREA_DPAD_N, aUp = new RectangleArea(xx, yy, xx+Bw, yy+Bh));
	xx = BaseX;
	aUpLeft = new RectangleArea(xx, yy, xx+Bw, yy+Bh);
	xx = BaseX + 2 * Bw;
	aUpRight = new RectangleArea(xx, yy, xx+Bw, yy+Bh);

	// Center of D-pad: Shift / Sneak button
	xx = BaseX + Bw; yy = BaseY + Bh;
	_model.addArea(AREA_SNEAK, aSneak = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	xx = BaseX + Bw; yy = BaseY + 2 * Bh;
	_model.addArea(AREA_DPAD_S, aDown = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	xx = BaseX; yy = BaseY + Bh;
	_model.addArea(AREA_DPAD_W, aLeft = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	xx = BaseX + 2 * Bw; yy = BaseY + Bh;
	_model.addArea(AREA_DPAD_E, aRight = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	// Right stack positioning with matching margin as Shift button from the left (8.0f + Bw)
	const float BaseX_Right = _options->getBooleanValue(OPTIONS_IS_LEFT_HANDED) ? 8.0f + Bw : w - 8.0f - 2.0f * Bw;
	const float BaseY_Right = BaseY;

	// 1. Jump / Fly Up (Default to middle height BaseY_Right + Bh when walking to align with Shift)
	xx = BaseX_Right; yy = BaseY_Right + Bh;
	_model.addArea(AREA_DPAD_C, aJump = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	// 2. Flight Toggle (Middle button on the right when flying)
	xx = BaseX_Right; yy = BaseY_Right + Bh;
	_model.addArea(AREA_FLIGHT_TOGGLE, aFlightToggle = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

	// 3. Fly Down (Bottom button on the right when flying)
	xx = BaseX_Right; yy = BaseY_Right + 2.0f * Bh;
	_model.addArea(AREA_FLY_DOWN, aFlyDown = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

    PixelCalc& pc = _minecraft->pixelCalc;
    float maxPixels = pc.millimetersToPixels(10);
    // float btnSize = Mth::Min(18 * Gui::GuiScale, maxPixels);
	float btnSize = pc.millimetersToPixels(18 * Gui::GuiScale);

	// Center the chat, pause and new third-person buttons at the top
	_model.addArea(AREA_PAUSE, aPause = new RectangleArea(w/2 + btnSize + 4, 4, w/2 + btnSize * 2 + 4, 4 + btnSize));
	_model.addArea(AREA_THIRD, aThird = new RectangleArea(w/2 - btnSize/2, 4, w/2 + btnSize/2, 4 + btnSize));
	_model.addArea(AREA_CHAT,  aChat  = new RectangleArea(w/2 - btnSize * 2 - 4, 4, w/2 - btnSize - 4, 4 + btnSize));

	//rebuild();
}

void TouchscreenInput_TestFps::setKey(int key, bool state)
{
	#ifdef WIN32
		//LOGI("key: %d, %d\n", key, state);

		int id = -1;
		// theres no keyUp etc???
		//if (key == _options->keyUp.key) id = KEY_UP;
		//if (key == _options->keyDown.key) id = KEY_DOWN;
		//if (key == _options->keyLeft.key) id = KEY_LEFT;
		//if (key == _options->keyRight.key) id = KEY_RIGHT;
		//if (key == _options->keyJump.key) id = KEY_JUMP;
		//if (key == _options->keySneak.key) id = KEY_SNEAK;
		//if (key == _options->keyCraft.key) id = KEY_CRAFT;
		//if (id >= 0) {
		//	_keys[id] = state;
		//}

		if (key == _options->getIntValue(OPTIONS_KEY_FORWARD)) id = KEY_UP;
		if (key == _options->getIntValue(OPTIONS_KEY_BACK)) id = KEY_DOWN;
		if (key == _options->getIntValue(OPTIONS_KEY_LEFT)) id = KEY_LEFT;
		if (key == _options->getIntValue(OPTIONS_KEY_RIGHT)) id = KEY_RIGHT;
		if (key == _options->getIntValue(OPTIONS_KEY_JUMP)) id = KEY_JUMP;
		if (key == _options->getIntValue(OPTIONS_KEY_SNEAK)) id = KEY_SNEAK;
		//if (key == _options->getIntValue(OPTIONS_KEY_CRAFT)) id = KEY_CRAFT;
	#endif
}

void TouchscreenInput_TestFps::releaseAllKeys()
{
	xa = 0;
	ya = 0;

	for (int i = 0; i<16; ++i)
		_buttons[i] = false;
#ifdef WIN32
	for (int i = 0; i<NumKeys; ++i)
		_keys[i] = false;
#endif
	_pressedJump = false;
	_allowHeightChange = false;
}

void TouchscreenInput_TestFps::tick( Player* player )
{
	xa = 0;
	ya = 0;
	jumping = false;
	wantUp = false;
	wantDown = false;

	//bool gotEvent = false;
	bool heldJump = false;
	bool tmpForward = false;
	bool tmpNorthJump = false;

	for (int i = 0; i < 16; ++i)
		_buttons[i] = false;

	const int* pointerIds;
	int pointerCount = Multitouch::getActivePointerIdsThisUpdate(&pointerIds);
	for (int i = 0; i < pointerCount; ++i) {
		int p = pointerIds[i];
		int x = Multitouch::getX(p);
		int y = Multitouch::getY(p);

		if (_boundingRectangle.isInside((float)x, (float)y) && _forward)
		{
			float angle = Mth::PI + Mth::atan2(y - _boundingRectangle.centerY(), x - _boundingRectangle.centerX());
			ya = Mth::sin(angle);
			xa = Mth::cos(angle);
			tmpForward = true;
		}

		int areaId = _model.getPointerId(x, y, p);
		if (areaId < AREA_DPAD_FIRST)
		{
			continue;
		}

		bool setButton = false;

		if (Multitouch::isPressed(p))
			_allowHeightChange = (areaId == AREA_DPAD_C);

		if (areaId == AREA_DPAD_C)
		{
			setButton = true;
			heldJump = true;
			// If we're in water or pressed down on the button: jump
			if (player->isInWater()) {
				jumping = true;
			}
			else if (Multitouch::isPressed(p)) {
				jumping = true;
			} // Or if we are walking forward, jump while going forward!
			else if (_forward && !player->abilities.flying) {
				areaId = AREA_DPAD_N;
				tmpNorthJump = true;
				//jumping = true;
				ya += 1;
			}
			if (player->abilities.flying) {
				wantUp = true;
			}
		}
		else if (areaId == AREA_DPAD_N)
		{
			setButton = true;
			if (player->isInWater())
				jumping = true;
			tmpForward = true;
			ya += 1;
		}
		else if (areaId == AREA_DPAD_S && !_forward)
		{
			setButton = true;
            ya -= 1;
        }
		else if (areaId == AREA_DPAD_W && !_forward)
		{
			setButton = true;
			xa += 1;
		}
		else if (areaId == AREA_DPAD_E && !_forward)
		{
			setButton = true;
			xa -= 1;
		}
		else if (areaId == AREA_SNEAK) {
			setButton = true;
			if (Multitouch::isPressed(p)) {
				sneaking = !sneaking;
				player->setSneaking(sneaking);
			}
		}
		else if (areaId == AREA_FLIGHT_TOGGLE) {
			setButton = true;
			if (Multitouch::isPressed(p)) {
				if (player->abilities.flying) {
					player->abilities.flying = false;
				} else if (player->abilities.mayfly && !player->onGround) {
					player->abilities.flying = true;
				}
			}
		}
		else if (areaId == AREA_FLY_DOWN) {
			setButton = true;
			if (player->abilities.flying) {
				wantDown = true;
			}
		}
		else if (areaId == AREA_PAUSE) {
			setButton = true;
			if (Multitouch::isReleased(p)) {
                _minecraft->soundEngine->playUI("random.click", 1, 1);
				_minecraft->screenChooser.setScreen(SCREEN_PAUSE);
            }
		}
		else if (areaId == AREA_CHAT) {
			setButton = true;
			if (Multitouch::isReleased(p)) {
                _minecraft->soundEngine->playUI("random.click", 1, 1);
				_minecraft->screenChooser.setScreen(SCREEN_CONSOLE);
				_minecraft->platform()->showKeyboard();
            }
		}
		else if (areaId == AREA_THIRD) {
			setButton = true;
			if (Multitouch::isReleased(p)) {
				_minecraft->soundEngine->playUI("random.click", 1, 1);
				// Toggle third person view like F5
				int currentMode = _minecraft->options.getIntValue(OPTIONS_CAMERA_MODE);
				currentMode = (currentMode + 1) % 4;
				_minecraft->options.set(OPTIONS_CAMERA_MODE, currentMode);
			}
		}

		_buttons[areaId - AREA_DPAD_FIRST] = setButton;
	}

	_forward = tmpForward;

	// Only jump once at a time
	if (tmpNorthJump) {
		if (!_northJump)
			jumping = true;
		_northJump = true;
	}
	else _northJump = false;

	isChangingFlightHeight = player->abilities.flying && (wantUp || wantDown);
	_renderFlightImage = player->abilities.flying;

#ifdef WIN32
	if (_keys[KEY_UP]) ya++;
	if (_keys[KEY_DOWN]) ya--;
	if (_keys[KEY_LEFT]) xa++;
	if (_keys[KEY_RIGHT]) xa--;
	if (_keys[KEY_JUMP]) jumping = true;
	if (_keys[KEY_SNEAK]) {
		sneaking = !_keys[KEY_SNEAK];
		player->setSneaking(sneaking);
	}

	if (_keys[KEY_CRAFT])
		player->startCrafting((int)player->x, (int)player->y, (int)player->z, Recipe::SIZE_2X2);
#endif

	if (sneaking) {
		xa *= 0.3f;
		ya *= 0.3f;
	}
	//printf("\n>- %f %f\n", xa, ya);
	_pressedJump = heldJump;
}

static void drawRectangleArea(Tesselator& t, RectangleArea* a, int ux, int vy, float ssz = 64.0f) {
	const float pm = 1.0f / 256.0f;
	const float sz = ssz * pm;
	const float uu = (float)(ux) * pm;
	const float vv = (float)(vy) * pm;
	const float x0 = a->_x0 * Gui::InvGuiScale;
	const float x1 = a->_x1 * Gui::InvGuiScale;
	const float y0 = a->_y0 * Gui::InvGuiScale;
	const float y1 = a->_y1 * Gui::InvGuiScale;

	t.vertexUV(x0, y1, 0, uu,	vv+sz);
	t.vertexUV(x1, y1, 0, uu+sz,vv+sz);
	t.vertexUV(x1, y0, 0, uu+sz,vv);
	t.vertexUV(x0, y0, 0, uu,	vv);
}

static void drawRectangleAreaStandalone(Tesselator& t, RectangleArea* a) {
	const float x0 = a->_x0 * Gui::InvGuiScale;
	const float x1 = a->_x1 * Gui::InvGuiScale;
	const float y0 = a->_y0 * Gui::InvGuiScale;
	const float y1 = a->_y1 * Gui::InvGuiScale;

	t.vertexUV(x0, y1, 0, 0, 1);
	t.vertexUV(x1, y1, 0, 1, 1);
	t.vertexUV(x1, y0, 0, 1, 0);
	t.vertexUV(x0, y0, 0, 0, 0);
}

static void drawPolygonArea(Tesselator& t, PolygonArea* a, int x, int y) {
	float pm = 1.0f / 256.0f;
	float sz = 64.0f * pm;
	float uu = (float)(x) * pm;
	float vv = (float)(y) * pm;

	float uvs[] = {uu, vv, uu+sz, vv, uu+sz, vv+sz, uu, vv+sz};
	const int o = 0;

	for (int j = 0; j < a->_numPoints; ++j) {
		t.vertexUV(a->_x[j] * Gui::InvGuiScale, a->_y[j] * Gui::InvGuiScale, 0, uvs[(o+j+j)&7], uvs[(o+j+j+1)&7]);
	}
}

void TouchscreenInput_TestFps::render( float a ) {
	//return;

	//static Stopwatch sw;
	//sw.start();


	//glColor4f2(1, 0, 1, 1.0f);
	//glDisable2(GL_CULL_FACE);
	GLState::setAlphaTestEnabled(false);

	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	_minecraft->textures->loadAndBindTexture("gui/gui.png");
	
	//GLState::setTextureEnabled(false);

	rebuild();
	//drawArrayVTC(_bufferId, 5 * 2 * 3, 24);

	glDisable2(GL_BLEND);
	//GLState::setTextureEnabled(true);
	//glEnable2(GL_CULL_FACE);

	//sw.stop();
	//sw.printEvery(100, "buttons");
}

const RectangleArea& TouchscreenInput_TestFps::getRectangleArea()
{
	return _boundingRectangle;
}
const RectangleArea& TouchscreenInput_TestFps::getPauseRectangleArea()
{
    return *aPause;
}

void TouchscreenInput_TestFps::rebuild() {
    if (_options->getBooleanValue(OPTIONS_HIDEGUI))
        return;
    
	Tesselator& t = Tesselator::instance;
	//LOGI("instance is: %p, %p, %p, %p, %p FOR %d\n", &t, aLeft, aRight, aUp, aDown, aJump, _bufferId);
	//t.setAccessMode(Tesselator::ACCESS_DYNAMIC);
	t.begin();

	const int imageU = 0;
	const int imageV = 107;
	const int imageSize = 26;

	bool northDiagonals = !isChangingFlightHeight && (_northJump || _forward);

	// render left button
	if (northDiagonals || isChangingFlightHeight) t.colorABGR(cDiscreet);
    else if (isButtonDown(AREA_DPAD_W)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	drawRectangleArea(t, aLeft, imageU + imageSize, imageV, (float)imageSize);

	// render right button
	if (northDiagonals || isChangingFlightHeight) t.colorABGR(cDiscreet);
	else if (isButtonDown(AREA_DPAD_E)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	drawRectangleArea(t, aRight, imageU + imageSize * 3, imageV, (float)imageSize);

	// render forward button
	if (isButtonDown(AREA_DPAD_N)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	if (isChangingFlightHeight)
	{
		drawRectangleArea(t, aUp, imageU + imageSize * 2, imageV + imageSize, (float)imageSize);
	}
	else
	{
		drawRectangleArea(t, aUp, imageU, imageV, (float)imageSize);
	}
	
	// render diagonals, if available
	if (northDiagonals)
	{
		t.colorABGR(cReleased);
		drawRectangleArea(t, aUpLeft, imageU, imageV + imageSize, (float)imageSize);
		drawRectangleArea(t, aUpRight, imageU + imageSize, imageV + imageSize, (float)imageSize);
	}

	// render backwards button
	if (northDiagonals) t.colorABGR(cDiscreet);
	else if (isButtonDown(AREA_DPAD_S)) t.colorABGR(cPressed);
	else						   t.colorABGR(cReleased);
	if (isChangingFlightHeight)
	{
		drawRectangleArea(t, aDown, imageU + imageSize * 3, imageV + imageSize, (float)imageSize);
	}
	else
	{
		drawRectangleArea(t, aDown, imageU + imageSize * 2, imageV, (float)imageSize);
	}


		
	if (!_minecraft->screen) {
		if (aJump && aLeft) {
			float Bh = aLeft->_y1 - aLeft->_y0;
			float BaseY = aUp ? aUp->_y0 : (aLeft->_y0 - Bh);
			
			if (_renderFlightImage) {
				aJump->_y0 = BaseY;
				aJump->_y1 = BaseY + Bh;
			} else {
				aJump->_y0 = BaseY + Bh;
				aJump->_y1 = BaseY + 2.0f * Bh;
			}
		}
	}

	t.draw(); // Finish rendering all standard D-Pad elements with gui/gui.png

	if (_renderFlightImage)
	{
		bool isPressed = isButtonDown(AREA_SNEAK);
		_minecraft->textures->loadAndBindTexture(isPressed ? "gui/shift_press_button.png" : "gui/shift_button.png");
		t.begin();
		t.colorABGR(0xFFFFFFFF);
		drawRectangleAreaStandalone(t, aSneak);
		t.draw();
	}

	if (!_minecraft->screen) {
		bool isPressed = isButtonDown(AREA_DPAD_C);
		std::string tex;
		if (_renderFlightImage) {
			tex = isPressed ? "gui/fly_up_button_press.png" : "gui/fly_up_button.png";
		} else {
			tex = isPressed ? "gui/jump_press_button.png" : "gui/jump_button.png";
		}

		_minecraft->textures->loadAndBindTexture(tex);
		t.begin();
		t.colorABGR(0xFFFFFFFF);
		drawRectangleAreaStandalone(t, aJump);
		t.draw();
	}

	// 8. Flight Toggle button (Middle on the right) - Only visible when flying
	if (_renderFlightImage)
	{
		bool isPressed = isButtonDown(AREA_FLIGHT_TOGGLE);
		_minecraft->textures->loadAndBindTexture(isPressed ? "gui/flight_press_button.png" : "gui/flight_button.png");
		t.begin();
		t.colorABGR(0xFFFFFFFF);
		drawRectangleAreaStandalone(t, aFlightToggle);
		t.draw();
	}

	// 9. Fly Down button (Bottom on the right) - Only visible when flying
	if (_renderFlightImage)
	{
		bool isPressed = isButtonDown(AREA_FLY_DOWN);
		_minecraft->textures->loadAndBindTexture(isPressed ? "gui/fly_down_button_press.png" : "gui/fly_down_button.png");
		t.begin();
		t.colorABGR(0xFFFFFFFF);
		drawRectangleAreaStandalone(t, aFlyDown);
		t.draw();
	}
		
	if (!_minecraft->screen) {
		// Chat button
		{
			bool isPressed = isButtonDown(AREA_CHAT);
			_minecraft->textures->loadAndBindTexture(isPressed ? "gui/chat_button_press.png" : "gui/chat_button.png");
			t.begin();
			t.colorABGR(0xFFFFFFFF);
			drawRectangleAreaStandalone(t, aChat);
			t.draw();
		}

		// Third-person / F5 button
		{
			bool isPressed = isButtonDown(AREA_THIRD);
			_minecraft->textures->loadAndBindTexture(isPressed ? "gui/f5_button_press.png" : "gui/f5_button.png");
			t.begin();
			t.colorABGR(0xFFFFFFFF);
			drawRectangleAreaStandalone(t, aThird);
			t.draw();
		}

		// Pause button
		{
			bool isPressed = isButtonDown(AREA_PAUSE);
			_minecraft->textures->loadAndBindTexture(isPressed ? "gui/pause_button_press.png" : "gui/pause_button.png");
			t.begin();
			t.colorABGR(0xFFFFFFFF);
			drawRectangleAreaStandalone(t, aPause);
			t.draw();
		}
	}
//t.end(true, _bufferId);
	//return;

	t.begin(); // Re-begin so that the final t.draw() at the end of rebuild() doesn't fail
	t.draw();
	//RenderChunk _render = t.end(true, _bufferId);
	//t.setAccessMode(Tesselator::ACCESS_STATIC);
	//_bufferId = _render.vboId;
}


